using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Shapes;
using System.Windows.Threading;
using System.Windows.Media;

namespace EpgTimer.EpgView
{
    public class EpgMainViewBase : EpgViewBase
    {
        protected bool viewCustNeedTimeOnly = false;
        protected SortedList<DateTime, List<ProgramViewItem>> timeList = new SortedList<DateTime, List<ProgramViewItem>>();
        protected List<ProgramViewItem> programList = new List<ProgramViewItem>();
        protected List<ReserveViewItem> reserveList = new List<ReserveViewItem>();
        protected Point clickPos;
        protected DispatcherTimer nowViewTimer;
        protected Line nowLine = null;

        protected virtual void ReDrawNowLine() { }

        private ProgramView programView = null;
        private TimeView timeView = null;
        private ScrollViewer horizontalViewScroll = null;

        protected ContextMenu cmdMenu = new ContextMenu();

        protected override void InitCommand()
        {
            base.InitCommand();

            //コマンド集の初期化の続き
            mc.SetFuncGetDataList(isAll => isAll == true ? reserveList.GetDataList() : reserveList.GetHitDataList(clickPos));
            mc.SetFuncGetEpgEventList(() => 
            {
                ProgramViewItem hitItem = programView.GetProgramViewData(clickPos);
                return hitItem != null && hitItem.EventInfo !=null ? CommonUtil.ToList(hitItem.EventInfo) : new List<EpgEventInfo>();
            });
        }
        public override void RefreshMenu()
        {
            mBinds.ResetInputBindings(this);
            mm.CtxmGenerateContextMenu(cmdMenu, CtxmCode.EpgView, false);
        }

        public void SetControls(ProgramView pv, TimeView tv, ScrollViewer hv, Button button_now)
        {
            programView = pv;
            timeView = tv;
            horizontalViewScroll = hv;

            programView.ScrollChanged += new ScrollChangedEventHandler(epgProgramView_ScrollChanged);
            programView.LeftDoubleClick += new ProgramView.PanelViewClickHandler(epgProgramView_LeftDoubleClick);
            programView.RightClick += new ProgramView.PanelViewClickHandler(epgProgramView_RightClick);
            
            nowViewTimer = new DispatcherTimer(DispatcherPriority.Normal);
            nowViewTimer.Tick += new EventHandler(WaitReDrawNowLine);

            button_now.Click += new RoutedEventHandler((sender, e) => MoveNowTime());
        }

        protected override void UpdateStatusData(int mode = 0)
        {
            this.status[1] = string.Format("番組数:{0}", programList.Count)
                + vutil.ConvertReserveStatus(reserveList.GetDataList(), "　予約");
        }

        /// <summary>現在ライン表示用タイマーイベント呼び出し</summary>
        private void WaitReDrawNowLine(object sender, EventArgs e)
        {
            ReDrawNowLine();
        }

        protected virtual DateTime SetNowTime()
        {
            return DateTime.Now;
        }

        /// <summary>表示位置を現在の時刻にスクロールする</summary>
        protected void MoveNowTime()
        {
            try
            {
                if (timeList.Count <= 0)
                {
                    return;
                }
                DateTime startTime = timeList.Keys[0];

                DateTime time = SetNowTime();

                if (time < startTime)
                {
                    programView.scrollViewer.ScrollToVerticalOffset(0);
                }
                else
                {
                    for (int i = 0; i < timeList.Count; i++)
                    {
                        if (time <= timeList.Keys[i])
                        {
                            double pos = ((i - 1) * 60 * Settings.Instance.MinHeight) - 100;
                            if (pos < 0)
                            {
                                pos = 0;
                            }
                            programView.scrollViewer.ScrollToVerticalOffset(Math.Ceiling(pos));
                            break;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>表示スクロールイベント呼び出し</summary>
        protected void epgProgramView_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            programView.view_ScrollChanged(programView.scrollViewer, timeView.scrollViewer, horizontalViewScroll);
        }

        /// <summary>左ボタンダブルクリックイベント呼び出し/summary>
        protected void epgProgramView_LeftDoubleClick(object sender, Point cursorPos)
        {
            clickPos = cursorPos;
            EpgCmds.ShowDialog.Execute(sender, cmdMenu);
        }
        
        /// <summary>右ボタンクリック</summary>
        protected void button_erea_MouseRightButtonUp(object sender, MouseButtonEventArgs e)
        {
            epgProgramView_RightClick(sender, new Point(-1, -1));
        }
        protected void epgProgramView_RightClick(object sender, Point cursorPos)
        {
            try
            {
                //右クリック表示メニューの作成
                clickPos = cursorPos;
                cmdMenu.Tag = setViewInfo.ViewMode;     //Viewの情報を与えておく
                mc.SupportContextMenuLoading(cmdMenu, null);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        protected override void MoveToReserveItem(ReserveData target, bool IsMarking)
        {
            uint ID = target.ReserveID;
            ReserveViewItem target_item = this.reserveList.Find(item => item.ReserveInfo.ReserveID == ID);
            this.programView.ScrollToFindItem(target_item, IsMarking);
        }

        protected override void MoveToProgramItem(EpgEventInfo target, bool IsMarking)
        {
            ulong PgKey = target.Create64PgKey();
            ProgramViewItem target_item = this.programList.Find(item => item.EventInfo.Create64PgKey() == PgKey);
            this.programView.ScrollToFindItem(target_item, IsMarking);
        }

        protected virtual void ReDrawNowLineBase(DateTime nowTime, DateTime chkNowTime)
        {
            try
            {
                nowViewTimer.Stop();

                double posY = 0;
                for (int i = 0; i < timeList.Count; i++)
                {
                    if (chkNowTime == timeList.Keys[i])
                    {
                        posY = Math.Ceiling((i * 60 + (nowTime - chkNowTime).TotalMinutes) * Settings.Instance.MinHeight);
                        break;
                    }
                    else if (chkNowTime < timeList.Keys[i])
                    {
                        //時間省かれてる
                        posY = Math.Ceiling(i * 60 * Settings.Instance.MinHeight);
                        break;
                    }
                }
                if (posY > programView.canvas.Height)
                {
                    NowLineDelete();
                    return;
                }

                if (nowLine == null)
                {
                    NowLineGenerate();
                }

                nowLine.X1 = 0;
                nowLine.Y1 = posY;
                nowLine.X2 = programView.canvas.Width;
                nowLine.Y2 = posY;

                nowViewTimer.Interval = TimeSpan.FromSeconds(60 - nowTime.Second);
                nowViewTimer.Start();
            }
            catch { }
        }

        protected virtual void NowLineGenerate()
        {
            nowLine = new Line();
            Canvas.SetZIndex(nowLine, 20);
            nowLine.Stroke = Brushes.Red;
            nowLine.StrokeThickness = 3;
            nowLine.Opacity = 0.7;
            nowLine.Effect = new System.Windows.Media.Effects.DropShadowEffect() { BlurRadius = 10 };
            nowLine.IsHitTestVisible = false;
            this.programView.canvas.Children.Add(nowLine);
        }

        protected virtual void NowLineDelete()
        {
            if (nowLine != null)
            {
                this.programView.canvas.Children.Remove(nowLine);
            }
            nowLine = null;
            nowViewTimer.Stop();
            return;
        }

    }
}

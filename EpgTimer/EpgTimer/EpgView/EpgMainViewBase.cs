using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Shapes;
using System.Windows.Threading;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

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
            mc.SetFuncGetDataList(isAll => vutil.GetPanelDataList<ReserveData>(GetReserveItem, clickPos));
            mc.SetFuncGetEpgEventList(() => vutil.GetPanelDataList<EpgEventInfo>(GetProgramItem, clickPos));
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

            programView.PreviewMouseWheel += new MouseWheelEventHandler(epgProgramView_PreviewMouseWheel);
            programView.ScrollChanged += new ScrollChangedEventHandler(epgProgramView_ScrollChanged);
            programView.LeftDoubleClick += new ProgramView.ProgramViewClickHandler(epgProgramView_LeftDoubleClick);
            programView.RightClick += new ProgramView.ProgramViewClickHandler(epgProgramView_RightClick);
            
            nowViewTimer = new DispatcherTimer(DispatcherPriority.Normal);
            nowViewTimer.Tick += new EventHandler(WaitReDrawNowLine);

            button_now.Click += new RoutedEventHandler((sender, e) => MoveNowTime());
        }

        /// <summary>保持情報のクリア</summary>
        public override bool ClearInfo()
        {
            base.ClearInfo();

            nowViewTimer.Stop();
            if (nowLine != null)
            {
                programView.canvas.Children.Remove(nowLine);
            }
            nowLine = null;

            programView.ClearInfo();
            timeView.ClearInfo();
            timeList = new SortedList<DateTime, List<ProgramViewItem>>();
            programList.Clear();
            reserveList.Clear();

            return true;
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
            vutil.view_ScrollChanged<ProgramView>(sender, e,
                programView.scrollViewer, timeView.scrollViewer, horizontalViewScroll);
        }

        /// <summary>マウスホイールイベント呼び出し</summary>
        protected void epgProgramView_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            vutil.view_PreviewMouseWheel<ProgramView>(sender, e, programView.scrollViewer);
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

        /// <summary>マウス位置から予約情報を取得する</summary>
        /// <param name="cursorPos">[IN]マウス位置</param>
        /// <param name="reserve">[OUT]予約情報</param>
        /// <returns>falseで存在しない</returns>
        protected bool GetReserveItem(Point cursorPos, ref ReserveData reserve)
        {
            return vutil.GetHitItem(cursorPos, ref reserve, reserveList);
        }

        /// <summary>マウス位置から番組情報を取得する</summary>
        /// <param name="cursorPos">[IN]マウス位置</param>
        /// <param name="program">[OUT]番組情報</param>
        /// <returns>falseで存在しない</returns>
        protected bool GetProgramItem(Point cursorPos, ref EpgEventInfo program)
        {
            try
            {
                int timeIndex = (int)(cursorPos.Y / (60 * Settings.Instance.MinHeight));
                if (0 <= timeIndex && timeIndex < timeList.Count)
                {
                    return vutil.GetHitItem(cursorPos, ref program, timeList.Values[timeIndex]);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return false;
        }

        protected override void MoveToReserveItem(ReserveItem target, bool JumpingTable)
        {
            uint ID = target.ReserveInfo.ReserveID;
            ReserveViewItem target_item = this.reserveList.Find(item => item.ReserveInfo.ReserveID == ID);
            ScrollToFindItem(target_item, JumpingTable);
        }

        protected override void MoveToProgramItem(SearchItem target, bool JumpingTable)
        {
            ulong PgKey = target.EventInfo.Create64PgKey();
            ProgramViewItem target_item = this.programList.Find(item => item.EventInfo.Create64PgKey() == PgKey);
            ScrollToFindItem(target_item, JumpingTable);
        }

        private void ScrollToFindItem<T>(ViewPanelItem<T> target_item, bool JumpingTable)
        {
            //可能性低いが0では無さそう
            if (target_item == null) return;

            this.programView.scrollViewer.ScrollToHorizontalOffset(target_item.LeftPos - 100);
            this.programView.scrollViewer.ScrollToVerticalOffset(target_item.TopPos - 100);
            if (JumpingTable || Settings.Instance.DisplayNotifyEpgChange)
            {
                //「番組表へジャンプ」の場合、またはオプションで指定のある場合に強調表示する。
                this.programView.SetFindItem<T>(target_item);
            }
        }

    }
}

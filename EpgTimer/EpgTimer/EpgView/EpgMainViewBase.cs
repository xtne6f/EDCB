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
        protected Dictionary<UInt64, ProgramViewItem> programList = new Dictionary<UInt64, ProgramViewItem>();
        protected List<ReserveViewItem> reserveList = new List<ReserveViewItem>();
        protected List<DateTime> timeList = new List<DateTime>();
        protected DispatcherTimer nowViewTimer;
        protected Line nowLine = null;
        protected Point clickPos;

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
                return hitItem != null && hitItem.EventInfo != null ? CommonUtil.ToList(hitItem.EventInfo) : new List<EpgEventInfo>();
            });

            //コマンド集からコマンドを登録
            mc.ResetCommandBindings(this, cmdMenu);

            //メニューの作成、ショートカットの登録
            RefreshMenu();
        }
        public override void RefreshMenu()
        {
            base.RefreshMenu();
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
            nowViewTimer.Tick += (sender, e) => ReDrawNowLine();
            this.Unloaded += (sender, e) => nowViewTimer.Stop();

            button_now.Click += new RoutedEventHandler((sender, e) => MoveNowTime());
        }

        protected override void UpdateStatusData(int mode = 0)
        {
            this.status[1] = string.Format("番組数:{0}", programList.Count)
                + ViewUtil.ConvertReserveStatus(reserveList.GetDataList(), "　予約");
        }

        protected virtual DateTime GetViewTime(DateTime time)
        {
            return time;
        }

        /// <summary>番組の縦表示位置設定</summary>
        protected virtual void SetProgramViewItemVertical()
        {
            //時間リストを構築
            if (viewCustNeedTimeOnly == true)
            {
                var timeSet = new HashSet<DateTime>();
                foreach (ProgramViewItem item in programList.Values)
                {
                    ViewUtil.AddTimeList(timeSet, GetViewTime(item.EventInfo.start_time), item.EventInfo.PgDurationSecond);
                }
                timeList.AddRange(timeSet.OrderBy(time => time));
            }

            //縦位置を設定
            foreach (ProgramViewItem item in programList.Values)
            {
                ViewUtil.SetItemVerticalPos(timeList, item, GetViewTime(item.EventInfo.start_time), item.EventInfo.durationSec, Settings.Instance.MinHeight, viewCustNeedTimeOnly);
            }

            //最低表示行数を適用。また、最低表示高さを確保して、位置も調整する。
            ViewUtil.ModifierMinimumLine(programList.Values, Settings.Instance.MinimumHeight, Settings.Instance.FontSizeTitle);

            //必要時間リストの修正。番組長の関係や、最低表示行数の適用で下に溢れた分を追加する。
            ViewUtil.AdjustTimeList(programList.Values, timeList, Settings.Instance.MinHeight);
        }

        protected virtual ReserveViewItem AddReserveViewItem(ReserveData resInfo, ref ProgramViewItem refPgItem, bool SearchEvent = false)
        {
            //マージン適用前
            DateTime startTime = GetViewTime(resInfo.StartTime);
            DateTime chkStartTime = startTime.Date.AddHours(startTime.Hour);

            //離れた時間のプログラム予約など、番組表が無いので表示不可
            int index = timeList.BinarySearch(chkStartTime);
            if (index < 0) return null;

            //EPG予約の場合は番組の外側に予約枠が飛び出さないようなマージンを作成。
            double StartMargin = resInfo.IsEpgReserve == false ? resInfo.StartMarginResActual : Math.Min(0, resInfo.StartMarginResActual);
            double EndMargin = resInfo.IsEpgReserve == false ? resInfo.EndMarginResActual : Math.Min(0, resInfo.EndMarginResActual);

            //duationがマイナスになる場合は後で処理される
            startTime = startTime.AddSeconds(-StartMargin);
            double duration = resInfo.DurationSecond + StartMargin + EndMargin;

            var resItem = new ReserveViewItem(resInfo);
            reserveList.Add(resItem);

            //予約情報から番組情報を特定し、枠表示位置を再設定する
            refPgItem = null;
            programList.TryGetValue(resInfo.CurrentPgUID(), out refPgItem);
            if (refPgItem == null && SearchEvent == true)
            {
                EpgEventInfo epgInfo = resInfo.SearchEventInfoLikeThat();
                if (epgInfo != null)
                {
                    EpgEventInfo epgRefInfo = epgInfo.GetGroupMainEvent();
                    if (epgRefInfo != null)
                    {
                        programList.TryGetValue(epgRefInfo.CurrentPgUID(), out refPgItem);
                    }
                }
            }

            if (resInfo.IsEpgReserve == true && refPgItem != null && resInfo.DurationSecond != 0)
            {
                resItem.Height = Math.Max(refPgItem.Height * duration / resInfo.DurationSecond, ViewUtil.PanelMinimumHeight);
                resItem.TopPos = refPgItem.TopPos + Math.Min(refPgItem.Height - resItem.Height, refPgItem.Height * (-StartMargin) / resInfo.DurationSecond);
            }
            else
            {
                resItem.Height = Math.Max(duration * Settings.Instance.MinHeight / 60, ViewUtil.PanelMinimumHeight);
                resItem.TopPos = Settings.Instance.MinHeight * (index * 60 + (startTime - chkStartTime).TotalMinutes);
            }
            return resItem;
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
            EpgCmds.ShowDialog.Execute(null, cmdMenu);
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
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        protected override void MoveToReserveItem(ReserveData target, bool IsMarking)
        {
            uint ID = target.ReserveID;
            ReserveViewItem target_item = this.reserveList.Find(item => item.ReserveInfo.ReserveID == ID);
            this.programView.ScrollToFindItem(target_item, IsMarking);
        }
        protected override void MoveToProgramItem(EpgEventInfo target, bool IsMarking)
        {
            ProgramViewItem target_item;
            this.programList.TryGetValue(target.CurrentPgUID(), out target_item);
            this.programView.ScrollToFindItem(target_item, IsMarking);
        }

        /// <summary>表示位置を現在の時刻にスクロールする</summary>
        protected void MoveNowTime()
        {
            try
            {
                int idx = timeList.BinarySearch(GetViewTime(DateTime.Now));
                double pos = ((idx < 0 ? ~idx : idx) - 2) * 60 * Settings.Instance.MinHeight;
                programView.scrollViewer.ScrollToVerticalOffset(Math.Max(0, Math.Ceiling(pos)));
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        /// <summary>現在ライン表示</summary>
        protected virtual void ReDrawNowLine()
        {
            try
            {
                nowViewTimer.Stop();

                if (timeList.Count == 0) return;

                DateTime nowTime = GetViewTime(DateTime.Now);
                int idx = timeList.BinarySearch(nowTime.Date.AddHours(nowTime.Hour));
                double posY = (idx < 0 ? ~idx * 60 : (idx * 60 + nowTime.Minute)) * Settings.Instance.MinHeight;

                if (nowLine == null) NowLineGenerate();

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
            Canvas.SetZIndex(nowLine, 15);
            nowLine.Stroke = Brushes.Red;
            nowLine.StrokeThickness = 3;
            nowLine.Opacity = 0.7;
            nowLine.Effect = new System.Windows.Media.Effects.DropShadowEffect() { BlurRadius = 10 };
            nowLine.IsHitTestVisible = false;
            this.programView.canvas.Children.Add(nowLine);
        }
        protected virtual void NowLineDelete()
        {
            nowViewTimer.Stop();
            this.programView.canvas.Children.Remove(nowLine);
            nowLine = null;
        }

        protected override void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (this.IsVisible == false)
            {
                nowViewTimer.Stop();
            }
            else if (nowLine != null)
            {
                ReDrawNowLine();
            }
            base.UserControl_IsVisibleChanged(sender, e);
        }
    }
}

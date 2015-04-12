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
        
        private ProgramView epgProgramView = null;
        private TimeView timeView = null;
        private ScrollViewer horizontalViewScroll = null;

        public void SetControls(ProgramView pv, TimeView tv, ScrollViewer hv, Button button_now)
        {
            epgProgramView = pv;
            timeView = tv;
            horizontalViewScroll = hv;

            epgProgramView.PreviewMouseWheel += new MouseWheelEventHandler(epgProgramView_PreviewMouseWheel);
            epgProgramView.ScrollChanged += new ScrollChangedEventHandler(epgProgramView_ScrollChanged);
            epgProgramView.LeftDoubleClick += new ProgramView.ProgramViewClickHandler(epgProgramView_LeftDoubleClick);
            epgProgramView.RightClick += new ProgramView.ProgramViewClickHandler(epgProgramView_RightClick);

            if (Settings.Instance.NoStyle == 1)
            {
                button_now.Style = null;
            }

            nowViewTimer = new DispatcherTimer(DispatcherPriority.Normal);
            nowViewTimer.Tick += new EventHandler(WaitReDrawNowLine);
        }

        /// <summary>
        /// 保持情報のクリア
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public override bool ClearInfo()
        {
            base.ClearInfo();

            nowViewTimer.Stop();
            if (nowLine != null)
            {
                epgProgramView.canvas.Children.Remove(nowLine);
            }
            nowLine = null;

            epgProgramView.ClearInfo();
            timeView.ClearInfo();
            timeList = new SortedList<DateTime, List<ProgramViewItem>>();
            programList = new List<ProgramViewItem>();
            reserveList = new List<ReserveViewItem>();

            return true;
        }

        /// <summary>
        /// 現在ライン表示用タイマーイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void WaitReDrawNowLine(object sender, EventArgs e)
        {
            ReDrawNowLine();
        }

        /// <summary>
        /// 現在ボタンクリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void button_now_Click(object sender, RoutedEventArgs e)
        {
            MoveNowTime();
        }

        protected virtual DateTime SetNowTime()
        {
            return DateTime.Now;
        }

        /// <summary>
        /// 表示位置を現在の時刻にスクロールする
        /// </summary>
        public void MoveNowTime()
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
                    epgProgramView.scrollViewer.ScrollToVerticalOffset(0);
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
                            epgProgramView.scrollViewer.ScrollToVerticalOffset(Math.Ceiling(pos));
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
        
        /// <summary>
        /// 表示スクロールイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void epgProgramView_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            vutil.view_ScrollChanged<ProgramView>(sender, e,
                epgProgramView.scrollViewer, timeView.scrollViewer, horizontalViewScroll);
        }

        /// <summary>
        /// マウスホイールイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void epgProgramView_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            vutil.view_PreviewMouseWheel<ProgramView>(sender, e, epgProgramView.scrollViewer);
        }

        /// <summary>
        /// 左ボタンダブルクリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="cursorPos"></param>
        protected void epgProgramView_LeftDoubleClick(object sender, Point cursorPos)
        {
            ShowReserveDialog(cursorPos);
        }
        
        protected void button_erea_MouseRightButtonUp(object sender, MouseButtonEventArgs e)
        {
            epgProgramView_RightClick(sender, new Point(-1, -1));
        }
        /// <summary>
        /// 右ボタンクリック
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="cursorPos"></param>
        protected void epgProgramView_RightClick(object sender, Point cursorPos)
        {
            try
            {
                //右クリック表示メニューの作成
                clickPos = cursorPos;
                ReserveData reserve = new ReserveData();
                EpgEventInfo program = new EpgEventInfo();
                bool noItem = false;
                bool hasReserved = false;
                bool hasNoReserved = false;

                if (GetReserveItem(cursorPos, ref reserve) == false)
                {
                    if (GetProgramItem(cursorPos, ref program) == false)
                    {
                        noItem = true;
                    }
                    hasNoReserved = true;
                }
                else
                {
                    hasReserved = true;
                }

                //右クリック表示メニューの作成
                ContextMenu menu = new ContextMenu();

                MenuItem menuItemReverse = new MenuItem();
                menuItemReverse.Header = "簡易予約/予約←→無効";
                menuItemReverse.Click += new RoutedEventHandler(cm_reverse_Click);

                //予約追加メニュー作成
                MenuItem menuItemAdd = mutil.GenerateAddMenu(new Action<object,
                    RoutedEventArgs>[] { cm_show_dialog_Click, cm_add_preset_Click });

                //予約変更メニュー作成
                MenuItem menuItemChg = mutil.GenerateChgMenu(new Action<object,
                    RoutedEventArgs>[] { cm_show_dialog_Click, cm_chg_recmode_Click, cm_chg_priority_Click });

                MenuItem menuItemDel = new MenuItem();
                menuItemDel.Header = "削除";
                menuItemDel.Click += new RoutedEventHandler(cm_del_Click);

                MenuItem menuItemAutoAdd = new MenuItem();
                menuItemAutoAdd.Header = "自動予約登録";
                menuItemAutoAdd.ToolTip = mutil.EpgKeyword_TrimMode();
                ToolTipService.SetShowOnDisabled(menuItemAutoAdd, true);
                menuItemAutoAdd.Click += new RoutedEventHandler(cm_autoadd_Click);
                MenuItem menuItemTimeshift = new MenuItem();
                menuItemTimeshift.Header = "追っかけ再生";
                menuItemTimeshift.Click += new RoutedEventHandler(cm_timeShiftPlay_Click);

                //表示メニュー作成
                MenuItem menuItemView = mutil.GenerateViewMenu(setViewInfo.ViewMode, new Action<object,
                    RoutedEventArgs>[] { cm_viewSet_Click, cm_chg_viewMode_Click });

                menuItemView.IsEnabled = true;

                if (noItem == true)
                {
                    menuItemReverse.Header = "簡易予約";
                    menuItemReverse.IsEnabled = false;
                    menuItemAdd.IsEnabled = false;
                    menuItemChg.IsEnabled = false;
                    menuItemDel.IsEnabled = false;
                    menuItemAutoAdd.IsEnabled = false;
                    menuItemTimeshift.IsEnabled = false;
                }
                else
                {
                    if (hasReserved == true)
                    {
                        menuItemReverse.Header = "予約←→無効";
                        menuItemReverse.IsEnabled = true;
                        menuItemAdd.IsEnabled = hasNoReserved;
                        menuItemChg.IsEnabled = true;
                        mutil.CheckChgItems(menuItemChg, mutil.GetList(reserve));//現在の状態(録画モード、優先度)にチェックを入れる
                        menuItemDel.IsEnabled = true;
                        menuItemAutoAdd.IsEnabled = true;
                        menuItemTimeshift.IsEnabled = true;
                    }
                    else
                    {
                        menuItemReverse.Header = "簡易予約";
                        menuItemReverse.IsEnabled = true;
                        menuItemAdd.IsEnabled = true;
                        menuItemChg.IsEnabled = false;
                        menuItemDel.IsEnabled = false;
                        menuItemAutoAdd.IsEnabled = true;
                        menuItemTimeshift.IsEnabled = false;
                    }
                }

                menu.Items.Add(menuItemReverse);
                menu.Items.Add(menuItemAdd);
                menu.Items.Add(menuItemChg);
                menu.Items.Add(menuItemDel);
                menu.Items.Add(menuItemAutoAdd);
                menu.Items.Add(menuItemTimeshift);

                //追加メニューの挿入
                mutil.InsertAppendMenu(menu, new Action<object,
                    RoutedEventArgs>[] { cm_CopyTitle_Click, cm_CopyContent_Click, cm_SearchTitle_Click }, !noItem);

                menu.Items.Add(new Separator());
                menu.Items.Add(menuItemView);
                menu.IsOpen = true;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// マウス位置から予約情報を取得する
        /// </summary>
        /// <param name="cursorPos">[IN]マウス位置</param>
        /// <param name="reserve">[OUT]予約情報</param>
        /// <returns>falseで存在しない</returns>
        protected bool GetReserveItem(Point cursorPos, ref ReserveData reserve)
        {
            return vutil.GetHitItem(cursorPos, ref reserve, reserveList);
        }

        /// <summary>
        /// マウス位置から番組情報を取得する
        /// </summary>
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

        /// <summary>
        /// 右クリックメニュー プリセットクリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_add_preset_Click(object sender, RoutedEventArgs e)
        {
            EpgEventInfo eventInfo = new EpgEventInfo();
            if (GetProgramItem(clickPos, ref eventInfo) == false) return;
            mutil.ReserveAdd(eventInfo, null, sender);
        }

        /// <summary>
        /// 右クリックメニュー ダイアログ表示クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_show_dialog_Click(object sender, RoutedEventArgs e)
        {
            ShowReserveDialog(clickPos);
        }

        /// <summary>
        /// 右クリックメニュー 予約削除クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_del_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;
            mutil.ReserveDelete(reserve);
        }

        /// <summary>
        /// 右クリックメニュー 予約モード変更イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_chg_recmode_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;
            mutil.ReserveChangeRecmode(reserve, sender);
        }

        /// <summary>
        /// 右クリックメニュー 優先度変更イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_chg_priority_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;
            mutil.ReserveChangePriority(reserve, sender);
        }

        /// <summary>
        /// 右クリックメニュー 自動予約登録イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_autoadd_Click(object sender, RoutedEventArgs e)
        {
            EpgEventInfo program = new EpgEventInfo();
            if (GetProgramItem(clickPos, ref program) == false) return;
            mutil.SendAutoAdd(program, this);
        }

        /// <summary>
        /// 右クリックメニュー 追っかけ再生イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_timeShiftPlay_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;
            CommonManager.Instance.TVTestCtrl.StartTimeShift(reserve.ReserveID);
        }

        /// <summary>
        /// 右クリックメニュー 簡易予約/予約←→無効クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_reverse_Click(object sender, RoutedEventArgs e)
        {
            //まず予約情報あるかチェック
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == true)
            {
                //予約←→無効
                mutil.ReserveChangeOnOff(reserve);
                return;
            }
            //番組情報あるかチェック
            EpgEventInfo program = new EpgEventInfo();
            if (GetProgramItem(clickPos, ref program) == true)
            {
                //簡易予約
                mutil.ReserveAdd(program);
                return;
            }
        }

        /// <summary>
        /// 右クリックメニュー 番組名をコピーイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_CopyTitle_Click(object sender, RoutedEventArgs e)
        {
            EpgEventInfo program = new EpgEventInfo();
            if (GetProgramItem(clickPos, ref program) == false) return;
            mutil.CopyTitle2Clipboard(program.Title());
        }

        /// <summary>
        /// 右クリックメニュー 番組情報をコピーイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_CopyContent_Click(object sender, RoutedEventArgs e)
        {
            EpgEventInfo program = new EpgEventInfo();
            if (GetProgramItem(clickPos, ref program) == false) return;
            mutil.CopyContent2Clipboard(program);
        }

        /// <summary>
        /// 右クリックメニュー 番組名で検索イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_SearchTitle_Click(object sender, RoutedEventArgs e)
        {
            EpgEventInfo program = new EpgEventInfo();
            if (GetProgramItem(clickPos, ref program) == false) return;
            mutil.SearchText(program.Title());
        }

        /// <summary>
        /// 右クリックメニュー 表示設定イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_viewSet_Click(object sender, RoutedEventArgs e)
        {
            base.ViewSetting(this, null);
        }

        /// <summary>
        /// 右クリックメニュー 表示モードイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void cm_chg_viewMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (sender.GetType() != typeof(MenuItem)) return;
                if (base.EnableViewSetting() == false) return;

                var item = sender as MenuItem;
                var setInfo = new CustomEpgTabInfo();
                setViewInfo.CopyTo(ref setInfo);
                setInfo.ViewMode = (int)item.DataContext;

                BlackoutWindow.Clear();
                var reserve = new ReserveData();
                var program = new EpgEventInfo();
                if (GetReserveItem(clickPos, ref reserve) == true)
                {
                    BlackoutWindow.SelectedReserveItem = new ReserveItem(reserve);
                }
                else if (GetProgramItem(clickPos, ref program) == true)
                {
                    BlackoutWindow.SelectedSearchItem = new SearchItem(program);
                }

                ViewSetting(this, setInfo);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// ダイアログ表示
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        protected void ShowReserveDialog(Point pos)
        {
            //まず予約情報あるかチェック
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(pos, ref reserve) == true)
            {
                //予約変更ダイアログ表示
                mutil.OpenChangeReserveWindow(reserve, this, Settings.Instance.EpgInfoOpenMode);
                return;
            }
            //番組情報あるかチェック
            EpgEventInfo program = new EpgEventInfo();
            if (GetProgramItem(pos, ref program) == true)
            {
                //予約追加ダイアログ表示
                mutil.OpenEpgReserveWindow(program, this, Settings.Instance.EpgInfoOpenMode);
                return;
            }
        }

        protected override void MoveToReserveItem(ReserveItem target)
        {
            uint ID = target.ReserveInfo.ReserveID;
            ReserveViewItem target_item = this.reserveList.Find(item => item.ReserveInfo.ReserveID == ID);
            ScrollToFindItem(target_item);
        }

        protected override void MoveToProgramItem(SearchItem target)
        {
            ulong PgKey = target.EventInfo.Create64PgKey();
            ProgramViewItem target_item = this.programList.Find(item => item.EventInfo.Create64PgKey() == PgKey);
            ScrollToFindItem(target_item);
        }

        private void ScrollToFindItem<T>(ViewPanelItem<T> target_item)
        {
            this.epgProgramView.scrollViewer.ScrollToHorizontalOffset(target_item.LeftPos - 100);
            this.epgProgramView.scrollViewer.ScrollToVerticalOffset(target_item.TopPos - 100);
        }

    }
}

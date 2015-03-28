using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;
using EpgTimer.TunerReserveViewCtrl;

namespace EpgTimer
{
    /// <summary>
    /// TunerReserveMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class TunerReserveMainView : UserControl
    {
        private List<TunerNameViewItem> tunerList = new List<TunerNameViewItem>();
        private List<ReserveViewItem> reserveList = new List<ReserveViewItem>();
        private Point clickPos;
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;

        private bool updateReserveData = true;

        public TunerReserveMainView()
        {
            InitializeComponent();

            tunerReserveView.PreviewMouseWheel += new MouseWheelEventHandler(tunerReserveView_PreviewMouseWheel);
            tunerReserveView.ScrollChanged += new ScrollChangedEventHandler(tunerReserveView_ScrollChanged);
            tunerReserveView.LeftDoubleClick += new TunerReserveView.ProgramViewClickHandler(tunerReserveView_LeftDoubleClick);
            tunerReserveView.RightClick += new TunerReserveView.ProgramViewClickHandler(tunerReserveView_RightClick);

        }

        /// <summary>
        /// 保持情報のクリア
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public bool ClearInfo()
        {
            tunerReserveView.ClearInfo();
            tunerReserveTimeView.ClearInfo();
            tunerReserveNameView.ClearInfo();
            tunerList.Clear();
            reserveList.Clear();

            return true;
        }

        /// <summary>
        /// 表示スクロールイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void tunerReserveView_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            try
            {
                if (sender.GetType() == typeof(TunerReserveView))
                {
                    //時間軸の表示もスクロール
                    tunerReserveTimeView.scrollViewer.ScrollToVerticalOffset(tunerReserveView.scrollViewer.VerticalOffset);
                    //サービス名表示もスクロール
                    tunerReserveNameView.scrollViewer.ScrollToHorizontalOffset(tunerReserveView.scrollViewer.HorizontalOffset);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// マウスホイールイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void tunerReserveView_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            try
            {
                e.Handled = true;
                if (sender.GetType() == typeof(TunerReserveView))
                {
                    TunerReserveView view = sender as TunerReserveView;
                    if (Settings.Instance.MouseScrollAuto == true)
                    {
                        view.scrollViewer.ScrollToVerticalOffset(view.scrollViewer.VerticalOffset - e.Delta);
                    }
                    else
                    {
                        if (e.Delta < 0)
                        {
                            //下方向
                            view.scrollViewer.ScrollToVerticalOffset(view.scrollViewer.VerticalOffset + Settings.Instance.ScrollSize);
                        }
                        else
                        {
                            //上方向
                            if (view.scrollViewer.VerticalOffset < Settings.Instance.ScrollSize)
                            {
                                view.scrollViewer.ScrollToVerticalOffset(0);
                            }
                            else
                            {
                                view.scrollViewer.ScrollToVerticalOffset(view.scrollViewer.VerticalOffset - Settings.Instance.ScrollSize);
                            }
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
        /// マウス位置から予約情報を取得する
        /// </summary>
        /// <param name="cursorPos">[IN]マウス位置</param>
        /// <param name="reserve">[OUT]予約情報</param>
        /// <returns>falseで存在しない</returns>
        private bool GetReserveItem(Point cursorPos, ref ReserveData reserve)
        {
            try
            {
                foreach (ReserveViewItem resInfo in reserveList)
                {
                    if (resInfo.LeftPos <= cursorPos.X && cursorPos.X < resInfo.LeftPos + resInfo.Width &&
                        resInfo.TopPos <= cursorPos.Y && cursorPos.Y < resInfo.TopPos + resInfo.Height)
                    {
                        reserve = resInfo.ReserveInfo;
                        return true;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return false;
        }

        /// <summary>
        /// 左ボタンダブルクリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="cursorPos"></param>
        void tunerReserveView_LeftDoubleClick(object sender, Point cursorPos)
        {
            //まず予約情報あるかチェック
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(cursorPos, ref reserve) == false) return;

            //予約変更ダイアログ表示
            ChangeReserve(reserve);
        }

        /// <summary>
        /// 右ボタンクリック
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="cursorPos"></param>
        void tunerReserveView_RightClick(object sender, Point cursorPos)
        {
            try
            {
                //右クリック表示メニューの作成
                clickPos = cursorPos;
                ReserveData reserve = new ReserveData();
                if (GetReserveItem(cursorPos, ref reserve) == false) return;
                ContextMenu menu = new ContextMenu();

                //予約変更メニュー作成
                MenuItem menuItemChg = mutil.GenerateChgMenu(new Action<object,
                    RoutedEventArgs>[] { cm_chg_Click, cm_chg_recmode_Click, cm_chg_priority_Click });
                mutil.CheckChgItems(menuItemChg, mutil.GetList(reserve));//現在の状態(録画モード、優先度)にチェックを入れる

                MenuItem menuItemDel = new MenuItem();
                menuItemDel.Header = "削除";
                menuItemDel.Click += new RoutedEventHandler(cm_del_Click);
                MenuItem menuItemPTable = new MenuItem();
                menuItemPTable.Header = "番組表へジャンプ";
                menuItemPTable.Click += new RoutedEventHandler(cm_programtable_Click);
                MenuItem menuItemAutoAdd = new MenuItem();
                menuItemAutoAdd.Header = "自動予約登録";
                menuItemAutoAdd.ToolTip = mutil.EpgKeyword_TrimMode();
                menuItemAutoAdd.Click += new RoutedEventHandler(cm_autoadd_Click);
                MenuItem menuItemTimeshift = new MenuItem();
                menuItemTimeshift.Header = "追っかけ再生";
                menuItemTimeshift.Click += new RoutedEventHandler(cm_timeShiftPlay_Click);

                //フォーカスに問題があり、一度クリックしないと正しく動かない‥。
                //仮対策コード
                menuItemPTable.IsEnabled = IsViewOnceClicked;
                if (menuItemPTable.IsEnabled == false)
                {
                    menuItemPTable.ToolTip = "一度画面を左クリックすると使用可能";
                    ToolTipService.SetShowOnDisabled(menuItemPTable, true);
                }

                menu.Items.Add(menuItemChg);
                menu.Items.Add(menuItemDel);
                menu.Items.Add(menuItemPTable);
                menu.Items.Add(menuItemAutoAdd);
                menu.Items.Add(menuItemTimeshift);

                //追加メニューの挿入
                mutil.InsertAppendMenu(menu, new Action<object,
                    RoutedEventArgs>[] { cm_CopyTitle_Click, cm_CopyContent_Click, cm_SearchTitle_Click });

                menu.IsOpen = true;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 右クリックメニュー 予約変更クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_chg_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;
            ChangeReserve(reserve);
        }

        /// <summary>
        /// 右クリックメニュー 予約削除クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_del_Click(object sender, RoutedEventArgs e)
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
        private void cm_chg_recmode_Click(object sender, RoutedEventArgs e)
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
        private void cm_chg_priority_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;
            mutil.ReserveChangePriority(reserve, sender);
        }

        /// <summary>
        /// 右クリックメニュー 番組表へジャンプイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_programtable_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;

            BlackoutWindow.selectedReserveItem = new ReserveItem(reserve);
            MainWindow mainWindow = (MainWindow)Application.Current.MainWindow;
            mainWindow.moveTo_tabItem_epg();
        }

        //フォーカスがうまくいかない仮対策
        private bool IsViewOnceClicked = false;

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            this.IsViewOnceClicked = false;
        }

        private void tunerReserveView_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            this.IsViewOnceClicked = true;
        }

        /// <summary>
        /// 右クリックメニュー 自動予約登録イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_autoadd_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;
            mutil.SendAutoAdd(reserve, this);
        }

        /// <summary>
        /// 右クリックメニュー 追っかけ再生イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_timeShiftPlay_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;
            CommonManager.Instance.TVTestCtrl.StartTimeShift(reserve.ReserveID);
        }

        /// <summary>
        /// 右クリックメニュー 番組名をコピーイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_CopyTitle_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;
            mutil.CopyTitle2Clipboard(reserve.Title);
        }

        /// <summary>
        /// 右クリックメニュー 番組情報をコピーイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_CopyContent_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;
            mutil.CopyContent2Clipboard(reserve);
        }

        /// <summary>
        /// 右クリックメニュー 番組名で検索イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_SearchTitle_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;
            mutil.SearchText(reserve.Title);
        }

        /// <summary>
        /// 予約変更
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ChangeReserve(ReserveData reserveInfo)
        {
            mutil.OpenChangeReserveWindow(reserveInfo, this);
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (this.IsVisible == true)
            {
                if (updateReserveData == true)
                {
                    if (ReloadReserveData() == true)
                    {
                        updateReserveData = false;
                    }
                }
            }
        }

        private bool ReloadReserveData()
        {
            try
            {
                if (CommonManager.Instance.NWMode == true)
                {
                    if (CommonManager.Instance.NW.IsConnected == false)
                    {
                        return false;
                    }
                } 
                ErrCode err = CommonManager.Instance.DB.ReloadReserveInfo();
                if (CommonManager.CmdErrMsgTypical(err, "予約情報の取得", this) == false)
                {
                    return false;
                }

                ReloadReserveViewItem();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return true;
        }

        /// <summary>
        /// 予約情報更新通知
        /// </summary>
        public void UpdateReserveData()
        {
            updateReserveData = true;
            if (this.IsVisible == true)
            {
                if (ReloadReserveData() == true)
                {
                    updateReserveData = false;
                }
            }
        }

        /// <summary>
        /// 予約情報の再描画
        /// </summary>
        private void ReloadReserveViewItem()
        {
            tunerReserveView.ClearInfo();
            tunerReserveTimeView.ClearInfo();
            tunerReserveNameView.ClearInfo();
            List<DateTime> timeList = new List<DateTime>();
            tunerList.Clear();
            reserveList.Clear();
            try
            {
                double leftPos = 0;
                for (int i = 0; i < CommonManager.Instance.DB.TunerReserveList.Count; i++)
                {
                    double width = 150;
                    TunerReserveInfo info = CommonManager.Instance.DB.TunerReserveList.Values.ElementAt(i);
                    TunerNameViewItem tunerInfo = new TunerNameViewItem(info, width);
                    tunerList.Add(tunerInfo);

                    List<ReserveViewItem> tunerAddList = new List<ReserveViewItem>();
                    for (int j = 0; j < info.reserveList.Count; j++ )
                    {
                        UInt32 reserveID = (UInt32)info.reserveList[j];
                        if (CommonManager.Instance.DB.ReserveList.ContainsKey(reserveID) == false)
                        {
                            continue;
                        }
                        ReserveData reserveInfo = CommonManager.Instance.DB.ReserveList[reserveID];
                        ReserveViewItem viewItem = new ReserveViewItem(CommonManager.Instance.DB.ReserveList[reserveID]);

                        Int32 duration = (Int32)reserveInfo.DurationSecond;
                        DateTime startTime = reserveInfo.StartTime;
                        if (reserveInfo.RecSetting.UseMargineFlag == 1)
                        {
                            if (reserveInfo.RecSetting.StartMargine < 0)
                            {
                                startTime = reserveInfo.StartTime.AddSeconds(reserveInfo.RecSetting.StartMargine*-1);
                                duration += reserveInfo.RecSetting.StartMargine;
                            }
                            if (reserveInfo.RecSetting.EndMargine < 0)
                            {
                                duration += reserveInfo.RecSetting.EndMargine;
                            }
                        }
                        DateTime EndTime;
                        EndTime = startTime.AddSeconds(duration);
                        //if ((duration / 60) < Settings.Instance.MinHeight)
                        //{
                        //    duration = (int)Settings.Instance.MinHeight;
                        //}

                        viewItem.Height = Math.Floor((duration / 60) * Settings.Instance.MinHeight);
                        if (viewItem.Height == 0)
                        {
                            viewItem.Height = Settings.Instance.MinHeight;
                        }
                        viewItem.Width = 150;
                        viewItem.LeftPos = leftPos;

                        foreach (ReserveViewItem addItem in tunerAddList)
                        {
                            ReserveData addInfo = addItem.ReserveInfo;
                            Int32 durationAdd = (Int32)addInfo.DurationSecond;
                            DateTime startTimeAdd = addInfo.StartTime;
                            if (addInfo.RecSetting.UseMargineFlag == 1)
                            {
                                if (addInfo.RecSetting.StartMargine < 0)
                                {
                                    startTimeAdd = addInfo.StartTime.AddSeconds(addInfo.RecSetting.StartMargine*-1);
                                    durationAdd += addInfo.RecSetting.StartMargine;
                                }
                                if (addInfo.RecSetting.EndMargine < 0)
                                {
                                    durationAdd += addInfo.RecSetting.EndMargine;
                                }
                            }
                            DateTime endTimeAdd;
                            endTimeAdd = startTimeAdd.AddSeconds(durationAdd);

                            if ((startTimeAdd <= startTime && startTime < endTimeAdd) ||
                                (startTimeAdd < EndTime && EndTime <= endTimeAdd) || 
                                (startTime <= startTimeAdd && startTimeAdd < EndTime) ||
                                (startTime < endTimeAdd && endTimeAdd <= EndTime)
                                )
                            {
                                if (addItem.LeftPos >= viewItem.LeftPos)
                                {
                                    viewItem.LeftPos += 150;
                                    if (viewItem.LeftPos - leftPos >= width)
                                    {
                                        width += 150;
                                    }
                                }
                            }
                        }

                        reserveList.Add(viewItem);
                        tunerAddList.Add(viewItem);

                        //必要時間リストの構築

                        DateTime chkStartTime = new DateTime(startTime.Year, startTime.Month, startTime.Day, startTime.Hour, 0, 0);
                        while (chkStartTime <= EndTime)
                        {
                            int index = timeList.BinarySearch(chkStartTime);
                            if (index < 0)
                            {
                                timeList.Insert(~index, chkStartTime);
                            }
                            chkStartTime = chkStartTime.AddHours(1);
                        }

                    }
                    tunerInfo.Width = width;
                    leftPos += width;
                }

                //表示位置設定
                foreach (ReserveViewItem item in reserveList)
                {
                    DateTime startTime = item.ReserveInfo.StartTime;
                    if (item.ReserveInfo.RecSetting.UseMargineFlag == 1)
                    {
                        if (item.ReserveInfo.RecSetting.StartMargine < 0)
                        {
                            startTime = item.ReserveInfo.StartTime.AddSeconds(item.ReserveInfo.RecSetting.StartMargine*-1);
                        }
                    }

                    DateTime chkStartTime = new DateTime(startTime.Year,
                        startTime.Month,
                        startTime.Day,
                        startTime.Hour,
                        0,
                        0);
                    int index = timeList.BinarySearch(chkStartTime);
                    if (index >= 0)
                    {
                        item.TopPos = (index * 60 + (startTime - chkStartTime).TotalMinutes) * Settings.Instance.MinHeight;
                    }
                }

                tunerReserveTimeView.SetTime(timeList, true);
                tunerReserveNameView.SetTunerInfo(tunerList);
                tunerReserveView.SetReserveList(reserveList,
                    leftPos,
                    timeList.Count * 60 * Settings.Instance.MinHeight);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
    }
}

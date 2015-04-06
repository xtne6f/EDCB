using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Collections;
using System.Windows.Threading;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;
using EpgTimer.EpgView;

namespace EpgTimer
{
    /// <summary>
    /// EpgWeekMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgWeekMainView : UserControl, IEpgDataViewItem
    {
        public event ViewSettingClickHandler ViewSettingClick = null;

        private CustomEpgTabInfo setViewInfo = null;

        private List<UInt64> viewCustServiceList = null;
        private Dictionary<UInt16, UInt16> viewCustContentKindList = new Dictionary<UInt16, UInt16>();
        private bool viewCustNeedTimeOnly = false;
        private SortedList<DateTime, List<ProgramViewItem>> timeList = new SortedList<DateTime, List<ProgramViewItem>>();
        private SortedList dayList = new SortedList();
        private List<ProgramViewItem> programList = new List<ProgramViewItem>();
        private List<ReserveViewItem> reserveList = new List<ReserveViewItem>();
        private Point clickPos;
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private ViewUtil vutil = CommonManager.Instance.VUtil;
        private DispatcherTimer nowViewTimer;
        private Line nowLine = null;
        private Dictionary<UInt64, EpgServiceEventInfo> searchEventList = new Dictionary<UInt64, EpgServiceEventInfo>();

        private bool updateEpgData = true;
        private bool updateReserveData = true;

        public EpgWeekMainView()
        {
            InitializeComponent();

            if (Settings.Instance.NoStyle == 1)
            {
                button_now.Style = null;

            }

            epgProgramView.PreviewMouseWheel += new MouseWheelEventHandler(epgProgramView_PreviewMouseWheel);
            epgProgramView.ScrollChanged += new ScrollChangedEventHandler(epgProgramView_ScrollChanged);
            epgProgramView.LeftDoubleClick += new ProgramView.ProgramViewClickHandler(epgProgramView_LeftDoubleClick);
            epgProgramView.RightClick += new ProgramView.ProgramViewClickHandler(epgProgramView_RightClick);

            nowViewTimer = new DispatcherTimer(DispatcherPriority.Normal);
            nowViewTimer.Tick += new EventHandler(WaitReDrawNowLine);
        }

        /// <summary>
        /// 保持情報のクリア
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        public bool ClearInfo()
        {
            nowViewTimer.Stop();
            if (nowLine != null)
            {
                epgProgramView.canvas.Children.Remove(nowLine);
            }
            nowLine = null;

            epgProgramView.ClearInfo();
            timeView.ClearInfo();
            weekDayView.ClearInfo();
            timeList.Clear();
            dayList.Clear();
            programList.Clear();
            reserveList.Clear();
            searchEventList.Clear();

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
        /// 現在ライン表示
        /// </summary>
        private void ReDrawNowLine()
        {
            try
            {
                nowViewTimer.Stop();
                DateTime nowTime = new DateTime(2001, 1, 1, DateTime.Now.Hour, DateTime.Now.Minute, DateTime.Now.Second);

                if (nowLine == null)
                {
                    nowLine = new Line();
                    Canvas.SetZIndex(nowLine, 20);
                    nowLine.Stroke = new SolidColorBrush(Colors.Red);
                    nowLine.StrokeThickness = Settings.Instance.MinHeight * 2;
                    nowLine.Opacity = 0.5;
                    epgProgramView.canvas.Children.Add(nowLine);
                }

                double posY = 0;
                DateTime chkNowTime = new DateTime(2001, 1, 1, nowTime.Hour, 0, 0);
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

                if (posY > epgProgramView.canvas.Height)
                {
                    if (nowLine != null)
                    {
                        epgProgramView.canvas.Children.Remove(nowLine);
                    }
                    nowLine = null;
                    return;
                }

                nowLine.X1 = 0;
                nowLine.Y1 = posY;
                nowLine.X2 = epgProgramView.canvas.Width;
                nowLine.Y2 = posY;

                nowViewTimer.Interval = TimeSpan.FromSeconds(60 - nowTime.Second);
                nowViewTimer.Start();
            }
            catch
            {
            }
        }

        /// <summary>
        /// 表示スクロールイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void epgProgramView_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            vutil.view_ScrollChanged<ProgramView>(sender, e,
                epgProgramView.scrollViewer, timeView.scrollViewer, weekDayView.scrollViewer);
        }

        /// <summary>
        /// マウスホイールイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void epgProgramView_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            vutil.view_PreviewMouseWheel<ProgramView>(sender, e, epgProgramView.scrollViewer);
        }

        /// <summary>
        /// 現在ボタンクリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_now_Click(object sender, RoutedEventArgs e)
        {
            MoveNowTime();
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

                DateTime time = new DateTime(2001, 1, 1, DateTime.Now.Hour, DateTime.Now.Minute, DateTime.Now.Second);

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
        /// マウス位置から予約情報を取得する
        /// </summary>
        /// <param name="cursorPos">[IN]マウス位置</param>
        /// <param name="reserve">[OUT]予約情報</param>
        /// <returns>falseで存在しない</returns>
        private bool GetReserveItem(Point cursorPos, ref ReserveData reserve)
        {
            return vutil.GetReserveItem(cursorPos, ref reserve, reserveList);
        }

        /// <summary>
        /// マウス位置から番組情報を取得する
        /// </summary>
        /// <param name="cursorPos">[IN]マウス位置</param>
        /// <param name="program">[OUT]番組情報</param>
        /// <returns>falseで存在しない</returns>
        private bool GetProgramItem(Point cursorPos, ref EpgEventInfo program)
        {
            return vutil.GetProgramItem(cursorPos, ref program, timeList);
        }

        /// <summary>
        /// 左ボタンダブルクリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="cursorPos"></param>
        void epgProgramView_LeftDoubleClick(object sender, Point cursorPos)
        {
            ShowReserveDialog(cursorPos);
        }

        private void button_erea_MouseRightButtonUp(object sender, MouseButtonEventArgs e)
        {
            epgProgramView_RightClick(sender, new Point(-1, -1));
        }
        /// <summary>
        /// 右ボタンクリック
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="cursorPos"></param>
        void epgProgramView_RightClick(object sender, Point cursorPos)
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
                MenuItem menuItemView = mutil.GenerateViewMenu(1, new Action<object,
                    RoutedEventArgs>[] { cm_viewSet_Click, cm_chg_viewMode_Click });

                if (noItem == true)
                {
                    menuItemReverse.Header = "簡易予約";
                    menuItemReverse.IsEnabled = false;
                    menuItemAdd.IsEnabled = false;
                    menuItemChg.IsEnabled = false;
                    menuItemDel.IsEnabled = false;
                    menuItemAutoAdd.IsEnabled = false;
                    menuItemTimeshift.IsEnabled = false;
                    menuItemView.IsEnabled = true;
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
                    menuItemView.IsEnabled = true;
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
        /// 右クリックメニュー プリセットクリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void cm_add_preset_Click(object sender, RoutedEventArgs e)
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
        private void cm_show_dialog_Click(object sender, RoutedEventArgs e)
        {
            ShowReserveDialog(clickPos);
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
        /// 右クリックメニュー 自動予約登録イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_autoadd_Click(object sender, RoutedEventArgs e)
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
        private void cm_timeShiftPlay_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = new ReserveData();
            if (GetReserveItem(clickPos, ref reserve) == false) return;
            CommonManager.Instance.TVTestCtrl.StartTimeShift(reserve.ReserveID);
        }

        /// <summary>
        /// 右クリックメニュー 表示設定イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_viewSet_Click(object sender, RoutedEventArgs e)
        {
            if (ViewSettingClick != null)
            {
                ViewSettingClick(this, null);
            }
        }

        /// <summary>
        /// 右クリックメニュー 簡易予約/予約←→無効クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_reverse_Click(object sender, RoutedEventArgs e)
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
        private void cm_CopyTitle_Click(object sender, RoutedEventArgs e)
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
        private void cm_CopyContent_Click(object sender, RoutedEventArgs e)
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
        private void cm_SearchTitle_Click(object sender, RoutedEventArgs e)
        {
            EpgEventInfo program = new EpgEventInfo();
            if (GetProgramItem(clickPos, ref program) == false) return;
            mutil.SearchText(program.Title());
        }

        /// <summary>
        /// 右クリックメニュー 表示モードイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_chg_viewMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (sender.GetType() != typeof(MenuItem))
                {
                    return;
                }
                if (ViewSettingClick != null)
                {
                    MenuItem item = sender as MenuItem;
                    CustomEpgTabInfo setInfo = new CustomEpgTabInfo();
                    setViewInfo.CopyTo(ref setInfo);
                    setInfo.ViewMode = (int)item.DataContext;
                    EpgEventInfo program = new EpgEventInfo();
                    if (GetProgramItem(clickPos, ref program) == true)
                    {
                        SearchItem searchitem = new SearchItem();
                        searchitem.EventInfo = program;
                        BlackoutWindow.SelectedSearchItem = searchitem;
                    }
                    else
                    {
                        BlackoutWindow.SelectedSearchItem = null;
                    }

                    ViewSettingClick(this, setInfo);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 予約変更
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ShowReserveDialog(Point pos)
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

        public void SetViewMode(CustomEpgTabInfo setInfo)
        {
            setViewInfo = setInfo;

            this.viewCustServiceList = setInfo.ViewServiceList;
            this.viewCustContentKindList.Clear();
            if (setInfo.ViewContentKindList != null)
            {
                foreach (UInt16 val in setInfo.ViewContentKindList)
                {
                    this.viewCustContentKindList.Add(val, val);
                }
            }
            this.viewCustNeedTimeOnly = setInfo.NeedTimeOnlyWeek;

            ClearInfo();
            if (ReloadEpgData() == true)
            {
                updateEpgData = false;
                if (ReloadReserveData() == true)//ここはUpdateReserveData()ではダメ
                {
                    updateReserveData = false;
                }
            }
        }

        /// <summary>
        /// EPGデータ更新通知
        /// </summary>
        public void UpdateEpgData()
        {
            updateEpgData = true;
            if (this.IsVisible == true || CommonManager.Instance.NWMode == false)
            {
                ClearInfo();
                if (ReloadEpgData() == true)
                {
                    updateEpgData = false;
                    UpdateReserveData();
                }
            }
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

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (this.IsVisible == true)
            {
                if (updateEpgData == true)
                {
                    ClearInfo();
                    if (ReloadEpgData() == true)
                    {
                        updateEpgData = false;
                        UpdateReserveData();
                    }
                }
                if (updateReserveData == true)
                {
                    if (ReloadReserveData() == true)
                    {
                        updateReserveData = false;
                    }
                }
            }
        }

        private bool ReloadEpgData()
        {
            try
            {
                if (setViewInfo != null)
                {
                    updateEpgData = false;
                    if (ReloadProgramViewItem() == false)
                    {
                        return false;
                    }
                    MoveNowTime();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return true;
        }

        private bool ReloadReserveData()
        {
            if (vutil.ReloadReserveData() == false) return false;

            ReloadReserveViewItem();
            return true;
        }

        /// <summary>
        /// 予約情報の再描画
        /// </summary>
        private void ReloadReserveViewItem()
        {
            reserveList.Clear();
            try
            {
                if (comboBox_service.Items.Count == 0)
                {
                    return;
                }

                UInt64 selectID = 0;
                if (comboBox_service.SelectedItem != null)
                {
                    ComboBoxItem item = comboBox_service.SelectedItem as ComboBoxItem;
                    EpgServiceInfo serviceInfo = item.DataContext as EpgServiceInfo;
                    selectID = serviceInfo.Create64Key();
                }
                else
                {
                    ComboBoxItem item = comboBox_service.Items.GetItemAt(0) as ComboBoxItem;
                    EpgServiceInfo serviceInfo = item.DataContext as EpgServiceInfo;
                    selectID = serviceInfo.Create64Key();
                }

                //TODO: ここでデフォルトマージンを確認するがEpgTimerNWでは無意味。根本的にはSendCtrlCmdの拡張が必要
                int defStartMargin = IniFileHandler.GetPrivateProfileInt("SET", "StartMargin", 0, SettingPath.TimerSrvIniPath);
                int defEndMargin = IniFileHandler.GetPrivateProfileInt("SET", "EndMargin", 0, SettingPath.TimerSrvIniPath);

                foreach (ReserveData info in CommonManager.Instance.DB.ReserveList.Values)
                {
                    UInt64 key = info.Create64Key();
                    if (selectID == key)
                    {
                        DateTime chkStartTime;
                        DateTime startTime;
                        if (info.StartTime.Hour < setViewInfo.StartTimeWeek)
                        {
                            chkStartTime = new DateTime(2001, 1, 2, info.StartTime.Hour, 0, 0);
                            startTime = new DateTime(2001, 1, 2, info.StartTime.Hour, info.StartTime.Minute, info.StartTime.Second);
                        }
                        else
                        {
                            chkStartTime = new DateTime(2001, 1, 1, info.StartTime.Hour, 0, 0);
                            startTime = new DateTime(2001, 1, 1, info.StartTime.Hour, info.StartTime.Minute, info.StartTime.Second);
                        }
                        DateTime baseStartTime = startTime;
                        Int32 duration = (Int32)info.DurationSecond;
                        if (info.RecSetting.UseMargineFlag == 1)
                        {
                            if (info.RecSetting.StartMargine < 0)
                            {
                                startTime = startTime.AddSeconds(info.RecSetting.StartMargine * -1);
                                duration += info.RecSetting.StartMargine;
                            }
                            if (info.RecSetting.EndMargine < 0)
                            {
                                duration += info.RecSetting.EndMargine;
                            }
                        }
                        else
                        {
                            if (defStartMargin < 0)
                            {
                                startTime = startTime.AddSeconds(defStartMargin * -1);
                                duration += defStartMargin;
                            }
                            if (defEndMargin < 0)
                            {
                                duration += defEndMargin;
                            }
                        }
                        DateTime EndTime;
                        EndTime = startTime.AddSeconds(duration);
                        //if ((duration / 60) < Settings.Instance.MinHeight)
                        //{
                        //    duration = (int)Settings.Instance.MinHeight;
                        //}

                        if (timeList.ContainsKey(chkStartTime) == false)
                        {
                            //時間ないので除外
                            continue;
                        }
                        ReserveViewItem viewItem = new ReserveViewItem(info);
                        //viewItem.LeftPos = i * Settings.Instance.ServiceWidth;

                        viewItem.Height = Math.Floor((duration / 60) * Settings.Instance.MinHeight);
                        if (viewItem.Height < Settings.Instance.MinHeight)
                        {
                            viewItem.Height = Settings.Instance.MinHeight;
                        }
                        viewItem.Width = Settings.Instance.ServiceWidth;

                        bool modified = false;
                        if (Settings.Instance.MinimumHeight > 0 && viewItem.ReserveInfo.EventID != 0xFFFF)
                        {
                            //予約情報から番組情報を特定し、枠表示位置を再設定する
                            foreach (ProgramViewItem pgInfo in timeList[chkStartTime])
                            {
                                if (CommonManager.EqualsPg(viewItem.ReserveInfo, pgInfo.EventInfo) == true &&
                                    info.DurationSecond != 0)
                                {
                                    viewItem.TopPos = pgInfo.TopPos + pgInfo.Height * (startTime - baseStartTime).TotalSeconds / info.DurationSecond;
                                    viewItem.Height = Math.Max(pgInfo.Height * duration / info.DurationSecond, Settings.Instance.MinHeight);
                                    modified = true;
                                    break;
                                }
                            }
                        }
                        if (modified == false)
                        {
                            int index = timeList.IndexOfKey(chkStartTime);
                            viewItem.TopPos = index * 60 * Settings.Instance.MinHeight;
                            viewItem.TopPos += Math.Floor((startTime - chkStartTime).TotalMinutes * Settings.Instance.MinHeight);
                        }

                        DateTime chkDay;
                        if (info.StartTime.Hour < setViewInfo.StartTimeWeek)
                        {
                            chkDay = info.StartTime.AddDays(-1);
                            chkDay = new DateTime(chkDay.Year, chkDay.Month, chkDay.Day, 0, 0, 0);
                        }
                        else
                        {
                            chkDay = new DateTime(info.StartTime.Year, info.StartTime.Month, info.StartTime.Day, 0, 0, 0);
                        }
                        viewItem.LeftPos = Settings.Instance.ServiceWidth * dayList.IndexOfKey(chkDay);

                        reserveList.Add(viewItem);
                    }
                }
                epgProgramView.SetReserveList(reserveList);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 番組情報の再描画処理
        /// </summary>
        private bool ReloadProgramViewItem()
        {
            try
            {
                Dictionary<UInt64, EpgServiceEventInfo> serviceEventList = vutil.ReloadEpgDataForEpgView(setViewInfo);
                if (serviceEventList == null) return false;

                if (setViewInfo.SearchMode == true)
                {
                    searchEventList = serviceEventList;
                }

                epgProgramView.ClearInfo();
                timeList.Clear();
                dayList.Clear();
                programList.Clear();
                nowViewTimer.Stop();

                //必要サービスの抽出
                int selectIndex = 0;
                UInt64 selectID = 0;
                if (comboBox_service.SelectedItem != null)
                {
                    ComboBoxItem item = comboBox_service.SelectedItem as ComboBoxItem;
                    EpgServiceInfo serviceInfo = item.DataContext as EpgServiceInfo;
                    selectID = serviceInfo.Create64Key();
                }
                comboBox_service.Items.Clear();

                foreach (UInt64 id in viewCustServiceList)
                {
                    EpgServiceEventInfo serviceInfo;
                    if (serviceEventList.TryGetValue(id, out serviceInfo) == false)
                    {
                        //サービス情報ないので無効
                        continue;
                    }

                    ComboBoxItem item = new ComboBoxItem();
                    item.Content = serviceInfo.serviceInfo.service_name;
                    item.DataContext = serviceInfo.serviceInfo;
                    int index = comboBox_service.Items.Add(item);
                    if (selectID == id || selectID == 0)
                    {
                        selectIndex = index;
                        selectID = id;
                    }
                }
                comboBox_service.SelectedIndex = selectIndex;

                //UpdateProgramView();
                return true;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return false;
            }
        }

        private void UpdateProgramView()
        {
            try
            {
                epgProgramView.ClearInfo();
                timeList.Clear();
                dayList.Clear();
                programList.Clear();

                nowViewTimer.Stop();

                if (comboBox_service.Items.Count == 0)
                {
                    return;
                }

                UInt64 selectID = 0;
                if (comboBox_service.SelectedItem != null)
                {
                    ComboBoxItem item = comboBox_service.SelectedItem as ComboBoxItem;
                    EpgServiceInfo serviceInfo = item.DataContext as EpgServiceInfo;
                    selectID = serviceInfo.Create64Key();
                }
                else
                {
                    ComboBoxItem item = comboBox_service.Items.GetItemAt(0) as ComboBoxItem;
                    EpgServiceInfo serviceInfo = item.DataContext as EpgServiceInfo;
                    selectID = serviceInfo.Create64Key();
                }

                Dictionary<UInt64, EpgServiceEventInfo> serviceEventList = null;
                if (setViewInfo.SearchMode == true)
                {
                    serviceEventList = searchEventList;
                }
                else
                {
                    serviceEventList = CommonManager.Instance.DB.ServiceEventList;
                }

                //まず日時のチェック
                foreach (EpgEventInfo eventInfo in serviceEventList[selectID].eventList)
                {
                    if (eventInfo.StartTimeFlag == 0)
                    {
                        //開始未定は除外
                        continue;
                    }
                    //ジャンル絞り込み
                    if (vutil.ContainsContent(eventInfo, this.viewCustContentKindList) == false)
                    {
                        continue;
                    }
                    ProgramViewItem viewItem = new ProgramViewItem(eventInfo);
                    viewItem.Height = (eventInfo.durationSec * Settings.Instance.MinHeight) / 60;
                    viewItem.Width = Settings.Instance.ServiceWidth;
                    programList.Add(viewItem);

                    //日付列の決定
                    DateTime dayInfo;
                    if (eventInfo.start_time.Hour < setViewInfo.StartTimeWeek)
                    {
                        DateTime time = eventInfo.start_time.AddDays(-1);
                        dayInfo = new DateTime(time.Year, time.Month, time.Day, 0, 0, 0);
                    }
                    else
                    {
                        dayInfo = new DateTime(eventInfo.start_time.Year, eventInfo.start_time.Month, eventInfo.start_time.Day, 0, 0, 0);
                    }

                    if (dayList.ContainsKey(dayInfo) == false)
                    {
                        dayList.Add(dayInfo, dayInfo);
                    }

                    //時間行の決定
                    DateTime chkStartTime;
                    DateTime startTime;
                    if (eventInfo.start_time.Hour < setViewInfo.StartTimeWeek)
                    {
                        chkStartTime = new DateTime(2001, 1, 2, eventInfo.start_time.Hour, 0, 0);
                        startTime = new DateTime(2001, 1, 2, eventInfo.start_time.Hour, eventInfo.start_time.Minute, eventInfo.start_time.Second);
                    }
                    else
                    {
                        chkStartTime = new DateTime(2001, 1, 1, eventInfo.start_time.Hour, 0, 0);
                        startTime = new DateTime(2001, 1, 1, eventInfo.start_time.Hour, eventInfo.start_time.Minute, eventInfo.start_time.Second);
                    }
                    DateTime EndTime;
                    if (eventInfo.DurationFlag == 0)
                    {
                        //終了未定
                        EndTime = startTime.AddSeconds(30 * 10);
                    }
                    else
                    {
                        EndTime = startTime.AddSeconds(eventInfo.durationSec);
                    }

                    while (chkStartTime <= EndTime)
                    {
                        if (timeList.ContainsKey(chkStartTime) == false)
                        {
                            timeList.Add(chkStartTime, new List<ProgramViewItem>());
                        }
                        chkStartTime = chkStartTime.AddHours(1);
                    }
                }

                //必要時間のチェック
                if (viewCustNeedTimeOnly == false)
                {
                    //番組のない時間帯を追加
                    DateTime chkStartTime = new DateTime(2001, 1, 1, setViewInfo.StartTimeWeek, 0, 0);
                    DateTime chkEndTime = new DateTime(2001, 1, 2, setViewInfo.StartTimeWeek, 0, 0);
                    while (chkStartTime < chkEndTime)
                    {
                        if (timeList.ContainsKey(chkStartTime) == false)
                        {
                            timeList.Add(chkStartTime, new List<ProgramViewItem>());
                        }
                        chkStartTime = chkStartTime.AddHours(1);
                    }
                }

                //番組の表示位置設定
                foreach (ProgramViewItem item in programList)
                {
                    DateTime chkStartTime;
                    DateTime startTime;
                    DateTime dayInfo;
                    if (item.EventInfo.start_time.Hour < setViewInfo.StartTimeWeek)
                    {
                        chkStartTime = new DateTime(2001, 1, 2, item.EventInfo.start_time.Hour, 0, 0);
                        startTime = new DateTime(2001, 1, 2, item.EventInfo.start_time.Hour, item.EventInfo.start_time.Minute, item.EventInfo.start_time.Second);
                        DateTime tmp = item.EventInfo.start_time.AddDays(-1);
                        dayInfo = new DateTime(tmp.Year, tmp.Month, tmp.Day, 0, 0, 0);
                    }
                    else
                    {
                        chkStartTime = new DateTime(2001, 1, 1, item.EventInfo.start_time.Hour, 0, 0);
                        startTime = new DateTime(2001, 1, 1, item.EventInfo.start_time.Hour, item.EventInfo.start_time.Minute, item.EventInfo.start_time.Second);
                        dayInfo = new DateTime(item.EventInfo.start_time.Year, item.EventInfo.start_time.Month, item.EventInfo.start_time.Day, 0, 0, 0);
                    }
                    if (timeList.ContainsKey(chkStartTime) == true)
                    {
                        int index = timeList.IndexOfKey(chkStartTime);
                        item.TopPos = (index * 60 + (startTime - chkStartTime).TotalMinutes) * Settings.Instance.MinHeight;
                    }
                    if (dayList.ContainsKey(dayInfo) == true)
                    {
                        int index = dayList.IndexOfKey(dayInfo);
                        item.LeftPos = index * Settings.Instance.ServiceWidth;
                    }
                }
                if (Settings.Instance.MinimumHeight > 0)
                {
                    //最低表示行数を適用
                    programList.Sort((x, y) => Math.Sign(x.LeftPos - y.LeftPos) * 2 + Math.Sign(x.TopPos - y.TopPos));
                    double minimum = (Settings.Instance.FontSizeTitle + 2) * Settings.Instance.MinimumHeight;
                    double lastLeft = double.MinValue;
                    double lastBottom = 0;
                    foreach (ProgramViewItem item in programList)
                    {
                        if (lastLeft != item.LeftPos)
                        {
                            lastLeft = item.LeftPos;
                            lastBottom = double.MinValue;
                        }
                        item.Height = Math.Max(item.Height, minimum);
                        if (item.TopPos < lastBottom)
                        {
                            item.Height = Math.Max(item.TopPos + item.Height - lastBottom, minimum);
                            item.TopPos = lastBottom;
                        }
                        lastBottom = item.TopPos + item.Height;
                    }
                }

                //必要時間リストと時間と番組の関連づけ
                foreach (ProgramViewItem item in programList)
                {
                    int index = Math.Max((int)(item.TopPos / (60 * Settings.Instance.MinHeight)), 0);
                    while (index < Math.Min((int)((item.TopPos + item.Height) / (60 * Settings.Instance.MinHeight)) + 1, timeList.Count))
                    {
                        timeList.Values[index++].Add(item);
                    }
                }

                epgProgramView.SetProgramList(
                    programList,
                    timeList,
                    dayList.Count * Settings.Instance.ServiceWidth,
                    timeList.Count * 60 * Settings.Instance.MinHeight);

                List<DateTime> dateTimeList = new List<DateTime>();
                foreach (var item in timeList)
                {
                    dateTimeList.Add(item.Key);
                }
                timeView.SetTime(dateTimeList, viewCustNeedTimeOnly, true);
                weekDayView.SetDay(dayList);

                ReDrawNowLine();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void comboBox_service_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            UpdateProgramView();
            ReloadReserveViewItem();
        }

        private void UserControl_IsVisibleChanged_1(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (this.IsVisible == false) return;
            if (BlackoutWindow.Create64Key() == 0) return;

            // サービス選択
            UInt64 serviceKey_Target1 = BlackoutWindow.Create64Key();

            foreach (ComboBoxItem item in this.comboBox_service.Items)
            {
                EpgServiceInfo serviceInfo = item.DataContext as EpgServiceInfo;
                UInt64 serviceKey_OnTab1 = serviceInfo.Create64Key();
                if (serviceKey_Target1 == serviceKey_OnTab1)
                {
                    this.comboBox_service.SelectedItem = item;
                    break;
                }
            }
            // スクロール
            if (BlackoutWindow.SelectedReserveItem != null)
            {
                foreach (ReserveViewItem reserveViewItem1 in this.reserveList)
                {
                    if (reserveViewItem1.ReserveInfo.ReserveID == BlackoutWindow.SelectedReserveItem.ReserveInfo.ReserveID)
                    {
                        this.epgProgramView.scrollViewer.ScrollToHorizontalOffset(reserveViewItem1.LeftPos - 100);
                        this.epgProgramView.scrollViewer.ScrollToVerticalOffset(reserveViewItem1.TopPos - 100);
                        break;
                    }
                }
                BlackoutWindow.SelectedReserveItem = null;
            }
            else if (BlackoutWindow.SelectedSearchItem != null)
            {
                foreach (ProgramViewItem programViewItem1 in this.programList)
                {
                    if (CommonManager.EqualsPg(programViewItem1.EventInfo, BlackoutWindow.SelectedSearchItem.EventInfo) == true)
                    {
                        this.epgProgramView.scrollViewer.ScrollToHorizontalOffset(programViewItem1.LeftPos - 100);
                        this.epgProgramView.scrollViewer.ScrollToVerticalOffset(programViewItem1.TopPos - 100);
                        break;
                    }
                }
                BlackoutWindow.SelectedSearchItem = null;
            }
        }

        private void button_go_Main_Click(object sender, RoutedEventArgs e)
        {
            CustomEpgTabInfo setInfo = new CustomEpgTabInfo();
            setViewInfo.CopyTo(ref setInfo);
            setInfo.ViewMode = 0;
            ViewSettingClick(this, setInfo);
        }
    }
}

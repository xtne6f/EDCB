using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;
using EpgTimer.EpgView;

namespace EpgTimer
{
    /// <summary>
    /// EpgMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgMainView : UserControl
    {
        public event ViewSettingClickHandler ViewSettingClick = null;

        private CustomEpgTabInfo setViewInfo = null;

        private List<UInt64> viewCustServiceList = null;
        private Dictionary<UInt16, UInt16> viewCustContentKindList = new Dictionary<UInt16, UInt16>();
        private bool viewCustNeedTimeOnly = false;
        private List<EpgServiceInfo> serviceList = new List<EpgServiceInfo>();
        private SortedList<DateTime, List<ProgramViewItem>> timeList = new SortedList<DateTime, List<ProgramViewItem>>();
        private List<ProgramViewItem> programList = new List<ProgramViewItem>();
        private List<ReserveViewItem> reserveList = new List<ReserveViewItem>();
        private Point clickPos;
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private DispatcherTimer nowViewTimer;
        private Line nowLine = null;

        private bool updateEpgData = true;
        private bool updateReserveData = true;

        public EpgMainView()
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
            dateView.TimeButtonClick += new RoutedEventHandler(epgDateView_TimeButtonClick);

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
            serviceView.ClearInfo();
            dateView.ClearInfo();
            timeList.Clear();
            serviceList.Clear();
            programList.Clear();
            reserveList.Clear();

            serviceList = null;
            serviceList = new List<EpgServiceInfo>();
            programList = null;
            programList = new List<ProgramViewItem>();
            reserveList = null;
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
        /// 現在ライン表示
        /// </summary>
        private void ReDrawNowLine()
        {
            try
            {
                nowViewTimer.Stop();
                DateTime nowTime = DateTime.Now;
                if (timeList.Count < 1 || nowTime < timeList.Keys[0])
                {
                    if (nowLine != null)
                    {
                        epgProgramView.canvas.Children.Remove(nowLine);
                    }
                    nowLine = null;
                    return;
                }
                if (nowLine == null)
                {
                    nowLine = new Line();
                    Canvas.SetZIndex(nowLine, 20);
                    nowLine.Stroke = new SolidColorBrush(Colors.Red);
                    //nowLine.StrokeThickness = Settings.Instance.MinHeight * 2;
                    //nowLine.Opacity = 0.5;
                    nowLine.StrokeThickness = 3;
                    nowLine.Opacity = 0.7;
                    nowLine.Effect = new System.Windows.Media.Effects.DropShadowEffect() { BlurRadius = 10 };
                    nowLine.IsHitTestVisible = false;
                    epgProgramView.canvas.Children.Add(nowLine);
                }

                double posY = 0;
                DateTime chkNowTime = new DateTime(nowTime.Year, nowTime.Month, nowTime.Day, nowTime.Hour, 0, 0);
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
            try
            {
                if (sender.GetType() == typeof(ProgramView))
                {
                    //時間軸の表示もスクロール
                    timeView.scrollViewer.ScrollToVerticalOffset(epgProgramView.scrollViewer.VerticalOffset);
                    //サービス名表示もスクロール
                    serviceView.scrollViewer.ScrollToHorizontalOffset(epgProgramView.scrollViewer.HorizontalOffset);
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
        void epgProgramView_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            try
            {
                e.Handled = true;
                if (sender.GetType() == typeof(ProgramView))
                {
                    ProgramView view = sender as ProgramView;
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
        /// マウス位置から番組情報を取得する
        /// </summary>
        /// <param name="cursorPos">[IN]マウス位置</param>
        /// <param name="program">[OUT]番組情報</param>
        /// <returns>falseで存在しない</returns>
        private bool GetProgramItem(Point cursorPos, ref EpgEventInfo program)
        {
            try
            {
                int timeIndex = (int)(cursorPos.Y / (60 * Settings.Instance.MinHeight));
                if (0 <= timeIndex && timeIndex < timeList.Count)
                {
                    foreach (ProgramViewItem pgInfo in timeList.Values[timeIndex])
                    {
                        if (pgInfo.LeftPos <= cursorPos.X && cursorPos.X < pgInfo.LeftPos + pgInfo.Width &&
                            pgInfo.TopPos <= cursorPos.Y && cursorPos.Y < pgInfo.TopPos + pgInfo.Height)
                        {
                            program = pgInfo.EventInfo;
                            return true;
                        }
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
        void epgProgramView_LeftDoubleClick(object sender, Point cursorPos)
        {
            ShowReserveDialog(cursorPos);
        }

        private void button_erea_MouseRightButtonUp(object sender, MouseButtonEventArgs e)
        {
            epgProgramView_RightClick(sender, new Point(-1,-1));
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
                bool addMode = false;
                if (GetReserveItem(cursorPos, ref reserve) == false)
                {
                    if (GetProgramItem(cursorPos, ref program) == false)
                    {
                        noItem = true;
                    }
                    addMode = true;
                }
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
                MenuItem menuItemView = mutil.GenerateViewMenu(0, new Action<object,
                    RoutedEventArgs>[] { cm_viewSet_Click, cm_chg_viewMode_Click});

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
                    if (addMode == false)
                    {
                        menuItemReverse.Header = "予約←→無効";
                        menuItemReverse.IsEnabled = true;
                        menuItemAdd.IsEnabled = false;
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
                        BlackoutWindow.selectedSearchItem = searchitem;
                    }
                    else
                    {
                        BlackoutWindow.selectedSearchItem = null;
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
        /// ダイアログ表示
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

        /// <summary>
        /// 表示位置変更
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void epgDateView_TimeButtonClick(object sender, RoutedEventArgs e)
        {
            try
            {
                Button timeButton = sender as Button;

                DateTime time = (DateTime)timeButton.DataContext;
                if (timeList.Count < 1 || time < timeList.Keys[0])
                {
                    epgProgramView.scrollViewer.ScrollToVerticalOffset(0);
                }
                else
                {
                    for (int i = 0; i < timeList.Count; i++)
                    {
                        if (time <= timeList.Keys[i])
                        {
                            epgProgramView.scrollViewer.ScrollToVerticalOffset(Math.Ceiling(i * 60 * Settings.Instance.MinHeight));
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
            this.viewCustNeedTimeOnly = setInfo.NeedTimeOnlyBasic;

            ClearInfo();
            if (ReloadEpgData() == true)
            {
                updateEpgData = false;
                if (ReloadReserveData() == true)
                {
                    updateReserveData = false;
                }
            }
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

                DateTime time = DateTime.Now;
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
                        if (ReloadReserveData() == true)
                        {
                            updateReserveData = false;
                        }
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
                    if (setViewInfo.SearchMode == true)
                    {
                        ReloadProgramViewItemForSearch();
                    }
                    else
                    {
                        if (CommonManager.Instance.NWMode == true)
                        {
                            if (CommonManager.Instance.NW.IsConnected == false)
                            {
                                return false;
                            }
                        }
                        ErrCode err = CommonManager.Instance.DB.ReloadEpgData();
                        if (CommonManager.CmdErrMsgTypical(err, "EPGデータの取得", this) == false)
                        {
                            return false;
                        }

                        ReloadProgramViewItem();

                    }
                    MoveNowTime();
                }
            }
            catch (Exception ex)
            {
                this.Dispatcher.BeginInvoke(new Action(() =>
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }), null);
                return false;
            }
            return true;
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
                if (CommonManager.CmdErrMsgTypical(err, "予約情報の取得") == false)
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
                    if (ReloadReserveData() == true)
                    {
                        updateReserveData = false;
                    }
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

        /// <summary>
        /// 予約情報の再描画
        /// </summary>
        private void ReloadReserveViewItem()
        {
            reserveList.Clear();
            reserveList = null;
            reserveList = new List<ReserveViewItem>();
            try
            {
                //TODO: ここでデフォルトマージンを確認するがEpgTimerNWでは無意味。根本的にはSendCtrlCmdの拡張が必要
                int defStartMargin = IniFileHandler.GetPrivateProfileInt("SET", "StartMargin", 0, SettingPath.TimerSrvIniPath);
                int defEndMargin = IniFileHandler.GetPrivateProfileInt("SET", "EndMargin", 0, SettingPath.TimerSrvIniPath);

                foreach (ReserveData info in CommonManager.Instance.DB.ReserveList.Values)
                {
                    {
                        int mergePos = 0;
                        int mergeNum = 0;
                        int servicePos = -1;
                        for (int i = 0; i < serviceList.Count; i++)
                        {
                            //TSIDが同じでSIDが逆順に登録されているときは併合する
                            if (--mergePos < i - mergeNum)
                            {
                                EpgServiceInfo curr = serviceList[i];
                                for (mergePos = i; mergePos + 1 < serviceList.Count; mergePos++)
                                {
                                    EpgServiceInfo next = serviceList[mergePos + 1];
                                    if (next.ONID != curr.ONID || next.TSID != curr.TSID || next.SID >= curr.SID)
                                    {
                                        break;
                                    }
                                    curr = next;
                                }
                                mergeNum = mergePos + 1 - i;
                                servicePos++;
                            }
                            EpgServiceInfo srvInfo = serviceList[mergePos];
                            if (srvInfo.Create64Key() == info.Create64Key())
                            {
                                ReserveViewItem viewItem = new ReserveViewItem(info);
                                viewItem.LeftPos = Settings.Instance.ServiceWidth * (servicePos + (double)((mergeNum + i - mergePos - 1) / 2) / mergeNum);

                                Int32 duration = (Int32)info.DurationSecond;
                                DateTime startTime = info.StartTime;
                                if (info.RecSetting.UseMargineFlag == 1)
                                {
                                    if (info.RecSetting.StartMargine < 0)
                                    {
                                        startTime = info.StartTime.AddSeconds(info.RecSetting.StartMargine * -1);
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
                                        startTime = info.StartTime.AddSeconds(defStartMargin * -1);
                                        duration += defStartMargin;
                                    }
                                    if (defEndMargin < 0)
                                    {
                                        duration += defEndMargin;
                                    }
                                }
                                //if ((duration/60) < Settings.Instance.MinHeight)
                                //{
                                //    duration = (int)Settings.Instance.MinHeight;
                                //}

                                //TimePosInfo topTime = timeList.GetByIndex(0) as TimePosInfo;
                                //viewItem.TopPos = Math.Floor((startTime - topTime.Time).TotalMinutes * Settings.Instance.MinHeight);

                                viewItem.Height = Math.Floor((duration / 60) * Settings.Instance.MinHeight);
                                if (viewItem.Height < Settings.Instance.MinHeight)
                                {
                                    viewItem.Height = Settings.Instance.MinHeight;
                                }
                                viewItem.Width = Settings.Instance.ServiceWidth / mergeNum;

                                reserveList.Add(viewItem);
                                DateTime chkTime = new DateTime(startTime.Year, startTime.Month, startTime.Day, startTime.Hour, 0, 0);
                                if (timeList.ContainsKey(chkTime) == true)
                                {
                                    bool modified = false;
                                    if (Settings.Instance.MinimumHeight > 0 && viewItem.ReserveInfo.EventID != 0xFFFF)
                                    {
                                        //予約情報から番組情報を特定し、枠表示位置を再設定する
                                        foreach (ProgramViewItem pgInfo in timeList[chkTime])
                                        {
                                            if (CommonManager.EqualsPg(viewItem.ReserveInfo, pgInfo.EventInfo) == true &&
                                                info.DurationSecond != 0)
                                            {
                                                viewItem.TopPos = pgInfo.TopPos + pgInfo.Height * (startTime - info.StartTime).TotalSeconds / info.DurationSecond;
                                                viewItem.Width = pgInfo.Width;
                                                viewItem.Height = Math.Max(pgInfo.Height * duration / info.DurationSecond, Settings.Instance.MinHeight);
                                                modified = true;
                                                break;
                                            }
                                        }
                                    }
                                    if (modified == false)
                                    {
                                        int index = timeList.IndexOfKey(chkTime);
                                        viewItem.TopPos = index * 60 * Settings.Instance.MinHeight;
                                        viewItem.TopPos += Math.Floor((startTime - chkTime).TotalMinutes * Settings.Instance.MinHeight);
                                        foreach (ProgramViewItem pgInfo in timeList.Values[index])
                                        {
                                            if (pgInfo.LeftPos == viewItem.LeftPos && pgInfo.TopPos <= viewItem.TopPos && viewItem.TopPos < pgInfo.TopPos + pgInfo.Height)
                                            {
                                                viewItem.Width = pgInfo.Width;
                                                break;
                                            }
                                        }
                                    }
                                }

                                break;
                            }
                        }
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
        private void ReloadProgramViewItem()
        {
            try
            {
                epgProgramView.ClearInfo();
                timeList.Clear();
                programList.Clear();
                programList = null;
                programList = new List<ProgramViewItem>();
                nowViewTimer.Stop();

                //必要サービスの抽出
                serviceList.Clear();

                foreach (UInt64 id in viewCustServiceList)
                {
                    if (CommonManager.Instance.DB.ServiceEventList.ContainsKey(id) == true)
                    {
                        EpgServiceInfo serviceInfo = CommonManager.Instance.DB.ServiceEventList[id].serviceInfo;
                        if (serviceList.Exists(i => i.Create64Key() == serviceInfo.Create64Key()) == false)
                        {
                            serviceList.Add(serviceInfo);
                        }
                    }
                }


                //必要番組の抽出と時間チェック
                List<EpgServiceInfo> primeServiceList = new List<EpgServiceInfo>();
                int mergePos = 0;
                int mergeNum = 0;
                int servicePos = -1;
                for (int i = 0; i < serviceList.Count; i++)
                {
                    //TSIDが同じでSIDが逆順に登録されているときは併合する
                    int spanCheckNum = 1;
                    if (--mergePos < i - mergeNum)
                    {
                        EpgServiceInfo curr = serviceList[i];
                        for (mergePos = i; mergePos + 1 < serviceList.Count; mergePos++)
                        {
                            EpgServiceInfo next = serviceList[mergePos + 1];
                            if (next.ONID != curr.ONID || next.TSID != curr.TSID || next.SID >= curr.SID)
                            {
                                break;
                            }
                            curr = next;
                        }
                        mergeNum = mergePos + 1 - i;
                        servicePos++;
                        //正順のときは貫きチェックするサービス数を調べる
                        for (; mergeNum == 1 && i + spanCheckNum < serviceList.Count; spanCheckNum++)
                        {
                            EpgServiceInfo next = serviceList[i + spanCheckNum];
                            if (next.ONID != curr.ONID || next.TSID != curr.TSID)
                            {
                                break;
                            }
                            else if (next.SID < curr.SID)
                            {
                                spanCheckNum--;
                                break;
                            }
                            curr = next;
                        }
                        primeServiceList.Add(serviceList[mergePos]);
                    }

                    EpgServiceInfo serviceInfo = serviceList[mergePos];
                    UInt64 id = CommonManager.Create64Key(serviceInfo.ONID, serviceInfo.TSID, serviceInfo.SID);
                    foreach (EpgEventInfo eventInfo in CommonManager.Instance.DB.ServiceEventList[id].eventList)
                    {
                        if (eventInfo.StartTimeFlag == 0)
                        {
                            //開始未定は除外
                            continue;
                        }
                        //ジャンル絞り込み
                        if (this.viewCustContentKindList.Count > 0)
                        {
                            bool find = false;
                            if (eventInfo.ContentInfo != null)
                            {
                                if (eventInfo.ContentInfo.nibbleList.Count > 0)
                                {
                                    foreach (EpgContentData contentInfo in eventInfo.ContentInfo.nibbleList)
                                    {
                                        UInt16 ID1 = (UInt16)(((UInt16)contentInfo.content_nibble_level_1) << 8 | 0xFF);
                                        UInt16 ID2 = (UInt16)(((UInt16)contentInfo.content_nibble_level_1) << 8 | contentInfo.content_nibble_level_2);
                                        if (ID2 == 0x0e01)
                                        {
                                            ID1 = (UInt16)(((UInt16)contentInfo.user_nibble_1) << 8 | 0x70FF);
                                            ID2 = (UInt16)(((UInt16)contentInfo.user_nibble_1) << 8 | 0x7000 | contentInfo.user_nibble_2);
                                        }
                                        if (this.viewCustContentKindList.ContainsKey(ID1) == true)
                                        {
                                            find = true;
                                            break;
                                        }
                                        else if (this.viewCustContentKindList.ContainsKey(ID2) == true)
                                        {
                                            find = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (find == false)
                            {
                                //ジャンル見つからないので除外
                                continue;
                            }
                        }
                        //イベントグループのチェック
                        int widthSpan = 1;
                        if (eventInfo.EventGroupInfo != null)
                        {
                            bool spanFlag = false;
                            foreach (EpgEventData data in eventInfo.EventGroupInfo.eventDataList)
                            {
                                if (serviceInfo.Create64Key() == data.Create64Key())
                                {
                                    spanFlag = true;
                                    break;
                                }
                            }

                            if (spanFlag == false)
                            {
                                //サービス２やサービス３の結合されるべきもの
                                continue;
                            }
                            else
                            {
                                //横にどれだけ貫くかチェック
                                int count = 1;
                                while (mergeNum == 1 ? count < spanCheckNum : count < mergeNum - (mergeNum+i-mergePos-1)/2)
                                {
                                    EpgServiceInfo nextInfo = serviceList[mergeNum == 1 ? i + count : mergePos - count];
                                    bool findNext = false;
                                    foreach (EpgEventData data in eventInfo.EventGroupInfo.eventDataList)
                                    {
                                        if (nextInfo.Create64Key() == data.Create64Key())
                                        {
                                            widthSpan++;
                                            findNext = true;
                                        }
                                    }
                                    if (findNext == false)
                                    {
                                        break;
                                    }
                                    count++;
                                }
                            }
                        }

                        ProgramViewItem viewItem = new ProgramViewItem(eventInfo);
                        viewItem.Height = (eventInfo.durationSec * Settings.Instance.MinHeight) / 60;
                        viewItem.Width = Settings.Instance.ServiceWidth * widthSpan / mergeNum;
                        viewItem.LeftPos = Settings.Instance.ServiceWidth * (servicePos + (double)((mergeNum+i-mergePos-1)/2) / mergeNum);
                        //viewItem.TopPos = (eventInfo.start_time - startTime).TotalMinutes * Settings.Instance.MinHeight;
                        programList.Add(viewItem);

                        //日付チェック
                        DateTime EndTime;
                        if (eventInfo.DurationFlag == 0)
                        {
                            //終了未定
                            EndTime = eventInfo.start_time.AddSeconds(30 * 10);
                        }
                        else
                        {
                            EndTime = eventInfo.start_time.AddSeconds(eventInfo.durationSec);
                        }
                        //必要時間リストの構築
                        DateTime chkStartTime = new DateTime(eventInfo.start_time.Year, eventInfo.start_time.Month, eventInfo.start_time.Day, eventInfo.start_time.Hour, 0, 0);
                        while (chkStartTime <= EndTime)
                        {
                            if (timeList.ContainsKey(chkStartTime) == false)
                            {
                                timeList.Add(chkStartTime, new List<ProgramViewItem>());
                            }
                            chkStartTime = chkStartTime.AddHours(1);
                        }
                    }
                }

                //必要時間のチェック
                if (viewCustNeedTimeOnly == false)
                {
                    //番組のない時間帯を追加
                    for (int i = 1; i < timeList.Count; i++)
                    {
                        if (timeList.Keys[i] > timeList.Keys[i - 1].AddHours(1))
                        {
                            timeList.Add(timeList.Keys[i - 1].AddHours(1), new List<ProgramViewItem>());
                        }
                    }

                    //番組の表示位置設定
                    foreach (ProgramViewItem item in programList)
                    {
                        item.TopPos = (item.EventInfo.start_time - timeList.Keys[0]).TotalMinutes * Settings.Instance.MinHeight;
                    }
                }
                else
                {
                    //番組の表示位置設定
                    foreach (ProgramViewItem item in programList)
                    {
                        DateTime chkStartTime = new DateTime(item.EventInfo.start_time.Year,
                            item.EventInfo.start_time.Month,
                            item.EventInfo.start_time.Day,
                            item.EventInfo.start_time.Hour,
                            0,
                            0);
                        if (timeList.ContainsKey(chkStartTime) == true)
                        {
                            int index = timeList.IndexOfKey(chkStartTime);
                            item.TopPos = (index * 60 + (item.EventInfo.start_time - chkStartTime).TotalMinutes) * Settings.Instance.MinHeight;
                        }
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
                    primeServiceList.Count() * Settings.Instance.ServiceWidth,
                    timeList.Count * 60 * Settings.Instance.MinHeight);

                List<DateTime> dateTimeList = new List<DateTime>();
                foreach (var item in timeList)
                {
                    dateTimeList.Add(item.Key);
                }
                timeView.SetTime(dateTimeList, viewCustNeedTimeOnly, false);
                dateView.SetTime(dateTimeList);
                serviceView.SetService(primeServiceList);

                ReDrawNowLine();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 番組情報の再描画処理
        /// </summary>
        private void ReloadProgramViewItemForSearch()
        {
            try
            {
                epgProgramView.ClearInfo();
                timeList.Clear();
                programList.Clear();
                nowViewTimer.Stop();

                serviceList.Clear();

                //番組情報の検索
                List<EpgSearchKeyInfo> keyList = new List<EpgSearchKeyInfo>();
                keyList.Add(setViewInfo.SearchKey);
                List<EpgEventInfo> list = new List<EpgEventInfo>();

                cmd.SendSearchPg(keyList, ref list);

                //サービス毎のリストに変換
                Dictionary<UInt64, EpgServiceEventInfo> serviceEventList = new Dictionary<UInt64, EpgServiceEventInfo>();
                foreach (EpgEventInfo eventInfo in list)
                {
                    UInt64 id = CommonManager.Create64Key(eventInfo.original_network_id, eventInfo.transport_stream_id, eventInfo.service_id);
                    EpgServiceEventInfo serviceInfo = null;
                    if (serviceEventList.ContainsKey(id) == false)
                    {
                        if (ChSet5.Instance.ChList.ContainsKey(id) == false)
                        {
                            //サービス情報ないので無効
                            continue;
                        }
                        serviceInfo = new EpgServiceEventInfo();
                        serviceInfo.serviceInfo = CommonManager.ConvertChSet5To(ChSet5.Instance.ChList[id]);

                        serviceEventList.Add(id, serviceInfo);
                    }
                    else
                    {
                        serviceInfo = serviceEventList[id];
                    }
                    serviceInfo.eventList.Add(eventInfo);
                }


                foreach (UInt64 id in viewCustServiceList)
                {
                    if (serviceEventList.ContainsKey(id) == true)
                    {
                        EpgServiceInfo serviceInfo = serviceEventList[id].serviceInfo;
                        if (serviceList.Exists(i => i.Create64Key() == serviceInfo.Create64Key()) == false)
                        {
                            serviceList.Add(serviceInfo);
                        }
                    }
                }


                //必要番組の抽出と時間チェック
                List<EpgServiceInfo> primeServiceList = new List<EpgServiceInfo>();
                int mergePos = 0;
                int mergeNum = 0;
                int servicePos = -1;
                for (int i = 0; i < serviceList.Count; i++)
                {
                    //TSIDが同じでSIDが逆順に登録されているときは併合する
                    int spanCheckNum = 1;
                    if (--mergePos < i - mergeNum)
                    {
                        EpgServiceInfo curr = serviceList[i];
                        for (mergePos = i; mergePos + 1 < serviceList.Count; mergePos++)
                        {
                            EpgServiceInfo next = serviceList[mergePos + 1];
                            if (next.ONID != curr.ONID || next.TSID != curr.TSID || next.SID >= curr.SID)
                            {
                                break;
                            }
                            curr = next;
                        }
                        mergeNum = mergePos + 1 - i;
                        servicePos++;
                        //正順のときは貫きチェックするサービス数を調べる
                        for (; mergeNum == 1 && i + spanCheckNum < serviceList.Count; spanCheckNum++)
                        {
                            EpgServiceInfo next = serviceList[i + spanCheckNum];
                            if (next.ONID != curr.ONID || next.TSID != curr.TSID)
                            {
                                break;
                            }
                            else if (next.SID < curr.SID)
                            {
                                spanCheckNum--;
                                break;
                            }
                            curr = next;
                        }
                        primeServiceList.Add(serviceList[mergePos]);
                    }

                    EpgServiceInfo serviceInfo = serviceList[mergePos];
                    UInt64 id = CommonManager.Create64Key(serviceInfo.ONID, serviceInfo.TSID, serviceInfo.SID);
                    foreach (EpgEventInfo eventInfo in serviceEventList[id].eventList)
                    {
                        if (eventInfo.StartTimeFlag == 0)
                        {
                            //開始未定は除外
                            continue;
                        }
                        //ジャンル絞り込み
                        if (this.viewCustContentKindList.Count > 0)
                        {
                            bool find = false;
                            if (eventInfo.ContentInfo != null)
                            {
                                if (eventInfo.ContentInfo.nibbleList.Count > 0)
                                {
                                    foreach (EpgContentData contentInfo in eventInfo.ContentInfo.nibbleList)
                                    {
                                        UInt16 ID1 = (UInt16)(((UInt16)contentInfo.content_nibble_level_1) << 8 | 0xFF);
                                        UInt16 ID2 = (UInt16)(((UInt16)contentInfo.content_nibble_level_1) << 8 | contentInfo.content_nibble_level_2);
                                        if (ID2 == 0x0e01)
                                        {
                                            ID1 = (UInt16)(((UInt16)contentInfo.user_nibble_1) << 8 | 0x70FF);
                                            ID2 = (UInt16)(((UInt16)contentInfo.user_nibble_1) << 8 | 0x7000 | contentInfo.user_nibble_2);
                                        }
                                        if (this.viewCustContentKindList.ContainsKey(ID1) == true)
                                        {
                                            find = true;
                                            break;
                                        }
                                        else if (this.viewCustContentKindList.ContainsKey(ID2) == true)
                                        {
                                            find = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (find == false)
                            {
                                //ジャンル見つからないので除外
                                continue;
                            }
                        }
                        //イベントグループのチェック
                        int widthSpan = 1;
                        if (eventInfo.EventGroupInfo != null)
                        {
                            bool spanFlag = false;
                            foreach (EpgEventData data in eventInfo.EventGroupInfo.eventDataList)
                            {
                                if (serviceInfo.Create64Key() == data.Create64Key())
                                {
                                    spanFlag = true;
                                    break;
                                }
                            }

                            if (spanFlag == false)
                            {
                                //サービス２やサービス３の結合されるべきもの
                                continue;
                            }
                            else
                            {
                                //横にどれだけ貫くかチェック
                                int count = 1;
                                while (mergeNum == 1 ? count < spanCheckNum : count < mergeNum - (mergeNum+i-mergePos-1)/2)
                                {
                                    EpgServiceInfo nextInfo = serviceList[mergeNum == 1 ? i + count : mergePos - count];
                                    bool findNext = false;
                                    foreach (EpgEventData data in eventInfo.EventGroupInfo.eventDataList)
                                    {
                                        if (nextInfo.Create64Key() == data.Create64Key())
                                        {
                                            widthSpan++;
                                            findNext = true;
                                        }
                                    }
                                    if (findNext == false)
                                    {
                                        break;
                                    }
                                    count++;
                                }
                            }
                        }

                        ProgramViewItem viewItem = new ProgramViewItem(eventInfo);
                        viewItem.Height = (eventInfo.durationSec * Settings.Instance.MinHeight) / 60;
                        viewItem.Width = Settings.Instance.ServiceWidth * widthSpan / mergeNum;
                        viewItem.LeftPos = Settings.Instance.ServiceWidth * (servicePos + (double)((mergeNum+i-mergePos-1)/2) / mergeNum);
                        //viewItem.TopPos = (eventInfo.start_time - startTime).TotalMinutes * Settings.Instance.MinHeight;
                        programList.Add(viewItem);

                        //日付チェック
                        DateTime EndTime;
                        if (eventInfo.DurationFlag == 0)
                        {
                            //終了未定
                            EndTime = eventInfo.start_time.AddSeconds(30 * 10);
                        }
                        else
                        {
                            EndTime = eventInfo.start_time.AddSeconds(eventInfo.durationSec);
                        }
                        //必要時間リストの構築
                        DateTime chkStartTime = new DateTime(eventInfo.start_time.Year, eventInfo.start_time.Month, eventInfo.start_time.Day, eventInfo.start_time.Hour, 0, 0);
                        while (chkStartTime <= EndTime)
                        {
                            if (timeList.ContainsKey(chkStartTime) == false)
                            {
                                timeList.Add(chkStartTime, new List<ProgramViewItem>());
                            }
                            chkStartTime = chkStartTime.AddHours(1);
                        }
                    }
                }

                //必要時間のチェック
                if (viewCustNeedTimeOnly == false)
                {
                    //番組のない時間帯を追加
                    for (int i = 1; i < timeList.Count; i++)
                    {
                        if (timeList.Keys[i] > timeList.Keys[i - 1].AddHours(1))
                        {
                            timeList.Add(timeList.Keys[i - 1].AddHours(1), new List<ProgramViewItem>());
                        }
                    }

                    //番組の表示位置設定
                    foreach (ProgramViewItem item in programList)
                    {
                        item.TopPos = (item.EventInfo.start_time - timeList.Keys[0]).TotalMinutes * Settings.Instance.MinHeight;
                    }
                }
                else
                {
                    //番組の表示位置設定
                    foreach (ProgramViewItem item in programList)
                    {
                        DateTime chkStartTime = new DateTime(item.EventInfo.start_time.Year,
                            item.EventInfo.start_time.Month,
                            item.EventInfo.start_time.Day,
                            item.EventInfo.start_time.Hour,
                            0,
                            0);
                        if (timeList.ContainsKey(chkStartTime) == true)
                        {
                            int index = timeList.IndexOfKey(chkStartTime);
                            item.TopPos = (index * 60 + (item.EventInfo.start_time - chkStartTime).TotalMinutes) * Settings.Instance.MinHeight;
                        }
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
                    primeServiceList.Count() * Settings.Instance.ServiceWidth,
                    timeList.Count * 60 * Settings.Instance.MinHeight);

                List<DateTime> dateTimeList = new List<DateTime>();
                foreach (var item in timeList)
                {
                    dateTimeList.Add(item.Key);
                }
                timeView.SetTime(dateTimeList, viewCustNeedTimeOnly, false);
                dateView.SetTime(dateTimeList);
                serviceView.SetService(primeServiceList);

                ReDrawNowLine();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (this.IsVisible == false) { return; }
            //
            if (BlackoutWindow.selectedReserveItem != null)
            {
                foreach (ReserveViewItem reserveViewItem1 in this.reserveList)
                {
                    if (reserveViewItem1.ReserveInfo.ReserveID == BlackoutWindow.selectedReserveItem.ReserveInfo.ReserveID)
                    {
                        this.epgProgramView.scrollViewer.ScrollToHorizontalOffset(reserveViewItem1.LeftPos - 100);
                        this.epgProgramView.scrollViewer.ScrollToVerticalOffset(reserveViewItem1.TopPos - 100);
                        break;
                    }
                }
                BlackoutWindow.selectedReserveItem = null;
            }
            else if (BlackoutWindow.selectedSearchItem != null)
            {
                foreach (ProgramViewItem programViewItem1 in this.programList)
                {
                    if (CommonManager.EqualsPg(programViewItem1.EventInfo, BlackoutWindow.selectedSearchItem.EventInfo) == true)
                    {
                        this.epgProgramView.scrollViewer.ScrollToHorizontalOffset(programViewItem1.LeftPos - 100);
                        this.epgProgramView.scrollViewer.ScrollToVerticalOffset(programViewItem1.TopPos - 100);
                        break;
                    }
                }
                BlackoutWindow.selectedSearchItem = null;
            }
        }
    }
}

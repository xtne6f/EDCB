using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Collections;
using System.Windows.Threading;

using EpgTimer.EpgView;

namespace EpgTimer
{
    /// <summary>
    /// EpgWeekMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgWeekMainView : UserControl
    {
        public event Action<object, CustomEpgTabInfo, object> ViewModeChangeRequested;
        private object scrollToTarget;

        private CustomEpgTabInfo setViewInfo = null;

        private SortedList<DateTime, List<ProgramViewItem>> timeList = new SortedList<DateTime, List<ProgramViewItem>>();
        private SortedList dayList = new SortedList();
        private List<ReserveViewItem> reserveList = new List<ReserveViewItem>();
        private Point clickPos;
        private DispatcherTimer nowViewTimer;
        private Line nowLine = null;
        private Dictionary<UInt64, EpgServiceAllEventInfo> searchEventList = new Dictionary<UInt64, EpgServiceAllEventInfo>();

        private bool updateEpgData = true;
        private bool updateReserveData = true;

        public EpgWeekMainView(CustomEpgTabInfo setInfo)
        {
            InitializeComponent();

            nowViewTimer = new DispatcherTimer(DispatcherPriority.Normal);
            nowViewTimer.Tick += new EventHandler(WaitReDrawNowLine);
            setViewInfo = setInfo;
        }

        /// <summary>
        /// 保持情報のクリア
        /// </summary>
        public void ClearInfo()
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
            reserveList.Clear();
            searchEventList.Clear();
        }

        public bool HasService(ushort onid, ushort tsid, ushort sid)
        {
            return setViewInfo.ViewServiceList.Contains(CommonManager.Create64Key(onid, tsid, sid));
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
                DateTime nowTime = DateTime.UtcNow.AddHours(9);
                nowTime = new DateTime(2001, 1, nowTime.Hour < setViewInfo.StartTimeWeek ? 2 : 1, nowTime.Hour, nowTime.Minute, nowTime.Second);

                if (nowLine == null)
                {
                    nowLine = new Line();
                    Canvas.SetZIndex(nowLine, 20);
                    nowLine.Stroke = new SolidColorBrush(Colors.Red);
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
                    weekDayView.scrollViewer.ScrollToHorizontalOffset(epgProgramView.scrollViewer.HorizontalOffset);
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

                DateTime time = DateTime.UtcNow.AddHours(9);
                time = new DateTime(2001, 1, time.Hour < setViewInfo.StartTimeWeek ? 2 : 1, time.Hour, time.Minute, time.Second);

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
            try
            {
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
        /// <returns>nullで存在しない</returns>
        private ProgramViewItem GetProgramItem(Point cursorPos)
        {
            {
                int timeIndex = (int)(cursorPos.Y / (60 * Settings.Instance.MinHeight));
                if (0 <= timeIndex && timeIndex < timeList.Count)
                {
                    foreach (ProgramViewItem pgInfo in timeList.Values[timeIndex])
                    {
                        if (pgInfo.LeftPos <= cursorPos.X && cursorPos.X < pgInfo.LeftPos + pgInfo.Width &&
                            pgInfo.TopPos <= cursorPos.Y && cursorPos.Y < pgInfo.TopPos + pgInfo.Height)
                        {
                            return pgInfo;
                        }
                    }
                }
            }
            return null;
        }

        /// <summary>
        /// 左ボタンダブルクリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="cursorPos"></param>
        void epgProgramView_LeftDoubleClick(object sender, Point cursorPos)
        {
            try
            {
                //まず予約情報あるかチェック
                ReserveData reserve = new ReserveData();
                if (GetReserveItem(cursorPos, ref reserve) == true)
                {
                    //予約変更ダイアログ表示
                    ChangeReserve(reserve);
                    return;
                }
                //番組情報あるかチェック
                ProgramViewItem program = GetProgramItem(cursorPos);
                if (program != null)
                {
                    //予約追加ダイアログ表示
                    AddReserve(program.EventInfo, program.Past == false);
                    return;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
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
                ProgramViewItem program = null;
                bool addMode = false;
                if (GetReserveItem(cursorPos, ref reserve) == false)
                {
                    program = GetProgramItem(cursorPos);
                    addMode = true;
                }
                ContextMenu menu = new ContextMenu();

                MenuItem menuItemNew = new MenuItem();
                menuItemNew.Header = "簡易予約";
                menuItemNew.Tag = (uint)0;
                menuItemNew.Click += new RoutedEventHandler(cm_add_preset_Click);

                MenuItem menuItemAdd = new MenuItem();
                menuItemAdd.Header = "予約追加 (_C)";

                MenuItem menuItemAddDlg = new MenuItem();
                menuItemAddDlg.Header = "ダイアログ表示 (_X)";
                menuItemAddDlg.Click += new RoutedEventHandler(cm_add_Click);

                menuItemAdd.Items.Add(menuItemAddDlg);
                menuItemAdd.Items.Add(new Separator());

                foreach (RecPresetItem info in Settings.Instance.RecPresetList)
                {
                    MenuItem menuItem = new MenuItem();
                    menuItem.Header = info.DisplayName;
                    menuItem.Tag = info.ID;
                    menuItem.Click += new RoutedEventHandler(cm_add_preset_Click);
                    menuItem.IsEnabled = program != null && program.Past == false;
                    menuItemAdd.Items.Add(menuItem);
                }

                MenuItem menuItemChg = new MenuItem();
                menuItemChg.Header = "予約変更 (_C)";
                MenuItem menuItemChgDlg = new MenuItem();
                menuItemChgDlg.Header = "ダイアログ表示 (_X)";
                menuItemChgDlg.Click += new RoutedEventHandler(cm_chg_Click);

                menuItemChg.Items.Add(menuItemChgDlg);
                menuItemChg.Items.Add(new Separator());

                for (byte i = 0; i < CommonManager.Instance.RecModeList.Length; i++)
                {
                    MenuItem menuItem = new MenuItem();
                    menuItem.Header = CommonManager.Instance.RecModeList[i] + " (_" + i + ")";
                    menuItem.Tag = i;
                    menuItem.Click += new RoutedEventHandler(cm_chg_recmode_Click);
                    menuItem.IsChecked = i == reserve.RecSetting.RecMode;
                    menuItemChg.Items.Add(menuItem);
                }
                menuItemChg.Items.Add(new Separator());

                MenuItem menuItemChgRecPri = new MenuItem();
                menuItemChgRecPri.Header = "優先度 " + reserve.RecSetting.Priority + " (_E)";

                for (byte i = 1; i <= 5; i++)
                {
                    MenuItem menuItem = new MenuItem();
                    menuItem.Header = "_" + i;
                    menuItem.Tag = i;
                    menuItem.Click += new RoutedEventHandler(cm_chg_priority_Click);
                    menuItem.IsChecked = i == reserve.RecSetting.Priority;
                    menuItemChgRecPri.Items.Add(menuItem);
                }
                menuItemChg.Items.Add(menuItemChgRecPri);

                MenuItem menuItemDel = new MenuItem();
                menuItemDel.Header = "予約削除";
                menuItemDel.Click += new RoutedEventHandler(cm_del_Click);

                MenuItem menuItemAutoAdd = new MenuItem();
                menuItemAutoAdd.Header = "自動予約登録";
                menuItemAutoAdd.Click += new RoutedEventHandler(cm_autoadd_Click);
                MenuItem menuItemTimeshift = new MenuItem();
                menuItemTimeshift.Header = "追っかけ再生 (_P)";
                menuItemTimeshift.Click += new RoutedEventHandler(cm_timeShiftPlay_Click);

                //表示モード
                MenuItem menuItemView = new MenuItem();
                menuItemView.Header = "表示モード (_W)";

                MenuItem menuItemViewSetDlg = new MenuItem();
                menuItemViewSetDlg.Header = "表示設定";
                menuItemViewSetDlg.Click += new RoutedEventHandler(cm_viewSet_Click);

                MenuItem menuItemChgViewMode1 = new MenuItem();
                menuItemChgViewMode1.Header = "標準モード (_1)";
                menuItemChgViewMode1.Tag = 0;
                menuItemChgViewMode1.Click += new RoutedEventHandler(cm_chg_viewMode_Click);
                MenuItem menuItemChgViewMode3 = new MenuItem();
                menuItemChgViewMode3.Header = "リスト表示モード (_3)";
                menuItemChgViewMode3.Tag = 2;
                menuItemChgViewMode3.Click += new RoutedEventHandler(cm_chg_viewMode_Click);

                menuItemView.Items.Add(menuItemChgViewMode1);
                menuItemView.Items.Add(menuItemChgViewMode3);
                menuItemView.Items.Add(new Separator());
                menuItemView.Items.Add(menuItemViewSetDlg);
                if (addMode && program == null)
                {
                    menuItemNew.IsEnabled = false;
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
                        menuItemNew.IsEnabled = false;
                        menuItemAdd.IsEnabled = false;
                        menuItemChg.IsEnabled = true;
                        menuItemDel.IsEnabled = true;
                        menuItemAutoAdd.IsEnabled = true;
                        menuItemTimeshift.IsEnabled = true;
                        menuItemView.IsEnabled = true;
                    }
                    else
                    {
                        menuItemNew.IsEnabled = program.Past == false;
                        menuItemAdd.IsEnabled = true;
                        menuItemChg.IsEnabled = false;
                        menuItemDel.IsEnabled = false;
                        menuItemAutoAdd.IsEnabled = true;
                        menuItemTimeshift.IsEnabled = false;
                        menuItemView.IsEnabled = true;
                    }
                }

                menu.Items.Add(menuItemNew);
                menu.Items.Add(menuItemAdd);
                menu.Items.Add(menuItemChg);
                menu.Items.Add(menuItemDel);
                menu.Items.Add(menuItemAutoAdd);
                menu.Items.Add(menuItemTimeshift);
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
            try
            {
                uint presetID = (uint)((MenuItem)sender).Tag;

                ProgramViewItem program = GetProgramItem(clickPos);
                if (program == null)
                {
                    return;
                }
                EpgEventInfo eventInfo = program.EventInfo;
                if (eventInfo.StartTimeFlag == 0)
                {
                    MessageBox.Show("開始時間未定のため予約できません");
                    return;
                }

                ReserveData reserveInfo = new ReserveData();
                if (eventInfo.ShortInfo != null)
                {
                    reserveInfo.Title = eventInfo.ShortInfo.event_name;
                }

                reserveInfo.StartTime = eventInfo.start_time;
                reserveInfo.StartTimeEpg = eventInfo.start_time;

                if (eventInfo.DurationFlag == 0)
                {
                    reserveInfo.DurationSecond = 10 * 60;
                }
                else
                {
                    reserveInfo.DurationSecond = eventInfo.durationSec;
                }

                UInt64 key = CommonManager.Create64Key(eventInfo.original_network_id, eventInfo.transport_stream_id, eventInfo.service_id);
                if (ChSet5.Instance.ChList.ContainsKey(key) == true)
                {
                    reserveInfo.StationName = ChSet5.Instance.ChList[key].ServiceName;
                }
                reserveInfo.OriginalNetworkID = eventInfo.original_network_id;
                reserveInfo.TransportStreamID = eventInfo.transport_stream_id;
                reserveInfo.ServiceID = eventInfo.service_id;
                reserveInfo.EventID = eventInfo.event_id;

                RecSettingData setInfo = new RecSettingData();
                Settings.GetDefRecSetting(presetID, ref setInfo);
                reserveInfo.RecSetting = setInfo;

                List<ReserveData> list = new List<ReserveData>();
                list.Add(reserveInfo);
                ErrCode err = CommonManager.CreateSrvCtrl().SendAddReserve(list);
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約登録でエラーが発生しました。終了時間がすでに過ぎている可能性があります。");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 右クリックメニュー 予約追加クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_add_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ProgramViewItem program = GetProgramItem(clickPos);
                if (program == null)
                {
                    return;
                }
                AddReserve(program.EventInfo, program.Past == false);
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
            try
            {
                ReserveData reserve = new ReserveData();
                if (GetReserveItem(clickPos, ref reserve) == false)
                {
                    return;
                }
                ChangeReserve(reserve);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 右クリックメニュー 予約削除クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_del_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ReserveData reserve = new ReserveData();
                if (GetReserveItem(clickPos, ref reserve) == false)
                {
                    return;
                }
                List<UInt32> list = new List<UInt32>();
                list.Add(reserve.ReserveID);
                ErrCode err = CommonManager.CreateSrvCtrl().SendDelReserve(list);
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約削除でエラーが発生しました。");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 右クリックメニュー 予約モード変更イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_chg_recmode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ReserveData reserve = new ReserveData();
                if (GetReserveItem(clickPos, ref reserve) == false)
                {
                    return;
                }
                reserve.RecSetting.RecMode = (byte)((MenuItem)sender).Tag;
                List<ReserveData> list = new List<ReserveData>();
                list.Add(reserve);
                ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 右クリックメニュー 優先度変更イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_chg_priority_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ReserveData reserve = new ReserveData();
                if (GetReserveItem(clickPos, ref reserve) == false)
                {
                    return;
                }
                reserve.RecSetting.Priority = (byte)((MenuItem)sender).Tag;
                List<ReserveData> list = new List<ReserveData>();
                list.Add(reserve);
                ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 右クリックメニュー 自動予約登録イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_autoadd_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (sender.GetType() != typeof(MenuItem))
                {
                    return;
                }

                ProgramViewItem programView = GetProgramItem(clickPos);
                if (programView == null)
                {
                    return;
                }
                EpgEventInfo program = programView.EventInfo;

                SearchWindow dlg = new SearchWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetViewMode(1);

                EpgSearchKeyInfo key = new EpgSearchKeyInfo();

                if (program.ShortInfo != null)
                {
                    key.andKey = program.ShortInfo.event_name;
                }
                Int64 sidKey = ((Int64)program.original_network_id) << 32 | ((Int64)program.transport_stream_id) << 16 | ((Int64)program.service_id);
                key.serviceList.Add(sidKey);

                dlg.SetSearchDefKey(key);
                dlg.ShowDialog();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 右クリックメニュー 追っかけ再生イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_timeShiftPlay_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (sender.GetType() != typeof(MenuItem))
                {
                    return;
                }

                ReserveData reserve = new ReserveData();
                if (GetReserveItem(clickPos, ref reserve) == false)
                {
                    return;
                }
                CommonManager.Instance.FilePlay(reserve.ReserveID);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 右クリックメニュー 表示設定イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_viewSet_Click(object sender, RoutedEventArgs e)
        {
            if (Settings.Instance.UseCustomEpgView == false)
            {
                MessageBox.Show("デフォルト表示では設定を変更することはできません。");
            }
            else
            {
                var dlg = new EpgDataViewSettingWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetDefSetting(setViewInfo);
                if (dlg.ShowDialog() == true)
                {
                    var setInfo = new CustomEpgTabInfo();
                    dlg.GetSetting(ref setInfo);
                    if (setInfo.ViewMode == setViewInfo.ViewMode)
                    {
                        setViewInfo = setInfo;
                        UpdateEpgData();
                    }
                    else if (ViewModeChangeRequested != null)
                    {
                        ViewModeChangeRequested(this, setInfo, null);
                    }
                }
            }
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
                if (ViewModeChangeRequested != null)
                {
                    CustomEpgTabInfo setInfo = setViewInfo.DeepClone();
                    setInfo.ViewMode = (int)((MenuItem)sender).Tag;
                    ProgramViewItem program = GetProgramItem(clickPos);
                    ViewModeChangeRequested(this, setInfo, (program != null ? program.EventInfo : null));
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
        private void ChangeReserve(ReserveData reserveInfo)
        {
            try
            {
                ChgReserveWindow dlg = new ChgReserveWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetOpenMode(Settings.Instance.EpgInfoOpenMode);
                dlg.SetReserveInfo(reserveInfo);
                if (dlg.ShowDialog() == true)
                {
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 予約追加
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void AddReserve(EpgEventInfo eventInfo, bool reservable)
        {
            try
            {
                AddReserveEpgWindow dlg = new AddReserveEpgWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetOpenMode(Settings.Instance.EpgInfoOpenMode);
                dlg.SetReservable(reservable);
                dlg.SetEventInfo(eventInfo);
                if (dlg.ShowDialog() == true)
                {
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            var ps = PresentationSource.FromVisual(this);
            if (ps != null)
            {
                //高DPI環境でProgramViewの位置を物理ピクセルに合わせるためにヘッダの幅を微調整する
                //RootにUseLayoutRoundingを適用できれば不要だがボタン等が低品質になるので自力でやる
                Point p = grid_PG.TransformToVisual(ps.RootVisual).Transform(new Point(40, 80));
                Matrix m = ps.CompositionTarget.TransformToDevice;
                grid_PG.ColumnDefinitions[0].Width = new GridLength(40 + Math.Floor(p.X * m.M11) / m.M11 - p.X);
                grid_PG.RowDefinitions[1].Height = new GridLength(40 + Math.Floor(p.Y * m.M22) / m.M22 - p.Y);
            }
        }

        private bool ReloadEpgData()
        {
            try
            {
                //EpgViewPanelがDPI倍率の情報を必要とするため
                if (PresentationSource.FromVisual(Application.Current.MainWindow) != null)
                {
                    if (setViewInfo.SearchMode == true)
                    {
                        //番組情報の検索
                        var list = new List<EpgEventInfo>();
                        CommonManager.CreateSrvCtrl().SendSearchPg(new List<EpgSearchKeyInfo>() { setViewInfo.SearchKey }, ref list);
                        searchEventList.Clear();

                        //サービス毎のリストに変換
                        foreach (EpgEventInfo info in list)
                        {
                            ulong id = CommonManager.Create64Key(info.original_network_id, info.transport_stream_id, info.service_id);
                            if (searchEventList.ContainsKey(id) == false)
                            {
                                if (ChSet5.Instance.ChList.ContainsKey(id) == false)
                                {
                                    //サービス情報ないので無効
                                    continue;
                                }
                                searchEventList.Add(id, new EpgServiceAllEventInfo(CommonManager.ConvertChSet5To(ChSet5.Instance.ChList[id])));
                            }
                            searchEventList[id].eventList.Add(info);
                        }
                        ReloadProgramViewItem(searchEventList);
                    }
                    else
                    {
                        if (CommonManager.Instance.NWMode && CommonManager.Instance.NWConnectedIP == null)
                        {
                            return false;
                        }
                        ErrCode err = CommonManager.Instance.DB.ReloadEpgData();
                        if (err != ErrCode.CMD_SUCCESS)
                        {
                            if (IsVisible && err != ErrCode.CMD_ERR_BUSY)
                            {
                                this.Dispatcher.BeginInvoke(new Action(() =>
                                {
                                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "EPGデータの取得でエラーが発生しました。EPGデータが読み込まれていない可能性があります。");
                                }), null);
                            }
                            return false;
                        }

                        searchEventList.Clear();
                        ReloadProgramViewItem(CommonManager.Instance.DB.ServiceEventList);
                    }
                    MoveNowTime();
                    return true;
                }
            }
            catch (Exception ex)
            {
                Dispatcher.BeginInvoke(new Action(() =>
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }), null);
            }
            return false;
        }

        /// <summary>
        /// EPGデータ更新通知
        /// </summary>
        public void UpdateEpgData()
        {
            ClearInfo();
            updateEpgData = true;
            if (IsVisible || (Settings.Instance.NgAutoEpgLoadNW == false && Settings.Instance.PrebuildEpg))
            {
                if (ReloadEpgData() == true)
                {
                    updateEpgData = false;
                    ReloadReserveViewItem();
                    updateReserveData = false;
                }
            }
        }

        /// <summary>
        /// 予約情報更新通知
        /// </summary>
        public void RefreshReserve()
        {
            updateReserveData = true;
            if (this.IsVisible == true)
            {
                ReloadReserveViewItem();
                updateReserveData = false;
            }
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
                    selectID = CommonManager.Create64Key(serviceInfo.ONID, serviceInfo.TSID, serviceInfo.SID);
                }
                else
                {
                    ComboBoxItem item = comboBox_service.Items.GetItemAt(0) as ComboBoxItem;
                    EpgServiceInfo serviceInfo = item.DataContext as EpgServiceInfo;
                    selectID = CommonManager.Create64Key(serviceInfo.ONID, serviceInfo.TSID, serviceInfo.SID);
                }

                foreach (ReserveData info in CommonManager.Instance.DB.ReserveList.Values)
                {
                    UInt64 key = CommonManager.Create64Key(info.OriginalNetworkID, info.TransportStreamID, info.ServiceID);
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
                        //総時間60秒を下限に縮小方向のマージンを反映させる
                        int startMargin = info.RecSetting.StartMargine;
                        int endMargin = info.RecSetting.EndMargine;
                        if (info.RecSetting.UseMargineFlag == 0)
                        {
                            startMargin = 0;
                            endMargin = 0;
                            if (CommonManager.Instance.DB.DefaultRecSetting != null)
                            {
                                startMargin = CommonManager.Instance.DB.DefaultRecSetting.StartMargine;
                                endMargin = CommonManager.Instance.DB.DefaultRecSetting.EndMargine;
                            }
                        }
                        startMargin = Math.Max(startMargin, -(duration - 60));
                        endMargin = Math.Max(endMargin, -Math.Min(startMargin, 0) - (duration - 60));
                        startTime = startTime.AddSeconds(-Math.Min(startMargin, 0));
                        duration += Math.Min(startMargin, 0) + Math.Min(endMargin, 0);

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
                                if (pgInfo.Past == false &&
                                    viewItem.ReserveInfo.OriginalNetworkID == pgInfo.EventInfo.original_network_id &&
                                    viewItem.ReserveInfo.TransportStreamID == pgInfo.EventInfo.transport_stream_id &&
                                    viewItem.ReserveInfo.ServiceID == pgInfo.EventInfo.service_id &&
                                    viewItem.ReserveInfo.EventID == pgInfo.EventInfo.event_id &&
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
        private void ReloadProgramViewItem(Dictionary<ulong, EpgServiceAllEventInfo> serviceEventList)
        {
            try
            {
                epgProgramView.ClearInfo();
                timeList.Clear();
                dayList.Clear();
                nowViewTimer.Stop();

                //必要サービスの抽出
                int selectIndex = 0;
                UInt64 selectID = 0;
                if (comboBox_service.SelectedItem != null)
                {
                    ComboBoxItem item = comboBox_service.SelectedItem as ComboBoxItem;
                    EpgServiceInfo serviceInfo = item.DataContext as EpgServiceInfo;
                    selectID = CommonManager.Create64Key(serviceInfo.ONID, serviceInfo.TSID, serviceInfo.SID);
                }
                comboBox_service.Items.Clear();

                for (int i = 0; i < setViewInfo.ViewServiceList.Count;)
                {
                    //TSIDが同じでSIDが逆順のときは正順にする
                    int skip = i + 1;
                    while (setViewInfo.ViewServiceList.Count > skip &&
                           setViewInfo.ViewServiceList[skip] >> 16 == setViewInfo.ViewServiceList[skip - 1] >> 16 &&
                           (setViewInfo.ViewServiceList[skip] & 0xFFFF) < (setViewInfo.ViewServiceList[skip - 1] & 0xFFFF))
                    {
                        skip++;
                    }
                    for (int j = skip - 1; j >= i; j--)
                    {
                        ulong id = setViewInfo.ViewServiceList[j];
                        if (serviceEventList.ContainsKey(id))
                        {
                            var item = new ComboBoxItem();
                            item.Content = serviceEventList[id].serviceInfo.service_name;
                            item.DataContext = serviceEventList[id].serviceInfo;
                            comboBox_service.Items.Add(item);
                            if (selectID == id || selectID == 0)
                            {
                                selectIndex = comboBox_service.Items.Count - 1;
                                selectID = id;
                            }
                        }
                    }
                    i = skip;
                }
                comboBox_service.SelectedIndex = selectIndex;

                //UpdateProgramView();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void UpdateProgramView()
        {
            try
            {
                epgProgramView.ClearInfo();
                timeList.Clear();
                dayList.Clear();
                var programList = new List<ProgramViewItem>();

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
                    selectID = CommonManager.Create64Key(serviceInfo.ONID, serviceInfo.TSID, serviceInfo.SID);
                }
                else
                {
                    ComboBoxItem item = comboBox_service.Items.GetItemAt(0) as ComboBoxItem;
                    EpgServiceInfo serviceInfo = item.DataContext as EpgServiceInfo;
                    selectID = CommonManager.Create64Key(serviceInfo.ONID, serviceInfo.TSID, serviceInfo.SID);
                }

                Dictionary<UInt64, EpgServiceAllEventInfo> serviceEventList = null;
                if (setViewInfo.SearchMode == true)
                {
                    serviceEventList = searchEventList;
                }
                else
                {
                    serviceEventList = CommonManager.Instance.DB.ServiceEventList;
                }
                List<ushort> contentKindList = setViewInfo.ViewContentKindList.ToList();
                contentKindList.Sort();

                //まず日時のチェック
                int eventInfoIndex = -1;
                foreach (EpgEventInfo eventInfo in Enumerable.Concat(serviceEventList[selectID].eventArcList, serviceEventList[selectID].eventList))
                {
                    bool past = ++eventInfoIndex < serviceEventList[selectID].eventArcList.Count;
                    if (eventInfo.StartTimeFlag == 0)
                    {
                        //開始未定は除外
                        continue;
                    }
                    //ジャンル絞り込み
                    if (contentKindList.Count > 0)
                    {
                        bool find = false;
                        if (eventInfo.ContentInfo == null || eventInfo.ContentInfo.nibbleList.Count == 0)
                        {
                            //ジャンル情報ない
                            find = contentKindList.BinarySearch(0xFFFF) >= 0;
                        }
                        else
                        {
                            foreach (EpgContentData contentInfo in eventInfo.ContentInfo.nibbleList)
                            {
                                int nibble1 = contentInfo.content_nibble_level_1;
                                int nibble2 = contentInfo.content_nibble_level_2;
                                if (nibble1 == 0x0E && nibble2 <= 0x01)
                                {
                                    nibble1 = contentInfo.user_nibble_1 | (0x60 + nibble2 * 16);
                                    nibble2 = contentInfo.user_nibble_2;
                                }
                                if (contentKindList.BinarySearch((ushort)(nibble1 << 8 | 0xFF)) >= 0 ||
                                    contentKindList.BinarySearch((ushort)(nibble1 << 8 | nibble2)) >= 0)
                                {
                                    find = true;
                                    break;
                                }
                            }
                        }
                        if (find == false)
                        {
                            //ジャンル見つからないので除外
                            continue;
                        }
                    }

                    ProgramViewItem viewItem = new ProgramViewItem(eventInfo, past);
                    viewItem.Height = ((eventInfo.DurationFlag == 0 ? 300 : eventInfo.durationSec) * Settings.Instance.MinHeight) / 60;
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
                if (setViewInfo.NeedTimeOnlyWeek == false)
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
                    double minimum = Math.Floor((Settings.Instance.FontSizeTitle + 2) * Settings.Instance.MinimumHeight);
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
                    dayList.Count * Settings.Instance.ServiceWidth,
                    timeList.Count * 60 * Settings.Instance.MinHeight);

                List<DateTime> dateTimeList = new List<DateTime>();
                foreach (var item in timeList)
                {
                    dateTimeList.Add(item.Key);
                }
                timeView.SetTime(dateTimeList, setViewInfo.NeedTimeOnlyWeek, true);
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

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (this.IsVisible == false) { return; }

            if (updateEpgData && ReloadEpgData())
            {
                updateEpgData = false;
                ReloadReserveViewItem();
                updateReserveData = false;
            }
            if (updateReserveData)
            {
                ReloadReserveViewItem();
                updateReserveData = false;
            }
            if (scrollToTarget != null)
            {
                ScrollTo(scrollToTarget);
            }
        }

        public void ScrollTo(object target)
        {
            scrollToTarget = null;
            if (IsVisible == false)
            {
                //Visibleになるまですこし待つ
                scrollToTarget = target;
                Dispatcher.BeginInvoke(DispatcherPriority.Render, new Action(() => scrollToTarget = null));
                return;
            }
            // サービス選択
            UInt64 serviceKey_Target1 = 0;
            if (target is ReserveData)
            {
                var reserveData1 = (ReserveData)target;
                serviceKey_Target1 = CommonManager.Create64Key(reserveData1.OriginalNetworkID, reserveData1.TransportStreamID, reserveData1.ServiceID);
            }
            else if (target is EpgEventInfo)
            {
                var eventInfo1 = (EpgEventInfo)target;
                serviceKey_Target1 = CommonManager.Create64Key(eventInfo1.original_network_id, eventInfo1.transport_stream_id, eventInfo1.service_id);
            }
            foreach (ComboBoxItem item in this.comboBox_service.Items)
            {
                EpgServiceInfo serviceInfo = item.DataContext as EpgServiceInfo;
                UInt64 serviceKey_OnTab1 = CommonManager.Create64Key(serviceInfo.ONID, serviceInfo.TSID, serviceInfo.SID);
                if (serviceKey_Target1 == serviceKey_OnTab1)
                {
                    this.comboBox_service.SelectedItem = item;
                    break;
                }
            }
            // スクロール
            if (target is ReserveData)
            {
                foreach (ReserveViewItem reserveViewItem1 in this.reserveList)
                {
                    if (reserveViewItem1.ReserveInfo.ReserveID == ((ReserveData)target).ReserveID)
                    {
                        this.epgProgramView.scrollViewer.ScrollToHorizontalOffset(reserveViewItem1.LeftPos - 100);
                        this.epgProgramView.scrollViewer.ScrollToVerticalOffset(reserveViewItem1.TopPos - 100);
                        break;
                    }
                }
            }
            else if (target is EpgEventInfo)
            {
                for (int i = 0; i < this.timeList.Count; i++)
                {
                    foreach (ProgramViewItem item in this.timeList.Values[i])
                    {
                        if (item.Past == false &&
                            item.EventInfo.event_id == ((EpgEventInfo)target).event_id &&
                            item.EventInfo.original_network_id == ((EpgEventInfo)target).original_network_id &&
                            item.EventInfo.service_id == ((EpgEventInfo)target).service_id &&
                            item.EventInfo.transport_stream_id == ((EpgEventInfo)target).transport_stream_id)
                        {
                            this.epgProgramView.scrollViewer.ScrollToHorizontalOffset(item.LeftPos - 100);
                            this.epgProgramView.scrollViewer.ScrollToVerticalOffset(item.TopPos - 100);
                            i = this.timeList.Count - 1;
                            break;
                        }
                    }
                }
            }
        }

        private void button1_Click(object sender, RoutedEventArgs e)
        {
            CustomEpgTabInfo setInfo = setViewInfo.DeepClone();
            setInfo.ViewMode = 0;
            if (ViewModeChangeRequested != null)
            {
                ViewModeChangeRequested(this, setInfo, null);
            }
        }
    }
}

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
        private DispatcherTimer nowViewTimer;
        private Line nowLine = null;

        private bool updateEpgData = true;
        private bool updateReserveData = true;

        public EpgMainView()
        {
            InitializeComponent();

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
                EpgEventInfo program = new EpgEventInfo();
                if (GetProgramItem(cursorPos, ref program) == true)
                {
                    //予約追加ダイアログ表示
                    AddReserve(program);
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

                MenuItem menuItemNew = new MenuItem();
                menuItemNew.Header = "簡易予約";
                menuItemNew.Click += new RoutedEventHandler(cm_new_Click);

                Separator separate = new Separator();
                MenuItem menuItemAdd = new MenuItem();
                menuItemAdd.Header = "予約追加 (_C)";

                MenuItem menuItemAddDlg = new MenuItem();
                menuItemAddDlg.Header = "ダイアログ表示 (_X)";
                menuItemAddDlg.Click += new RoutedEventHandler(cm_add_Click);

                menuItemAdd.Items.Add(menuItemAddDlg);
                menuItemAdd.Items.Add(separate);

                MenuItem menuItemPreset = new MenuItem();
                menuItemPreset.Header = "プリセット (_Z)";

                foreach (RecPresetItem info in Settings.Instance.RecPresetList)
                {
                    MenuItem menuItem = new MenuItem();
                    menuItem.Header = info.DisplayName;
                    menuItem.DataContext = info.ID;
                    menuItem.Click += new RoutedEventHandler(cm_add_preset_Click);

                    menuItemPreset.Items.Add(menuItem);
                }

                menuItemAdd.Items.Add(menuItemPreset);

                Separator separate2 = new Separator();
                MenuItem menuItemChg = new MenuItem();
                menuItemChg.Header = "予約変更 (_C)";
                MenuItem menuItemChgDlg = new MenuItem();
                menuItemChgDlg.Header = "ダイアログ表示 (_X)";
                menuItemChgDlg.Click += new RoutedEventHandler(cm_chg_Click);

                menuItemChg.Items.Add(menuItemChgDlg);
                menuItemChg.Items.Add(separate2);

                MenuItem menuItemChgRecMode0 = new MenuItem();
                menuItemChgRecMode0.Header = "全サービス (_0)";
                menuItemChgRecMode0.DataContext = 0;
                menuItemChgRecMode0.Click += new RoutedEventHandler(cm_chg_recmode_Click);
                MenuItem menuItemChgRecMode1 = new MenuItem();
                menuItemChgRecMode1.Header = "指定サービス (_1)";
                menuItemChgRecMode1.DataContext = 1;
                menuItemChgRecMode1.Click += new RoutedEventHandler(cm_chg_recmode_Click);
                MenuItem menuItemChgRecMode2 = new MenuItem();
                menuItemChgRecMode2.Header = "全サービス（デコード処理なし） (_2)";
                menuItemChgRecMode2.DataContext = 2;
                menuItemChgRecMode2.Click += new RoutedEventHandler(cm_chg_recmode_Click);
                MenuItem menuItemChgRecMode3 = new MenuItem();
                menuItemChgRecMode3.Header = "指定サービス（デコード処理なし） (_3)";
                menuItemChgRecMode3.DataContext = 3;
                menuItemChgRecMode3.Click += new RoutedEventHandler(cm_chg_recmode_Click);
                MenuItem menuItemChgRecMode4 = new MenuItem();
                menuItemChgRecMode4.Header = "視聴 (_4)";
                menuItemChgRecMode4.DataContext = 4;
                menuItemChgRecMode4.Click += new RoutedEventHandler(cm_chg_recmode_Click);
                MenuItem menuItemChgRecMode5 = new MenuItem();
                menuItemChgRecMode5.Header = "無効 (_5)";
                menuItemChgRecMode5.DataContext = 5;
                menuItemChgRecMode5.Click += new RoutedEventHandler(cm_chg_recmode_Click);

                menuItemChg.Items.Add(menuItemChgRecMode0);
                menuItemChg.Items.Add(menuItemChgRecMode1);
                menuItemChg.Items.Add(menuItemChgRecMode2);
                menuItemChg.Items.Add(menuItemChgRecMode3);
                menuItemChg.Items.Add(menuItemChgRecMode4);
                menuItemChg.Items.Add(menuItemChgRecMode5);

                menuItemChg.Items.Add(new Separator());

                MenuItem menuItemChgRecPri = new MenuItem();
                menuItemChgRecPri.Tag = "優先度 {0} (_E)";

                MenuItem menuItemChgRecPri1 = new MenuItem();
                menuItemChgRecPri1.Header = "1 (_1)";
                menuItemChgRecPri1.DataContext = 1;
                menuItemChgRecPri1.Click += new RoutedEventHandler(cm_chg_priority_Click);
                MenuItem menuItemChgRecPri2 = new MenuItem();
                menuItemChgRecPri2.Header = "2 (_2)";
                menuItemChgRecPri2.DataContext = 2;
                menuItemChgRecPri2.Click += new RoutedEventHandler(cm_chg_priority_Click);
                MenuItem menuItemChgRecPri3 = new MenuItem();
                menuItemChgRecPri3.Header = "3 (_3)";
                menuItemChgRecPri3.DataContext = 3;
                menuItemChgRecPri3.Click += new RoutedEventHandler(cm_chg_priority_Click);
                MenuItem menuItemChgRecPri4 = new MenuItem();
                menuItemChgRecPri4.Header = "4 (_4)";
                menuItemChgRecPri4.DataContext = 4;
                menuItemChgRecPri4.Click += new RoutedEventHandler(cm_chg_priority_Click);
                MenuItem menuItemChgRecPri5 = new MenuItem();
                menuItemChgRecPri5.Header = "5 (_5)";
                menuItemChgRecPri5.DataContext = 5;
                menuItemChgRecPri5.Click += new RoutedEventHandler(cm_chg_priority_Click);

                menuItemChgRecPri.Items.Add(menuItemChgRecPri1);
                menuItemChgRecPri.Items.Add(menuItemChgRecPri2);
                menuItemChgRecPri.Items.Add(menuItemChgRecPri3);
                menuItemChgRecPri.Items.Add(menuItemChgRecPri4);
                menuItemChgRecPri.Items.Add(menuItemChgRecPri5);

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
                Separator separate3 = new Separator();
                MenuItem menuItemView = new MenuItem();
                menuItemView.Header = "表示モード (_W)";

                MenuItem menuItemViewSetDlg = new MenuItem();
                menuItemViewSetDlg.Header = "表示設定";
                menuItemViewSetDlg.Click += new RoutedEventHandler(cm_viewSet_Click);

                MenuItem menuItemChgViewMode1 = new MenuItem();
                menuItemChgViewMode1.Header = "標準モード (_1)";
                menuItemChgViewMode1.DataContext = 0;
                menuItemChgViewMode1.Click += new RoutedEventHandler(cm_chg_viewMode_Click);
                MenuItem menuItemChgViewMode2 = new MenuItem();
                menuItemChgViewMode2.Header = "1週間モード (_2)";
                menuItemChgViewMode2.DataContext = 1;
                menuItemChgViewMode2.Click += new RoutedEventHandler(cm_chg_viewMode_Click);
                MenuItem menuItemChgViewMode3 = new MenuItem();
                menuItemChgViewMode3.Header = "リスト表示モード (_3)";
                menuItemChgViewMode3.DataContext = 2;
                menuItemChgViewMode3.Click += new RoutedEventHandler(cm_chg_viewMode_Click);

                if (setViewInfo.ViewMode == 1)
                {
                    menuItemChgViewMode2.IsChecked = true;
                }
                else if (setViewInfo.ViewMode == 2)
                {
                    menuItemChgViewMode3.IsChecked = true;
                }
                else
                {
                    menuItemChgViewMode1.IsChecked = true;
                }
                //menuItemView.Items.Add(menuItemChgViewMode1);
                menuItemView.Items.Add(menuItemChgViewMode2);
                menuItemView.Items.Add(menuItemChgViewMode3);
                menuItemView.Items.Add(separate3);
                menuItemView.Items.Add(menuItemViewSetDlg);                
                if (noItem == true)
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
                        ((MenuItem)menuItemChg.Items[menuItemChg.Items.IndexOf(menuItemChgRecMode0) + Math.Min((int)reserve.RecSetting.RecMode, 5)]).IsChecked = true;
                        ((MenuItem)menuItemChgRecPri.Items[Math.Min((int)(reserve.RecSetting.Priority - 1), 4)]).IsChecked = true;
                        menuItemChgRecPri.Header = string.Format((string)menuItemChgRecPri.Tag, reserve.RecSetting.Priority);
                        menuItemDel.IsEnabled = true;
                        menuItemAutoAdd.IsEnabled = true;
                        menuItemTimeshift.IsEnabled = true;
                        menuItemView.IsEnabled = true;
                    }
                    else
                    {
                        menuItemNew.IsEnabled = true;
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
                if (sender.GetType() != typeof(MenuItem))
                {
                    return;
                }
                MenuItem meun = sender as MenuItem;
                UInt32 presetID = (UInt32)meun.DataContext;

                EpgEventInfo eventInfo = new EpgEventInfo();
                if (GetProgramItem(clickPos, ref eventInfo) == false)
                {
                    return;
                }
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
                ErrCode err = (ErrCode)cmd.SendAddReserve(list);
                if (err == ErrCode.CMD_ERR_CONNECT)
                {
                    MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                }
                if (err == ErrCode.CMD_ERR_TIMEOUT)
                {
                    MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                }
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show("予約登録でエラーが発生しました。終了時間がすでに過ぎている可能性があります。");
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
                EpgEventInfo program = new EpgEventInfo();
                if (GetProgramItem(clickPos, ref program) == false)
                {
                    return;
                }
                AddReserve(program);
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
                ErrCode err = (ErrCode)cmd.SendDelReserve(list);
                if (err == ErrCode.CMD_ERR_CONNECT)
                {
                    MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                }
                if (err == ErrCode.CMD_ERR_TIMEOUT)
                {
                    MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                }
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show("予約削除でエラーが発生しました。");
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
                if (sender.GetType() != typeof(MenuItem))
                {
                    return;
                }

                ReserveData reserve = new ReserveData();
                if (GetReserveItem(clickPos, ref reserve) == false)
                {
                    return;
                }
                MenuItem item = sender as MenuItem;
                Int32 val = (Int32)item.DataContext;
                reserve.RecSetting.RecMode = (byte)val;
                List<ReserveData> list = new List<ReserveData>();
                list.Add(reserve);
                ErrCode err = (ErrCode)cmd.SendChgReserve(list);
                if (err == ErrCode.CMD_ERR_CONNECT)
                {
                    MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                }
                if (err == ErrCode.CMD_ERR_TIMEOUT)
                {
                    MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                }
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show("予約変更でエラーが発生しました。");
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
                if (sender.GetType() != typeof(MenuItem))
                {
                    return;
                }

                ReserveData reserve = new ReserveData();
                if (GetReserveItem(clickPos, ref reserve) == false)
                {
                    return;
                }
                MenuItem item = sender as MenuItem;
                Int32 val = (Int32)item.DataContext;
                reserve.RecSetting.Priority = (byte)val;
                List<ReserveData> list = new List<ReserveData>();
                list.Add(reserve);
                ErrCode err = (ErrCode)cmd.SendChgReserve(list);
                if (err == ErrCode.CMD_ERR_CONNECT)
                {
                    MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                }
                if (err == ErrCode.CMD_ERR_TIMEOUT)
                {
                    MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                }
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show("予約変更でエラーが発生しました。");
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

                EpgEventInfo program = new EpgEventInfo();
                if (GetProgramItem(clickPos, ref program) == false)
                {
                    return;
                }

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
        /// 右クリックメニュー 簡易予約イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_new_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                EpgEventInfo eventInfo = new EpgEventInfo();
                if (GetProgramItem(clickPos, ref eventInfo) == false)
                {
                    return;
                }
                
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
                Settings.GetDefRecSetting(0, ref setInfo);  //  デフォルトをとって来てくれる？
                reserveInfo.RecSetting = setInfo;

                List<ReserveData> list = new List<ReserveData>();
                list.Add(reserveInfo);
                ErrCode err = (ErrCode)cmd.SendAddReserve(list);
                if (err == ErrCode.CMD_ERR_CONNECT)
                {
                    MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                }
                if (err == ErrCode.CMD_ERR_TIMEOUT)
                {
                    MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                }
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show("簡易予約でエラーが発生しました。終了時間がすでに過ぎている可能性があります。");
                }
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
            if (ViewSettingClick != null)
            {
                ViewSettingClick(this, null);
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
        private void AddReserve(EpgEventInfo eventInfo)
        {
            try
            {
                AddReserveEpgWindow dlg = new AddReserveEpgWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetOpenMode(Settings.Instance.EpgInfoOpenMode);
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
                        if (err == ErrCode.CMD_ERR_CONNECT)
                        {
                            if (this.IsVisible == true)
                            {
                                this.Dispatcher.BeginInvoke(new Action(() =>
                                {
                                    MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                                }), null);
                            }
                            return false;
                        }
                        if (err == ErrCode.CMD_ERR_BUSY)
                        {
                            /*if (this.IsVisible == true)
                            {
                                this.Dispatcher.BeginInvoke(new Action(() =>
                                {
                                    MessageBox.Show("EPGデータの読み込みを行える状態ではありません。\r\n（EPGデータ読み込み中。など）");
                                }), null);
                            }*/
                            return false;
                        }
                        if (err == ErrCode.CMD_ERR_TIMEOUT)
                        {
                            if (this.IsVisible == true)
                            {
                                this.Dispatcher.BeginInvoke(new Action(() =>
                                {
                                    MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                                }), null);
                            }
                            return false;
                        }
                        if (err != ErrCode.CMD_SUCCESS)
                        {
                            if (this.IsVisible == true)
                            {
                                this.Dispatcher.BeginInvoke(new Action(() =>
                                {
                                    MessageBox.Show("EPGデータの取得でエラーが発生しました。EPGデータが読み込まれていない可能性があります。");
                                }), null);
                            }
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
                if (err == ErrCode.CMD_ERR_CONNECT)
                {
                    MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                    return false;
                }
                if (err == ErrCode.CMD_ERR_TIMEOUT)
                {
                    MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                    return false;
                }
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show("予約情報の取得でエラーが発生しました。");
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
                            if (srvInfo.ONID == info.OriginalNetworkID &&
                                srvInfo.TSID == info.TransportStreamID &&
                                srvInfo.SID == info.ServiceID)
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
                                            if (viewItem.ReserveInfo.OriginalNetworkID == pgInfo.EventInfo.original_network_id &&
                                                viewItem.ReserveInfo.TransportStreamID == pgInfo.EventInfo.transport_stream_id &&
                                                viewItem.ReserveInfo.ServiceID == pgInfo.EventInfo.service_id &&
                                                viewItem.ReserveInfo.EventID == pgInfo.EventInfo.event_id &&
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
                        if (serviceList.Exists(i => i.ONID == serviceInfo.ONID && i.TSID == serviceInfo.TSID && i.SID == serviceInfo.SID) == false)
                        {
                            serviceList.Add(serviceInfo);
                        }
                    }
                }


                //必要番組の抽出と時間チェック
                List<EpgServiceInfo> primeServiceList = new List<EpgServiceInfo>();
                //番組表でまとめて描画する矩形の幅と番組集合のリスト
                var programGroupList = new List<Tuple<double, List<ProgramViewItem>>>();
                int groupSpan = 1;
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
                        if (--groupSpan <= 0)
                        {
                            groupSpan = spanCheckNum;
                            programGroupList.Add(new Tuple<double, List<ProgramViewItem>>(Settings.Instance.ServiceWidth * groupSpan, new List<ProgramViewItem>()));
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
                            if (eventInfo.ContentInfo == null || eventInfo.ContentInfo.nibbleList.Count == 0)
                            {
                                //ジャンル情報ない
                                find = this.viewCustContentKindList.ContainsKey(0xFFFF);
                            }
                            else
                            {
                                {
                                    foreach (EpgContentData contentInfo in eventInfo.ContentInfo.nibbleList)
                                    {
                                        UInt16 ID1 = (UInt16)(((UInt16)contentInfo.content_nibble_level_1) << 8 | 0xFF);
                                        UInt16 ID2 = (UInt16)(((UInt16)contentInfo.content_nibble_level_1) << 8 | contentInfo.content_nibble_level_2);
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
                                if (serviceInfo.ONID == data.original_network_id &&
                                    serviceInfo.TSID == data.transport_stream_id &&
                                    serviceInfo.SID == data.service_id)
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
                                        if (nextInfo.ONID == data.original_network_id &&
                                            nextInfo.TSID == data.transport_stream_id &&
                                            nextInfo.SID == data.service_id)
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
                        viewItem.Height = ((eventInfo.DurationFlag == 0 ? 300 : eventInfo.durationSec) * Settings.Instance.MinHeight) / 60;
                        viewItem.Width = Settings.Instance.ServiceWidth * widthSpan / mergeNum;
                        viewItem.LeftPos = Settings.Instance.ServiceWidth * (servicePos + (double)((mergeNum+i-mergePos-1)/2) / mergeNum);
                        //viewItem.TopPos = (eventInfo.start_time - startTime).TotalMinutes * Settings.Instance.MinHeight;
                        programGroupList[programGroupList.Count - 1].Item2.Add(viewItem);
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
                    programGroupList,
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
                        if (serviceList.Exists(i => i.ONID == serviceInfo.ONID && i.TSID == serviceInfo.TSID && i.SID == serviceInfo.SID) == false)
                        {
                            serviceList.Add(serviceInfo);
                        }
                    }
                }


                //必要番組の抽出と時間チェック
                List<EpgServiceInfo> primeServiceList = new List<EpgServiceInfo>();
                //番組表でまとめて描画する矩形の幅と番組集合のリスト
                var programGroupList = new List<Tuple<double, List<ProgramViewItem>>>();
                int groupSpan = 1;
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
                        if (--groupSpan <= 0)
                        {
                            groupSpan = spanCheckNum;
                            programGroupList.Add(new Tuple<double, List<ProgramViewItem>>(Settings.Instance.ServiceWidth * groupSpan, new List<ProgramViewItem>()));
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
                            if (eventInfo.ContentInfo == null || eventInfo.ContentInfo.nibbleList.Count == 0)
                            {
                                //ジャンル情報ない
                                find = this.viewCustContentKindList.ContainsKey(0xFFFF);
                            }
                            else
                            {
                                {
                                    foreach (EpgContentData contentInfo in eventInfo.ContentInfo.nibbleList)
                                    {
                                        UInt16 ID1 = (UInt16)(((UInt16)contentInfo.content_nibble_level_1) << 8 | 0xFF);
                                        UInt16 ID2 = (UInt16)(((UInt16)contentInfo.content_nibble_level_1) << 8 | contentInfo.content_nibble_level_2);
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
                                if (serviceInfo.ONID == data.original_network_id &&
                                    serviceInfo.TSID == data.transport_stream_id &&
                                    serviceInfo.SID == data.service_id)
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
                                        if (nextInfo.ONID == data.original_network_id &&
                                            nextInfo.TSID == data.transport_stream_id &&
                                            nextInfo.SID == data.service_id)
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
                        viewItem.Height = ((eventInfo.DurationFlag == 0 ? 300 : eventInfo.durationSec) * Settings.Instance.MinHeight) / 60;
                        viewItem.Width = Settings.Instance.ServiceWidth * widthSpan / mergeNum;
                        viewItem.LeftPos = Settings.Instance.ServiceWidth * (servicePos + (double)((mergeNum+i-mergePos-1)/2) / mergeNum);
                        //viewItem.TopPos = (eventInfo.start_time - startTime).TotalMinutes * Settings.Instance.MinHeight;
                        programGroupList[programGroupList.Count - 1].Item2.Add(viewItem);
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
                    programGroupList,
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
                    if (programViewItem1.EventInfo.event_id == BlackoutWindow.selectedSearchItem.EventInfo.event_id &&
                        programViewItem1.EventInfo.original_network_id == BlackoutWindow.selectedSearchItem.EventInfo.original_network_id &&
                        programViewItem1.EventInfo.service_id == BlackoutWindow.selectedSearchItem.EventInfo.service_id &&
                        programViewItem1.EventInfo.transport_stream_id == BlackoutWindow.selectedSearchItem.EventInfo.transport_stream_id)
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

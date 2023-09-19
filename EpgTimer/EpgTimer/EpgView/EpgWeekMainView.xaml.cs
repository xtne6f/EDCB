﻿using System;
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
using System.Windows.Threading;

using EpgTimer.EpgView;

namespace EpgTimer
{
    /// <summary>
    /// EpgWeekMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgWeekMainView : UserControl
    {
        public event Action<object, CustomEpgTabInfo, DateTime, object> ViewModeChangeRequested;
        private object scrollToTarget;
        private bool scrollToMoveBaseTime;

        private CustomEpgTabInfo setViewInfo;
        private DateTime baseTime;

        private SortedList<DateTime, List<ProgramViewItem>> timeList = new SortedList<DateTime, List<ProgramViewItem>>();
        private List<DateTime> dayList = new List<DateTime>();
        private List<ReserveViewItem> reserveList = new List<ReserveViewItem>();
        private DispatcherTimer nowViewTimer;
        private Dictionary<UInt64, EpgServiceAllEventInfo> serviceEventList = new Dictionary<UInt64, EpgServiceAllEventInfo>();

        private bool updateEpgData = true;
        private bool updateReserveData = true;

        public EpgWeekMainView(CustomEpgTabInfo setInfo, DateTime _baseTime)
        {
            InitializeComponent();

            nowViewTimer = new DispatcherTimer(DispatcherPriority.Normal);
            nowViewTimer.Tick += (sender, e) => ReDrawNowLine();
            setViewInfo = setInfo;
            epgProgramView.EpgSetting = setInfo.EpgSetting;
            baseTime = _baseTime;
        }

        /// <summary>
        /// 保持情報のクリア
        /// </summary>
        public void ClearInfo()
        {
            epgProgramView.ClearInfo();
            timeView.ClearInfo();
            weekDayView.ClearInfo();
            timeList.Clear();
            dayList.Clear();
            reserveList.Clear();
            serviceEventList.Clear();
            button_prev.IsEnabled = false;
            button_next.IsEnabled = false;
            button_prev.Visibility = Visibility.Hidden;
            ReDrawNowLine();
        }

        public bool HasService(ushort onid, ushort tsid, ushort sid)
        {
            return setViewInfo.ViewServiceList.Contains(CommonManager.Create64Key(onid, tsid, sid)) ||
                   setViewInfo.ViewServiceList.Contains(
                       (ulong)(ChSet5.IsDttv(onid) ? CustomEpgTabInfo.SpecialViewServices.ViewServiceDttv :
                               ChSet5.IsBS(onid) ? CustomEpgTabInfo.SpecialViewServices.ViewServiceBS :
                               ChSet5.IsCS(onid) ? CustomEpgTabInfo.SpecialViewServices.ViewServiceCS :
                               ChSet5.IsCS3(onid) ? CustomEpgTabInfo.SpecialViewServices.ViewServiceCS3 :
                               CustomEpgTabInfo.SpecialViewServices.ViewServiceOther));
        }

        /// <summary>
        /// 現在ライン表示
        /// </summary>
        private void ReDrawNowLine()
        {
            nowViewTimer.Stop();
            if (timeList.Count == 0 || baseTime < CommonManager.Instance.DB.EventBaseTime)
            {
                epgProgramView.nowLine.Visibility = Visibility.Hidden;
            }
            else
            {
                DateTime nowTime = DateTime.UtcNow.AddHours(9);
                nowTime = new DateTime(2001, 1, nowTime.Hour < setViewInfo.StartTimeWeek ? 2 : 1, nowTime.Hour, nowTime.Minute, nowTime.Second);
                double posY = 0;
                DateTime chkNowTime = new DateTime(nowTime.Year, nowTime.Month, nowTime.Day, nowTime.Hour, 0, 0);
                for (int i = 0; i < timeList.Count; i++)
                {
                    if (chkNowTime == timeList.Keys[i])
                    {
                        posY = Math.Ceiling((i * 60 + (nowTime - chkNowTime).TotalMinutes) * setViewInfo.EpgSetting.MinHeight);
                        break;
                    }
                    else if (chkNowTime < timeList.Keys[i])
                    {
                        //時間省かれてる
                        posY = Math.Ceiling(i * 60 * setViewInfo.EpgSetting.MinHeight);
                        break;
                    }
                }
                epgProgramView.nowLine.X1 = 0;
                epgProgramView.nowLine.Y1 = posY;
                epgProgramView.nowLine.X2 = epgProgramView.canvas.Width;
                epgProgramView.nowLine.Y2 = posY;
                epgProgramView.nowLine.Visibility = Visibility.Visible;

                nowViewTimer.Interval = TimeSpan.FromSeconds(60 - nowTime.Second);
                nowViewTimer.Start();
            }
        }

        /// <summary>
        /// 表示スクロールイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void epgProgramView_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            {
                {
                    //時間軸の表示もスクロール
                    timeView.scrollViewer.ScrollToVerticalOffset(epgProgramView.scrollViewer.VerticalOffset);
                    //サービス名表示もスクロール
                    weekDayView.scrollViewer.ScrollToHorizontalOffset(epgProgramView.scrollViewer.HorizontalOffset);
                }
            }
        }

        /// <summary>
        /// 表示する週の(EventBaseTimeを上限とする)実際の値
        /// </summary>
        private DateTime ActualBaseTime()
        {
            return baseTime > CommonManager.Instance.DB.EventBaseTime ? CommonManager.Instance.DB.EventBaseTime : baseTime;
        }

        /// <summary>
        /// 表示する週を移動する
        /// </summary>
        private bool MoveTime(DateTime time)
        {
            DateTime lastTime = baseTime;
            baseTime = time < CommonManager.Instance.DB.EventBaseTime ? time : DateTime.MaxValue;
            if (ReloadEpgData())
            {
                updateEpgData = false;
                ReloadReserveViewItem();
                updateReserveData = false;
                if (baseTime < CommonManager.Instance.DB.EventBaseTime)
                {
                    epgProgramView.scrollViewer.ScrollToVerticalOffset(0);
                }
                return true;
            }
            baseTime = lastTime;
            return false;
        }

        /// <summary>
        /// 表示週変更
        /// </summary>
        void button_time_Click(object sender, RoutedEventArgs e)
        {
            MoveTime(ActualBaseTime().AddDays(sender == button_prev ? -7 : 7));
        }

        /// <summary>
        /// 表示週変更
        /// </summary>
        void button_time_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            ContextMenu menu = ((Button)sender).ContextMenu;
            menu.Items.Clear();
            bool prev = sender == button_prev;
            for (int i = 1; i <= 15; i++)
            {
                var menuItem = new MenuItem();
                int days = i * (prev ? -7 : 7);
                menuItem.Click += (sender2, e2) => MoveTime(ActualBaseTime().AddDays(days));
                menuItem.FontWeight = i == 1 ? FontWeights.Bold : FontWeights.Normal;
                menuItem.Header = ActualBaseTime().AddDays(days).ToString("yyyy\\/MM\\/dd～");
                if (prev ? ActualBaseTime().AddDays(days) <= CommonManager.Instance.DB.EventMinTime :
                           ActualBaseTime().AddDays(days) >= CommonManager.Instance.DB.EventBaseTime)
                {
                    menu.Items.Insert(prev ? menu.Items.Count : 0, menuItem);
                    break;
                }
                if (i == 15)
                {
                    menuItem.Header += prev ? " ↓" : " ↑";
                }
                menu.Items.Insert(prev ? menu.Items.Count : 0, menuItem);
            }
        }

        /// <summary>
        /// 現在ボタンクリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void button_now_Click(object sender, RoutedEventArgs e)
        {
            MoveNowTime(true);
        }

        /// <summary>
        /// 表示位置を現在の時刻にスクロールする
        /// </summary>
        private void MoveNowTime(bool moveBaseTime)
        {
            if (baseTime != DateTime.MaxValue && baseTime < CommonManager.Instance.DB.EventBaseTime)
            {
                if (moveBaseTime == false)
                {
                    return;
                }
                if (MoveTime(DateTime.MaxValue) == false)
                {
                    return;
                }
            }
            DateTime now = DateTime.UtcNow.AddHours(9);
            now = new DateTime(2001, 1, now.Hour < setViewInfo.StartTimeWeek ? 2 : 1, now.Hour, now.Minute, now.Second);
            for (int i = 0; i < timeList.Count; i++)
            {
                if (now <= timeList.Keys[i])
                {
                    double pos = Math.Max((i - 1) * 60 * setViewInfo.EpgSetting.MinHeight - 100, 0);
                    epgProgramView.scrollViewer.ScrollToVerticalOffset(Math.Ceiling(pos));
                    break;
                }
            }
        }

        /// <summary>
        /// マウス位置から予約情報を取得する
        /// </summary>
        /// <param name="cursorPos">[IN]マウス位置</param>
        /// <returns>nullで存在しない</returns>
        private ReserveData GetReserveItem(Point cursorPos)
        {
            //ヒットテストは逆順
            ReserveViewItem ret = reserveList.FindLast(a => a.LeftPos <= cursorPos.X && cursorPos.X < a.LeftPos + a.Width &&
                                                            a.TopPos <= cursorPos.Y && cursorPos.Y < a.TopPos + a.Height);
            return ret == null ? null : ret.ReserveInfo;
        }

        /// <summary>
        /// マウス位置から番組情報を取得する
        /// </summary>
        /// <param name="cursorPos">[IN]マウス位置</param>
        /// <returns>nullで存在しない</returns>
        private ProgramViewItem GetProgramItem(Point cursorPos)
        {
            {
                int timeIndex = (int)(cursorPos.Y / (60 * setViewInfo.EpgSetting.MinHeight));
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
            {
                //まず予約情報あるかチェック
                ReserveData reserve = GetReserveItem(cursorPos);
                if (reserve != null)
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

                ProgramViewItem program = ((Tuple<ReserveData, ProgramViewItem>)((MenuItem)sender).DataContext).Item2;
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
                reserveInfo.RecSetting = Settings.CreateRecSetting(presetID);

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
                MessageBox.Show(ex.ToString());
            }
        }

        /// <summary>
        /// 右クリックメニュー 予約追加クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_add_Click(object sender, RoutedEventArgs e)
        {
            ProgramViewItem program = ((Tuple<ReserveData, ProgramViewItem>)((MenuItem)sender).DataContext).Item2;
            AddReserve(program.EventInfo, program.Past == false);
        }

        /// <summary>
        /// 右クリックメニュー 予約変更クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_chg_Click(object sender, RoutedEventArgs e)
        {
            ReserveData reserve = ((Tuple<ReserveData, ProgramViewItem>)((MenuItem)sender).DataContext).Item1;
            ChangeReserve(reserve);
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
                ReserveData reserve = ((Tuple<ReserveData, ProgramViewItem>)((MenuItem)sender).DataContext).Item1;
                ErrCode err = CommonManager.CreateSrvCtrl().SendDelReserve(new List<uint>() { reserve.ReserveID });
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約削除でエラーが発生しました。");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        /// <summary>
        /// 右クリックメニュー 有効無効イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_chg_no_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                ReserveData reserve = ((Tuple<ReserveData, ProgramViewItem>)((MenuItem)sender).DataContext).Item1;
                byte recMode = reserve.RecSetting.GetRecMode();
                if (reserve.RecSetting.IsNoRec() == false)
                {
                    //録画モード情報を維持して無効化
                    recMode = (byte)(CommonManager.Instance.DB.FixNoRecToServiceOnly ? 5 : 5 + (recMode + 4) % 5);
                }
                reserve.RecSetting.RecMode = recMode;
                ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(new List<ReserveData>() { reserve });
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
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
                ReserveData reserve = ((Tuple<ReserveData, ProgramViewItem>)((MenuItem)sender).DataContext).Item1;
                byte recMode = (byte)(sender == recmode_all ? 0 :
                                      sender == recmode_only ? 1 :
                                      sender == recmode_all_nodec ? 2 :
                                      sender == recmode_only_nodec ? 3 : 4);
                if (reserve.RecSetting.IsNoRec())
                {
                    //録画モード情報を維持して無効化
                    recMode = (byte)(CommonManager.Instance.DB.FixNoRecToServiceOnly ? 5 : 5 + (recMode + 4) % 5);
                }
                reserve.RecSetting.RecMode = recMode;
                ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(new List<ReserveData>() { reserve });
                if (err != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
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
                ReserveData reserve = ((Tuple<ReserveData, ProgramViewItem>)((MenuItem)sender).DataContext).Item1;
                reserve.RecSetting.Priority = (byte)(sender == priority_1 ? 1 :
                                                     sender == priority_2 ? 2 :
                                                     sender == priority_3 ? 3 :
                                                     sender == priority_4 ? 4 : 5);
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
                MessageBox.Show(ex.ToString());
            }
        }

        /// <summary>
        /// 右クリックメニュー 自動予約登録イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_autoadd_Click(object sender, RoutedEventArgs e)
        {
            {
                ProgramViewItem programView = ((Tuple<ReserveData, ProgramViewItem>)((MenuItem)sender).DataContext).Item2;
                EpgEventInfo program = programView.EventInfo;

                SearchWindow dlg = new SearchWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;

                EpgSearchKeyInfo key = new EpgSearchKeyInfo();

                if (program.ShortInfo != null)
                {
                    key.andKey = program.ShortInfo.event_name;
                }
                key.serviceList.Add((long)CommonManager.Create64Key(program.original_network_id, program.transport_stream_id, program.service_id));

                dlg.SetSearchDefKey(key);
                dlg.ShowDialog();
            }
        }

        /// <summary>
        /// 右クリックメニュー 追っかけ再生イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_timeShiftPlay_Click(object sender, RoutedEventArgs e)
        {
            {
                ReserveData reserve = ((Tuple<ReserveData, ProgramViewItem>)((MenuItem)sender).DataContext).Item1;
                CommonManager.Instance.FilePlay(reserve.ReserveID);
            }
        }

        /// <summary>
        /// 右クリックメニュー 表示設定イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_viewSet_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new EpgDataViewSettingWindow();
            dlg.Title += " (一時的)";
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            dlg.SetDefSetting(setViewInfo);
            if (dlg.ShowDialog() == true)
            {
                var setInfo = dlg.GetSetting();
                if (setInfo.ViewMode == setViewInfo.ViewMode)
                {
                    setViewInfo = setInfo;
                    epgProgramView.EpgSetting = setInfo.EpgSetting;
                    UpdateEpgData();
                }
                else if (ViewModeChangeRequested != null)
                {
                    ViewModeChangeRequested(this, setInfo, baseTime, null);
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
            {
                if (ViewModeChangeRequested != null)
                {
                    CustomEpgTabInfo setInfo = setViewInfo.DeepClone();
                    setInfo.ViewMode = sender == cm_chg_viewMode1 ? 0 : 2;
                    ProgramViewItem program = ((Tuple<ReserveData, ProgramViewItem>)((MenuItem)sender).DataContext).Item2;
                    ViewModeChangeRequested(this, setInfo, baseTime, (program != null ? program.EventInfo : null));
                }
            }
        }

        private void grid_content_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            Point cursorPos = Mouse.GetPosition(epgProgramView.canvas);
            ReserveData reserve = GetReserveItem(cursorPos);
            ProgramViewItem program = GetProgramItem(cursorPos);
            grid_content.ContextMenu.DataContext = new Tuple<ReserveData, ProgramViewItem>(reserve, program);

            cm_new.IsEnabled = program != null && reserve == null && program.Past == false;
            cm_add.IsEnabled = program != null;
            cm_dupadd.IsEnabled = program != null;
            cm_add.Visibility = reserve != null ? Visibility.Collapsed : Visibility.Visible;
            cm_dupadd.Visibility = reserve != null ? Visibility.Visible : Visibility.Collapsed;
            cm_chg.IsEnabled = reserve != null;
            cm_new.Visibility = reserve != null ? Visibility.Collapsed : Visibility.Visible;
            cm_chg.Visibility = reserve != null ? Visibility.Visible : Visibility.Collapsed;
            cm_del.IsEnabled = reserve != null;
            cm_autoadd.IsEnabled = program != null;
            cm_timeshift.IsEnabled = reserve != null;
            if (reserve != null)
            {
                cm_chg_no.Visibility = reserve.RecSetting.IsNoRec() ? Visibility.Collapsed : Visibility.Visible;
                cm_chg_no_inv.Visibility = reserve.RecSetting.IsNoRec() ? Visibility.Visible : Visibility.Collapsed;
                for (int i = 0; i <= 4; i++)
                {
                    ((MenuItem)cm_chg.Items[cm_chg.Items.IndexOf(recmode_all) + i]).IsChecked = (i == reserve.RecSetting.GetRecMode());
                }
                for (int i = 0; i < cm_pri.Items.Count; i++)
                {
                    ((MenuItem)cm_pri.Items[i]).IsChecked = (i + 1 == reserve.RecSetting.Priority);
                }
                cm_pri.Header = string.Format((string)cm_pri.Tag, reserve.RecSetting.Priority);
            }
            for (int i = cm_add.Items.Count - 1; cm_add.Items[i] is MenuItem; i--)
            {
                cm_add.Items.RemoveAt(i);
            }
            for (int i = cm_dupadd.Items.Count - 1; cm_dupadd.Items[i] is MenuItem; i--)
            {
                cm_dupadd.Items.RemoveAt(i);
            }
            if (program != null)
            {
                foreach (RecPresetItem info in Settings.GetRecPresetList())
                {
                    var menuItem = new MenuItem();
                    menuItem.Header = info.DisplayName;
                    menuItem.Tag = info.ID;
                    menuItem.Click += cm_add_preset_Click;
                    menuItem.IsEnabled = program.Past == false;
                    (reserve != null ? cm_dupadd : cm_add).Items.Add(menuItem);
                }
            }
            cm_new.Tag = (uint)0;
        }

        /// <summary>
        /// 予約変更
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void ChangeReserve(ReserveData reserveInfo)
        {
            {
                ChgReserveWindow dlg = new ChgReserveWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetOpenMode(setViewInfo.EpgSetting.EpgInfoOpenMode);
                dlg.SetReserveInfo(reserveInfo);
                if (dlg.ShowDialog() == true)
                {
                }
            }
        }

        /// <summary>
        /// 予約追加
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void AddReserve(EpgEventInfo eventInfo, bool reservable)
        {
            {
                AddReserveEpgWindow dlg = new AddReserveEpgWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetOpenMode(setViewInfo.EpgSetting.EpgInfoOpenMode);
                dlg.SetReservable(reservable);
                dlg.SetEventInfo(eventInfo);
                if (dlg.ShowDialog() == true)
                {
                }
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
            //EpgViewPanelがDPI倍率の情報を必要とするため
            if (PresentationSource.FromVisual(Application.Current.MainWindow) != null &&
                (CommonManager.Instance.NWMode == false || CommonManager.Instance.NWConnectedIP != null))
            {
                Dictionary<ulong, EpgServiceAllEventInfo> list;
                ErrCode err;
                if (setViewInfo.SearchMode)
                {
                    err = CommonManager.Instance.DB.SearchWeeklyEpgData(baseTime, setViewInfo.SearchKey, out list);
                }
                else
                {
                    err = CommonManager.Instance.DB.LoadWeeklyEpgData(baseTime, out list);
                }
                if (err == ErrCode.CMD_SUCCESS)
                {
                    serviceEventList = list;
                    ReloadProgramViewItem(ActualBaseTime() > CommonManager.Instance.DB.EventMinTime, baseTime < CommonManager.Instance.DB.EventBaseTime);
                    MoveNowTime(false);
                    return true;
                }
                if (IsVisible && err != ErrCode.CMD_ERR_BUSY)
                {
                    Dispatcher.BeginInvoke(new Action(() => MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "EPGデータの取得でエラーが発生しました。")));
                }
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
            if (comboBox_service.SelectedItem != null && baseTime >= CommonManager.Instance.DB.EventBaseTime)
            {
                EpgServiceInfo selectInfo = (EpgServiceInfo)((ComboBoxItem)comboBox_service.SelectedItem).DataContext;
                ulong selectID = CommonManager.Create64Key(selectInfo.ONID, selectInfo.TSID, selectInfo.SID);

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
                        viewItem.Height = Math.Floor((duration / 60) * setViewInfo.EpgSetting.MinHeight);
                        if (viewItem.Height < setViewInfo.EpgSetting.MinHeight)
                        {
                            viewItem.Height = setViewInfo.EpgSetting.MinHeight;
                        }
                        viewItem.Width = setViewInfo.EpgSetting.ServiceWidth;

                        bool modified = false;
                        if (setViewInfo.EpgSetting.MinimumHeight > 0 && viewItem.ReserveInfo.EventID != 0xFFFF)
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
                                    viewItem.Height = Math.Max(pgInfo.Height * duration / info.DurationSecond, setViewInfo.EpgSetting.MinHeight);
                                    modified = true;
                                    break;
                                }
                            }
                        }
                        if (modified == false)
                        {
                            int index = timeList.IndexOfKey(chkStartTime);
                            viewItem.TopPos = index * 60 * setViewInfo.EpgSetting.MinHeight;
                            viewItem.TopPos += Math.Floor((startTime - chkStartTime).TotalMinutes * setViewInfo.EpgSetting.MinHeight);
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
                        viewItem.LeftPos = setViewInfo.EpgSetting.ServiceWidth * Math.Max(dayList.BinarySearch(chkDay), 0);

                        reserveList.Add(viewItem);
                    }
                }
            }
            //ほかの枠を完全に覆ってしまう場合は少しだけ縮める
            reserveList.Sort((a, b) => Math.Sign(a.LeftPos - b.LeftPos) * 2 + Math.Sign((int)a.ReserveInfo.ReserveID - (int)b.ReserveInfo.ReserveID));
            for (int i = 1; i < reserveList.Count; i++)
            {
                for (int j = i - 1; j >= 0 && reserveList[j].LeftPos == reserveList[i].LeftPos; j--)
                {
                    if (reserveList[j].Width >= 18 &&
                        reserveList[j].Width <= reserveList[i].Width &&
                        reserveList[j].TopPos >= reserveList[i].TopPos &&
                        reserveList[j].TopPos + reserveList[j].Height <= reserveList[i].TopPos + reserveList[i].Height)
                    {
                        reserveList[i].Width = reserveList[j].Width - 6;
                    }
                }
            }
            epgProgramView.SetReserveList(reserveList);
        }

        /// <summary>
        /// 番組情報の再描画処理
        /// </summary>
        private void ReloadProgramViewItem(bool enablePrev, bool enableNext)
        {
            try
            {
                epgProgramView.ClearInfo();
                timeList.Clear();
                dayList.Clear();

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

                //特殊なサービス指定の展開と重複除去
                var viewIDList = new List<ulong>();
                foreach (ulong id in setViewInfo.ViewServiceList)
                {
                    IEnumerable<EpgServiceAllEventInfo> sel =
                        id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceDttv ?
                            serviceEventList.Values.Where(info => ChSet5.IsDttv(info.serviceInfo.ONID)) :
                        id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceBS ?
                            serviceEventList.Values.Where(info => ChSet5.IsBS(info.serviceInfo.ONID)) :
                        id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceCS ?
                            serviceEventList.Values.Where(info => ChSet5.IsCS(info.serviceInfo.ONID)) :
                        id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceCS3 ?
                            serviceEventList.Values.Where(info => ChSet5.IsCS3(info.serviceInfo.ONID)) :
                        id == (ulong)CustomEpgTabInfo.SpecialViewServices.ViewServiceOther ?
                            serviceEventList.Values.Where(info => ChSet5.IsOther(info.serviceInfo.ONID)) : null;
                    if (sel == null)
                    {
                        if (viewIDList.Contains(id) == false)
                        {
                            viewIDList.Add(id);
                        }
                        continue;
                    }
                    foreach (EpgServiceInfo info in DBManager.SelectServiceEventList(sel).Select(allInfo => allInfo.serviceInfo))
                    {
                        if (viewIDList.Contains(CommonManager.Create64Key(info.ONID, info.TSID, info.SID)) == false)
                        {
                            viewIDList.Add(CommonManager.Create64Key(info.ONID, info.TSID, info.SID));
                        }
                    }
                }
                for (int i = 0; i < viewIDList.Count;)
                {
                    //TSIDが同じでSIDが逆順のときは正順にする
                    int skip = i + 1;
                    while (viewIDList.Count > skip &&
                           viewIDList[skip] >> 16 == viewIDList[skip - 1] >> 16 &&
                           (viewIDList[skip] & 0xFFFF) < (viewIDList[skip - 1] & 0xFFFF))
                    {
                        skip++;
                    }
                    for (int j = skip - 1; j >= i; j--)
                    {
                        ulong id = viewIDList[j];
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
                comboBox_service.SelectedIndex = Math.Min(selectIndex, comboBox_service.Items.Count - 1);

                //UpdateProgramView();
                button_prev.IsEnabled = enablePrev;
                button_next.IsEnabled = enableNext;
                button_prev.Visibility = enablePrev || enableNext ? Visibility.Visible : Visibility.Hidden;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void UpdateProgramView()
        {
            epgProgramView.ClearInfo();
            timeList.Clear();
            dayList.Clear();

            EpgServiceAllEventInfo allInfo = null;
            if (comboBox_service.SelectedItem != null)
            {
                EpgServiceInfo selectInfo = (EpgServiceInfo)((ComboBoxItem)comboBox_service.SelectedItem).DataContext;
                serviceEventList.TryGetValue(CommonManager.Create64Key(selectInfo.ONID, selectInfo.TSID, selectInfo.SID), out allInfo);
            }
            if (allInfo != null)
            {
                var programList = new List<ProgramViewItem>();
                List<ushort> contentKindList = setViewInfo.ViewContentKindList.ToList();
                contentKindList.Sort();

                //まず日時のチェック
                int eventInfoIndex = -1;
                foreach (EpgEventInfo eventInfo in Enumerable.Concat(allInfo.eventArcList, allInfo.eventList))
                {
                    bool past = ++eventInfoIndex < allInfo.eventArcList.Count;
                    if (eventInfo.StartTimeFlag == 0)
                    {
                        //開始未定は除外
                        continue;
                    }
                    //ジャンル絞り込み
                    bool filtered = false;
                    if (contentKindList.Count > 0)
                    {
                        if (eventInfo.ContentInfo == null || eventInfo.ContentInfo.nibbleList.Count == 0)
                        {
                            //ジャンル情報ない
                            filtered = contentKindList.BinarySearch(0xFFFF) < 0;
                        }
                        else
                        {
                            filtered = true;
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
                                    filtered = false;
                                    break;
                                }
                            }
                        }
                        if (filtered && setViewInfo.HighlightContentKind == false)
                        {
                            //ジャンル見つからないので除外
                            continue;
                        }
                    }

                    var viewItem = new ProgramViewItem(eventInfo, past, filtered);
                    viewItem.Height = ((eventInfo.DurationFlag == 0 ? 300 : eventInfo.durationSec) * setViewInfo.EpgSetting.MinHeight) / 60;
                    viewItem.Width = setViewInfo.EpgSetting.ServiceWidth;
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

                    if (dayList.BinarySearch(dayInfo) < 0)
                    {
                        dayList.Insert(~dayList.BinarySearch(dayInfo), dayInfo);
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
                        item.TopPos = (index * 60 + (startTime - chkStartTime).TotalMinutes) * setViewInfo.EpgSetting.MinHeight;
                    }
                    item.LeftPos = setViewInfo.EpgSetting.ServiceWidth * Math.Max(dayList.BinarySearch(dayInfo), 0);
                }
                if (setViewInfo.EpgSetting.MinimumHeight > 0)
                {
                    //最低表示行数を適用
                    programList.Sort((x, y) => Math.Sign(x.LeftPos - y.LeftPos) * 2 + Math.Sign(x.TopPos - y.TopPos));
                    double minimum = Math.Floor((setViewInfo.EpgSetting.FontSizeTitle + 2) * setViewInfo.EpgSetting.MinimumHeight);
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
                    int index = Math.Max((int)(item.TopPos / (60 * setViewInfo.EpgSetting.MinHeight)), 0);
                    while (index < Math.Min((int)((item.TopPos + item.Height) / (60 * setViewInfo.EpgSetting.MinHeight)) + 1, timeList.Count))
                    {
                        timeList.Values[index++].Add(item);
                    }
                }

                epgProgramView.SetProgramList(
                    programList,
                    dayList.Count * setViewInfo.EpgSetting.ServiceWidth,
                    timeList.Count * 60 * setViewInfo.EpgSetting.MinHeight);

                var timeBrushList = new List<Brush>();
                for (int i = 0; i < setViewInfo.EpgSetting.TimeColorList.Count; i++)
                {
                    SolidColorBrush brush = ColorDef.CustColorBrush(setViewInfo.EpgSetting.TimeColorList[i], setViewInfo.EpgSetting.TimeCustColorList[i]);
                    timeBrushList.Add(setViewInfo.EpgSetting.EpgGradationHeader ? (Brush)ColorDef.GradientBrush(brush.Color) : brush);
                }
                timeView.SetTime(timeList.Keys, 60 * setViewInfo.EpgSetting.MinHeight, setViewInfo.NeedTimeOnlyWeek, timeBrushList, true);
                weekDayView.SetDay(dayList, setViewInfo.EpgSetting.ServiceWidth, setViewInfo.EpgSetting.EpgGradationHeader);
            }

            ReDrawNowLine();
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
                ScrollTo(scrollToTarget, scrollToMoveBaseTime);
            }
        }

        public void ScrollTo(object target, bool moveBaseTime)
        {
            scrollToTarget = null;
            if (IsVisible == false)
            {
                //Visibleになるまですこし待つ
                scrollToTarget = target;
                scrollToMoveBaseTime = moveBaseTime;
                Dispatcher.BeginInvoke(DispatcherPriority.Render, new Action(() => scrollToTarget = null));
                return;
            }
            MoveNowTime(moveBaseTime);
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
            else if (target is EpgServiceInfo)
            {
                var info = (EpgServiceInfo)target;
                serviceKey_Target1 = CommonManager.Create64Key(info.ONID, info.TSID, info.SID);
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
                var info = (EpgEventInfo)target;
                for (int i = 0; i < this.timeList.Count; i++)
                {
                    foreach (ProgramViewItem item in this.timeList.Values[i])
                    {
                        if (item.EventInfo.original_network_id == info.original_network_id &&
                            item.EventInfo.transport_stream_id == info.transport_stream_id &&
                            item.EventInfo.service_id == info.service_id &&
                            (item.Past ? item.EventInfo.StartTimeFlag != 0 && info.StartTimeFlag != 0 && item.EventInfo.start_time == info.start_time :
                                         item.EventInfo.event_id == info.event_id))
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
                ViewModeChangeRequested(this, setInfo, baseTime, null);
            }
        }
    }
}

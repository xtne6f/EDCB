using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Collections;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;
using EpgTimer.EpgView;

namespace EpgTimer
{
    /// <summary>
    /// EpgWeekMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgWeekMainView : EpgMainViewBase
    {
        private SortedList dayList = new SortedList();

        public EpgWeekMainView()
        {
            InitializeComponent();
            SetControls(epgProgramView, timeView, weekDayView.scrollViewer, button_now);
        }

        public override bool ClearInfo()
        {
            base.ClearInfo();

            weekDayView.ClearInfo();
            dayList = new SortedList();

            return true;
        }

        public override void SetViewMode(CustomEpgTabInfo setInfo)
        {
            this.viewCustNeedTimeOnly = setInfo.NeedTimeOnlyWeek;
            base.SetViewMode(setInfo);
        }

        protected override bool ReloadEpgData()
        {
            if (base.ReloadEpgData() == false) return false;

            ReloadProgramViewItem();
            return true;
        }

        protected override bool ReloadReserveData()
        {
            if (base.ReloadReserveData() == false) return false;

            ReloadReserveViewItem();
            return true;
        }

        private enum TimeSelect : int
        {
            Normal,     //2001/1/1からの時刻
            HourOnly,   //2001/1/1からの時刻で時間のみ
            DayOnly     //日付
        };

        //週間番組表での時刻表現用のメソッド。
        //でも、全部並べてから最後に折った方が見通しいいんじゃないかな‥。
        private DateTime GetWeekMainViewTime(DateTime refTime, TimeSelect mode = TimeSelect.Normal)
        {
            int plus_1day = (refTime.Hour < setViewInfo.StartTimeWeek ? 1 : 0);
            switch (mode)
            {
                case TimeSelect.HourOnly:
                    return new DateTime(2001, 1, 1 + plus_1day, refTime.Hour, 0, 0);
                case TimeSelect.DayOnly:
                    refTime = refTime.AddDays(-1 * plus_1day);
                    return new DateTime(refTime.Year, refTime.Month, refTime.Day, 0, 0, 0);
                default:
                    return new DateTime(2001, 1, 1 + plus_1day, refTime.Hour, refTime.Minute, refTime.Second);
            }
        }

        /// <summary>
        /// 現在ライン表示
        /// </summary>
        protected override void ReDrawNowLine()
        {
            try
            {
                nowViewTimer.Stop();
                DateTime nowTime = GetWeekMainViewTime(DateTime.Now);

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
                DateTime chkNowTime = GetWeekMainViewTime(DateTime.Now, TimeSelect.HourOnly);
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

        protected override DateTime SetNowTime()
        {
            return GetWeekMainViewTime(DateTime.Now);
        }

        private UInt64 GetSelectID(bool alternativeSelect = false)
        {
            var item = comboBox_service.SelectedItem as ComboBoxItem;
            if (item == null)
            {
                if (alternativeSelect = false || comboBox_service.Items.Count == 0) return 0;

                item = comboBox_service.Items.GetItemAt(0) as ComboBoxItem;
            }

            return (item.DataContext as EpgServiceInfo).Create64Key();
        }

        /// <summary>
        /// 予約情報の再描画
        /// </summary>
        private void ReloadReserveViewItem()
        {
            try
            {
                reserveList.Clear();

                if (comboBox_service.Items.Count == 0) return;

                UInt64 selectID = GetSelectID(true);

                //TODO: ここでデフォルトマージンを確認するがEpgTimerNWでは無意味。根本的にはSendCtrlCmdの拡張が必要
                int defStartMargin = IniFileHandler.GetPrivateProfileInt("SET", "StartMargin", 0, SettingPath.TimerSrvIniPath);
                int defEndMargin = IniFileHandler.GetPrivateProfileInt("SET", "EndMargin", 0, SettingPath.TimerSrvIniPath);

                foreach (ReserveData info in CommonManager.Instance.DB.ReserveList.Values)
                {
                    UInt64 key = info.Create64Key();
                    if (selectID == key)
                    {
                        var startTime = GetWeekMainViewTime(info.StartTime);
                        var chkStartTime = GetWeekMainViewTime(info.StartTime, TimeSelect.HourOnly);

                        //時間ないので除外
                        if (timeList.ContainsKey(chkStartTime) == false) continue;

                        DateTime baseStartTime = startTime;

                        //マージンを適用
                        Int32 duration = (Int32)info.DurationSecond;
                        vutil.ApplyMarginForPanelView(info,
                            ref duration, ref startTime, defStartMargin, defEndMargin, true);

                        ReserveViewItem viewItem = new ReserveViewItem(info);
                        //viewItem.LeftPos = i * Settings.Instance.ServiceWidth;
                        viewItem.Height = Math.Max((duration * Settings.Instance.MinHeight) / 60, Settings.Instance.MinHeight);
                        viewItem.Width = Settings.Instance.ServiceWidth;

                        bool modified = false;
                        if (Settings.Instance.MinimumHeight > 0 && viewItem.ReserveInfo.EventID != 0xFFFF)
                        {
                            //予約情報から番組情報を特定し、枠表示位置を再設定する
                            foreach (ProgramViewItem pgInfo in timeList[chkStartTime])
                            {
                                if (viewItem.ReserveInfo.Create64PgKey() == pgInfo.EventInfo.Create64PgKey() &&
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

                        DateTime chkDay = GetWeekMainViewTime(info.StartTime, TimeSelect.DayOnly);
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
                Dictionary<UInt64, EpgServiceEventInfo> serviceEventList =
                    setViewInfo.SearchMode == true ? searchEventList : CommonManager.Instance.DB.ServiceEventList;

                //必要サービスの抽出
                int selectIndex = 0;
                UInt64 selectID = GetSelectID();
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

                //サービスの選択イベントから勝手に走る
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

                if (comboBox_service.Items.Count == 0) return;

                UInt64 selectID = GetSelectID(true);

                Dictionary<UInt64, EpgServiceEventInfo> serviceEventList =
                    setViewInfo.SearchMode == true ? base.searchEventList : CommonManager.Instance.DB.ServiceEventList;

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
                    //後で最低行数修正入前提で、高さが0になっても取りあえず気にしない
                    viewItem.Height = (eventInfo.durationSec * Settings.Instance.MinHeight) / 60;
                    viewItem.Width = Settings.Instance.ServiceWidth;
                    programList.Add(viewItem);

                    //日付列の決定
                    DateTime dayInfo = GetWeekMainViewTime(eventInfo.start_time, TimeSelect.DayOnly);

                    if (dayList.ContainsKey(dayInfo) == false)
                    {
                        dayList.Add(dayInfo, dayInfo);
                    }

                    //時間行の決定
                    DateTime chkStartTime = GetWeekMainViewTime(eventInfo.start_time, TimeSelect.HourOnly);
                    DateTime startTime = GetWeekMainViewTime(eventInfo.start_time);

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
                    DateTime chkStartTime = GetWeekMainViewTime(item.EventInfo.start_time, TimeSelect.HourOnly);
                    DateTime startTime = GetWeekMainViewTime(item.EventInfo.start_time);
                    DateTime dayInfo = GetWeekMainViewTime(item.EventInfo.start_time, TimeSelect.DayOnly);

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
                //最低表示行数を適用
                vutil.ModifierMinimumHeight<EpgEventInfo, ProgramViewItem>(programList);

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
                MoveNowTime();
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

        protected override void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (this.IsVisible == false) return;

            // サービス選択
            UInt64 serviceKey_Target = BlackoutWindow.Create64Key();
            if (serviceKey_Target == 0) return;

            foreach (ComboBoxItem item in this.comboBox_service.Items)
            {
                EpgServiceInfo serviceInfo = item.DataContext as EpgServiceInfo;
                if (serviceKey_Target == serviceInfo.Create64Key())
                {
                    this.comboBox_service.SelectedItem = item;
                    break;
                }
            }

            base.UserControl_IsVisibleChanged(sender, e);
        }

        private void button_go_Main_Click(object sender, RoutedEventArgs e)
        {
            if (base.EnableViewSetting() == false) return;

            CustomEpgTabInfo setInfo = new CustomEpgTabInfo();
            setViewInfo.CopyTo(ref setInfo);
            setInfo.ViewMode = 0;
            ViewSetting(this, setInfo);
        }
    }
}

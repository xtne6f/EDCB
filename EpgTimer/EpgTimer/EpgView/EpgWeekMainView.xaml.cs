using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Collections;

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

            base.InitCommand();

            //コマンド集からコマンドを登録
            mc.ResetCommandBindings(this, cmdMenu);

            //ボタンの設定
            mBinds.SetCommandToButton(button_go_Main, EpgCmds.ViewChgMode, 0);

            //メニューの作成、ショートカットの登録
            base.RefreshMenu();
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
            var nowTime = DateTime.Now;
            ReDrawNowLineBase(GetWeekMainViewTime(nowTime), GetWeekMainViewTime(nowTime, TimeSelect.HourOnly));
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

                foreach (ReserveData info in CommonManager.Instance.DB.ReserveList.Values)
                {
                    if (selectID == info.Create64Key())
                    {
                        //timeListは番組表ベースなので、chkStartTimeはマージン適用前に作成する。
                        DateTime startTime = GetWeekMainViewTime(info.StartTime);
                        DateTime chkStartTime = GetWeekMainViewTime(startTime, TimeSelect.HourOnly);
                        DateTime baseStartTime = startTime;

                        //離れた時間のプログラム予約など、番組表が無いので表示不可
                        if (timeList.ContainsKey(chkStartTime) == false) continue;

                        //マージンを適用
                        Int32 duration = (Int32)info.DurationSecond;
                        vutil.ApplyMarginForPanelView(info, ref startTime, ref duration);

                        var viewItem = new ReserveViewItem(info);
                        reserveList.Add(viewItem);

                        DateTime chkDay = GetWeekMainViewTime(info.StartTime, TimeSelect.DayOnly);
                        viewItem.LeftPos = Settings.Instance.ServiceWidth * dayList.IndexOfKey(chkDay);
                        viewItem.Width = Settings.Instance.ServiceWidth;

                        //最低表示行数の適用の際、最低表示高さを設定しているので、Settings.Instance.MinimumHeight == 0 でも検索するようにする
                        ProgramViewItem pgInfo = null;
                        if (info.EventID != 0xFFFF && info.DurationSecond != 0)
                        {
                            //予約情報から番組情報を特定し、枠表示位置を再設定する
                            UInt64 key = info.Create64PgKey();
                            pgInfo = timeList[chkStartTime].Find(info1 => key == info1.EventInfo.Create64PgKey());
                        }

                        if (pgInfo != null)
                        {
                            viewItem.TopPos = pgInfo.TopPos + pgInfo.Height * (startTime - baseStartTime).TotalSeconds / info.DurationSecond;
                            viewItem.Height = Math.Max(pgInfo.Height * duration / info.DurationSecond, ViewUtil.PanelMinimumHeight);
                        }
                        else
                        {
                            int index = timeList.IndexOfKey(chkStartTime);
                            viewItem.TopPos = Settings.Instance.MinHeight * (index * 60 + (startTime - chkStartTime).TotalMinutes);
                            viewItem.Height = Math.Max(duration * Settings.Instance.MinHeight / 60, ViewUtil.PanelMinimumHeight);
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

                    var viewItem = new ProgramViewItem(eventInfo);
                    viewItem.Height = Settings.Instance.MinHeight * (eventInfo.DurationFlag == 0 ? 300 : eventInfo.durationSec) / 60;
                    viewItem.HeightDef = viewItem.Height;//元の情報も保存
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

                    while (chkStartTime <= startTime.AddSeconds((eventInfo.DurationFlag == 0 ? 300 : eventInfo.durationSec)))
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
                    var chkStartTime = new DateTime(2001, 1, 1, setViewInfo.StartTimeWeek, 0, 0);
                    var chkEndTime = new DateTime(2001, 1, 2, setViewInfo.StartTimeWeek, 0, 0);
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
                    var chkStartTime = GetWeekMainViewTime(item.EventInfo.start_time, TimeSelect.HourOnly);
                    var startTime = GetWeekMainViewTime(item.EventInfo.start_time);
                    var dayInfo = GetWeekMainViewTime(item.EventInfo.start_time, TimeSelect.DayOnly);

                    if (timeList.ContainsKey(chkStartTime) == true)
                    {
                        int index = timeList.IndexOfKey(chkStartTime);
                        item.TopPos = (index * 60 + (startTime - chkStartTime).TotalMinutes) * Settings.Instance.MinHeight;
                        item.TopPosDef = item.TopPos;//元の情報も保存
                    }
                    if (dayList.ContainsKey(dayInfo) == true)
                    {
                        int index = dayList.IndexOfKey(dayInfo);
                        item.LeftPos = index * Settings.Instance.ServiceWidth;
                    }
                }
                //最低表示行数を適用。また、最低表示高さを確保して、位置も調整する。
                vutil.ModifierMinimumLine<EpgEventInfo, ProgramViewItem>(programList, Settings.Instance.MinimumHeight, Settings.Instance.FontSizeTitle);

                //必要時間リストと時間と番組の関連づけ
                vutil.SetTimeList(programList, timeList);
                
                epgProgramView.SetProgramList(
                    programList,
                    dayList.Count * Settings.Instance.ServiceWidth,
                    timeList.Count * 60 * Settings.Instance.MinHeight);

                var dateTimeList = new List<DateTime>();
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

            // Loaded イベントでは Reload*Data を省略したので
            // この IsVisibleChanged で Reload*Data を見逃してはいけない
            base.UserControl_IsVisibleChanged(sender, e);

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
        }

        //private void button_go_Main_Click(object sender, RoutedEventArgs e)
        //{
        //    EpgCmds.ViewChgMode.Execute(new EpgCmdParam(typeof(MenuItem), CtxmCode.EpgView, 0), cmdMenu);
        //}
    }
}

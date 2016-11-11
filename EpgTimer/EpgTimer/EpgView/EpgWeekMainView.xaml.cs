using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

namespace EpgTimer
{
    using EpgView;
    using ComboItem = KeyValuePair<UInt64, string>;

    /// <summary>
    /// EpgWeekMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgWeekMainView : EpgMainViewBase
    {
        private List<DateTime> dayList = new List<DateTime>();

        //class RestoreDateとか用意するところだけど、今回はこれだけなので手抜き
        public override object GetViewState() { return GetSelectID(); }

        public EpgWeekMainView()
        {
            InitializeComponent();
            SetControls(epgProgramView, timeView, weekDayView.scrollViewer, button_now);

            base.InitCommand();

            //コマンド集の初期化の続き、ボタンの設定
            mBinds.SetCommandToButton(button_go_Main, EpgCmds.ViewChgMode, 0);
        }

        public override void SetViewMode(CustomEpgTabInfo setInfo)
        {
            this.viewCustNeedTimeOnly = setInfo.NeedTimeOnlyWeek;
            base.SetViewMode(setInfo);
        }

        //週間番組表での時刻表現用のメソッド。
        protected override DateTime GetViewTime(DateTime time)
        {
            return new DateTime(2001, 1, time.Hour >= setViewInfo.StartTimeWeek ? 1 : 2).Add(time.TimeOfDay);
        }
        private DateTime GetViewDay(DateTime time)
        {
            return time.AddHours(-setViewInfo.StartTimeWeek).Date;
        }

        /// <summary>予約情報の再描画</summary>
        protected override void ReloadReserveViewItem()
        {
            try
            {
                reserveList.Clear();

                UInt64 selectID = GetSelectID(true);
                foreach (ReserveData info in CommonManager.Instance.DB.ReserveList.Values)
                {
                    if (selectID == info.Create64Key())
                    {
                        ProgramViewItem dummy = null;
                        ReserveViewItem resItem = AddReserveViewItem(info, ref dummy);
                        if (resItem != null)
                        {
                            //横位置の設定
                            resItem.Width = Settings.Instance.ServiceWidth;
                            resItem.LeftPos = resItem.Width * dayList.BinarySearch(GetViewDay(info.StartTime));
                        }
                    }
                }

                epgProgramView.SetReserveList(reserveList);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        /// <summary>番組情報の再描画</summary>
        protected override void ReloadProgramViewItem()
        {
            serviceChanging = true;
            try
            {
                //表示していたサービスがあれば維持
                ulong selectID = (restoreData as ulong?) ?? GetSelectID();
                comboBox_service.ItemsSource = serviceEventList.Select(item => new ComboItem(item.serviceInfo.Create64Key(), item.serviceInfo.service_name));
                comboBox_service.SelectedIndex = Math.Max(0, serviceEventList.FindIndex(info => info.serviceInfo.Create64Key() == selectID));

                UpdateProgramView();

                ReDrawNowLine();
                MoveNowTime();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            serviceChanging = false;
        }
        private void UpdateProgramView()
        {
            try
            {
                timeView.ClearInfo();
                weekDayView.ClearInfo();
                epgProgramView.ClearInfo();
                timeList.Clear();
                programList.Clear();
                nowViewTimer.Stop();
                dayList.Clear();

                UInt64 selectID = GetSelectID(true);
                if (selectID == 0) return;

                //リストの作成
                int idx = serviceEventList.FindIndex(item => item.serviceInfo.Create64Key() == selectID);
                if (idx < 0) return;

                serviceEventList[idx].eventList.ForEach(eventInfo =>
                {
                    try { programList.Add(eventInfo.CurrentPgUID(), new ProgramViewItem(eventInfo)); }
                    catch { }//無いはずだが一応保険
                });

                //日付リスト構築
                dayList.AddRange(programList.Values.Select(d => GetViewDay(d.EventInfo.start_time)).Distinct().OrderBy(day => day));

                //横位置の設定
                foreach (ProgramViewItem item in programList.Values)
                {
                    item.Width = Settings.Instance.ServiceWidth;
                    item.LeftPos = item.Width * dayList.BinarySearch(GetViewDay(item.EventInfo.start_time));
                }

                //縦位置の設定
                if (viewCustNeedTimeOnly == false)
                {
                    ViewUtil.AddTimeList(timeList, new DateTime(2001, 1, 1, setViewInfo.StartTimeWeek, 0, 0), 86400);
                }
                SetProgramViewItemVertical();

                epgProgramView.SetProgramList(programList.Values.ToList(),
                    dayList.Count * Settings.Instance.ServiceWidth,
                    timeList.Count * 60 * Settings.Instance.MinHeight);

                timeView.SetTime(timeList, true);
                weekDayView.SetDay(dayList);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        bool serviceChanging = false;
        private void comboBox_service_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (serviceChanging == true) return;
            UpdateProgramView();
            ReloadReserveViewItem();
            UpdateStatus();
        }
        private UInt64 GetSelectID(bool alternativeSelect = false)
        {
            var idx = comboBox_service.SelectedIndex;
            if (idx < 0)
            {
                if (alternativeSelect == false || comboBox_service.Items.Count == 0) return 0;
                idx = 0;
            }
            return ((ComboItem)comboBox_service.Items[idx]).Key;
        }

        protected override void MoveToReserveItem(ReserveData target, bool IsMarking)
        {
            ChangeViewService(target.Create64Key());
            base.MoveToReserveItem(target, IsMarking);
        }
        protected override void MoveToProgramItem(EpgEventInfo target, bool IsMarking)
        {
            ChangeViewService(target.Create64Key());
            base.MoveToProgramItem(target, IsMarking);
        }
        protected void ChangeViewService(UInt64 id)
        {
            var target = comboBox_service.Items.OfType<ComboItem>().FirstOrDefault(item => item.Key == id);
            if (target.Key != default(UInt64)) comboBox_service.SelectedItem = target;
        }
    }
}

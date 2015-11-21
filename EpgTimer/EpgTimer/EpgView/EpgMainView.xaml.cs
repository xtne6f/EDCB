using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;

using EpgTimer.EpgView;

namespace EpgTimer
{
    /// <summary>
    /// EpgMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgMainView : EpgMainViewBase
    {
        private List<EpgServiceInfo> serviceList = new List<EpgServiceInfo>();

        public EpgMainView()
        {
            InitializeComponent();
            SetControls(epgProgramView, timeView, serviceView.scrollViewer, button_now);

            dateView.TimeButtonClick += new RoutedEventHandler(epgDateView_TimeButtonClick);

            base.InitCommand();

            //コマンド集からコマンドを登録
            mc.ResetCommandBindings(this, cmdMenu);

            //メニューの作成、ショートカットの登録
            base.RefreshMenu();
        }

        public override bool ClearInfo()
        {
            base.ClearInfo();

            serviceView.ClearInfo();
            serviceList = new List<EpgServiceInfo>();
            dateView.ClearInfo();

            return true;
        }

        public override void SetViewMode(CustomEpgTabInfo setInfo)
        {
            this.viewCustNeedTimeOnly = setInfo.NeedTimeOnlyBasic;
            base.SetViewMode(setInfo);
        }

        protected override bool ReloadEpgData()
        {
            if (base.ReloadEpgData() == false) return false;

            ReloadProgramViewItem();
            ReDrawNowLine();
            base.MoveNowTime();
            return true;
        }

        protected override bool ReloadReserveData()
        {
            if (base.ReloadReserveData() == false) return false;

            ReloadReserveViewItem();
            return true;
        }

        /// <summary>
        /// 現在ライン表示
        /// </summary>
        protected override void ReDrawNowLine()
        {
            var nowTime = DateTime.Now;
            if (timeList.Count < 1 || nowTime < timeList.Keys[0])
            {
                NowLineDelete();
                return;
            }

            ReDrawNowLineBase(nowTime, new DateTime(nowTime.Year, nowTime.Month, nowTime.Day, nowTime.Hour, 0, 0));
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

        /// <summary>
        /// 予約情報の再描画
        /// </summary>
        private void ReloadReserveViewItem()
        {
            try
            {
                reserveList.Clear();

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
                                //マージンを適用
                                Int32 duration = (Int32)info.DurationSecond;
                                DateTime startTime = info.StartTime;
                                vutil.ApplyMarginForPanelView(info, ref duration, ref startTime, true);

                                ReserveViewItem viewItem = new ReserveViewItem(info);
                                viewItem.LeftPos = Settings.Instance.ServiceWidth * (servicePos + (double)((mergeNum + i - mergePos - 1) / 2) / mergeNum);
                                viewItem.Height = Math.Max((duration * Settings.Instance.MinHeight) / 60, Settings.Instance.MinHeight);
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
                                            if (viewItem.ReserveInfo.Create64PgKey() == pgInfo.EventInfo.Create64PgKey() &&
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
                Dictionary<UInt64, EpgServiceEventInfo> serviceEventList =
                    setViewInfo.SearchMode == true ? searchEventList : CommonManager.Instance.DB.ServiceEventList;

                //直前にthis.ClearInfo()が走ってるので無くても同じ
                epgProgramView.ClearInfo();
                timeList.Clear();
                programList.Clear();
                nowViewTimer.Stop();
                serviceList.Clear();

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
                    UInt64 id = serviceInfo.Create64Key();
                    foreach (EpgEventInfo eventInfo in serviceEventList[id].eventList)
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
                        //後で最低行数修正入前提で、高さが0になっても取りあえず気にしない
                        viewItem.Height = ((eventInfo.DurationFlag == 0 ? 300 : eventInfo.durationSec) * Settings.Instance.MinHeight) / 60;
                        viewItem.Width = Settings.Instance.ServiceWidth * widthSpan / mergeNum;
                        viewItem.LeftPos = Settings.Instance.ServiceWidth * (servicePos + (double)((mergeNum+i-mergePos-1)/2) / mergeNum);
                        //viewItem.TopPos = (eventInfo.start_time - startTime).TotalMinutes * Settings.Instance.MinHeight;
                        programList.Add(viewItem);

                        //必要時間リストの構築
                        DateTime chkStartTime = new DateTime(eventInfo.start_time.Year, eventInfo.start_time.Month, eventInfo.start_time.Day, eventInfo.start_time.Hour, 0, 0);
                        while (chkStartTime <= eventInfo.start_time.AddSeconds((eventInfo.DurationFlag == 0 ? 300 : eventInfo.durationSec)))
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

                //最低表示行数を適用
                vutil.ModifierMinimumLine<EpgEventInfo, ProgramViewItem>(programList, Settings.Instance.MinimumHeight);

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
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

namespace EpgTimer
{
    using EpgView;

    /// <summary>
    /// EpgMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgMainView : EpgMainViewBase
    {
        public EpgMainView()
        {
            InitializeComponent();
            SetControls(epgProgramView, timeView, serviceView.scrollViewer, button_now);

            dateView.TimeButtonClick += new RoutedEventHandler(epgDateView_TimeButtonClick);

            base.InitCommand();
        }

        public override void SetViewMode(CustomEpgTabInfo setInfo)
        {
            this.viewCustNeedTimeOnly = setInfo.NeedTimeOnlyBasic;
            base.SetViewMode(setInfo);
        }

        /// <summary>表示位置変更</summary>
        void epgDateView_TimeButtonClick(object sender, RoutedEventArgs e)
        {
            try
            {
                var time = (DateTime)((Button)sender).DataContext;

                if (timeList.Count < 1 || time < timeList[0])
                {
                    epgProgramView.scrollViewer.ScrollToVerticalOffset(0);
                }
                else
                {
                    for (int i = 0; i < timeList.Count; i++)
                    {
                        if (time <= timeList[i])
                        {
                            epgProgramView.scrollViewer.ScrollToVerticalOffset(Math.Ceiling(i * 60 * Settings.Instance.MinHeight));
                            break;
                        }
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        /// <summary>予約情報の再描画</summary>
        protected override void ReloadReserveViewItem()
        {
            try
            {
                reserveList.Clear();

                var serviceReserveList = CommonManager.Instance.DB.ReserveList.Values.ToLookup(data => data.Create64Key());
                int mergePos = 0;
                int mergeNum = 0;
                int servicePos = -1;
                for (int i = 0; i < serviceEventList.Count; i++)
                {
                    //TSIDが同じでSIDが逆順に登録されているときは併合する
                    if (--mergePos < i - mergeNum)
                    {
                        EpgServiceInfo curr = serviceEventList[i].serviceInfo;
                        for (mergePos = i; mergePos + 1 < serviceEventList.Count; mergePos++)
                        {
                            EpgServiceInfo next = serviceEventList[mergePos + 1].serviceInfo;
                            if (next.ONID != curr.ONID || next.TSID != curr.TSID || next.SID >= curr.SID)
                            {
                                break;
                            }
                            curr = next;
                        }
                        mergeNum = mergePos + 1 - i;
                        servicePos++;
                    }
                    var key = serviceEventList[mergePos].serviceInfo.Create64Key();
                    if (serviceReserveList.Contains(key) == true)
                    {
                        foreach (var info in serviceReserveList[key])
                        {
                            ProgramViewItem refPgItem = null;
                            ReserveViewItem resItem = AddReserveViewItem(info, ref refPgItem, true);
                            if (resItem != null)
                            {
                                //横位置の設定
                                if (refPgItem != null && refPgItem.Data.Create64Key() != key)
                                {
                                    refPgItem = null;
                                }
                                resItem.Width = refPgItem != null ? refPgItem.Width : Settings.Instance.ServiceWidth / mergeNum;
                                resItem.LeftPos = Settings.Instance.ServiceWidth * (servicePos + (double)((mergeNum + i - mergePos - 1) / 2) / mergeNum);
                            }
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
            try
            {
                dateView.ClearInfo();
                timeView.ClearInfo();
                serviceView.ClearInfo();
                epgProgramView.ClearInfo();
                timeList.Clear();
                programList.Clear();
                NowLineDelete();

                if (serviceEventList.Count == 0) return;

                //必要番組の抽出と時間チェック
                var primeServiceList = new List<EpgServiceInfo>();
                //番組表でまとめて描画する矩形の幅と番組集合のリスト
                var programGroupList = new List<PanelItem<List<ProgramViewItem>>>();
                int groupSpan = 1;
                int mergePos = 0;
                int mergeNum = 0;
                int servicePos = -1;
                for (int i = 0; i < serviceEventList.Count; i++)
                {
                    //TSIDが同じでSIDが逆順に登録されているときは併合する
                    int spanCheckNum = 1;
                    if (--mergePos < i - mergeNum)
                    {
                        EpgServiceInfo curr = serviceEventList[i].serviceInfo;
                        for (mergePos = i; mergePos + 1 < serviceEventList.Count; mergePos++)
                        {
                            EpgServiceInfo next = serviceEventList[mergePos + 1].serviceInfo;
                            if (next.ONID != curr.ONID || next.TSID != curr.TSID || next.SID >= curr.SID)
                            {
                                break;
                            }
                            curr = next;
                        }
                        mergeNum = mergePos + 1 - i;
                        servicePos++;
                        //正順のときは貫きチェックするサービス数を調べる
                        for (; mergeNum == 1 && i + spanCheckNum < serviceEventList.Count; spanCheckNum++)
                        {
                            EpgServiceInfo next = serviceEventList[i + spanCheckNum].serviceInfo;
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
                            programGroupList.Add(new PanelItem<List<ProgramViewItem>>(new List<ProgramViewItem>()) { Width = Settings.Instance.ServiceWidth * groupSpan });
                        }
                        primeServiceList.Add(serviceEventList[mergePos].serviceInfo);
                    }

                    foreach (EpgEventInfo eventInfo in serviceEventList[mergePos].eventList)
                    {
                        //イベントグループのチェック
                        int widthSpan = 1;
                        if (eventInfo.EventGroupInfo != null)
                        {
                            //サービス2やサービス3の結合されるべきもの
                            if (eventInfo.IsGroupMainEvent == false) continue;

                            //横にどれだけ貫くかチェック
                            int count = 1;
                            while (mergeNum == 1 ? count < spanCheckNum : count < mergeNum - (mergeNum + i - mergePos - 1) / 2)
                            {
                                EpgServiceInfo nextInfo = serviceEventList[mergeNum == 1 ? i + count : mergePos - count].serviceInfo;
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

                        //continueが途中にあるので登録はこの位置
                        var viewItem = new ProgramViewItem(eventInfo);
                        try { programList.Add(eventInfo.CurrentPgUID(), viewItem); }
                        catch { }//無いはずだが一応保険
                        programGroupList.Last().Data.Add(viewItem);

                        //横位置の設定
                        viewItem.Width = Settings.Instance.ServiceWidth * widthSpan / mergeNum;
                        viewItem.LeftPos = Settings.Instance.ServiceWidth * (servicePos + (double)((mergeNum + i - mergePos - 1) / 2) / mergeNum);
                    }
                }

                //縦位置の設定
                if (viewCustNeedTimeOnly == false)
                {
                    ViewUtil.AddTimeList(timeList, programList.Values.Min(item => item.EventInfo.start_time), 0);
                }
                SetProgramViewItemVertical();

                epgProgramView.SetProgramList(programGroupList, timeList.Count * 60 * Settings.Instance.MinHeight);
                timeView.SetTime(timeList, false);
                dateView.SetTime(timeList);
                serviceView.SetService(primeServiceList);

                ReDrawNowLine();
                MoveNowTime();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

    }
}

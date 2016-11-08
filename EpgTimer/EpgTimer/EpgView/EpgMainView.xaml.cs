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

        /// <summary>現在ライン表示</summary>
        protected override void ReDrawNowLine()
        {
            if (timeList.Count == 0 || DateTime.Now < timeList[0])
            {
                NowLineDelete();
                return;
            }
            base.ReDrawNowLine();
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

                foreach (ReserveData info in CommonManager.Instance.DB.ReserveList.Values)
                {
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
                        if (serviceEventList[mergePos].serviceInfo.Create64Key() == info.Create64Key())
                        {
                            ProgramViewItem refPgItem = null;
                            ReserveViewItem resItem = AddReserveViewItem(info, ref refPgItem);
                            if (resItem != null)
                            {
                                //横位置の設定
                                resItem.LeftPos = Settings.Instance.ServiceWidth * (servicePos + (double)((mergeNum + i - mergePos - 1) / 2) / mergeNum);
                                if (refPgItem == null)
                                {
                                    //eventIDはあるが、統合されていて表示から見つけられない場合の処理もここに含まれる。
                                    refPgItem = programList.Values.FirstOrDefault(info1 => info1.LeftPos == resItem.LeftPos
                                        && info1.TopPos <= resItem.TopPos && resItem.TopPos < info1.TopPos + info1.Height);
                                }
                                resItem.Width = refPgItem != null ? refPgItem.Width : Settings.Instance.ServiceWidth / mergeNum;
                            }
                            break;
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
                nowViewTimer.Stop();

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
                timeView.SetTime(timeList, viewCustNeedTimeOnly, false);
                dateView.SetTime(timeList);
                serviceView.SetService(primeServiceList);

                ReDrawNowLine();
                MoveNowTime();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;
using EpgTimer.EpgView;

namespace EpgTimer
{
    /// <summary>
    /// EpgListMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgListMainView : EpgViewBase
    {
        private ListViewController<SearchItem> lstCtrl;
        private List<ServiceItem> serviceList = new List<ServiceItem>();

        private Dictionary<UInt64, UInt64> lastChkSID = new Dictionary<UInt64, UInt64>();
        private bool listBox_service_need_initialized = true;

        public EpgListMainView()
        {
            InitializeComponent();

            //リストビュー関連の設定
            lstCtrl = new ListViewController<SearchItem>(this);
            lstCtrl.SetInitialSortKey("StartTime");
            lstCtrl.SetViewSetting(listView_event, gridView_event, true);

            InitCommand();
        }
        protected override void InitCommand()
        {
            base.InitCommand();

            //コマンド集の初期化の続き
            mc.SetFuncGetSearchList(isAll => (isAll == true ? lstCtrl.dataList.ToList() : lstCtrl.GetSelectedItemsList()));
            mc.SetFuncSelectSingleSearchData(lstCtrl.SelectSingleItem);
            mc.SetFuncReleaseSelectedData(() => listView_event.UnselectAll());

            //コマンド集からコマンドを登録
            mc.ResetCommandBindings(this, listView_event.ContextMenu);

            //コンテキストメニューの設定
            listView_event.ContextMenu.Tag = (int)2;//setViewInfo.ViewMode;
            listView_event.ContextMenu.Opened += new RoutedEventHandler(mc.SupportContextMenuLoading);

            //メニューの作成、ショートカットの登録
            RefreshMenu();
        }
        public override void RefreshMenu()
        {
            mBinds.ResetInputBindings(this, listView_event);
            mm.CtxmGenerateContextMenu(listView_event.ContextMenu, CtxmCode.EpgView, true);
        }

        public override bool ClearInfo()
        {
            base.ClearInfo(); 

            BackUpChkSID();
            listBox_service.ItemsSource = null;
            serviceList.Clear();
            listView_event.ItemsSource = null;
            lstCtrl.dataList.Clear();
            richTextBox_eventInfo.Document.Blocks.Clear();

            return true;
        }

        private void BackUpChkSID()
        {
            if (listBox_service_need_initialized == false && listBox_service.ItemsSource != null)
            {
                lastChkSID.Clear();
                foreach (ServiceItem info in serviceList)
                {
                    if (info.IsSelected == true)
                    {
                        lastChkSID.Add(info.ID, info.ID);
                    }
                }
            }
        }

        protected override bool ReloadViewData()
        {
            //ReloadReserveViewItem()の実質的な重複実行を阻止。
            //ReloadEpgData()より先に実行させておく必要がある。※パネル系Viewでは後に実行させる必要がある。
            ClearInfo();
            updateReserveData = !base.ReloadReserveData();
            return ReloadEpgData();
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

        private void ReloadReserveViewItem()
        {
            //予約チェック
            mutil.SetSearchItemReserved(lstCtrl.dataList);
            listView_event.Items.Refresh();
        }

        private bool ReloadProgramViewItem()
        {
            try
            {
                Dictionary<UInt64, EpgServiceEventInfo> serviceEventList =
                    setViewInfo.SearchMode == true ? searchEventList : CommonManager.Instance.DB.ServiceEventList;

                BackUpChkSID();

                listBox_service.ItemsSource = null;
                serviceList.Clear();

                foreach (UInt64 id in viewCustServiceList)
                {
                    if (serviceEventList.ContainsKey(id) == true)
                    {
                        ServiceItem item = new ServiceItem();
                        item.ServiceInfo = serviceEventList[id].serviceInfo;
                        item.IsSelected = listBox_service_need_initialized || lastChkSID.ContainsKey(id);
                        serviceList.Add(item);
                    }
                }

                listBox_service.ItemsSource = serviceList;
                listBox_service_need_initialized = false;

                UpdateEventList();

                return true;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return false;
            }
        }

        private void UpdateEventList()
        {
            //更新前の選択情報の保存。
            //なお、EPG更新の場合はReloadEpgData()でも追加で保存・復元コードを実施する必要があるが、
            //大きく番組表が変化するEPG更新前後で選択情報を保存する意味もないのでほっておくことにする。
            lstCtrl.ReloadInfoData(reloadEventList);
            return;
        }

        private bool reloadEventList(List<SearchItem> dataList)
        {
            Dictionary<UInt64, EpgServiceEventInfo> serviceEventList =
                setViewInfo.SearchMode == true ? base.searchEventList : CommonManager.Instance.DB.ServiceEventList;

            DateTime now = DateTime.Now;
            foreach (ServiceItem info in serviceList)
            {
                if (info.IsSelected == true)
                {
                    if (serviceEventList.ContainsKey(info.ID) == true)
                    {
                        foreach (EpgEventInfo eventInfo in serviceEventList[info.ID].eventList)
                        {
                            if (eventInfo.StartTimeFlag == 0)
                            {
                                //開始未定は除外
                                continue;
                            }
                            if (setViewInfo.FilterEnded)
                            {
                                if (eventInfo.start_time.AddSeconds(eventInfo.DurationFlag == 0 ? 0 : eventInfo.durationSec) < now)
                                    continue;
                            }
                            //ジャンル絞り込み
                            if (vutil.ContainsContent(eventInfo, this.viewCustContentKindList) == false)
                            {
                                continue;
                            }
                            //イベントグループのチェック
                            if (eventInfo.EventGroupInfo != null)
                            {
                                bool spanFlag = false;
                                foreach (EpgEventData data in eventInfo.EventGroupInfo.eventDataList)
                                {
                                    if (info.ServiceInfo.Create64Key() == data.Create64Key())
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
                            }

                            lstCtrl.dataList.Add(new SearchItem(eventInfo));
                        }
                    }
                }
            }
            //予約チェック
            mutil.SetSearchItemReserved(lstCtrl.dataList);
            return true;
        }

        private void CheckBox_Changed(object sender, RoutedEventArgs e)
        {
            UpdateEventList();
        }

        private void button_chkAll_Click(object sender, RoutedEventArgs e)
        {
            listBox_service.ItemsSource = null;
            serviceList.ForEach(info => info.IsSelected = true);
            listBox_service.ItemsSource = serviceList;
            UpdateEventList();
        }

        private void button_clearAll_Click(object sender, RoutedEventArgs e)
        {
            listBox_service.ItemsSource = null;
            serviceList.ForEach(info => info.IsSelected = false);
            listBox_service.ItemsSource = serviceList;
            UpdateEventList();
        }

        private void listView_event_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            richTextBox_eventInfo.Document.Blocks.Clear();
            scrollViewer1.ScrollToHome();
            if (listView_event.SelectedItem != null)
            {
                SearchItem item = listView_event.SelectedItem as SearchItem;
                EpgEventInfo eventInfo = item.EventInfo;
                richTextBox_eventInfo.Document = CommonManager.Instance.ConvertDisplayText(eventInfo);
            }
        }

        private void listView_event_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            EpgCmds.ShowDialog.Execute(sender, this);
        }

        protected override void MoveToReserveItem(ReserveItem target, bool JumpingTable)
        {
            uint ID = target.ReserveInfo.ReserveID;
            SearchItem target_item = lstCtrl.dataList.Find(item => item.ReserveInfo != null && item.ReserveInfo.ReserveID == ID);
            if (target_item != null)
            {
                ScrollToFindItem(target_item, JumpingTable);
            }
            else
            {
                //プログラム予約だと見つからないので、それらしい番組を引っ張ってきて再度確認する。
                //でもリスト番組表で探すより、プログラム予約でも表示させられる標準モードへ投げてしまった方が良いのかも？
                target_item = new SearchItem(mutil.SearchEventLikeThat(target.ReserveInfo));
                if (target_item.EventInfo != null)
                {
                    MoveToProgramItem(target_item, JumpingTable);
                }
                else
                {
                    //それでもダメなら諦める。EPG範囲外の予約などは標準モードでもまともには表示されていないので。
                    listView_event.SelectedIndex = listView_event.Items.Count - 1;
                    listView_event.ScrollIntoView(listView_event.SelectedItem);
                }
                return;
            }
        }

        protected override void MoveToProgramItem(SearchItem target, bool JumpingTable)
        {
            ulong PgKey = target.EventInfo.Create64PgKey();
            SearchItem target_item = lstCtrl.dataList.Find(item => item.EventInfo.Create64PgKey() == PgKey);
            ScrollToFindItem(target_item, JumpingTable);
        }

        private void ScrollToFindItem(SearchItem target_item, bool JumpingTable)
        {
            try
            {
                //可能性低いが0では無さそう。検索ダイアログからの検索などではあり得る。
                if (target_item == null) return;

                listView_event.SelectedItem = target_item;
                listView_event.ScrollIntoView(target_item);

                //いまいちな感じ
                //listView_event.ScrollIntoView(listView_event.Items[0]);
                //listView_event.ScrollIntoView(listView_event.Items[listView_event.Items.Count-1]);
                //int scrollpos = ((listView_event.SelectedIndex - 5) >= 0 ? listView_event.SelectedIndex - 5 : 0);
                //listView_event.ScrollIntoView(listView_event.Items[scrollpos]);

                //「番組表へジャンプ」の場合、またはオプションで指定のある場合に強調表示する。
                //パネルビューと比較して、こちらでは最後までゆっくり点滅させる。全表示時間は同じ。
                //ただ、結局スクロールさせる位置がうまく調整できてないので効果は限定的。
                if (JumpingTable || Settings.Instance.DisplayNotifyEpgChange)
                {
                    listView_event.SelectedItem = null;

                    var notifyTimer = new System.Windows.Threading.DispatcherTimer();
                    notifyTimer.Interval = TimeSpan.FromSeconds(0.2);
                    TimeSpan RemainTime = TimeSpan.FromSeconds(Settings.Instance.DisplayNotifyJumpTime);
                    notifyTimer.Tick += (sender, e) =>
                    {
                        RemainTime -= notifyTimer.Interval;
                        if (RemainTime <= TimeSpan.FromSeconds(0))
                        {
                            target_item.NowJumpingTable = 0;
                            listView_event.SelectedItem = target_item;
                            notifyTimer.Stop();
                        }
                        else
                        {
                            target_item.NowJumpingTable = target_item.NowJumpingTable != 1 ? 1 : 2;
                        }
                        listView_event.Items.Refresh();
                    };
                    notifyTimer.Start();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

    }
}

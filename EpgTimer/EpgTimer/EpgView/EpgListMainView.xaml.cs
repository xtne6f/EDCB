using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

using EpgTimer.EpgView;

namespace EpgTimer
{
    /// <summary>
    /// EpgListMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgListMainView : EpgViewBase
    {
        private static long lastActivateClass = new DateTime().Ticks;
        private long lastActivate = long.MinValue;
        public override bool IsLastActivate { get { return lastActivate == lastActivateClass; } }

        private ListViewController<SearchItem> lstCtrl;
        private List<ServiceItem> serviceList = new List<ServiceItem>();

        private Dictionary<UInt64, UInt64> lastChkSID = new Dictionary<UInt64, UInt64>();
        private bool listBox_service_need_initialized = true;

        public EpgListMainView()
        {
            InitializeComponent();

            //リストビュー関連の設定
            var list_columns = Resources["ReserveItemViewColumns"] as GridViewColumnList;
            list_columns.AddRange(Resources["RecSettingViewColumns"] as GridViewColumnList);

            lstCtrl = new ListViewController<SearchItem>(this);
            lstCtrl.SetSavePath(CommonUtil.GetMemberName(() => Settings.Instance.EpgListColumn)
                    , CommonUtil.GetMemberName(() => Settings.Instance.EpgListColumnHead)
                    , CommonUtil.GetMemberName(() => Settings.Instance.EpgListSortDirection));
            lstCtrl.SetViewSetting(listView_event, gridView_event, true, list_columns);
            lstCtrl.SetSelectedItemDoubleClick(EpgCmds.ShowDialog);

            InitCommand();
        }
        protected override void InitCommand()
        {
            base.InitCommand();

            //コマンド集の初期化の続き
            mc.SetFuncGetSearchList(isAll => (isAll == true ? lstCtrl.dataList.ToList() : lstCtrl.GetSelectedItemsList()));
            mc.SetFuncSelectSingleSearchData(lstCtrl.SelectSingleItem);
            mc.SetFuncReleaseSelectedData(() => listView_event.UnselectAll());

            //コマンド集に無いもの
            mc.AddReplaceCommand(EpgCmds.ChgOnOffCheck, (sender, e) => lstCtrl.ChgOnOffFromCheckbox(e.Parameter, EpgCmds.ChgOnOff));

            //コマンド集からコマンドを登録
            mc.ResetCommandBindings(this, listView_event.ContextMenu);

            //コンテキストメニューの設定
            lstCtrl.SetCtxmTargetSave(listView_event.ContextMenu);//こっちが先
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
            lstCtrl.dataList.SetReserveData();
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
        }

        private bool reloadEventList(List<SearchItem> dataList)
        {
            Dictionary<UInt64, EpgServiceEventInfo> serviceEventList =
                setViewInfo.SearchMode == true ? base.searchEventList : CommonManager.Instance.DB.ServiceEventList;

            //絞り込んだイベントリストを作成
            var eventList = new List<EpgEventInfo>();
            foreach (ServiceItem info in serviceList)
            {
                if (info.IsSelected == true)
                {
                    if (serviceEventList.ContainsKey(info.ID) == true)
                    {
                        foreach (EpgEventInfo eventInfo in serviceEventList[info.ID].eventList)
                        {
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
                            eventList.Add(eventInfo);
                        }
                    }
                }
            }

            //SearchItemリストを作成
            lstCtrl.dataList.AddFromEventList(eventList, true, setViewInfo.FilterEnded);

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

        protected override void MoveToReserveItem(ReserveData target, bool IsMarking)
        {
            uint ID = target.ReserveID;
            SearchItem target_item = lstCtrl.dataList.Find(item => item.ReserveInfo != null && item.ReserveInfo.ReserveID == ID);
            if (target_item != null)
            {
                vutil.ScrollToFindItem(target_item, listView_event, IsMarking);
            }
            else
            {
                //プログラム予約だと見つからないので、それらしい番組を引っ張ってきて再度確認する。
                //でもリスト番組表で探すより、プログラム予約でも表示させられる標準モードへ投げてしまった方が良いのかも？
                EpgEventInfo target_like = target.SearchEventInfoLikeThat();
                if (target_like != null)
                {
                    MoveToProgramItem(target_like, IsMarking);
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

        protected override void MoveToProgramItem(EpgEventInfo target, bool IsMarking)
        {
            ulong PgKey = target.Create64PgKey();
            SearchItem target_item = lstCtrl.dataList.Find(item => item.EventInfo.Create64PgKey() == PgKey);
            vutil.ScrollToFindItem(target_item, listView_event, IsMarking);
        }

        protected override void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            base.UserControl_IsVisibleChanged(sender, e);
            if (IsVisible == true)
            {
                lastActivate = DateTime.Now.Ticks;
                lastActivateClass = lastActivate;
            }
        }

        public override void SaveViewData(bool IfThisLastView = false)
        {
            if (IfThisLastView == false || IsLastActivate == true) lstCtrl.SaveViewDataToSettings();
        }
    }
}

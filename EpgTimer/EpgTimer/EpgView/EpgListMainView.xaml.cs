using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

using EpgTimer.EpgView;

namespace EpgTimer
{
    /// <summary>
    /// EpgListMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgListMainView : EpgViewBase
    {
        private static int? lastActivateClass = null;

        private ListViewController<SearchItem> lstCtrl;
        private List<ServiceItem> serviceList = new List<ServiceItem>();

        public EpgListMainView()
        {
            InitializeComponent();

            //リストビュー関連の設定
            var list_columns = Resources["ReserveItemViewColumns"] as GridViewColumnList;
            list_columns.AddRange(Resources["RecSettingViewColumns"] as GridViewColumnList);

            lstCtrl = new ListViewController<SearchItem>(this);
            lstCtrl.SetSavePath(CommonUtil.NameOf(() => Settings.Instance.EpgListColumn)
                    , CommonUtil.NameOf(() => Settings.Instance.EpgListColumnHead)
                    , CommonUtil.NameOf(() => Settings.Instance.EpgListSortDirection));
            lstCtrl.SetViewSetting(listView_event, gridView_event, true, true, list_columns);
            lstCtrl.SetSelectedItemDoubleClick(EpgCmds.ShowDialog);

            //ステータス変更の設定
            lstCtrl.SetSelectionChangedEventHandler((sender, e) => this.UpdateStatus(1));

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
            listView_event.ContextMenu.Tag = (int)2;//setViewInfo.ViewMode;
            listView_event.ContextMenu.Opened += new RoutedEventHandler(mc.SupportContextMenuLoading);

            //メニューの作成、ショートカットの登録
            RefreshMenu();
        }
        public override void RefreshMenu()
        {
            base.RefreshMenu();
            mBinds.ResetInputBindings(this, listView_event);
            mm.CtxmGenerateContextMenu(listView_event.ContextMenu, CtxmCode.EpgView, true);
        }

        protected override void UpdateStatusData(int mode = 0)
        {
            if (mode == 0) this.status[1] = ViewUtil.ConvertSearchItemStatus(lstCtrl.dataList, "番組数");
            List<SearchItem> sList = lstCtrl.GetSelectedItemsList();
            this.status[2] = sList.Count == 0 ? "" : ViewUtil.ConvertSearchItemStatus(sList, "　選択中");
        }

        protected override void ReloadReserveViewItem()
        {
            //予約チェック
            lstCtrl.dataList.SetReserveData();
            listView_event.Items.Refresh();
        }

        protected override void ReloadProgramViewItem()
        {
            try
            {
                //表示していたサービスの保存
                Dictionary<UInt64, bool> lastSID = serviceList.ToDictionary(s => s.ID, s => s.IsSelected);

                listBox_service.ItemsSource = null;
                serviceList.Clear();

                foreach (UInt64 id in viewCustServiceList)
                {
                    if (serviceEventList.ContainsKey(id) == true)
                    {
                        var item = new ServiceItem();
                        item.ServiceInfo = serviceEventList[id].serviceInfo;
                        item.IsSelected = lastSID.ContainsKey(id) == false || lastSID[id];
                        serviceList.Add(item);
                    }
                }

                listBox_service.ItemsSource = serviceList;

                UpdateEventList(true);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        private void UpdateEventList(bool scroll = false)
        {
            lstCtrl.ReloadInfoData(reloadEventList);

            if (scroll == true)
            {
                //lstCtrl内のジャンプがReloadReserveData()にキャンセルされるので、ReloadProgramViewItem()からのとき再実行しておく。
                Dispatcher.BeginInvoke(new Action(() => listView_event.ScrollIntoView(listView_event.SelectedItem)));
            }
        }

        private bool reloadEventList(List<SearchItem> dataList)
        {
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
                            if (ViewUtil.ContainsContent(eventInfo, this.viewCustContentKindList) == false)
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
            UpdateStatus();
        }
        private void button_chkAll_Click(object sender, RoutedEventArgs e)
        {
            button_All_Click(true);
        }
        private void button_clearAll_Click(object sender, RoutedEventArgs e)
        {
            button_All_Click(false);
        }
        private void button_All_Click(bool selected)
        {
            listBox_service.ItemsSource = null;
            serviceList.ForEach(info => info.IsSelected = selected);
            listBox_service.ItemsSource = serviceList;
            CheckBox_Changed(null, null);
        }

        private void listView_event_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            richTextBox_eventInfo.Document.Blocks.Clear();
            scrollViewer1.ScrollToHome();
            if (listView_event.SelectedItem != null)
            {
                var item = listView_event.SelectedItem as SearchItem;
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
                ViewUtil.ScrollToFindItem(target_item, listView_event, IsMarking);
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
            ViewUtil.JumpToListItem(new SearchItem(target), listView_event, IsMarking);
        }

        protected override void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            base.UserControl_IsVisibleChanged(sender, e);
            if (IsVisible == true)
            {
                lastActivateClass = this.GetHashCode();
            }
        }

        public override void SaveViewData()
        {
            if (lastActivateClass == this.GetHashCode()) lstCtrl.SaveViewDataToSettings();
        }
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

namespace EpgTimer
{
    using EpgView;

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
            lstCtrl.RefreshListView(true);
        }

        protected override void ReloadProgramViewItem()
        {
            try
            {
                //表示していたサービスの保存
                var lastSID = new HashSet<UInt64>(serviceList.Where(s => s.IsSelected == false).Select(s => s.ID));

                listBox_service.ItemsSource = null;

                serviceList = serviceEventList.Select(item => new ServiceItem(item.serviceInfo)).ToList();
                serviceList.ForEach(item => item.IsSelected = lastSID.Contains(item.ID) == false);

                listBox_service.ItemsSource = serviceList;

                UpdateEventList(true);
                ReloadReserveInfoFlg = false;//リストビューでは処理済みになる
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        private void UpdateEventList(bool scroll = false)
        {
            lstCtrl.ReloadInfoData(dataList =>
            {
                if (serviceEventList.Count != serviceList.Count) return false;

                foreach (var list in serviceEventList.Zip(serviceList, (f, s) => new { f, s })
                                            .Where(d => d.s.IsSelected == true).Select(d => d.f.eventList))
                {
                    dataList.AddRange(list.Where(e => e.IsGroupMainEvent == true).ToSearchList(setViewInfo.FilterEnded));
                }
                return true;
            });

            if (scroll == true)
            {
                listView_event.ScrollIntoView(listView_event.SelectedItem);
            }
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
            if (listView_event.SelectedItem == null) return;
            //
            richTextBox_eventInfo.Document = CommonManager.ConvertDisplayText((listView_event.SelectedItem as SearchItem).EventInfo);
        }

        protected override void MoveToReserveItem(ReserveData target, bool IsMarking)
        {
            //プログラム予約だと見つからないので、それらしい番組へジャンプする。
            MoveToItem(target.IsEpgReserve == true ? (IAutoAddTargetData)target : target.SearchEventInfoLikeThat(), IsMarking);
        }
        protected override void MoveToProgramItem(EpgEventInfo target, bool IsMarking)
        {
            MoveToItem(target, IsMarking);
        }
        protected void MoveToItem(IAutoAddTargetData target, bool IsMarking)
        {
            //過去番組表でイベントIDが重複している場合があるので開始時間も考慮する
            ViewUtil.JumpToListItem(target == null ? 0 : target.CurrentPgUID(), listView_event, IsMarking);
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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.ComponentModel;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// EpgListMainView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgListMainView : UserControl
    {
        public event ViewSettingClickHandler ViewSettingClick = null;

        private CustomEpgTabInfo setViewInfo = null;

        private List<UInt64> viewCustServiceList = null;
        private Dictionary<UInt16, UInt16> viewCustContentKindList = new Dictionary<UInt16, UInt16>();
        private List<SearchItem> programList = new List<SearchItem>();
        private List<ServiceItem> serviceList = new List<ServiceItem>();

        string _lastHeaderClicked = null;
        ListSortDirection _lastDirection = ListSortDirection.Ascending;
        string _lastHeaderClicked2 = null;
        ListSortDirection _lastDirection2 = ListSortDirection.Ascending;
        
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;

        private Dictionary<UInt64, EpgServiceEventInfo> searchEventList = new Dictionary<UInt64, EpgServiceEventInfo>();

        private bool updateEpgData = true;
        private bool updateReserveData = true;

        private Dictionary<UInt64, UInt64> lastChkSID = null;

        public EpgListMainView()
        {
            InitializeComponent();
        }

        public bool ClearInfo()
        {
            if (lastChkSID != null && listBox_service.ItemsSource != null)
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
            listBox_service.ItemsSource = null;
            serviceList.Clear();
            listView_event.ItemsSource = null;
            programList.Clear();
            searchEventList.Clear();
            richTextBox_eventInfo.Document.Blocks.Clear();

            return true;
        }

        public void SetViewMode(CustomEpgTabInfo setInfo)
        {
            ClearInfo();
            setViewInfo = setInfo;
            this.viewCustServiceList = setInfo.ViewServiceList;
            this.viewCustContentKindList.Clear();
            if (setInfo.ViewContentKindList != null)
            {
                foreach (UInt16 val in setInfo.ViewContentKindList)
                {
                    this.viewCustContentKindList.Add(val, val);
                }
            }

            if (ReloadEpgData() == true)
            {
                updateEpgData = false;
                if (ReloadReserveData() == true)
                {
                    updateReserveData = false;
                }
            }
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (this.IsVisible == true)
            {
                if (updateEpgData == true)
                {
                    if (ReloadEpgData() == true)
                    {
                        updateEpgData = false;
                        if (ReloadReserveData() == true)
                        {
                            updateReserveData = false;
                        }
                    }
                }
                if (updateReserveData == true)
                {
                    if (ReloadReserveData() == true)
                    {
                        updateReserveData = false;
                    }
                }
            }
        }

        /// <summary>
        /// EPGデータ更新通知
        /// </summary>
        public void UpdateEpgData()
        {
            updateEpgData = true;
            if (this.IsVisible == true || CommonManager.Instance.NWMode == false)
            {
                ClearInfo();
                if (ReloadEpgData() == true)
                {
                    updateEpgData = false;
                    if (ReloadReserveData() == true)
                    {
                        updateReserveData = false;
                    }
                }
            }
        }

        /// <summary>
        /// 予約情報更新通知
        /// </summary>
        public void UpdateReserveData()
        {
            updateReserveData = true;
            if (this.IsVisible == true)
            {
                if (ReloadReserveData() == true)
                {
                    updateReserveData = false;
                }
            }
        }

        private bool ReloadEpgData()
        {
            try
            {
                if (setViewInfo != null)
                {
                    if (lastChkSID != null && listBox_service.ItemsSource != null)
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
                    listBox_service.ItemsSource = null;
                    serviceList.Clear();
                    listView_event.ItemsSource = null;
                    programList.Clear();
                    searchEventList.Clear();

                    updateEpgData = false;
                    return ReloadProgramViewItem(true);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return true;
        }

        private bool ReloadReserveData()
        {
            try
            {
                if (CommonManager.Instance.NWMode == true)
                {
                    if (CommonManager.Instance.NW.IsConnected == false)
                    {
                        return false;
                    }
                }
                ErrCode err = CommonManager.Instance.DB.ReloadReserveInfo();
                if (CommonManager.CmdErrMsgTypical(err, "予約情報の取得") == false)
                {
                    return false;
                }

                return ReloadProgramViewItem(false);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            return true;
        }

        private bool ReloadProgramViewItem(bool ReloadEpg = false)
        {
            try
            {
                if (CommonManager.Instance.NWMode == true)
                {
                    if (ReloadEpg == true || setViewInfo.SearchMode == true)
                    {
                        if (CommonManager.Instance.NW.IsConnected == false)
                        {
                            return false;
                        }
                    }
                }

                Dictionary<UInt64, EpgServiceEventInfo> serviceEventList = null;

                if (setViewInfo.SearchMode == false)
                {
                    if (ReloadEpg == true)
                    {
                        ErrCode err = CommonManager.Instance.DB.ReloadEpgData();
                        if (CommonManager.CmdErrMsgTypical(err, "EPGデータの取得", this) == false)
                        {
                            return false;
                        }
                    }
                    //DB.ServiceEventListは更新で新しくなるので上へは持って行けない
                    serviceEventList = CommonManager.Instance.DB.ServiceEventList;
                }
                else
                {
                    serviceEventList = searchEventList;

                    //番組情報の検索
                    var keyList = new List<EpgSearchKeyInfo>();
                    keyList.Add(setViewInfo.SearchKey);
                    var list = new List<EpgEventInfo>();

                    ErrCode err = (ErrCode)cmd.SendSearchPg(keyList, ref list);
                    if (CommonManager.CmdErrMsgTypical(err, "EPGデータの取得", this) == false)
                    {
                        return false;
                    }

                    //サービス毎のリストに変換
                    serviceEventList.Clear();
                    foreach (EpgEventInfo eventInfo in list)
                    {
                        UInt64 id = eventInfo.Create64Key();
                        EpgServiceEventInfo serviceInfo = null;
                        if (serviceEventList.ContainsKey(id) == false)
                        {
                            if (ChSet5.Instance.ChList.ContainsKey(id) == false)
                            {
                                //サービス情報ないので無効
                                continue;
                            }
                            serviceInfo = new EpgServiceEventInfo();
                            serviceInfo.serviceInfo = CommonManager.ConvertChSet5To(ChSet5.Instance.ChList[id]);

                            serviceEventList.Add(id, serviceInfo);
                        }
                        else
                        {
                            serviceInfo = serviceEventList[id];
                        }
                        serviceInfo.eventList.Add(eventInfo);
                    }
                }

                if (lastChkSID != null && listBox_service.ItemsSource != null)
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

                listBox_service.ItemsSource = null;
                serviceList.Clear();

                foreach (UInt64 id in viewCustServiceList)
                {
                    if (serviceEventList.ContainsKey(id) == true)
                    {
                        ServiceItem item = new ServiceItem();
                        item.ServiceInfo = serviceEventList[id].serviceInfo;
                        item.IsSelected = true;
                        if (lastChkSID != null)
                        {
                            if (lastChkSID.ContainsKey(id) == false)
                            {
                                item.IsSelected = false;
                            }
                        }
                        serviceList.Add(item);
                    }
                }
                if (lastChkSID == null)
                {
                    lastChkSID = new Dictionary<ulong, ulong>();
                }

                listBox_service.ItemsSource = serviceList;

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
            try
            {
                //更新前の選択情報の保存。
                //なお、EPG更新の場合はReloadEpgData()でも追加で保存・復元コードを実施する必要があるが、
                //大きく番組表が変化するEPG更新前後で選択情報を保存する意味もないのでほっておくことにする。
                var oldItems = new ListViewSelectedKeeper<SearchItem>(listView_event, true);

                ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_event.DataContext);
                if (dataView != null)
                {
                    dataView.SortDescriptions.Clear();
                    dataView.Refresh();
                }
                listView_event.DataContext = null;

                //listView_event.ItemsSource = null;
                programList.Clear();

                Dictionary<UInt64, EpgServiceEventInfo> eventList = null;
                if (setViewInfo.SearchMode == true)
                {
                    eventList = searchEventList;
                }
                else
                {
                    eventList = CommonManager.Instance.DB.ServiceEventList;
                }

                DateTime now = DateTime.Now;
                foreach (ServiceItem info in serviceList)
                {
                    if (info.IsSelected == true)
                    {
                        if (eventList.ContainsKey(info.ID) == true)
                        {
                            foreach (EpgEventInfo eventInfo in eventList[info.ID].eventList)
                            {
                                if (eventInfo.StartTimeFlag == 0)
                                {
                                    //開始未定は除外
                                    continue;
                                }
                                if (setViewInfo.FilterEnded)
                                {
                                    if (eventInfo.start_time.AddSeconds(eventInfo.durationSec) < now)
                                        continue;
                                }
                                //ジャンル絞り込み
                                if (this.viewCustContentKindList.Count > 0)
                                {
                                    bool find = false;
                                    if (eventInfo.ContentInfo != null)
                                    {
                                        if (eventInfo.ContentInfo.nibbleList.Count > 0)
                                        {
                                            foreach (EpgContentData contentInfo in eventInfo.ContentInfo.nibbleList)
                                            {
                                                UInt16 ID1 = (UInt16)(((UInt16)contentInfo.content_nibble_level_1) << 8 | 0xFF);
                                                UInt16 ID2 = (UInt16)(((UInt16)contentInfo.content_nibble_level_1) << 8 | contentInfo.content_nibble_level_2);
                                                if (ID2 == 0x0e01)
                                                {
                                                    ID1 = (UInt16)(((UInt16)contentInfo.user_nibble_1) << 8 | 0x70FF);
                                                    ID2 = (UInt16)(((UInt16)contentInfo.user_nibble_1) << 8 | 0x7000 | contentInfo.user_nibble_2);
                                                }
                                                if (this.viewCustContentKindList.ContainsKey(ID1) == true)
                                                {
                                                    find = true;
                                                    break;
                                                }
                                                else if (this.viewCustContentKindList.ContainsKey(ID2) == true)
                                                {
                                                    find = true;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    if (find == false)
                                    {
                                        //ジャンル見つからないので除外
                                        continue;
                                    }
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

                                SearchItem item = new SearchItem();
                                item.EventInfo = eventInfo;

                                //予約チェック
                                foreach (ReserveData resInfo in CommonManager.Instance.DB.ReserveList.Values)
                                {
                                    if (CommonManager.EqualsPg(resInfo, eventInfo) == true)
                                    {
                                        item.ReserveInfo = resInfo;
                                        break;
                                    }
                                }

                                programList.Add(item);
                            }
                        }
                    }
                }
                //listView_event.DataContext = programList;
                listView_event.ItemsSource = programList;

                if (_lastHeaderClicked != null)
                {
                    //string header = ((Binding)_lastHeaderClicked.DisplayMemberBinding).Path.Path;
                    Sort(_lastHeaderClicked, _lastDirection);
                }
                else
                {
                    string header = ((Binding)gridView_event.Columns[1].DisplayMemberBinding).Path.Path;
                    Sort(header, _lastDirection);
                    _lastHeaderClicked = header;
                }

                //選択情報の復元
                oldItems.RestoreListViewSelected();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void CheckBox_Checked(object sender, RoutedEventArgs e)
        {
            UpdateEventList();
        }

        private void CheckBox_Unchecked(object sender, RoutedEventArgs e)
        {
            UpdateEventList();
        }

        private void button_chkAll_Click(object sender, RoutedEventArgs e)
        {
            listBox_service.ItemsSource = null;
            foreach (ServiceItem info in serviceList)
            {
                info.IsSelected = true;
            }
            listBox_service.ItemsSource = serviceList;
            UpdateEventList();
        }

        private void button_clearAll_Click(object sender, RoutedEventArgs e)
        {
            listBox_service.ItemsSource = null;
            foreach (ServiceItem info in serviceList)
            {
                info.IsSelected = false;
            }
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

        private SearchItem SelectSingleItem()
        {
            return mutil.SelectSingleItem<SearchItem>(listView_event);
        }

        private void listView_event_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            this.cm_show_dialog_click(listView_event.SelectedItem, new RoutedEventArgs());
        }

        private void listView_event_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            if (sender.GetType() == typeof(ListView))
            {
                try
                {
                    cm_chg_viewMode3.IsChecked = true;

                    if (listView_event.SelectedItem != null)
                    {
                        List<SearchItem> list = GetSelectedItemsList();
                        bool hasReserved = list.HasReserved();
                        bool hasNoReserved = list.HasNoReserved();

                        if (hasReserved == true)
                        {
                            cm_Reverse.Header = "予約←→無効";
                            cm_del.IsEnabled = true;
                            cm_chg.IsEnabled = true;
                            mutil.CheckChgItems(cm_chg, list);//現在の状態(録画モード、優先度)にチェックを入れる
                            cm_add.IsEnabled = hasNoReserved;
                            cm_autoadd.IsEnabled = true;
                            cm_timeshift.IsEnabled = true;
                        }
                        else
                        {
                            cm_Reverse.Header = "簡易予約";
                            cm_del.IsEnabled = false;
                            cm_chg.IsEnabled = false;
                            cm_add.IsEnabled = true;
                            cm_autoadd.IsEnabled = true;
                            cm_timeshift.IsEnabled = false;
                            mutil.ExpandPresetItems(cm_add, cm_add_preset_Click);
                        }

                        //個別に設定できるが、リスト系の設定メソッドを使い回す。
                        foreach (object item in listView_event_contextMenu.Items)
                        {
                            mutil.AppendMenuVisibleControl(item);
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }
            }
        }

        void listView_event_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.P:
                        this.cm_timeShiftPlay_Click(this.listView_event.SelectedItem, new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.D:
                        this.deleteItem();
                        break;
                    case Key.S:
                        this.cm_reverse_Click(this, new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.C:
                        this.CopyTitle2Clipboard();
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Enter:
                        this.cm_show_dialog_click(this, new RoutedEventArgs(Button.ClickEvent));
                        e.Handled = true;
                        break;
                    case Key.Delete:
                        this.deleteItem();
                        e.Handled = true;
                        break;
                }
            }
        }

        void deleteItem()
        {
            var delList = GetSelectedItemsList().ReserveInfoList();
            if (delList.Count == 0) { return; }

            string text1 = "削除しますか?　[削除アイテム数: " + delList.Count + "]" + "\r\n\r\n";
            delList.ForEach(item => text1 += " ・ " + item.Title + "\r\n");

            string caption1 = "登録項目削除の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) == MessageBoxResult.OK)
            {
                this.cm_del_Click(this.listView_event.SelectedItem, new RoutedEventArgs(Button.ClickEvent));
            }
        }

        void cm_add_preset_Click(object sender, RoutedEventArgs e)
        {
            mutil.ReserveAdd(GetSelectedItemsList(), null, sender);
        }

        private void cm_del_Click(object sender, RoutedEventArgs e)
        {
            mutil.ReserveDelete(GetSelectedItemsList());
        }

        private List<SearchItem> GetSelectedItemsList()
        {
            return listView_event.SelectedItems.Cast<SearchItem>().ToList();
        }

        private void cm_show_dialog_click(object sender, RoutedEventArgs e)
        {
            if (listView_event.SelectedItem != null)
            {
                mutil.OpenSearchItemWithWindow(SelectSingleItem(), this, Settings.Instance.EpgInfoOpenMode);
            }
        }

        private void cm_chg_recmode_Click(object sender, RoutedEventArgs e)
        {
            mutil.ReserveChangeRecmode(GetSelectedItemsList(), sender);
        }

        private void cm_chg_priority_Click(object sender, RoutedEventArgs e)
        {
            mutil.ReserveChangePriority(GetSelectedItemsList(), sender);
        }

        private void cm_autoadd_Click(object sender, RoutedEventArgs e)
        {
            if (listView_event.SelectedItem != null)
            {
                mutil.SendAutoAdd(SelectSingleItem(), this);
            }
        }

        private void cm_timeShiftPlay_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_event.SelectedItem != null)
                {
                    SearchItem item = SelectSingleItem();
                    if (item.IsReserved == true)
                    {
                        CommonManager.Instance.TVTestCtrl.StartTimeShift(item.ReserveInfo.ReserveID);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 右クリックメニュー 表示設定イベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_viewSet_Click(object sender, RoutedEventArgs e)
        {
            if (ViewSettingClick != null)
            {
                ViewSettingClick(this, null);
            }
        }

        /// <summary>
        /// 右クリックメニュー 表示モードイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_chg_viewMode_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (sender.GetType() != typeof(MenuItem))
                {
                    return;
                }
                if (ViewSettingClick != null)
                {
                    MenuItem item = sender as MenuItem;
                    var setInfo = new CustomEpgTabInfo();
                    setViewInfo.CopyTo(ref setInfo);
                    if (sender == cm_chg_viewMode2)
                    {
                        setInfo.ViewMode = 1;
                    }
                    else if (sender == cm_chg_viewMode3)
                    {
                        setInfo.ViewMode = 2;
                    }
                    else
                    {
                        setInfo.ViewMode = 0;
                    }
                    ViewSettingClick(this, setInfo);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void MenuItem_Click_CopyTitle(object sender, RoutedEventArgs e)
        {
            CopyTitle2Clipboard();
        }

        private void CopyTitle2Clipboard()
        {
            if (listView_event.SelectedItem != null)
            {
                SearchItem item = SelectSingleItem();
                mutil.CopyTitle2Clipboard(item.EventName);
            }
        }

        private void MenuItem_Click_CopyContent(object sender, RoutedEventArgs e)
        {
            if (listView_event.SelectedItem != null)
            {
                SearchItem item = SelectSingleItem();
                mutil.CopyContent2Clipboard(item.EventInfo);
            }
        }

        private void MenuItem_Click_SearchTitle(object sender, RoutedEventArgs e)
        {
            if (listView_event.SelectedItem != null)
            {
                SearchItem item = SelectSingleItem();
                mutil.SearchText(item.EventName);
            }
        }

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;
            ListSortDirection direction;

            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    string header = "";
                    if (headerClicked.Column.DisplayMemberBinding != null)
                    {
                        header = ((Binding)headerClicked.Column.DisplayMemberBinding).Path.Path;
                    }
                    else if (headerClicked.Tag as string != null)
                    {
                        header = headerClicked.Tag as string;
                    }
                    if (header == null || header == "")
                    {
                        return;
                    }

                    if (String.Compare(header, _lastHeaderClicked) != 0)
                    {
                        direction = ListSortDirection.Ascending;
                        _lastHeaderClicked2 = _lastHeaderClicked;
                        _lastDirection2 = _lastDirection;
                    }
                    else
                    {
                        if (_lastDirection == ListSortDirection.Ascending)
                        {
                            direction = ListSortDirection.Descending;
                        }
                        else
                        {
                            direction = ListSortDirection.Ascending;
                        }
                    }

                    Sort(header, direction);

                    _lastHeaderClicked = header;
                    _lastDirection = direction;
                }
            }
        }

        private void Sort(string sortBy, ListSortDirection direction)
        {
            try
            {
                ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_event.ItemsSource);

                dataView.SortDescriptions.Clear();

                SortDescription sd = new SortDescription(sortBy, direction);
                dataView.SortDescriptions.Add(sd);
                if (_lastHeaderClicked2 != null)
                {
                    if (String.Compare(sortBy, _lastHeaderClicked2) != 0)
                    {
                        SortDescription sd2 = new SortDescription(_lastHeaderClicked2, _lastDirection2);
                        dataView.SortDescriptions.Add(sd2);
                    }
                }
                dataView.Refresh();

                //Settings.Instance.ResColumnHead = sortBy;
                //Settings.Instance.ResSortDirection = direction;

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// 右クリックメニュー 予約←→無効クリックイベント呼び出し
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void cm_reverse_Click(object sender, RoutedEventArgs e)
        {
            mutil.ReserveChangeOnOff(GetSelectedItemsList(), null);
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (this.IsVisible == false) { return; }

            EpgEventInfo selectedItem = null;
            if (BlackoutWindow.selectedReserveItem != null)
            {
                selectedItem = BlackoutWindow.selectedReserveItem.EventInfo;
                BlackoutWindow.selectedReserveItem = null;
            }
            else if (BlackoutWindow.selectedSearchItem != null)
            {
                selectedItem = BlackoutWindow.selectedSearchItem.EventInfo;
                BlackoutWindow.selectedSearchItem = null;
            }

            if (selectedItem != null)
            {
                foreach (SearchItem item in listView_event.Items)
                {
                    if (CommonManager.EqualsPg(selectedItem, item.EventInfo) == true)
                    {
                        listView_event.SelectedItem = item;
                        listView_event.ScrollIntoView(item);
                        //画面更新されないので無意味
                        //listView_event.ScrollIntoView(listView_event.Items[0]);
                        //listView_event.ScrollIntoView(listView_event.Items[listView_event.Items.Count-1]);
                        //int scrollpos = ((listView_event.SelectedIndex - 5) >=0 ? scrollpos : 0);
                        //listView_event.ScrollIntoView(listView_event.Items[scrollpos]);
                        break;
                    }
                }
            }

        }

    }
}

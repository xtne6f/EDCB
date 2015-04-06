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
    public partial class EpgListMainView : UserControl, IEpgDataViewItem
    {
        public event ViewSettingClickHandler ViewSettingClick = null;

        private CustomEpgTabInfo setViewInfo = null;

        private List<UInt64> viewCustServiceList = null;
        private Dictionary<UInt16, UInt16> viewCustContentKindList = new Dictionary<UInt16, UInt16>();
        private List<SearchItem> programList = new List<SearchItem>();
        private List<ServiceItem> serviceList = new List<ServiceItem>();

        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private ViewUtil vutil = CommonManager.Instance.VUtil;

        private Dictionary<UInt64, EpgServiceEventInfo> searchEventList = new Dictionary<UInt64, EpgServiceEventInfo>();

        private bool updateEpgData = true;
        private bool updateReserveData = true;

        private Dictionary<UInt64, UInt64> lastChkSID = new Dictionary<UInt64, UInt64>();
        private bool listBox_service_initialized = true;

        public EpgListMainView()
        {
            InitializeComponent();
        }

        public bool ClearInfo()
        {
            BackUpChkSID();
            listBox_service.ItemsSource = null;
            serviceList.Clear();
            listView_event.ItemsSource = null;
            programList.Clear();
            searchEventList.Clear();
            richTextBox_eventInfo.Document.Blocks.Clear();

            return true;
        }

        private void BackUpChkSID()
        {
            if (listBox_service_initialized == false && listBox_service.ItemsSource != null)
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
                if (ReloadReserveData() == true)//ここはUpdateReserveData()ではダメ
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
                        UpdateReserveData();
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
                    UpdateReserveData();
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
                    updateEpgData = false;
                    return ReloadProgramViewItem();
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
            if (vutil.ReloadReserveData() == false) return false;
            
            //予約チェック
            mutil.SetSearchItemReserved(programList);
            listView_event.Items.Refresh();
            
            return true;
        }

        private bool ReloadProgramViewItem()
        {
            try
            {
                Dictionary<UInt64, EpgServiceEventInfo> serviceEventList = vutil.ReloadEpgDataForEpgView(setViewInfo);
                if (serviceEventList == null) return false;

                if (setViewInfo.SearchMode == true)
                {
                    searchEventList = serviceEventList;
                }

                BackUpChkSID();

                listBox_service.ItemsSource = null;
                serviceList.Clear();

                foreach (UInt64 id in viewCustServiceList)
                {
                    if (serviceEventList.ContainsKey(id) == true)
                    {
                        ServiceItem item = new ServiceItem();
                        item.ServiceInfo = serviceEventList[id].serviceInfo;
                        item.IsSelected = listBox_service_initialized || lastChkSID.ContainsKey(id);
                        serviceList.Add(item);
                    }
                }

                listBox_service.ItemsSource = serviceList;
                listBox_service_initialized = false;

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

                listView_event.DataContext = null;
                programList.Clear();

                Dictionary<UInt64, EpgServiceEventInfo> serviceEventList = null;
                if (setViewInfo.SearchMode == true)
                {
                    serviceEventList = searchEventList;
                }
                else
                {
                    serviceEventList = CommonManager.Instance.DB.ServiceEventList;
                }

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
                                    if (eventInfo.start_time.AddSeconds(eventInfo.durationSec) < now)
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

                                programList.Add(new SearchItem(eventInfo));
                            }
                        }
                    }
                }
                //予約チェック
                mutil.SetSearchItemReserved(programList);

                listView_event.ItemsSource = programList;

                if (this.gridViewSorter.IsExistSortParams)
                {
                    this.gridViewSorter.SortByMultiHeader(this.programList);
                }
                else
                {
                    this.gridViewSorter.ResetSortParams();
                    this.gridViewSorter.SortByMultiHeaderWithKey(this.programList, gridView_event.Columns, "StartTime");
                }
                listView_event.Items.Refresh();

                //選択情報の復元
                oldItems.RestoreListViewSelected();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
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

        private SearchItem SelectSingleItem()
        {
            return mutil.SelectSingleItem<SearchItem>(listView_event);
        }

        private void listView_event_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            this.cm_show_dialog_Click(listView_event.SelectedItem, new RoutedEventArgs());
        }

        private void listView_event_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            if (sender.GetType() == typeof(ListView))
            {
                try
                {
                    cm_chg_viewMode3.IsChecked = true;

                    bool noItem = listView_event.SelectedItems.Count == 0;

                    cm_CopyTitle.IsEnabled = !noItem;
                    cm_CopyContent.IsEnabled = !noItem;
                    cm_SearchTitle.IsEnabled = !noItem;

                    if (noItem == true)
                    {
                        cm_Reverse.Header = "簡易予約";
                        cm_Reverse.IsEnabled = false;
                        cm_del.IsEnabled = false;
                        cm_chg.IsEnabled = false;
                        cm_add.IsEnabled = false;
                        cm_autoadd.IsEnabled = false;
                        cm_timeshift.IsEnabled = false;
                    }
                    else
                    {
                        List<SearchItem> list = GetSelectedItemsList();
                        bool hasReserved = list.HasReserved();
                        bool hasNoReserved = list.HasNoReserved();

                        if (hasReserved == true)
                        {
                            cm_Reverse.Header = "予約←→無効";
                            cm_Reverse.IsEnabled = true;
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
                            cm_Reverse.IsEnabled = true;
                            cm_del.IsEnabled = false;
                            cm_chg.IsEnabled = false;
                            cm_add.IsEnabled = true;
                            cm_autoadd.IsEnabled = true;
                            cm_timeshift.IsEnabled = false;
                            mutil.ExpandPresetItems(cm_add, cm_add_preset_Click);
                        }
                    }

                    //個別に設定できるが、リスト系の設定メソッドを使い回す。
                    foreach (object item in listView_event_contextMenu.Items)
                    {
                        mutil.AppendMenuVisibleControl(item);
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
                        this.cm_show_dialog_Click(this, new RoutedEventArgs(Button.ClickEvent));
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

        private void cm_show_dialog_Click(object sender, RoutedEventArgs e)
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

        private void cm_CopyTitle_Click(object sender, RoutedEventArgs e)
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

        private void cm_CopyContent_Click(object sender, RoutedEventArgs e)
        {
            if (listView_event.SelectedItem != null)
            {
                SearchItem item = SelectSingleItem();
                mutil.CopyContent2Clipboard(item.EventInfo);
            }
        }

        private void cm_SearchTitle_Click(object sender, RoutedEventArgs e)
        {
            if (listView_event.SelectedItem != null)
            {
                SearchItem item = SelectSingleItem();
                mutil.SearchText(item.EventName);
            }
        }

        GridViewSorter<SearchItem> gridViewSorter = new GridViewSorter<SearchItem>();

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;
            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    this.gridViewSorter.SortByMultiHeader(this.programList, headerClicked);
                    listView_event.Items.Refresh();
                }
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
            if (this.IsVisible == false) return;
            if (BlackoutWindow.Create64Key() == 0) return;

            EpgEventInfo selectedItem = null;
            if (BlackoutWindow.SelectedReserveItem != null)
            {
                selectedItem = BlackoutWindow.SelectedReserveItem.EventInfo;
                BlackoutWindow.SelectedReserveItem = null;
            }
            else if (BlackoutWindow.SelectedSearchItem != null)
            {
                selectedItem = BlackoutWindow.SelectedSearchItem.EventInfo;
                BlackoutWindow.SelectedSearchItem = null;
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

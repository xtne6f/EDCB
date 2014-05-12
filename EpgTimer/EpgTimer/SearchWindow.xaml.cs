using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.ComponentModel;
using System.Collections.ObjectModel;
using System.Collections;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// SearchWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SearchWindow : Window
    {
        private List<SearchItem> resultList = new List<SearchItem>();
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;

        //string _lastHeaderClicked = null;
        //ListSortDirection _lastDirection = ListSortDirection.Ascending;
        //string _lastHeaderClicked2 = null;
        //ListSortDirection _lastDirection2 = ListSortDirection.Ascending;

        private UInt32 autoAddID = 0;

        public SearchWindow()
        {
            InitializeComponent();

            try
            {
                if (Settings.Instance.NoStyle == 0)
                {
                    ResourceDictionary rd = new ResourceDictionary();
                    rd.MergedDictionaries.Add(
                        Application.LoadComponent(new Uri("/PresentationFramework.Aero, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aero.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                        //Application.LoadComponent(new Uri("/PresentationFramework.Classic, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35, ProcessorArchitecture=MSIL;component/themes/Classic.xaml", UriKind.Relative)) as ResourceDictionary
                        );
                    this.Resources = rd;
                }
                else
                {
                    button_search.Style = null;
                    button_add_reserve.Style = null;
                    button_add_epgAutoAdd.Style = null;
                    button_chg_epgAutoAdd.Style = null;
                }

                //ウインドウ位置の復元
                if (Settings.Instance.SearchWndTop != 0)
                {
                    this.Top = Settings.Instance.SearchWndTop;
                }
                if (Settings.Instance.SearchWndLeft != 0)
                {
                    this.Left = Settings.Instance.SearchWndLeft;
                }
                if (Settings.Instance.SearchWndWidth != 0)
                {
                    this.Width = Settings.Instance.SearchWndWidth;
                }
                if (Settings.Instance.SearchWndHeight != 0)
                {
                    this.Height = Settings.Instance.SearchWndHeight;
                }

                EpgSearchKeyInfo defKey = new EpgSearchKeyInfo();
                Settings.GetDefSearchSetting(ref defKey);

                searchKeyView.SetSearchKey(defKey);
            }
            catch
            {
            }
        }

        public void GetSearchKey(ref EpgSearchKeyInfo key)
        {
            searchKeyView.GetSearchKey(ref key);
        }

        public void SetSearchDefKey(EpgSearchKeyInfo key)
        {
            searchKeyView.SetSearchKey(key);
        }

        public void SetRecInfoDef(RecSettingData set)
        {
            recSettingView.SetDefSetting(set);
        }

        public void SetViewMode(UInt16 mode)
        {
            if (mode == 1)//新規
            {
                Title = "EPG予約条件";
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_del_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
            }
            else if (mode == 2)//変更
            {
                Title = "EPG予約条件";
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
                button_del_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
            }
            else
            {
                Title = "検索";
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_del_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
            }
        }

        public void SetChgAutoAddID(UInt32 id)
        {
            autoAddID = id;
        }

        private void tabItem_searchKey_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SearchPg();
            }
        }

        private void button_search_Click(object sender, RoutedEventArgs e)
        {
            SearchPg();
        }

        private void SearchPg()
        {
            try
            {
                //更新前の選択情報の保存
                EpgEventInfo oldItem = null;
                List<EpgEventInfo> oldItems = new List<EpgEventInfo>();
                StoreListViewSelected(ref oldItem, ref oldItems);

                ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_result.DataContext);
                if (dataView != null)
                {
                    dataView.SortDescriptions.Clear();
                    dataView.Refresh();
                }
                listView_result.DataContext = null;

                resultList.Clear();


                EpgSearchKeyInfo key = new EpgSearchKeyInfo();
                searchKeyView.GetSearchKey(ref key);
                key.andKey = key.andKey.Substring(key.andKey.StartsWith("^!{999}") ? 7 : 0);
                List<EpgSearchKeyInfo> keyList = new List<EpgSearchKeyInfo>();

                keyList.Add(key);
                List<EpgEventInfo> list = new List<EpgEventInfo>();

                cmd.SendSearchPg(keyList, ref list);
                foreach (EpgEventInfo info in list)
                {
                    SearchItem item = new SearchItem();
                    item.EventInfo = info;

                    if (item.EventInfo.start_time.AddSeconds(item.EventInfo.durationSec) > DateTime.Now)
                    {
                        foreach (ReserveData info2 in CommonManager.Instance.DB.ReserveList.Values)
                        {
                            if (info.original_network_id == info2.OriginalNetworkID &&
                                info.transport_stream_id == info2.TransportStreamID &&
                                info.service_id == info2.ServiceID &&
                                info.event_id == info2.EventID)
                            {
                                item.ReserveInfo = info2;
                                break;
                            }
                        }
                        UInt64 serviceKey = CommonManager.Create64Key(info.original_network_id, info.transport_stream_id, info.service_id);
                        if (ChSet5.Instance.ChList.ContainsKey(serviceKey) == true)
                        {
                            item.ServiceName = ChSet5.Instance.ChList[serviceKey].ServiceName;
                        }
                        if (Settings.Instance.FixSearchResult)
                        {
                            item.EventInfo.ShortInfo.text_char = "省略";
                        }

                        resultList.Add(item);
                    }
                }

                listView_result.DataContext = resultList;
                //if (_lastHeaderClicked != null) {
                //    Sort(_lastHeaderClicked, _lastDirection);
                //} else {
                //    string header = ((Binding)gridView_result.Columns[1].DisplayMemberBinding).Path.Path;
                //    Sort(header, _lastDirection);
                //    _lastHeaderClicked = header;
                //}
                if (this.gridViewSorter.isExistSortParams)
                {
                    this.gridViewSorter.SortByMultiHeader(this.resultList);
                }
                else
                {
                    this.gridViewSorter.resetSortParams();
                    this.gridViewSorter.SortByMultiHeader(
                        this.resultList,
                        gridView_result.Columns[1].Header as GridViewColumnHeader);
                }

                searchKeyView.SaveSearchLog();

                //選択情報の復元
                RestoreListViewSelected(oldItem, oldItems);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void StoreListViewSelected(ref EpgEventInfo oldItem, ref List<EpgEventInfo> oldItems)
        {
            if (listView_result.SelectedItem != null)
            {
                oldItem = (listView_result.SelectedItem as SearchItem).EventInfo;
                foreach (SearchItem item in listView_result.SelectedItems)
                {
                    oldItems.Add(item.EventInfo);
                }
            }
        }

        private void RestoreListViewSelected(EpgEventInfo oldItem, List<EpgEventInfo> oldItems)
        {
            if (oldItem != null)
            {
                //このUnselectAll()は無いと正しく復元出来ない状況があり得る
                listView_result.UnselectAll();

                foreach (SearchItem item in listView_result.Items)
                {
                    if (item.EventInfo.original_network_id == oldItem.original_network_id &&
                        item.EventInfo.transport_stream_id == oldItem.transport_stream_id &&
                        item.EventInfo.service_id == oldItem.service_id &&
                        item.EventInfo.event_id == oldItem.event_id)
                    {
                        listView_result.SelectedItem = item;
                    }
                }

                foreach (EpgEventInfo oldItem1 in oldItems)
                {
                    foreach (SearchItem item in listView_result.Items)
                    {
                        if (item.EventInfo.original_network_id == oldItem1.original_network_id &&
                            item.EventInfo.transport_stream_id == oldItem1.transport_stream_id &&
                            item.EventInfo.service_id == oldItem1.service_id &&
                            item.EventInfo.event_id == oldItem1.event_id)
                        {
                            listView_result.SelectedItems.Add(item);
                            break;
                        }
                    }
                }
            }
        }

        private void button_add_reserve_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_result.SelectedItem != null)
                {
                    List<ReserveData> list = new List<ReserveData>();
                    RecSettingData setInfo = new RecSettingData();

                    if ((sender.GetType() == typeof(MenuItem)) &&
                        (((MenuItem)sender).Parent != null) &&
                        ((((MenuItem)sender).Parent).GetType() == typeof(MenuItem)) &&
                        (((MenuItem)(((MenuItem)sender).Parent)).Name == "cmdPreset"))
                    {
                        //プリセットからの呼び出し
                        UInt32 presetID = (UInt32)(((MenuItem)sender).DataContext);
                        Settings.GetDefRecSetting(presetID, ref setInfo);
                    }
                    else
                    {
                        //現在の設定画面の設定を呼び出し
                        recSettingView.GetRecSetting(ref setInfo);
                    }

                    foreach (SearchItem item in listView_result.SelectedItems)
                    {
                        EpgEventInfo eventInfo = item.EventInfo;
                        if (item.IsReserved == false && eventInfo.StartTimeFlag != 0)
                        {
                            ReserveData reserveInfo = new ReserveData();
                            if (eventInfo.ShortInfo != null)
                            {
                                reserveInfo.Title = eventInfo.ShortInfo.event_name;
                            }

                            reserveInfo.StartTime = eventInfo.start_time;
                            reserveInfo.StartTimeEpg = eventInfo.start_time;

                            if (eventInfo.DurationFlag == 0)
                            {
                                reserveInfo.DurationSecond = 10 * 60;
                            }
                            else
                            {
                                reserveInfo.DurationSecond = eventInfo.durationSec;
                            }

                            UInt64 key = CommonManager.Create64Key(eventInfo.original_network_id, eventInfo.transport_stream_id, eventInfo.service_id);
                            if (ChSet5.Instance.ChList.ContainsKey(key) == true)
                            {
                                reserveInfo.StationName = ChSet5.Instance.ChList[key].ServiceName;
                            }
                            reserveInfo.OriginalNetworkID = eventInfo.original_network_id;
                            reserveInfo.TransportStreamID = eventInfo.transport_stream_id;
                            reserveInfo.ServiceID = eventInfo.service_id;
                            reserveInfo.EventID = eventInfo.event_id;

                            reserveInfo.RecSetting = setInfo;

                            list.Add(reserveInfo);
                        }
                    }

                    if (list.Count > 0)
                    {
                        ErrCode err = (ErrCode)cmd.SendAddReserve(list);
                        if (err == ErrCode.CMD_ERR_CONNECT)
                        {
                            MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                        }
                        if (err == ErrCode.CMD_ERR_TIMEOUT)
                        {
                            MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                        }
                        if (err != ErrCode.CMD_SUCCESS)
                        {
                            MessageBox.Show("予約登録でエラーが発生しました。終了時間がすでに過ぎている可能性があります。");
                        }

                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                        CommonManager.Instance.DB.ReloadReserveInfo();
                        SearchPg();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_delall_reserve_Click(object sender, RoutedEventArgs e)
        {
            string text1 = "予約されている全ての項目を削除しますか?";
            string caption1 = "予約全削除の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) == MessageBoxResult.OK)
            {
                //更新前の選択情報の保存
                EpgEventInfo oldItem = null;
                List<EpgEventInfo> oldItems = new List<EpgEventInfo>();
                StoreListViewSelected(ref oldItem, ref oldItems);

                listView_result.SelectAll();
                MenuItem_Click_DeleteItem(listView_result.SelectedItem, new RoutedEventArgs());
                listView_result.UnselectAll();//未選択から実行された場合は後ろのRestoreが動かないので先に解除する。

                RestoreListViewSelected(oldItem, oldItems);
            }
        }

        private void button_add_epgAutoAdd_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                EpgAutoAddData addItem = new EpgAutoAddData();
                EpgSearchKeyInfo searchKey = new EpgSearchKeyInfo();
                searchKeyView.GetSearchKey(ref searchKey);

                RecSettingData recSetKey = new RecSettingData();
                recSettingView.GetRecSetting(ref recSetKey);

                addItem.searchInfo = searchKey;
                addItem.recSetting = recSetKey;

                List<EpgAutoAddData> addList = new List<EpgAutoAddData>();
                addList.Add(addItem);

                if (CommonManager.Instance.DB.EpgAutoAddList.Count == 0)
                {
                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                    CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                }

                if (cmd.SendAddEpgAutoAdd(addList) != 1)
                {
                    MessageBox.Show("追加に失敗しました");
                }
                else
                {
                    List<uint> oldlist = CommonManager.Instance.DB.EpgAutoAddList.Keys.ToList();

                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                    CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                    
                    List<uint> newlist = CommonManager.Instance.DB.EpgAutoAddList.Keys.ToList();
                    List<uint> diflist = newlist.Except(oldlist).ToList();

                    if (diflist.Count == 1)
                    {
                        EpgAutoAddData newinfo = CommonManager.Instance.DB.EpgAutoAddList[diflist[0]];
                        this.SetViewMode(2);
                        this.SetChgAutoAddID(newinfo.dataID);
                        
                        //情報の再読み込みは不要なはずだが、安全のため実行しておく
                        this.SetSearchDefKey(newinfo.searchInfo);
                        this.SetRecInfoDef(newinfo.recSetting);

                        MainWindow mainWindow = Application.Current.MainWindow as MainWindow;
                        mainWindow.autoAddView.epgAutoAddView.UpdateInfo();
                        UpdateEpgAutoAddViewSelection();
                    }
                    //

                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                    CommonManager.Instance.DB.ReloadReserveInfo();
                    SearchPg();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private bool CheckExistAutoAddItem()
        {
            bool retval = CommonManager.Instance.DB.EpgAutoAddList.ContainsKey(this.autoAddID);
            if (retval == false)
            {
                MessageBox.Show("項目がありません。\r\n" +
                    "既に削除されています。\r\n" +
                    "(別のEpgtimerによる操作など)");
                SetViewMode(1);
                this.autoAddID = 0;
            }
            return retval;
        }

        private void button_chg_epgAutoAdd_Click(object sender, RoutedEventArgs e)
        {
            if (CheckExistAutoAddItem() == false) return;

            try
            {
                EpgAutoAddData addItem = new EpgAutoAddData();
                addItem.dataID = autoAddID;
                EpgSearchKeyInfo searchKey = new EpgSearchKeyInfo();
                searchKeyView.GetSearchKey(ref searchKey);

                RecSettingData recSetKey = new RecSettingData();
                recSettingView.GetRecSetting(ref recSetKey);

                addItem.searchInfo = searchKey;
                addItem.recSetting = recSetKey;

                List<EpgAutoAddData> addList = new List<EpgAutoAddData>();
                addList.Add(addItem);

                if (cmd.SendChgEpgAutoAdd(addList) != 1)
                {
                    MessageBox.Show("変更に失敗しました");
                }
                else
                {
                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                    CommonManager.Instance.DB.ReloadReserveInfo();
                    SearchPg();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_del_epgAutoAdd_Click(object sender, RoutedEventArgs e)
        {
            if (CheckExistAutoAddItem() == false) return;

            try
            {
                List<UInt32> delIDList = new List<uint>();
                delIDList.Add(autoAddID);
                cmd.SendDelEpgAutoAdd(delIDList);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

            SetViewMode(1);
            this.autoAddID = 0;
        }

        private void listView_result_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            this.MenuItem_Click_ShowDialog(listView_result.SelectedItem, new RoutedEventArgs());
        }

        private void ChangeReserve(ReserveData reserveInfo)
        {
            try
            {
                ChgReserveWindow dlg = new ChgReserveWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetReserveInfo(reserveInfo);
                if (dlg.ShowDialog() == true)
                {
                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                    CommonManager.Instance.DB.ReloadReserveInfo();
                    SearchPg();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void AddReserve(EpgEventInfo eventInfo)
        {
            try
            {
                AddReserveEpgWindow dlg = new AddReserveEpgWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetEventInfo(eventInfo);
                if (dlg.ShowDialog() == true)
                {
                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                    CommonManager.Instance.DB.ReloadReserveInfo();
                    SearchPg();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }


        GridViewSorter<SearchItem> gridViewSorter = new GridViewSorter<SearchItem>();

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;
            //ListSortDirection direction;

            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {

                    this.gridViewSorter.SortByMultiHeader(this.resultList, headerClicked);
                    listView_result.Items.Refresh();

                    //string header = "Reserved";
                    //if (headerClicked.Column.DisplayMemberBinding != null) {
                    //    header = ((Binding)headerClicked.Column.DisplayMemberBinding).Path.Path;
                    //}
                    //if (String.Compare(header, _lastHeaderClicked) != 0) {
                    //    direction = ListSortDirection.Ascending;
                    //    _lastHeaderClicked2 = _lastHeaderClicked;
                    //    _lastDirection2 = _lastDirection;
                    //} else {
                    //    if (_lastDirection == ListSortDirection.Ascending) {
                    //        direction = ListSortDirection.Descending;
                    //    } else {
                    //        direction = ListSortDirection.Ascending;
                    //    }
                    //}

                    //Sort(header, direction);

                    //_lastHeaderClicked = header;
                    //_lastDirection = direction;
                }
            }
        }

        //private void Sort(string sortBy, ListSortDirection direction) {
        //    try {
        //        ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_result.DataContext);

        //        dataView.SortDescriptions.Clear();

        //        SortDescription sd = new SortDescription(sortBy, direction);
        //        dataView.SortDescriptions.Add(sd);
        //        if (_lastHeaderClicked2 != null) {
        //            if (String.Compare(sortBy, _lastHeaderClicked2) != 0) {
        //                SortDescription sd2 = new SortDescription(_lastHeaderClicked2, _lastDirection2);
        //                dataView.SortDescriptions.Add(sd2);
        //            }
        //        }
        //        dataView.Refresh();

        //        //Settings.Instance.ResColumnHead = sortBy;
        //        //Settings.Instance.ResSortDirection = direction;

        //    } catch (Exception ex) {
        //        MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
        //    }
        //}

        private void Window_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (this.WindowState == WindowState.Normal)
            {
                if (this.Visibility == System.Windows.Visibility.Visible && this.Width > 0 && this.Height > 0)
                {
                    Settings.Instance.SearchWndWidth = this.Width;
                    Settings.Instance.SearchWndHeight = this.Height;
                }
            }
        }

        private void Window_LocationChanged(object sender, EventArgs e)
        {
            if (this.WindowState == WindowState.Normal)
            {
                if (this.Visibility == System.Windows.Visibility.Visible && this.Top > 0 && this.Left > 0)
                {
                    Settings.Instance.SearchWndTop = this.Top;
                    Settings.Instance.SearchWndLeft = this.Left;
                }
            }
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control) && Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
            {
                switch (e.Key)
                {
                    case Key.F:
                        new BlackoutWindow(this).showWindow(this.button_search.Content.ToString());
                        this.button_search.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.S:
                        new BlackoutWindow(this).showWindow(this.button_add_reserve.Content.ToString());
                        this.button_add_reserve.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.A:
                        new BlackoutWindow(this).showWindow(this.button_add_epgAutoAdd.Content.ToString());
                        this.button_add_epgAutoAdd.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        this.Close();
                        break;
                    case Key.C:
                        new BlackoutWindow(this).showWindow(this.button_chg_epgAutoAdd.Content.ToString());
                        this.button_chg_epgAutoAdd.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        this.Close();
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.F3:
                        this.MenuItem_Click_ProgramTable(this, new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.Escape:
                        this.Close();
                        break;
                    case Key.Delete:
                        this.deleteItem();
                        break;
                }
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if (Title == "検索")
            {
                this.searchKeyView.ComboBox_andKey.Focus();
            }
            else
            {
                this.SearchPg();
            }
        }

        void listView_result_KeyDown(object sender, KeyEventArgs e)
        {
            switch (e.Key)
            {
                case Key.Enter:
                    this.MenuItem_Click_ShowDialog(listView_result.SelectedItem, new RoutedEventArgs());
                    break;
            }
        }

        void deleteItem()
        {
            if (listView_result.SelectedItem == null) { return; }
            //
            List<ReserveData> delList = new List<ReserveData>();
            foreach (SearchItem item in listView_result.SelectedItems)
            {
                if (item.IsReserved == true)
                {
                    delList.Add(item.ReserveInfo);
                }
            }
            if (delList.Count == 0) { return; }

            string text1 = "削除しますか？" + "　[削除アイテム数: " + delList.Count + "]" + "\n\n";
            foreach (ReserveData item in delList)
            {
                text1 += " ・ " + item.Title + "\n";
            }
            string caption1 = "登録項目削除の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) == MessageBoxResult.OK)
            {
                this.MenuItem_Click_DeleteItem(this.listView_result.SelectedItem, new RoutedEventArgs(Button.ClickEvent));
            }
        }

        private void MenuItem_Click_DeleteItem(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                List<UInt32> list = new List<UInt32>();

                foreach (SearchItem item in listView_result.SelectedItems)
                {
                    if (item.IsReserved == true)
                    {
                        list.Add(item.ReserveInfo.ReserveID);
                    }
                }

                if (list.Count > 0)
                {
                    ErrCode err = (ErrCode)cmd.SendDelReserve(list);
                    if (err == ErrCode.CMD_ERR_CONNECT)
                    {
                        MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                    }
                    if (err == ErrCode.CMD_ERR_TIMEOUT)
                    {
                        MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                    }
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show("予約削除でエラーが発生しました。");
                    }
                }
                //
                CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                CommonManager.Instance.DB.ReloadReserveInfo();
                SearchPg();
                //
                //listView_result.SelectedItem = null;
            }
        }

        private void MenuItem_Click_ChangeOnOff(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                //new BlackoutWindow(this).showWindow("予約←→無効");

                List<ReserveData> list = new List<ReserveData>();
                bool IsExistNewReserve = false;

                foreach (SearchItem item in listView_result.SelectedItems)
                {
                    if (item.IsReserved == true)
                    {
                        if (item.ReserveInfo.RecSetting.RecMode == 5)
                        {
                            // 無効 => 予約
                            RecSettingData defSet = new RecSettingData();
                            Settings.GetDefRecSetting(0, ref defSet);
                            item.ReserveInfo.RecSetting.RecMode = defSet.RecMode;
                        }
                        else
                        {
                            //予約 => 無効
                            item.ReserveInfo.RecSetting.RecMode = 5;
                        }

                        list.Add(item.ReserveInfo);
                    }
                    else
                    {
                        IsExistNewReserve = true;
                    }
                }

                if (list.Count > 0)
                {
                    try
                    {
                        ErrCode err = (ErrCode)cmd.SendChgReserve(list);
                        if (err == ErrCode.CMD_ERR_CONNECT)
                        {
                            MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                        }
                        if (err == ErrCode.CMD_ERR_TIMEOUT)
                        {
                            MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                        }
                        if (err != ErrCode.CMD_SUCCESS)
                        {
                            MessageBox.Show("予約変更でエラーが発生しました。");
                        }
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                    }
                }

                if (IsExistNewReserve == true)
                {
                    button_add_reserve_Click(sender, e);
                }
                else
                {
                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                    CommonManager.Instance.DB.ReloadReserveInfo();
                    SearchPg();
                }
            }
        }
        
        private void MenuItem_Click_ShowDialog(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_result.SelectedItem != null)
                {
                    SearchItem item = listView_result.SelectedItems[listView_result.SelectedItems.Count-1] as SearchItem;
                    listView_result.UnselectAll();
                    listView_result.SelectedItem = item;
                    if (item.IsReserved == true)
                    {
                        ChangeReserve(item.ReserveInfo);
                    }
                    else
                    {
                        AddReserve(item.EventInfo);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void MenuItem_Click_RecMode(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                List<ReserveData> list = new List<ReserveData>();

                foreach (SearchItem item in listView_result.SelectedItems)
                {
                    if (item.IsReserved == true)
                    {
                        item.ReserveInfo.RecSetting.RecMode = byte.Parse((string)((MenuItem)sender).Tag);
                        list.Add(item.ReserveInfo);
                    }
                }

                if (list.Count > 0)
                {
                    try
                    {
                        ErrCode err = (ErrCode)cmd.SendChgReserve(list);
                        if (err == ErrCode.CMD_ERR_CONNECT)
                        {
                            MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                        }
                        if (err == ErrCode.CMD_ERR_TIMEOUT)
                        {
                            MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                        }
                        if (err != ErrCode.CMD_SUCCESS)
                        {
                            MessageBox.Show("予約変更でエラーが発生しました。");
                        }

                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                        CommonManager.Instance.DB.ReloadReserveInfo();
                        SearchPg();
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                    }
                }
            }
        }

        private void MenuItem_Click_Priority(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                List<ReserveData> list = new List<ReserveData>();

                foreach (SearchItem item in listView_result.SelectedItems)
                {
                    if (item.IsReserved == true)
                    {
                        item.ReserveInfo.RecSetting.Priority = byte.Parse((string)((MenuItem)sender).Tag);
                        list.Add(item.ReserveInfo);
                    }
                }

                if (list.Count > 0)
                {
                    try
                    {
                        ErrCode err = (ErrCode)cmd.SendChgReserve(list);
                        if (err == ErrCode.CMD_ERR_CONNECT)
                        {
                            MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                        }
                        if (err == ErrCode.CMD_ERR_TIMEOUT)
                        {
                            MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                        }
                        if (err != ErrCode.CMD_SUCCESS)
                        {
                            MessageBox.Show("予約変更でエラーが発生しました。");
                        }

                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                        CommonManager.Instance.DB.ReloadReserveInfo();
                        SearchPg();
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                    }
                }
            }
        }

        private void MenuItem_Click_ProgramTable(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                SearchItem item1 = listView_result.SelectedItems[listView_result.SelectedItems.Count-1] as SearchItem;
                listView_result.UnselectAll();
                listView_result.SelectedItem = item1;
                BlackoutWindow.selectedSearchItem = item1;
                MainWindow mainWindow1 = this.Owner as MainWindow;
                if (mainWindow1 != null)
                {
                    if (BlackoutWindow.unvisibleSearchWindow != null)
                    {
                        // 非表示で保存するSearchWindowを1つに限定するため
                        this.Close();
                    }
                    else
                    {
                        this.Hide();
                        mainWindow1.EmphasizeSearchButton(true);
                        BlackoutWindow.unvisibleSearchWindow = this;
                    }
                    mainWindow1.moveTo_tabItem_epg();
                    mainWindow1.Hide(); // EpgDataView.UserControl_IsVisibleChangedイベントを発生させる
                    mainWindow1.Show();
                }
            }
        }

        private void Window_IsVisibleChanged_1(object sender, DependencyPropertyChangedEventArgs e)
        {
            MainWindow mainWindow1 = this.Owner as MainWindow;
            if (this.IsVisible)
            {
                if (BlackoutWindow.unvisibleSearchWindow == this)
                {
                    mainWindow1.EmphasizeSearchButton(false);
                    BlackoutWindow.unvisibleSearchWindow = null;
                }
            }
        }

        private void MenuItem_Click_Research(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                SearchItem item = listView_result.SelectedItems[listView_result.SelectedItems.Count - 1] as SearchItem;
                EpgSearchKeyInfo defKey = new EpgSearchKeyInfo();
                defKey.andKey = item.EventName;
                defKey.serviceList.Clear();

                foreach (ServiceItem info in searchKeyView.searchKeyDescView.listView_service.Items)
                {
                    if (info.ServiceName.Equals(item.ServiceName))
                    {
                        defKey.serviceList.Add((long)info.ID);
                    }
                }
                searchKeyView.SetSearchKey(defKey);

                button_search_Click(sender, e);
            }
        }

        private void MenuItem_Click_Research2(object sender, RoutedEventArgs e)
        {
            //「番組名で再検索」と比べてどうなのという感じだが、元の検索を残したまま作業できる
            //新番組チェックなんかには向いてるかもしれないが、機能としては微妙なところ。
            if (listView_result.SelectedItem != null)
            {
                SearchItem item = listView_result.SelectedItems[listView_result.SelectedItems.Count - 1] as SearchItem;
                listView_result.UnselectAll();
                listView_result.SelectedItem = item;

                SearchWindow dlg = new SearchWindow();

                //SearchWindowからの呼び出しを記録する。表示制御などでも使う。
                dlg.Owner = this;
                dlg.SetViewMode((ushort)(Title == "検索" ? 3 : 1));

                EpgSearchKeyInfo key = new EpgSearchKeyInfo();
                key.andKey = item.EventName;

                Int64 sidKey = ((Int64)item.EventInfo.original_network_id) << 32 | ((Int64)item.EventInfo.transport_stream_id) << 16 | ((Int64)item.EventInfo.service_id);
                key.serviceList.Add(sidKey);

                dlg.SetSearchDefKey(key);

                //録画条件を現在のウィンドウから引っ張れるが、他との整合からしてもデフォルトでよさそう
                //if (CommonManager.Instance.DB.EpgAutoAddList.ContainsKey(this.autoAddID) == true)
                //{
                //    dlg.SetRecInfoDef(CommonManager.Instance.DB.EpgAutoAddList[this.autoAddID].recSetting);
                //}

                dlg.Title += "(サブウィンドウ)";

                //dlg.Left += 50;//なぜか動かせない‥
                //dlg.Top += 50;
                dlg.ShowDialog();
            }
        }

        private void cmdMenu_Loaded(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                foreach (object item in ((ContextMenu)sender).Items)
                {
                    //孫ウィンドウは禁止。番組表表示とも相性悪いのでキャンセル。
                    if (item is MenuItem && ((((MenuItem)item).Name == "cmdResearch2") || (((MenuItem)item).Name == "cmdProgramTable")))
                    {
                        if (this.Owner as SearchWindow != null)
                        {
                            ((MenuItem)item).IsEnabled = false;
                            ((MenuItem)item).Header += ((MenuItem)item).Header.ToString().EndsWith("(無効)") ? "" : "(無効)";
                        }
                    }
                    else if (item is MenuItem && (((MenuItem)item).Name == "cmdDlt"))
                    {
                        bool isReserved = false;
                        foreach (SearchItem selItem in listView_result.SelectedItems)
                        {
                            isReserved |= selItem.IsReserved;
                        }
                        ((MenuItem)item).IsEnabled = isReserved;
                    }
                    else if (item is MenuItem && (((MenuItem)item).Name == "cmdAdd"))
                    {
                        bool isReserved = true;
                        foreach (SearchItem selItem in listView_result.SelectedItems)
                        {
                            isReserved &= selItem.IsReserved;
                        }
                        ((MenuItem)item).IsEnabled = !isReserved;

                        if (((MenuItem)item).IsEnabled == true)
                        {
                            MenuItem subItem = ((MenuItem)item).Items[2] as MenuItem;
                            subItem.Items.Clear();

                            foreach (RecPresetItem info in Settings.Instance.RecPresetList)
                            {
                                MenuItem menuItem = new MenuItem();
                                menuItem.Header = info.DisplayName;
                                menuItem.DataContext = info.ID;
                                menuItem.Click += new RoutedEventHandler(button_add_reserve_Click);

                                subItem.Items.Add(menuItem);
                            }
                        }
                    }
                    else if (item is MenuItem && ((MenuItem)item).Name == "cmdChg")
                    {
                        //選択されているすべての予約が同じ設定の場合だけチェックを表示する
                        byte recMode = 0xFF;
                        byte priority = 0xFF;
                        foreach (SearchItem selItem in listView_result.SelectedItems)
                        {
                            if (selItem.IsReserved == true)
                            {
                                if (recMode == 0xFF)
                                {
                                    recMode = selItem.ReserveInfo.RecSetting.RecMode;
                                }
                                else if (recMode != selItem.ReserveInfo.RecSetting.RecMode)
                                {
                                    recMode = 0xFE;
                                }
                                if (priority == 0xFF)
                                {
                                    priority = selItem.ReserveInfo.RecSetting.Priority;
                                }
                                else if (priority != selItem.ReserveInfo.RecSetting.Priority)
                                {
                                    priority = 0xFE;
                                }
                            }
                        }

                        if (recMode != 0xFF)
                        {
                            ((MenuItem)item).IsEnabled = true;
                            for (int i = 0; i <= 5; i++)
                            {
                                ((MenuItem)((MenuItem)item).Items[i + 2]).IsChecked = (i == recMode);
                            }
                            for (int i = 6 + 2; i < ((MenuItem)item).Items.Count; i++)
                            {
                                MenuItem subItem = ((MenuItem)item).Items[i] as MenuItem;
                                if (subItem != null && subItem.Name == "cmdPri")
                                {
                                    for (int j = 0; j < subItem.Items.Count; j++)
                                    {
                                        ((MenuItem)subItem.Items[j]).IsChecked = (j + 1 == priority);
                                    }
                                    subItem.Header = string.Format((string)subItem.Tag, priority < 0xFE ? "" + priority : "*");
                                    break;
                                }
                            }
                        }
                        else
                        {
                            ((MenuItem)item).IsEnabled = false;
                        }
                    }
                }
            }
        }

        private void Window_Closed(object sender, System.EventArgs e)
        {
            if (this.Owner as SearchWindow != null)
            {
                (this.Owner as SearchWindow).SearchPg();
            }
        }
        
        private void Window_Activated(object sender, EventArgs e)
        {
            UpdateEpgAutoAddViewSelection();
        }

        private void UpdateEpgAutoAddViewSelection()
        {
            MainWindow mainWindow = Application.Current.MainWindow as MainWindow;
            mainWindow.autoAddView.epgAutoAddView.UpdateListViewSelection(this.autoAddID);
        }
    }
}

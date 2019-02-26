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

namespace EpgTimer
{
    /// <summary>
    /// SearchWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SearchWindow : Window
    {
        private List<KeyValuePair<string, bool>> listSortHistory =
            new List<KeyValuePair<string, bool>>() { new KeyValuePair<string, bool>("StartTime", false) };

        private UInt32 autoAddID = 0;

        public SearchWindow()
        {
            InitializeComponent();

            try
            {
                //ウインドウ位置の復元
                if (Settings.Instance.SearchWndWidth != 0)
                {
                    this.Width = Settings.Instance.SearchWndWidth;
                }
                if (Settings.Instance.SearchWndHeight != 0)
                {
                    this.Height = Settings.Instance.SearchWndHeight;
                }
                if (Settings.Instance.SearchWndTabsHeight > 405)
                {
                    //操作不可能な値をセットしないよう努める
                    grid_Tabs.Height = new GridLength(Math.Min(Settings.Instance.SearchWndTabsHeight, Height));
                }
                searchKeyView.SetSearchKey(Settings.Instance.CreateDefSearchSetting());
            }
            catch
            {
            }
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
            if (mode == 1)
            {
                button_add_reserve.Visibility = System.Windows.Visibility.Hidden;
                button_add_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;

                Title = "EPG予約条件";
            }
            else if (mode == 2)
            {
                button_add_reserve.Visibility = System.Windows.Visibility.Hidden;
                button_add_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;

                Title = "EPG予約条件";
            }
            else
            {
                button_add_reserve.Visibility = System.Windows.Visibility.Visible;
                button_add_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;

                Title = "検索";
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
                var resultList = new List<SearchItem>();

                EpgSearchKeyInfo key = searchKeyView.GetSearchKey();
                key.andKey = key.andKey.Substring(key.andKey.StartsWith("^!{999}", StringComparison.Ordinal) ? 7 : 0);
                List<EpgEventInfo> list;
                if (Settings.Instance.NgAutoEpgLoadNW)
                {
                    //EPGデータの遅延更新のため内部キャッシュは利用しない
                    list = new List<EpgEventInfo>();
                    CommonManager.CreateSrvCtrl().SendSearchPg(new List<EpgSearchKeyInfo>() { key }, ref list);
                }
                else
                {
                    CommonManager.Instance.DB.SearchPg(new List<EpgSearchKeyInfo>() { key }, out list);
                }
                DateTime now = DateTime.UtcNow.AddHours(9);
                foreach (EpgEventInfo info in list ?? Enumerable.Empty<EpgEventInfo>())
                {
                    SearchItem item = new SearchItem(info, false);

                    if (item.EventInfo.start_time.AddSeconds(item.EventInfo.DurationFlag == 0 ? 0 : item.EventInfo.durationSec) > now)
                    {
                        UInt64 serviceKey = CommonManager.Create64Key(info.original_network_id, info.transport_stream_id, info.service_id);
                        if (ChSet5.Instance.ChList.ContainsKey(serviceKey) == true)
                        {
                            item.ServiceName = ChSet5.Instance.ChList[serviceKey].ServiceName;
                        }
                        resultList.Add(item);
                    }
                }

                listView_result.ItemsSource = resultList;
                RefreshReserve();
                Sort();

                searchKeyView.SaveSearchLog();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void RefreshReserve()
        {
            if (listView_result.ItemsSource != null)
            {
                foreach (SearchItem item in listView_result.ItemsSource)
                {
                    item.ReserveInfo = null;
                    foreach (ReserveData info in CommonManager.Instance.DB.ReserveList.Values)
                    {
                        if (item.EventInfo.original_network_id == info.OriginalNetworkID &&
                            item.EventInfo.transport_stream_id == info.TransportStreamID &&
                            item.EventInfo.service_id == info.ServiceID &&
                            item.EventInfo.event_id == info.EventID)
                        {
                            item.ReserveInfo = info;
                            break;
                        }
                    }
                }
                CollectionViewSource.GetDefaultView(listView_result.ItemsSource).Refresh();
            }
        }

        private void button_add_reserve_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_result.SelectedItem != null)
                {
                    List<ReserveData> list = new List<ReserveData>();
                    RecSettingData setInfo = recSettingView.GetRecSetting();

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
                        ErrCode err = CommonManager.CreateSrvCtrl().SendAddReserve(list);
                        if (err != ErrCode.CMD_SUCCESS)
                        {
                            MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約登録でエラーが発生しました。終了時間がすでに過ぎている可能性があります。");
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_add_epgAutoAdd_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                EpgAutoAddData addItem = new EpgAutoAddData();
                addItem.searchInfo = searchKeyView.GetSearchKey();
                addItem.recSetting = recSettingView.GetRecSetting();

                List<EpgAutoAddData> addList = new List<EpgAutoAddData>();
                addList.Add(addItem);

                if (CommonManager.CreateSrvCtrl().SendAddEpgAutoAdd(addList) != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show("追加に失敗しました");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_chg_epgAutoAdd_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                EpgAutoAddData addItem = new EpgAutoAddData();
                addItem.dataID = autoAddID;
                addItem.searchInfo = searchKeyView.GetSearchKey();
                addItem.recSetting = recSettingView.GetRecSetting();

                List<EpgAutoAddData> addList = new List<EpgAutoAddData>();
                addList.Add(addItem);

                if (CommonManager.CreateSrvCtrl().SendChgEpgAutoAdd(addList) != ErrCode.CMD_SUCCESS)
                {
                    MessageBox.Show("変更に失敗しました");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void listView_result_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            try
            {
                if (listView_result.SelectedItem != null)
                {
                    SearchItem item = listView_result.SelectedItem as SearchItem;
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

        private void ChangeReserve(ReserveData reserveInfo)
        {
            try
            {
                ChgReserveWindow dlg = new ChgReserveWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetReserveInfo(reserveInfo);
                dlg.ShowDialog();
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
                dlg.ShowDialog();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;

            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    int index = listSortHistory.FindIndex(item => item.Key == (string)headerClicked.Tag);
                    if (index == 0)
                    {
                        listSortHistory[0] = new KeyValuePair<string, bool>(listSortHistory[0].Key, !listSortHistory[0].Value);
                    }
                    else
                    {
                        if (index > 0)
                        {
                            listSortHistory.RemoveAt(index);
                        }
                        listSortHistory.Insert(0, new KeyValuePair<string, bool>((string)headerClicked.Tag, false));
                    }
                    Sort();
                }
            }
        }

        private void Sort()
        {
            if (listView_result.ItemsSource == null)
            {
                return;
            }
            ICollectionView dataView = CollectionViewSource.GetDefaultView(listView_result.ItemsSource);

            using (dataView.DeferRefresh())
            {
                dataView.SortDescriptions.Clear();
                foreach (KeyValuePair<string, bool> item in listSortHistory)
                {
                    dataView.SortDescriptions.Add(
                        new SortDescription(item.Key, item.Value ? ListSortDirection.Descending : ListSortDirection.Ascending));
                }
            }
        }

        private void Window_Closing(object sender, CancelEventArgs e)
        {
            Settings.Instance.SearchWndWidth = Width;
            Settings.Instance.SearchWndHeight = Height;
            Settings.Instance.SearchWndTabsHeight = grid_Tabs.Height.Value;
            CommonManager.Instance.DB.ReserveInfoChanged -= RefreshReserve;
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control) && Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
            {
                switch (e.Key)
                {
                    case Key.F:
                        this.button_search.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.S:
                        this.button_add_reserve.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.A:
                        this.button_add_epgAutoAdd.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        this.Close();
                        break;
                    case Key.C:
                        this.button_chg_epgAutoAdd.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        this.Close();
                        break;
                }
            }
            else
            {
                switch (e.Key)
                {
                    case Key.F3:
                        this.MenuItem_Click_ProgramTable(this, new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.Escape:
                        {
                            this.Close();
                        }
                        break;
                }
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if (Title == "検索")
            {
                this.searchKeyView.FocusAndKey();
            }
            else
            {
                this.SearchPg();
            }
            CommonManager.Instance.DB.ReserveInfoChanged -= RefreshReserve;
            CommonManager.Instance.DB.ReserveInfoChanged += RefreshReserve;
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
                    ErrCode err = CommonManager.CreateSrvCtrl().SendDelReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約削除でエラーが発生しました。");
                    }
                }
            }
        }

        private void MenuItem_Click_ShowDialog(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                SearchItem item = listView_result.SelectedItem as SearchItem;
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
                        ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                        if (err != ErrCode.CMD_SUCCESS)
                        {
                            MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。");
                        }
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
                        ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                        if (err != ErrCode.CMD_SUCCESS)
                        {
                            MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。");
                        }
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
            SearchItem item1 = this.listView_result.SelectedItem as SearchItem;
            if (item1 != null)
            {
                MainWindow mainWindow1 = this.Owner as MainWindow;
                if (mainWindow1 != null)
                {
                    if (mainWindow1.OwnedWindows.OfType<SearchWindow>().Count() > 1)
                    {
                        // 非表示で保存するSearchWindowを1つに限定するため
                        this.Close();
                    }
                    else
                    {
                        this.Hide();
                        mainWindow1.EmphasizeSearchButton(true);
                    }
                    mainWindow1.SearchJumpTargetProgram(item1.EventInfo);
                }
            }
        }

        private void Window_IsVisibleChanged_1(object sender, DependencyPropertyChangedEventArgs e)
        {
            MainWindow mainWindow1 = this.Owner as MainWindow;
            if (this.IsVisible)
            {
                if (mainWindow1.OwnedWindows.OfType<SearchWindow>().Count() <= 1)
                {
                    mainWindow1.EmphasizeSearchButton(false);
                }
            }
        }

        private void MenuItem_Click_Research(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                SearchItem item = listView_result.SelectedItem as SearchItem;
                EpgSearchKeyInfo defKey = new EpgSearchKeyInfo();
                defKey.andKey = item.EventName;
                defKey.serviceList.Clear();

                foreach (ChSet5Item info in ChSet5.Instance.ChListSelected)
                {
                    if (info.ServiceName.Equals(item.ServiceName))
                    {
                        defKey.serviceList.Add((long)info.Key);
                    }
                }
                searchKeyView.SetSearchKey(defKey);

                button_search_Click(sender, e);
            }
        }

        private void cmdMenu_Loaded(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                foreach (object item in ((ContextMenu)sender).Items)
                {
                    if (item is MenuItem && (((MenuItem)item).Name == "cmdDlt"))
                    {
                        bool isReserved = false;
                        foreach (SearchItem selItem in listView_result.SelectedItems)
                        {
                            isReserved |= selItem.IsReserved;
                        }
                        ((MenuItem)item).IsEnabled = isReserved;
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
                                ((MenuItem)((MenuItem)item).Items[i]).IsChecked = (i == recMode);
                            }
                            for (int i = 6; i < ((MenuItem)item).Items.Count; i++)
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

    }
}

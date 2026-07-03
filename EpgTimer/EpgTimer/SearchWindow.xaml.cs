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

namespace EpgTimer
{
    /// <summary>
    /// SearchWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SearchWindow : Window
    {
        private List<KeyValuePair<string, bool>> listSortHistory =
            new List<KeyValuePair<string, bool>>() { new KeyValuePair<string, bool>("StartTime", false) };

        private uint autoAddID = 0;
        private bool searchOnLoaded = false;

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
            listView_result.AlternationCount = Settings.Instance.ResAlternationCount;
        }

        public void SetSearchDefKey(EpgSearchKeyInfo key)
        {
            searchKeyView.SetSearchKey(key);
            searchOnLoaded = true;
        }

        /// <summary>
        /// 自動登録情報をセットし、ウィンドウを変更モードにする
        /// </summary>
        public void SetChangeModeData(EpgAutoAddData item)
        {
            autoAddID = item.dataID;
            SetSearchDefKey(item.searchInfo);
            recSettingView.SetDefSetting(item.recSetting);
            Title = "EPG予約条件";
            button_chg_epgAutoAdd.Visibility = Visibility.Visible;
        }

        private void tabItem_searchKey_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter && e.IsRepeat == false)
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
                    var item = new SearchItem(info, false, false, false);
                    if (item.EventInfo.start_time.AddSeconds(item.EventInfo.DurationFlag == 0 ? 0 : item.EventInfo.durationSec) > now)
                    {
                        ulong serviceKey = CommonManager.Create64Key(info.original_network_id, info.transport_stream_id, info.service_id);
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

                if (Settings.Instance.SaveSearchKeyword)
                {
                    searchKeyView.SaveSearchLog();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void RefreshReserve()
        {
            var resultList = listView_result.ItemsSource as List<SearchItem>;
            if (resultList != null)
            {
                List<decimal> list = CommonManager.Instance.DB.ReserveList.Values.Select(a => CommonManager.Create64PgKey(
                    a.OriginalNetworkID, a.TransportStreamID, a.ServiceID, a.EventID) * ((decimal)uint.MaxValue + 1) + a.ReserveID).ToList();
                list.Sort();
                for (int i = 0; i < resultList.Count; i++)
                {
                    SearchItem item = resultList[i];
                    if (item.Duplicate)
                    {
                        resultList.RemoveAt(i--);
                    }
                    else
                    {
                        item.ReserveInfo = null;
                        decimal key = CommonManager.Create64PgKey(item.EventInfo.original_network_id, item.EventInfo.transport_stream_id,
                                                                  item.EventInfo.service_id, item.EventInfo.event_id) * ((decimal)uint.MaxValue + 1);
                        int index = list.BinarySearch(key);
                        index = index < 0 ? ~index : index;
                        for (; index < list.Count && list[index] <= key + uint.MaxValue; index++)
                        {
                            //予約情報が見つかった
                            if (item.ReserveInfo != null)
                            {
                                //さらに見つかった
                                item = new SearchItem(item.EventInfo, false, item.Filtered, true) { ServiceName = item.ServiceName };
                                resultList.Insert(++i, item);
                            }
                            item.ReserveInfo = CommonManager.Instance.DB.ReserveList[(uint)(list[index] % ((decimal)uint.MaxValue + 1))];
                        }
                    }
                }
                CollectionViewSource.GetDefaultView(listView_result.ItemsSource).Refresh();
            }
        }

        private void buttonOrMenuItem_add_reserve_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_result.SelectedItem != null)
                {
                    List<ReserveData> list = new List<ReserveData>();
                    RecSettingData setInfo = sender is MenuItem ? Settings.CreateRecSetting((uint)((MenuItem)sender).Tag) :
                                                                  recSettingView.GetRecSetting();

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

                            ulong key = CommonManager.Create64Key(eventInfo.original_network_id, eventInfo.transport_stream_id, eventInfo.service_id);
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
                MessageBox.Show(ex.ToString());
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
                Close();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
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
                Close();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void listView_result_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            MenuItem_Click_ShowDialog(sender, e);
        }

        private void ChangeReserve(ReserveData reserveInfo)
        {
            ((MainWindow)Application.Current.MainWindow).SwapOwnedReserveWindow(null);
            var dlg = new ChgReserveWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            dlg.SetReserveInfo(reserveInfo);
            dlg.ShowDialog();
        }

        private void AddReserve(EpgEventInfo eventInfo)
        {
            ((MainWindow)Application.Current.MainWindow).SwapOwnedReserveWindow(null);
            var dlg = new AddReserveEpgWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            dlg.SetEventInfo(eventInfo);
            dlg.ShowDialog();
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

        private void Window_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.F:
                        if (e.IsRepeat == false)
                        {
                            SearchPg();
                        }
                        e.Handled = true;
                        break;
                    case Key.N:
                        // バインディング更新のためフォーカスを移す
                        button_add_epgAutoAdd.Focus();
                        button_add_epgAutoAdd.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        e.Handled = true;
                        break;
                    case Key.S:
                        if (button_chg_epgAutoAdd.Visibility == Visibility.Visible)
                        {
                            button_chg_epgAutoAdd.Focus();
                            button_chg_epgAutoAdd.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        }
                        e.Handled = true;
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Escape:
                        Close();
                        e.Handled = true;
                        break;
                }
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if (searchOnLoaded)
            {
                searchOnLoaded = false;
                SearchPg();
            }
            else
            {
                searchKeyView.FocusAndKey();
            }
            CommonManager.Instance.DB.ReserveInfoChanged -= RefreshReserve;
            CommonManager.Instance.DB.ReserveInfoChanged += RefreshReserve;
        }

        void listView_result_KeyDown(object sender, KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.R:
                        if (e.IsRepeat == false)
                        {
                            //予約が1つでもあれば有効無効切り替えとみなす
                            if (listView_result.SelectedItems.Cast<SearchItem>().Any(a => a.IsReserved))
                            {
                                MenuItem_Click_No(sender, e);
                            }
                            else
                            {
                                buttonOrMenuItem_add_reserve_Click(sender, e);
                            }
                        }
                        e.Handled = true;
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.F3:
                        MenuItem_Click_ProgramTable(sender, e);
                        e.Handled = true;
                        break;
                    case Key.Enter:
                        MenuItem_Click_ShowDialog(sender, e);
                        e.Handled = true;
                        break;
                }
            }
        }

        private void MenuItem_Click_DeleteItem(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                List<uint> list = new List<uint>();

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

        private void MenuItem_Click_No(object sender, RoutedEventArgs e)
        {
            var list = new List<ReserveData>();
            var originalRecModeList = new List<byte>();
            foreach (SearchItem item in listView_result.SelectedItems)
            {
                if (item.IsReserved)
                {
                    originalRecModeList.Add(item.ReserveInfo.RecSetting.RecMode);
                    byte recMode = item.ReserveInfo.RecSetting.GetRecMode();
                    item.ReserveInfo.RecSetting.RecMode = CommonManager.Instance.DB.CombineRecModeAndNoRec(recMode, !item.ReserveInfo.RecSetting.IsNoRec());
                    list.Add(item.ReserveInfo);
                }
            }
            if (list.Count > 0)
            {
                string message = null;
                try
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        message = CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。";
                    }
                }
                catch (Exception ex)
                {
                    message = ex.ToString();
                }
                for (int i = 0; i < list.Count; i++)
                {
                    list[i].RecSetting.RecMode = originalRecModeList[i];
                }
                if (message != null)
                {
                    MessageBox.Show(message);
                }
            }
        }

        private void MenuItem_Click_RecMode(object sender, RoutedEventArgs e)
        {
            var list = new List<ReserveData>();
            var originalRecModeList = new List<byte>();
            foreach (SearchItem item in listView_result.SelectedItems)
            {
                if (item.IsReserved)
                {
                    originalRecModeList.Add(item.ReserveInfo.RecSetting.RecMode);
                    byte recMode = byte.Parse((string)((MenuItem)sender).Tag);
                    item.ReserveInfo.RecSetting.RecMode = CommonManager.Instance.DB.CombineRecModeAndNoRec(recMode, item.ReserveInfo.RecSetting.IsNoRec());
                    list.Add(item.ReserveInfo);
                }
            }
            if (list.Count > 0)
            {
                string message = null;
                try
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        message = CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。";
                    }
                }
                catch (Exception ex)
                {
                    message = ex.ToString();
                }
                for (int i = 0; i < list.Count; i++)
                {
                    list[i].RecSetting.RecMode = originalRecModeList[i];
                }
                if (message != null)
                {
                    MessageBox.Show(message);
                }
            }
        }

        private void MenuItem_Click_Priority(object sender, RoutedEventArgs e)
        {
            var list = new List<ReserveData>();
            var originalPriorityList = new List<byte>();
            foreach (SearchItem item in listView_result.SelectedItems)
            {
                if (item.IsReserved)
                {
                    originalPriorityList.Add(item.ReserveInfo.RecSetting.Priority);
                    item.ReserveInfo.RecSetting.Priority = byte.Parse((string)((MenuItem)sender).Tag);
                    list.Add(item.ReserveInfo);
                }
            }
            if (list.Count > 0)
            {
                string message = null;
                try
                {
                    ErrCode err = CommonManager.CreateSrvCtrl().SendChgReserve(list);
                    if (err != ErrCode.CMD_SUCCESS)
                    {
                        message = CommonManager.GetErrCodeText(err) ?? "予約変更でエラーが発生しました。";
                    }
                }
                catch (Exception ex)
                {
                    message = ex.ToString();
                }
                for (int i = 0; i < list.Count; i++)
                {
                    list[i].RecSetting.Priority = originalPriorityList[i];
                }
                if (message != null)
                {
                    MessageBox.Show(message);
                }
            }
        }

        private void MenuItem_Click_ProgramTable(object sender, RoutedEventArgs e)
        {
            SearchItem item1 = this.listView_result.SelectedItem as SearchItem;
            if (item1 != null)
            {
                ((MainWindow)Application.Current.MainWindow).SearchJumpTargetProgram(item1.EventInfo);
            }
        }

        private void MenuItem_Click_Research(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                SearchItem item = listView_result.SelectedItem as SearchItem;
                searchKeyView.SetAndKey(item.EventName);
                searchKeyView.SetServiceList(new List<long> { (long)CommonManager.Create64Key(item.EventInfo.original_network_id,
                                                                                              item.EventInfo.transport_stream_id,
                                                                                              item.EventInfo.service_id) });
                SearchPg();
            }
        }

        private void listView_result_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            {
                foreach (FrameworkElement item in ((ListViewItem)sender).ContextMenu.Items)
                {
                    if (item.Name == "cmdDlt")
                    {
                        item.IsEnabled = listView_result.SelectedItems.Cast<SearchItem>().Any(a => a.IsReserved);
                    }
                    else if (item.Name == "cmdChg")
                    {
                        //選択されているすべての予約が同じ設定の場合だけチェックを表示する
                        int noRec = -1;
                        int recMode = -1;
                        int priority = -1;
                        foreach (SearchItem selItem in listView_result.SelectedItems)
                        {
                            if (selItem.IsReserved == true)
                            {
                                if (noRec < 0)
                                {
                                    noRec = selItem.ReserveInfo.RecSetting.IsNoRec() ? 1 : 0;
                                    recMode = selItem.ReserveInfo.RecSetting.GetRecMode();
                                    priority = selItem.ReserveInfo.RecSetting.Priority;
                                }
                                else
                                {
                                    noRec = noRec != (selItem.ReserveInfo.RecSetting.IsNoRec() ? 1 : 0) ? 2 : noRec;
                                    recMode = recMode != selItem.ReserveInfo.RecSetting.GetRecMode() ? -1 : recMode;
                                    priority = priority != selItem.ReserveInfo.RecSetting.Priority ? -1 : priority;
                                }
                            }
                        }
                        item.IsEnabled = noRec >= 0;
                        var itemChg = (MenuItem)item;
                        itemChg.Items.Cast<FrameworkElement>().First(a => a.Name == "cmdNo").Visibility =
                            (noRec == 0 ? Visibility.Visible : Visibility.Collapsed);
                        itemChg.Items.Cast<FrameworkElement>().First(a => a.Name == "cmdNoInv").Visibility =
                            (noRec == 1 ? Visibility.Visible : Visibility.Collapsed);
                        itemChg.Items.Cast<FrameworkElement>().First(a => a.Name == "cmdNoToggle").Visibility =
                            (noRec == 2 ? Visibility.Visible : Visibility.Collapsed);
                        int i = itemChg.Items.IndexOf(itemChg.Items.Cast<FrameworkElement>().First(a => a.Name == "cmdRecModeAll"));
                        for (int j = 0; j <= 4; j++)
                        {
                            ((MenuItem)itemChg.Items[i + j]).IsChecked = (j == recMode);
                        }
                        var itemPri = (MenuItem)itemChg.Items.Cast<FrameworkElement>().First(a => a.Name == "cmdPri");
                        for (int j = 0; j < itemPri.Items.Count; j++)
                        {
                            ((MenuItem)itemPri.Items[j]).IsChecked = (j + 1 == priority);
                        }
                        itemPri.Header = string.Format((string)itemPri.Tag, priority >= 0 ? "" + priority : "*");
                    }
                    else if (item.Name == "cmdAdd")
                    {
                        ((MenuItem)item).Items.Clear();
                        item.IsEnabled = listView_result.SelectedItems.Cast<SearchItem>().Any(a => a.IsReserved == false);
                        if (item.IsEnabled)
                        {
                            foreach (RecPresetItem info in Settings.GetRecPresetList())
                            {
                                var menuItem = new MenuItem();
                                menuItem.Header = info.DisplayName;
                                menuItem.Tag = info.ID;
                                menuItem.Click += buttonOrMenuItem_add_reserve_Click;
                                ((MenuItem)item).Items.Add(menuItem);
                            }
                        }
                    }
                }
            }
        }

    }
}

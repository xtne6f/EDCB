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
using System.Windows.Navigation;
using System.Windows.Shapes;


namespace EpgTimer
{
    /// <summary>
    /// EpgAutoAddView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgAutoAddView : UserControl
    {
        private List<EpgAutoDataItem> resultList = new List<EpgAutoDataItem>();
        private List<EpgAutoDataItem> resultListMoved;
        private string lastAscendingSortedHeader;
        private ListViewHorizontalMouseScroller horizontalScroller = new ListViewHorizontalMouseScroller();
        private bool ReloadInfo = true;

        private Dictionary<string, GridViewColumn> columnList;

        public EpgAutoAddView()
        {
            InitializeComponent();
            columnList = gridView_key.Columns.ToDictionary(info => (string)((GridViewColumnHeader)info.Header).Tag);
            gridView_key.Columns.Clear();
            foreach (ListColumnInfo info in Settings.Instance.AutoAddEpgColumn)
            {
                if (columnList.ContainsKey(info.Tag))
                {
                    columnList[info.Tag].Width = info.Width;
                    gridView_key.Columns.Add(columnList[info.Tag]);
                }
            }
            if (Settings.Instance.AutoAddEpgHideButton)
            {
                stackPanel_button.Visibility = Visibility.Collapsed;
            }
            listView_key.AlternationCount = Settings.Instance.ResAlternationCount;
        }

        public void SaveSize()
        {
            Settings.Instance.AutoAddEpgColumn.Clear();
            Settings.Instance.AutoAddEpgColumn.AddRange(
                gridView_key.Columns.Select(info => new ListColumnInfo((string)((GridViewColumnHeader)info.Header).Tag, info.Width)));
        }

        /// <summary>
        /// リストの更新通知
        /// </summary>
        public void UpdateInfo()
        {
            ReloadInfo = true;
            if (IsVisible && ReloadInfoData())
            {
                ReloadInfo = false;
            }
        }

        private bool ReloadInfoData()
        {
            if (CommonManager.Instance.NWMode && CommonManager.Instance.NWConnectedIP == null)
            {
                resultList = new List<EpgAutoDataItem>();
                reloadItemOrder();
                return false;
            }
            ErrCode err = CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
            if (err != ErrCode.CMD_SUCCESS)
            {
                Dispatcher.BeginInvoke(new Action(() => MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "情報の取得でエラーが発生しました。")));
                resultList = new List<EpgAutoDataItem>();
                reloadItemOrder();
                return false;
            }
            resultList = CommonManager.Instance.DB.EpgAutoAddList.Values.Select(info => new EpgAutoDataItem(info)).ToList();
            reloadItemOrder();
            return true;
        }

        private void button_add_Click(object sender, RoutedEventArgs e)
        {
            ((MainWindow)Application.Current.MainWindow).CreateSearchWindow().Show();
        }

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_key.SelectedItems.Count > 0)
                {
                    List<uint> dataIDList = new List<uint>();
                    foreach (EpgAutoDataItem info in listView_key.SelectedItems)
                    {
                        dataIDList.Add(info.EpgAutoAddInfo.dataID);
                    }
                    CommonManager.CreateSrvCtrl().SendDelEpgAutoAdd(dataIDList);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void button_del2_Click(object sender, RoutedEventArgs e)
        {
            if (listView_key.SelectedItems.Count == 0) { return; }

            string text1 = "予約項目ごと削除してよろしいですか?\r\n"
                            + "(無効の「自動予約登録項目」による予約も削除されます。)";
            string caption1 = "[予約ごと削除]の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, 
                MessageBoxImage.Question, MessageBoxResult.OK) != MessageBoxResult.OK)
            {
                return;
            }

            //EpgTimerSrvでの自動予約登録の実行タイミングに左右されず確実に予約を削除するため、
            //先に自動予約登録項目を削除する。

            //自動予約登録項目の検索条件を保持
            List<EpgSearchKeyInfo> keyList = listView_key.SelectedItems.Cast<EpgAutoDataItem>().Select(a => a.EpgAutoAddInfo.searchInfo.DeepClone()).ToList();

            button_del_Click(sender, e);

            try
            {
                //配下の予約の削除

                //検索リストの取得
                List<EpgEventInfo> list = new List<EpgEventInfo>();

                foreach (EpgSearchKeyInfo key in keyList)
                {
                    key.andKey = key.andKey.Substring(key.andKey.StartsWith("^!{999}", StringComparison.Ordinal) ? 7 : 0);//無効解除
                }

                CommonManager.CreateSrvCtrl().SendSearchPg(keyList, ref list);

                List<uint> dellist = new List<uint>();

                foreach (EpgEventInfo info in list)
                {
                    if (info.StartTimeFlag != 0 && info.start_time.AddSeconds(info.DurationFlag == 0 ? 0 : info.durationSec) > DateTime.UtcNow.AddHours(9))
                    {
                        foreach (ReserveData info2 in CommonManager.Instance.DB.ReserveList.Values)
                        {
                            if (info.original_network_id == info2.OriginalNetworkID &&
                                info.transport_stream_id == info2.TransportStreamID &&
                                info.service_id == info2.ServiceID &&
                                info.event_id == info2.EventID)
                            {
                                //重複したEpgEventInfoは送られてこないので、登録時の重複チェックは不要
                                dellist.Add(info2.ReserveID);
                                break;
                            }
                        }
                    }
                }

                if (dellist.Count > 0)
                {
                    CommonManager.CreateSrvCtrl().SendDelReserve(dellist);
                }
                
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void button_change_Click(object sender, RoutedEventArgs e)
        {
            showDialog();
        }

        private void listView_key_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            showDialog();
            e.Handled = true;
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true)
            {
                if (ReloadInfoData() == true)
                {
                    ReloadInfo = false;
                }
            }
        }


        private void ContextMenu_Header_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            foreach (object item in listView_key.ContextMenu.Items)
            {
                MenuItem menuItem = item as MenuItem;
                if (menuItem != null)
                {
                    if (menuItem.Name == "HideButton")
                    {
                        menuItem.IsChecked = Settings.Instance.AutoAddEpgHideButton;
                    }
                    else
                    {
                        menuItem.IsChecked = Settings.Instance.AutoAddEpgColumn.Any(info => info.Tag == menuItem.Name);
                    }
                }
            }
        }

        private void headerSelect_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                MenuItem menuItem = sender as MenuItem;
                if (menuItem.IsChecked == true)
                {

                    Settings.Instance.AutoAddEpgColumn.Add(new ListColumnInfo(menuItem.Name, double.NaN));
                    gridView_key.Columns.Add(columnList[menuItem.Name]);
                }
                else
                {
                    foreach (ListColumnInfo info in Settings.Instance.AutoAddEpgColumn)
                    {
                        if (info.Tag == menuItem.Name)
                        {
                            Settings.Instance.AutoAddEpgColumn.Remove(info);
                            gridView_key.Columns.Remove(columnList[menuItem.Name]);
                            break;
                        }
                    }
                }

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void hideButton_Click(object sender, RoutedEventArgs e)
        {
            Settings.Instance.AutoAddEpgHideButton = ((MenuItem)sender).IsChecked;
            stackPanel_button.Visibility = Settings.Instance.AutoAddEpgHideButton ? Visibility.Collapsed : Visibility.Visible;
        }

        private void listView_key_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            foreach (FrameworkElement item in ((ListViewItem)sender).ContextMenu.Items)
            {
                if (item.Name == "saveItemOrder")
                {
                    item.IsEnabled = button_saveItemOrder.IsEnabled;
                }
                else if (item.Name == "reloadItem")
                {
                    item.IsEnabled = button_reloadItem.IsEnabled;
                }
            }
        }

        /*****************************************************
        *
        *  追加
        *
        ******************************************************/

        void showDialog()
        {
            if (listView_key.SelectedItem != null)
            {
                EpgAutoDataItem info = listView_key.SelectedItem as EpgAutoDataItem;
                SearchWindow search = ((MainWindow)Application.Current.MainWindow).CreateSearchWindow();
                search.SetChangeModeData(info.EpgAutoAddInfo);
                search.Show();
            }
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="up0">true: up, false: down</param>
        void moveItem(bool up0)
        {
            EpgAutoDataItem item_Src1 = listView_key.SelectedItem as EpgAutoDataItem;
            if (item_Src1 == null) { return; }
            int index_Src1 = (resultListMoved ?? resultList).IndexOf(item_Src1);
            int index_Dst1 = index_Src1 - 1;
            if (up0 == false)
            {
                index_Dst1 = index_Src1 + 1;
            }
            if (0 <= index_Dst1 && index_Dst1 < (resultListMoved ?? resultList).Count)
            {
                resultListMoved = (resultListMoved ?? resultList).ToList();
                resultListMoved.RemoveAt(index_Src1);
                resultListMoved.Insert(index_Dst1, item_Src1);

                lastAscendingSortedHeader = null;
                listView_key.ItemsSource = resultListMoved;
                button_saveItemOrder.IsEnabled = true;
                button_reloadItem.IsEnabled = true;
                textBox_ItemOrderStatus.Visibility = Visibility.Visible;
            }
        }

        void saveItemOrder()
        {
            if (resultListMoved == null) { return; }
            //
            List<uint> originalDataIdList = resultListMoved.Select(a => a.EpgAutoAddInfo.dataID).ToList();
            List<uint> dataIdList = originalDataIdList.ToList();
            dataIdList.Sort();
            //
            for (int i = 0; i < resultListMoved.Count; i++)
            {
                resultListMoved[i].EpgAutoAddInfo.dataID = dataIdList[i];
            }
            string message = null;
            try
            {
                if (((MainWindow)Application.Current.MainWindow).OwnedWindows.OfType<SearchWindow>().FirstOrDefault() != null)
                {
                    // IDを交換するのでEPG予約条件の不慮の上書きを防ぐため
                    message = "検索・EPG予約条件ウィンドウを閉じてください";
                }
                else if (CommonManager.CreateSrvCtrl().SendChgEpgAutoAdd(resultListMoved.Select(a => a.EpgAutoAddInfo).ToList()) != ErrCode.CMD_SUCCESS)
                {
                    message = "変更に失敗しました";
                }
            }
            catch (Exception ex)
            {
                message = ex.ToString();
            }
            for (int i = 0; i < resultListMoved.Count; i++)
            {
                resultListMoved[i].EpgAutoAddInfo.dataID = originalDataIdList[i];
            }
            if (message != null)
            {
                MessageBox.Show(message);
            }
        }

        void reloadItemOrder()
        {
            resultListMoved = null;
            lastAscendingSortedHeader = null;
            listView_key.ItemsSource = resultList;
            button_saveItemOrder.IsEnabled = false;
            button_reloadItem.IsEnabled = false;
            textBox_ItemOrderStatus.Visibility = Visibility.Hidden;
        }

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked1 = e.OriginalSource as GridViewColumnHeader;
            //
            if (headerClicked1 != null)
            {
                if (headerClicked1.Role != GridViewColumnHeaderRole.Padding)
                {
                    // ソートの実行
                    bool desc = lastAscendingSortedHeader == (string)headerClicked1.Tag;
                    lastAscendingSortedHeader = desc ? null : (string)headerClicked1.Tag;
                    var p = typeof(EpgAutoDataItem).GetProperty((string)headerClicked1.Tag);
                    resultListMoved = resultListMoved ?? resultList;
                    resultListMoved = (desc ? resultListMoved.OrderByDescending(a => p.GetValue(a, null)) : resultListMoved.OrderBy(a => p.GetValue(a, null))).ToList();
                    // UI更新
                    listView_key.ItemsSource = resultListMoved;
                    button_saveItemOrder.IsEnabled = true;
                    button_reloadItem.IsEnabled = true;
                    textBox_ItemOrderStatus.Visibility = Visibility.Visible;
                }
            }
        }

        private void button_saveItemOrder_Click(object sender, RoutedEventArgs e)
        {
            this.saveItemOrder();
        }

        private void button_reloadItem_Click(object sender, RoutedEventArgs e)
        {
            this.reloadItemOrder();
        }

        private void listView_key_KeyDown(object sender, KeyEventArgs e)
        {
            switch (e.Key)
            {
                case Key.Enter:
                    showDialog();
                    e.Handled = true;
                    break;
                case Key.Delete:
                    if (listView_key.SelectedItems.Count > 0 &&
                        MessageBox.Show(listView_key.SelectedItems.Count + "項目を削除してよろしいですか?", "確認",
                                        MessageBoxButton.OKCancel, MessageBoxImage.Question, MessageBoxResult.OK) == MessageBoxResult.OK)
                    {
                        button_del_Click(sender, e);
                    }
                    e.Handled = true;
                    break;
            }
        }

        private void UserControl_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.Up:
                        moveItem(true);
                        e.Handled = true;
                        break;
                    case Key.Down:
                        moveItem(false);
                        e.Handled = true;
                        break;
                    case Key.S:
                        if (e.IsRepeat == false)
                        {
                            saveItemOrder();
                        }
                        e.Handled = true;
                        break;
                    case Key.Z:
                        if (e.IsRepeat == false)
                        {
                            reloadItemOrder();
                        }
                        e.Handled = true;
                        break;
                }
            }
        }

        private void button_up_Click(object sender, RoutedEventArgs e)
        {
            moveItem(true);
        }

        private void button_down_Click(object sender, RoutedEventArgs e)
        {
            moveItem(false);
        }

        private void listView_key_MouseEnter(object sender, MouseEventArgs e)
        {
            horizontalScroller.OnMouseEnter(listView_key, Settings.Instance.EpgSettingList[0].MouseHorizontalScrollAuto,
                                            Settings.Instance.EpgSettingList[0].HorizontalScrollSize);
        }

        private void listView_key_MouseLeave(object sender, MouseEventArgs e)
        {
            horizontalScroller.OnMouseLeave();
        }
    }
}

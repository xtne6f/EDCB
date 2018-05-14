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
            try
            {
                resultList = new List<EpgAutoDataItem>();

                ErrCode err = CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                if (err != ErrCode.CMD_SUCCESS)
                {
                    this.Dispatcher.BeginInvoke(new Action(() =>
                    {
                        MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "情報の取得でエラーが発生しました。");
                    }), null);
                    reloadItemOrder();
                    return false;
                }

                foreach (EpgAutoAddData info in CommonManager.Instance.DB.EpgAutoAddList.Values)
                {
                    EpgAutoDataItem item = new EpgAutoDataItem(info);
                    resultList.Add(item);
                }

                reloadItemOrder();
            }
            catch (Exception ex)
            {
                this.Dispatcher.BeginInvoke(new Action(() =>
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }), null);
                return false;
            }
            return true;
        }

        private void button_add_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                SearchWindow dlg = new SearchWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetViewMode(1);
                dlg.ShowDialog();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_key.SelectedItems.Count > 0)
                {
                    List<UInt32> dataIDList = new List<uint>();
                    foreach (EpgAutoDataItem info in listView_key.SelectedItems)
                    {
                        dataIDList.Add(info.EpgAutoAddInfo.dataID);
                    }
                    CommonManager.CreateSrvCtrl().SendDelEpgAutoAdd(dataIDList);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_del2_Click(object sender, RoutedEventArgs e)
        {
            if (listView_key.SelectedItems.Count == 0) { return; }

            string text1 = "予約項目ごと削除してよろしいですか?\r\n"
                            + "(無効の「自動予約登録項目」による予約も削除されます。)";
            string caption1 = "[予約ごと削除]の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, 
                MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK)
            {
                return;
            }

            //EpgTimerSrvでの自動予約登録の実行タイミングに左右されず確実に予約を削除するため、
            //先に自動予約登録項目を削除する。

            //自動予約登録項目のリストを保持
            List<EpgAutoDataItem> autoaddlist = new List<EpgAutoDataItem>();
            foreach (EpgAutoDataItem item in listView_key.SelectedItems)
            {
                autoaddlist.Add(item);
            }

            button_del_Click(sender, e);

            try
            {
                //配下の予約の削除

                //検索リストの取得
                List<EpgSearchKeyInfo> keyList = new List<EpgSearchKeyInfo>();
                List<EpgEventInfo> list = new List<EpgEventInfo>();

                foreach (EpgAutoDataItem item in autoaddlist)
                {
                    EpgSearchKeyInfo key = item.EpgAutoAddInfo.searchInfo;
                    key.andKey = key.andKey.Substring(key.andKey.StartsWith("^!{999}", StringComparison.Ordinal) ? 7 : 0);//無効解除
                    keyList.Add(key);
                }

                CommonManager.CreateSrvCtrl().SendSearchPg(keyList, ref list);

                List<UInt32> dellist = new List<UInt32>();

                foreach (EpgEventInfo info in list)
                {
                    if (info.start_time.AddSeconds(info.DurationFlag == 0 ? 0 : info.durationSec) > DateTime.UtcNow.AddHours(9))
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
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_change_Click(object sender, RoutedEventArgs e)
        {
            showDialog();
        }

        private void listView_key_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            showDialog();
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
            try
            {
                foreach (MenuItem item in listView_key.ContextMenu.Items)
                {
                    item.IsChecked = Settings.Instance.AutoAddEpgColumn.Any(info => info.Tag == item.Name);
                }


            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void headerSelect_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                MenuItem menuItem = sender as MenuItem;
                if (menuItem.IsChecked == true)
                {

                    Settings.Instance.AutoAddEpgColumn.Add(new ListColumnInfo(menuItem.Name, Double.NaN));
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
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /*****************************************************
        *
        *  追加
        *
        ******************************************************/

        void deleteItem()
        {
            if (listView_key.SelectedItems.Count == 0) { return; }
            //
            try
            {
                string text1 = "削除しますか？" + "　[削除アイテム数: " + listView_key.SelectedItems.Count + "]" + "\n\n";
                List<UInt32> dataIDList = new List<uint>();
                foreach (EpgAutoDataItem info in listView_key.SelectedItems)
                {
                    dataIDList.Add(info.EpgAutoAddInfo.dataID);
                    text1 += "「" + info.AndKey + "」" + "\n";
                }
                string caption1 = "登録項目削除の確認";
                if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) == MessageBoxResult.OK)
                {
                    CommonManager.CreateSrvCtrl().SendDelEpgAutoAdd(dataIDList);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        void showDialog()
        {
            if (listView_key.SelectedItem == null) { return; }
            //
            try
            {
                EpgAutoDataItem info = listView_key.SelectedItem as EpgAutoDataItem;
                SearchWindow dlg = new SearchWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                dlg.SetViewMode(2);
                dlg.SetChgAutoAddID(info.EpgAutoAddInfo.dataID);
                dlg.SetSearchDefKey(info.EpgAutoAddInfo.searchInfo);
                dlg.SetRecInfoDef(info.EpgAutoAddInfo.recSetting);
                dlg.ShowDialog();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
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
                listView_key.DataContext = resultListMoved;
                button_saveItemOrder.IsEnabled = true;
                button_reloadItem.IsEnabled = true;
                textBox_ItemOrderStatus.Visibility = Visibility.Visible;
            }
        }

        void saveItemOrder()
        {
            if (resultListMoved == null) { return; }
            //
            List<uint> dataIdList1 = new List<uint>();
            foreach (EpgAutoDataItem item1 in resultListMoved)
            {
                dataIdList1.Add(item1.EpgAutoAddInfo.dataID);
            }
            dataIdList1.Sort();
            //
            List<EpgAutoAddData> addList1 = new List<EpgAutoAddData>();
            for (int i1 = 0; i1 < resultListMoved.Count; i1++)
            {
                EpgAutoDataItem item1 = resultListMoved[i1];
                item1.EpgAutoAddInfo.dataID = dataIdList1[i1];
                addList1.Add(item1.EpgAutoAddInfo);

            }
            if (CommonManager.CreateSrvCtrl().SendChgEpgAutoAdd(addList1) != ErrCode.CMD_SUCCESS)
            {
                MessageBox.Show("変更に失敗しました");
            }
        }

        void reloadItemOrder()
        {
            resultListMoved = null;
            lastAscendingSortedHeader = null;
            listView_key.DataContext = resultList;
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
                    listView_key.DataContext = resultListMoved;
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

        protected override void OnPreviewKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control))
            {
                switch (e.Key)
                {
                    case Key.Up:
                        moveItem(true);
                        break;
                    case Key.Down:
                        moveItem(false);
                        break;
                    case Key.S:
                        this.saveItemOrder();
                        break;
                    case Key.R:
                        this.reloadItemOrder();
                        break;
                }
            }
            else
            {
                switch (e.Key)
                {
                    case Key.Enter:
                        this.showDialog();
                        break;
                    case Key.Delete:
                        this.deleteItem();
                        break;
                }
            }
            //
            base.OnPreviewKeyDown(e);
        }

        private void button_up_Click2(object sender, RoutedEventArgs e)
        {
            moveItem(true);
        }

        private void button_down_Click2(object sender, RoutedEventArgs e)
        {
            moveItem(false);
        }

    }
}

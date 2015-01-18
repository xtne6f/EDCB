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

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// EpgAutoAddView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgAutoAddView : UserControl
    {
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private List<EpgAutoDataItem> resultList = new List<EpgAutoDataItem>();
        private bool ReloadInfo = true;

        private Dictionary<String, GridViewColumn> columnList = new Dictionary<String, GridViewColumn>();

        public EpgAutoAddView()
        {
            InitializeComponent();
            try
            {
                if (Settings.Instance.NoStyle == 1)
                {
                    button_add.Style = null;
                    button_del.Style = null;
                    button_change.Style = null;
                    button_up.Style = null;
                    button_down.Style = null;
                }

                foreach (GridViewColumn info in gridView_key.Columns)
                {
                    GridViewColumnHeader header = info.Header as GridViewColumnHeader;
                    columnList.Add((string)header.Tag, info);
                }
                gridView_key.Columns.Clear();

                foreach (ListColumnInfo info in Settings.Instance.AutoAddEpgColumn)
                {
                    columnList[info.Tag].Width = info.Width;
                    gridView_key.Columns.Add(columnList[info.Tag]);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void SaveSize()
        {
            try
            {
                Settings.Instance.AutoAddEpgColumn.Clear();
                foreach (GridViewColumn info in gridView_key.Columns)
                {
                    GridViewColumnHeader header = info.Header as GridViewColumnHeader;

                    Settings.Instance.AutoAddEpgColumn.Add(new ListColumnInfo((String)header.Tag, info.Width));
                }

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// リストの更新通知
        /// </summary>
        public void UpdateInfo()
        {
            if (this.IsVisible == true)
            {
                ReloadInfoData();
                ReloadInfo = false;
            }
            else
            {
                ReloadInfo = true;
            }
        }

        public void UpdateListViewSelection(uint autoAddID)
        {
            if (this.IsVisible == true)
            {
                listView_key.UnselectAll();

                if (autoAddID == 0) return;//無くても結果は同じ

                foreach (EpgAutoDataItem item in listView_key.Items)
                {
                    if (item.EpgAutoAddInfo.dataID == autoAddID)
                    {
                        listView_key.SelectedItem = item;
                        break;
                    }
                }
            }
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true)
            {
                ReloadInfoData();
                ReloadInfo = false;
            }
        }

        private bool ReloadInfoData()
        {
            try
            {
                //更新前の選択情報の保存
                uint oldItem = 0;
                List<uint> oldItems = new List<uint>();
                StoreListViewSelected(ref oldItem, ref oldItems);

                listView_key.DataContext = null;
                resultList.Clear();

                ErrCode err = CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                if (err == ErrCode.CMD_ERR_CONNECT)
                {
                    this.Dispatcher.BeginInvoke(new Action(() =>
                    {
                        MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
                    }), null);
                    return false;
                }
                if (err == ErrCode.CMD_ERR_TIMEOUT)
                {
                    this.Dispatcher.BeginInvoke(new Action(() =>
                    {
                        MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
                    }), null);
                    return false;
                }
                if (err != ErrCode.CMD_SUCCESS)
                {
                    this.Dispatcher.BeginInvoke(new Action(() =>
                    {
                        MessageBox.Show("情報の取得でエラーが発生しました。");
                    }), null);
                    return false;
                }

                foreach (EpgAutoAddData info in CommonManager.Instance.DB.EpgAutoAddList.Values)
                {
                    EpgAutoDataItem item = new EpgAutoDataItem(info);
                    resultList.Add(item);
                }

                listView_key.DataContext = resultList;

                //選択情報の復元
                RestoreListViewSelected(oldItem, oldItems);
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

        private void StoreListViewSelected(ref uint oldItem, ref List<uint> oldItems)
        {
            if (listView_key.SelectedItem != null)
            {
                oldItem = (listView_key.SelectedItem as EpgAutoDataItem).EpgAutoAddInfo.dataID;
                foreach (EpgAutoDataItem item in listView_key.SelectedItems)
                {
                    oldItems.Add(item.EpgAutoAddInfo.dataID);
                }
            }
        }

        private void RestoreListViewSelected(uint oldItem, List<uint> oldItems)
        {
            if (oldItem != 0)
            {
                //このUnselectAll()は無いと正しく復元出来ない状況があり得る
                listView_key.UnselectAll();

                foreach (EpgAutoDataItem item in listView_key.Items)
                {
                    if (item.EpgAutoAddInfo.dataID == oldItem)
                    {
                        listView_key.SelectedItem = item;
                        listView_key.ScrollIntoView(item);
                    }
                }

                foreach (uint oldItem1 in oldItems)
                {
                    foreach (EpgAutoDataItem item in listView_key.Items)
                    {
                        if (item.EpgAutoAddInfo.dataID == oldItem1)
                        {
                            listView_key.SelectedItems.Add(item);
                            break;
                        }
                    }
                }
            }
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
                    cmd.SendDelEpgAutoAdd(dataIDList);
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

            string text1 = "予約項目ごと削除してよろしいですか?　[削除アイテム数: " + listView_key.SelectedItems.Count + "]\r\n"
                            + "(無効の「自動予約登録項目」による予約も削除されます。)\r\n\r\n";
            foreach (EpgAutoDataItem info in listView_key.SelectedItems)
            {
                text1 += " ・ " + info.AndKey + "\r\n";
            }

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
                    key.andKey = key.andKey.Substring(key.andKey.StartsWith("^!{999}") ? 7 : 0);//無効解除
                    keyList.Add(key);
                }

                cmd.SendSearchPg(keyList, ref list);

                List<UInt32> dellist = new List<UInt32>();

                foreach (EpgEventInfo info in list)
                {
                    if (info.start_time.AddSeconds(info.durationSec) > DateTime.Now)
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
                    cmd.SendDelReserve(dellist);
                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
                    CommonManager.Instance.DB.ReloadReserveInfo();
                }
                
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_change_Click(object sender, RoutedEventArgs e)
        {
            this.showDialog();
        }

        //SearchWindowからのリスト選択状態の変更を優先するために、MouseUpイベントによる
        //listViewによるアイテム選択処理より後でダイアログを出すようにする。
        private bool doubleClicked = false;
        private void listView_key_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            doubleClicked = true;
        }

        private void listView_key_MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (doubleClicked == true)
            {
                doubleClicked = false;
                this.showDialog();
            }
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
                    item.IsChecked = false;
                    foreach (ListColumnInfo info in Settings.Instance.AutoAddEpgColumn)
                    {
                        if (info.Tag.CompareTo(item.Name) == 0)
                        {
                            item.IsChecked = true;
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
                        if (info.Tag.CompareTo(menuItem.Name) == 0)
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

        bool _ItemOrderNotSaved = false;
        enum itemMoveDirections { up, down };
        EpgAutoDataItem dragItem = null;
        GridViewSorter<EpgAutoDataItem> gridViewSorter = new GridViewSorter<EpgAutoDataItem>();

        bool ItemOrderNotSaved
        {
            get { return this._ItemOrderNotSaved; }
            set
            {
                this._ItemOrderNotSaved = value;
                this.button_saveItemOrder.IsEnabled = value;
                this.button_reloadItem.IsEnabled = value;
                if (value)
                {
                    this.textBox_ItemOrderStatus.Text = "並びが変更されましたが、保存されていません。";
                }
                else
                {
                    this.textBox_ItemOrderStatus.Text = "";
                }
            }
        }

        void deleteItem()
        {
            if (listView_key.SelectedItems.Count == 0) { return; }
            //
            string text1 = "削除しますか?　[削除アイテム数: " + listView_key.SelectedItems.Count + "]" + "\r\n\r\n";
            foreach (EpgAutoDataItem info in listView_key.SelectedItems)
            {
                text1 += " ・ " + info.AndKey + "\r\n";
            }
            string caption1 = "登録項目削除の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) == MessageBoxResult.OK)
            {
                button_del_Click(listView_key, new RoutedEventArgs());
            }
        }

        void showDialog()
        {
            if (listView_key.SelectedItem == null) { return; }
            //
            try
            {
                EpgAutoDataItem info = listView_key.SelectedItems[listView_key.SelectedItems.Count - 1] as EpgAutoDataItem;
                listView_key.UnselectAll();
                listView_key.SelectedItem = info;
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

        void moveItem(EpgAutoDataItem item_Src1, EpgAutoDataItem item_Dst1)
        {
            int index_Src1 = resultList.IndexOf(item_Src1);
            int index_Dst1 = resultList.IndexOf(item_Dst1);

            resultList.Remove(item_Src1);
            resultList.Insert(index_Dst1, item_Src1);
            listView_key.SelectedItem = item_Src1;

            listView_key.Items.Refresh();
            this.ItemOrderNotSaved = true;
            this.gridViewSorter.resetSortParams();
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="up0">true: up, false: down</param>
        void moveItem(itemMoveDirections moveDirection0)
        {
            if (listView_key.SelectedItem == null) { return; }

            //更新前の選択情報の保存
            uint oldItem = 0;
            List<uint> oldItems = new List<uint>();
            StoreListViewSelected(ref oldItem, ref oldItems);

            //選択状態の維持しながらSelectedItemを切り替える
            EpgAutoDataItem item1 = listView_key.SelectedItems[listView_key.SelectedItems.Count - 1] as EpgAutoDataItem;
            oldItem = (uint)(item1.EpgAutoAddInfo.dataID);

            //選択情報の復元
            RestoreListViewSelected(oldItem, oldItems);

            EpgAutoDataItem item_Src1 = listView_key.SelectedItem as EpgAutoDataItem;
            int index_Src1 = resultList.IndexOf(item_Src1);
            int index_Dst1 = index_Src1 - 1;
            if (moveDirection0 == itemMoveDirections.down)
            {
                index_Dst1 = index_Src1 + 1;
            }
            if (0 <= index_Dst1 && index_Dst1 < resultList.Count)
            {
                EpgAutoDataItem item_Dst1 = resultList[index_Dst1];
                this.moveItem(item_Src1, item_Dst1);
            }
        }

        void saveItemOrder()
        {
            if (this.ItemOrderNotSaved == false) { return; }
            //
            List<uint> dataIdList1 = new List<uint>();
            foreach (EpgAutoDataItem item1 in this.resultList)
            {
                dataIdList1.Add(item1.EpgAutoAddInfo.dataID);
            }
            dataIdList1.Sort();
            //
            List<EpgAutoAddData> addList1 = new List<EpgAutoAddData>();
            for (int i1 = 0; i1 < this.resultList.Count; i1++)
            {
                EpgAutoDataItem item1 = this.resultList[i1];
                item1.EpgAutoAddInfo.dataID = dataIdList1[i1];
                addList1.Add(item1.EpgAutoAddInfo);

            }
            if (cmd.SendChgEpgAutoAdd(addList1) != 1)
            {
                MessageBox.Show("変更に失敗しました");
            }
            else
            {
                this.ItemOrderNotSaved = false;
            }
        }

        void reloadItemOrder()
        {
            if (this.ItemOrderNotSaved==false) { return; }
            //
            this.ReloadInfoData();
            this.ItemOrderNotSaved = false;
            this.gridViewSorter.resetSortParams();
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
                    this.gridViewSorter.SortByMultiHeader(this.resultList, headerClicked1);
                    // UI更新
                    this.listView_key.Items.Refresh();
                    //
                    this.ItemOrderNotSaved = true;
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

        private void listViewItem_PreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.LeftButton == MouseButtonState.Pressed)
            {
                ListViewItem item1 = sender as ListViewItem;
                this.dragItem = item1.Content as EpgAutoDataItem;
            }
        }

        private void listViewItem_MouseEnter(object sender, MouseEventArgs e)
        {
            if (this.dragItem != null
                && this.dragItem != sender)
            {
                if (Mouse.LeftButton == MouseButtonState.Released)
                {
                    this.dragItem = null;
                }
                else
                {
                    ListViewItem item1 = sender as ListViewItem;
                    EpgAutoDataItem eadi1 = item1.Content as EpgAutoDataItem;
                    this.moveItem(this.dragItem, eadi1);
                }
            }
        }

        void listView_key_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control) && Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
            {
                switch (e.Key)
                {
                    case Key.D:
                        this.button_del2_Click(this, new RoutedEventArgs(Button.ClickEvent));
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.Up:
                        this.moveItem(itemMoveDirections.up);
                        break;
                    case Key.Down:
                        this.moveItem(itemMoveDirections.down);
                        break;
                    case Key.S:
                        if (this.ItemOrderNotSaved==true)
                        {
                            if (MessageBox.Show("並びの変更を保存します。\r\nよろしいですか？", "保存の確認", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                            {
                                this.saveItemOrder();
                            }
                        }
                        break;
                    case Key.R:
                        if (this.ItemOrderNotSaved == true)
                        {
                            if (MessageBox.Show("元の並びに復元します。\r\nよろしいですか？", "復元の確認", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                            {
                                this.reloadItemOrder();
                            }
                        }
                        break;
                    case Key.D:
                        this.deleteItem();
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Enter:
                        this.showDialog();
                        e.Handled = true;
                        break;
                    case Key.Delete:
                        this.deleteItem();
                        e.Handled = true;
                        break;
                }
            }
        }

        private void button_up_Click2(object sender, RoutedEventArgs e)
        {
            this.moveItem(itemMoveDirections.up);
        }

        private void button_down_Click2(object sender, RoutedEventArgs e)
        {
            this.moveItem(itemMoveDirections.down);
        }

        private void myPopup_MouseLeave(object sender, MouseEventArgs e)
        {
            this.myPopup.IsOpen = false;
        }

    }
}

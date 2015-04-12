using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

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
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private List<EpgAutoDataItem> resultList = new List<EpgAutoDataItem>();
        private bool ReloadInfo = true;

        private GridViewSelector gridViewSelector = null;
        private Action<object, RoutedEventArgs> headerSelect_Click = null;

        public EpgAutoAddView()
        {
            InitializeComponent();
            try
            {
                if (Settings.Instance.NoStyle == 1)
                {
                    button_add.Style = null;
                    button_del.Style = null;
                    button_del2.Style = null;
                    button_change.Style = null;
                    button_up.Style = null;
                    button_down.Style = null;
                }

                gridViewSelector = new GridViewSelector(gridView_key, Settings.Instance.AutoAddEpgColumn);
                headerSelect_Click = gridViewSelector.HeaderSelectClick;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void ContextMenu_Header_ContextMenuOpening(object sender, ContextMenuEventArgs e)
        {
            gridViewSelector.ContextMenuOpening(listView_key.ContextMenu);
        }

        public void SaveSize()
        {
            gridViewSelector.SaveSize();
        }

        /// <summary>
        /// リストの更新通知
        /// </summary>
        public void UpdateInfo()
        {
            ReloadInfo = true;
            if (this.IsVisible == true)
            {
                ReloadInfo = !ReloadInfoData();
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
                ReloadInfo = !ReloadInfoData();
            }
        }

        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true)
            {
                ReloadInfo = !ReloadInfoData();
            }
        }

        private bool ReloadInfoData()
        {
            try
            {
                //更新前の選択情報の保存
                var oldItems = new ListViewSelectedKeeper<EpgAutoDataItem>(listView_key, true);

                listView_key.DataContext = null;
                resultList.Clear();

                if (CommonManager.Instance.VUtil.EpgTimerNWNotConnect() == true) return false;

                ErrCode err = CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                if (CommonManager.CmdErrMsgTypical(err, "情報の取得", this) == false) return false;

                foreach (EpgAutoAddData info in CommonManager.Instance.DB.EpgAutoAddList.Values)
                {
                    EpgAutoDataItem item = new EpgAutoDataItem(info);
                    resultList.Add(item);
                }
                listView_key.DataContext = resultList;

                //選択情報の復元
                oldItems.RestoreListViewSelected();
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
            mutil.EpgAutoAddDelete(GetSelectedItemsList());
        }

        private List<EpgAutoDataItem> GetSelectedItemsList()
        {
            return listView_key.SelectedItems.Cast<EpgAutoDataItem>().ToList();
        }

        private void button_del2_Click(object sender, RoutedEventArgs e)
        {
            if (listView_key.SelectedItems.Count == 0) { return; }

            string text1 = "予約項目ごと削除してよろしいですか?　[削除アイテム数: " + listView_key.SelectedItems.Count + "]\r\n"
                            + "(無効の「自動予約登録項目」による予約も削除されます。)\r\n\r\n";
            GetSelectedItemsList().ForEach(info => text1 += " ・ " + info.AndKey + "\r\n");

            string caption1 = "[予約ごと削除]の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, 
                MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK)
            {
                return;
            }

            //EpgTimerSrvでの自動予約登録の実行タイミングに左右されず確実に予約を削除するため、
            //先に自動予約登録項目を削除する。

            //自動予約登録項目のリストを保持
            List<EpgAutoDataItem> autoaddlist = GetSelectedItemsList();

            button_del_Click(sender, e);

            try
            {
                //配下の予約の削除

                //検索リストの取得
                var keyList = new List<EpgSearchKeyInfo>();
                var list = new List<EpgEventInfo>();

                foreach (EpgAutoDataItem item in autoaddlist)
                {
                    EpgSearchKeyInfo key = item.EpgAutoAddInfo.searchInfo;
                    key.andKey = key.andKey.Substring(key.andKey.StartsWith("^!{999}") ? 7 : 0);//無効解除
                    keyList.Add(key);
                }

                cmd.SendSearchPg(keyList, ref list);

                var dellist = new List<ReserveData>();

                foreach (EpgEventInfo info in list)
                {
                    if (info.start_time.AddSeconds(info.durationSec) > DateTime.Now)
                    {
                        foreach (ReserveData info2 in CommonManager.Instance.DB.ReserveList.Values)
                        {
                            if (CommonManager.EqualsPg(info, info2) == true)
                            {
                                //重複したEpgEventInfoは送られてこないので、登録時の重複チェックは不要
                                dellist.Add(info2);
                                break;
                            }
                        }
                    }
                }

                mutil.ReserveDelete(dellist);
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

        /*****************************************************
        *
        *  追加
        *
        ******************************************************/

        bool _ItemOrderNotSaved = false;
        GridViewSorter<EpgAutoDataItem> gridViewSorter = new GridViewSorter<EpgAutoDataItem>("RecFolder");

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
            GetSelectedItemsList().ForEach(info => text1 += " ・ " + info.AndKey + "\r\n");

            string caption1 = "登録項目削除の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) == MessageBoxResult.OK)
            {
                button_del_Click(listView_key, new RoutedEventArgs());
            }
        }

        private EpgAutoDataItem SelectSingleItem()
        {
            return mutil.SelectSingleItem<EpgAutoDataItem>(listView_key);
        }

        void showDialog()
        {
            if (listView_key.SelectedItem == null) { return; }
            //
            try
            {
                EpgAutoDataItem info = SelectSingleItem();

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

        void saveItemOrder()
        {
            if (this.ItemOrderNotSaved == false) { return; }
            //
            List<uint> dataIdList1 = new List<uint>();
            resultList.ForEach(item1 => dataIdList1.Add(item1.EpgAutoAddInfo.dataID));
            dataIdList1.Sort();
            //
            List<EpgAutoAddData> addList1 = new List<EpgAutoAddData>();
            for (int i1 = 0; i1 < this.resultList.Count; i1++)
            {
                EpgAutoDataItem item1 = this.resultList[i1];
                item1.EpgAutoAddInfo.dataID = dataIdList1[i1];
                addList1.Add(item1.EpgAutoAddInfo);

            }
            if (mutil.EpgAutoAddChange(addList1, false) == true)
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
            this.gridViewSorter.ResetSortParams();
        }

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked1 = e.OriginalSource as GridViewColumnHeader;
            if (headerClicked1 != null)
            {
                if (headerClicked1.Role != GridViewColumnHeaderRole.Padding)
                {
                    // 無効列の場合は無視。ItemOrderNotSavedが無ければ、この条件節無くても動作に支障はない。
                    if (this.gridViewSorter.IsExceptionHeader(headerClicked1) == false)
                    {
                        // ソートの実行、リフレッシュ後、保存を促す表示をする。
                        this.gridViewSorter.SortByMultiHeader(this.resultList, headerClicked1);
                        this.listView_key.Items.Refresh();
                        this.ItemOrderNotSaved = true;
                    }
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

        //移動関連
        enum itemMoveDirections { up, down };
        EpgAutoDataItem dragItem = null;
        List<EpgAutoDataItem> dragItems = null;

        private void listViewItem_PreviewMouseDown(object sender, MouseButtonEventArgs e)
        {
            if (e.LeftButton == MouseButtonState.Pressed)
            {
                ListViewItem item1 = (ListViewItem)sender;
                this.dragItem = (EpgAutoDataItem)item1.Content;
                if (listView_key.SelectedItems.Contains(this.dragItem))
                {
                    this.dragItems = listView_key.SelectedItems.Cast<EpgAutoDataItem>().ToList();
                    this.dragItems.Sort((i1, i2) => resultList.IndexOf(i1) - resultList.IndexOf(i2));
                }
                else
                {
                    this.dragItems = mutil.GetList(this.dragItem);
                }
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
                    this.dragItems = null;
                }
                else
                {
                    ListViewItem item1 = (ListViewItem)sender;
                    EpgAutoDataItem dropTo = (EpgAutoDataItem)item1.Content;
                    this.moveItem(dropTo);
                }
            }
        }

        void moveItem(EpgAutoDataItem dropTo)
        {
            try
            {
                int idx_dropItems = resultList.IndexOf(this.dragItem);
                int idx_dropTo = resultList.IndexOf(dropTo);

                //一番上と一番下を選択できるように調整
                idx_dropTo += (idx_dropTo > idx_dropItems ? 1 : 0);

                //挿入位置で分割→バラのも含め選択アイテムを除去→分割前部+選択アイテム+分割後部で連結
                var work1 = resultList.Take(idx_dropTo).Where(item => !dragItems.Contains(item)).ToList();
                var work2 = resultList.Skip(idx_dropTo).Where(item => !dragItems.Contains(item)).ToList();

                resultList.Clear();
                resultList.AddRange(work1.Concat(dragItems).Concat(work2));
                listView_key.SelectedItem = dragItem;//これがないと移動中余分な選択があるように見える
                dragItems.ForEach(item=>listView_key.SelectedItems.Add(item));

                listView_key.Items.Refresh();
                this.ItemOrderNotSaved = true;
                this.gridViewSorter.ResetSortParams();
            }
            catch { }
        }

        /// <summary>
        /// ボタンから上下させる場合
        /// </summary>
        /// <param name="up0">true: up, false: down</param>
        void moveItem(itemMoveDirections moveDirection0)
        {
            try
            {
                if (listView_key.SelectedItem == null) { return; }

                //選択状態+順序のペアを作る
                var srcList = new List<EpgAutoDataItem>(resultList);
                var list = srcList.Select((item, index) => new KeyValuePair<int, bool>(index, listView_key.SelectedItems.Contains(item))).ToList();

                //逆方向の時はリストひっくり返す
                if (moveDirection0 == itemMoveDirections.down) list.Reverse();

                //移動対象でないアイテムが上下ループを超えないよう細工
                //超えているように見えるときでも良く見ると超えていない
                list.Insert(0, new KeyValuePair<int, bool>(-1, false));
                int end = list[1].Value == true ? 2 : list.Count;
                for (int i = 1; i < end; i++)
                {
                    //選択状態のものだけ移動
                    if (list[i].Value == true)
                    {
                        var tmp = list[i - 1];
                        list.RemoveAt(i - 1);
                        list.Insert(i, tmp);
                    }
                }
                //ループしたものを下へ持って行く。ループしてなければダミーがコピーされるだけ
                list.Add(list[0]);

                //リンク張り替えながらダミー以外(Key!=-1)をコピー
                resultList.Clear();
                resultList.AddRange(list.Skip(1).Where(item => item.Key != -1).Select(item => srcList[item.Key]));

                if (moveDirection0 == itemMoveDirections.down) resultList.Reverse();

                listView_key.Items.Refresh();
                this.ItemOrderNotSaved = true;
                this.gridViewSorter.ResetSortParams();
            }
            catch { }
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

        protected override void OnPreviewKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.Up:
                        this.moveItem(itemMoveDirections.up);
                        e.Handled = true;
                        break;
                    case Key.Down:
                        this.moveItem(itemMoveDirections.down);
                        e.Handled = true;
                        break;
                }
            }
            base.OnPreviewKeyDown(e);
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.S:
                        if (this.ItemOrderNotSaved == true)
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
                }
            }
            base.OnKeyDown(e);
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

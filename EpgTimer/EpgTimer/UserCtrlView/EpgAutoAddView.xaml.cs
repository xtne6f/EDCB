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
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private MenuManager mm = CommonManager.Instance.MM;
        private List<EpgAutoDataItem> resultList = new List<EpgAutoDataItem>();
        private bool ReloadInfo = true;

        private CmdExeEpgAutoAdd mc;
        private MenuBinds mBinds = new MenuBinds();

        private GridViewSelector gridViewSelector = null;
        private RoutedEventHandler headerSelect_Click = null;
        private GridViewSorter<EpgAutoDataItem> gridViewSorter = new GridViewSorter<EpgAutoDataItem>("RecFolder");

        public EpgAutoAddView()
        {
            InitializeComponent();
            try
            {
                //最初にコマンド集の初期化
                mc = new CmdExeEpgAutoAdd(this);
                mc.SetFuncGetDataList(isAll => (isAll == true ? resultList : this.GetSelectedItemsList()).EpgAutoAddInfoList());
                mc.SetFuncSelectSingleData((noChange) =>
                {
                    var item = this.SelectSingleItem(noChange);
                    return item == null ? null : item.EpgAutoAddInfo;
                });
                mc.SetFuncReleaseSelectedData(() => listView_key.UnselectAll());

                //コマンド集に無いものを追加
                mc.AddReplaceCommand(EpgCmds.UpItem, button_up_Click);
                mc.AddReplaceCommand(EpgCmds.DownItem, button_down_Click);
                mc.AddReplaceCommand(EpgCmds.SaveOrder, button_saveItemOrder_Click, (sender, e) => e.CanExecute = this.ItemOrderNotSaved == true);
                mc.AddReplaceCommand(EpgCmds.RestoreOrder, button_reloadItem_Click, (sender, e) => e.CanExecute = this.ItemOrderNotSaved == true);

                //コマンドをコマンド集から登録
                mc.ResetCommandBindings(this, listView_key.ContextMenu);

                //コンテキストメニューを開く時の設定
                listView_key.ContextMenu.Opened += new RoutedEventHandler(mc.SupportContextMenuLoading);

                //ボタンの設定
                mBinds.View = CtxmCode.EpgAutoAddView;
                mBinds.SetCommandToButton(button_add, EpgCmds.ShowAddDialog);
                mBinds.SetCommandToButton(button_change, EpgCmds.ShowDialog);
                mBinds.SetCommandToButton(button_del, EpgCmds.Delete);
                mBinds.SetCommandToButton(button_del2, EpgCmds.Delete2);
                mBinds.SetCommandToButton(button_up, EpgCmds.UpItem);
                mBinds.SetCommandToButton(button_down, EpgCmds.DownItem);
                mBinds.SetCommandToButton(button_saveItemOrder, EpgCmds.SaveOrder);
                mBinds.SetCommandToButton(button_reloadItem, EpgCmds.RestoreOrder);

                //メニューの作成、ショートカットの登録
                RefreshMenu();

                //グリッドビュー関連の設定
                gridViewSelector = new GridViewSelector(gridView_key, Settings.Instance.AutoAddEpgColumn);
                headerSelect_Click += new RoutedEventHandler(gridViewSelector.HeaderSelectClick);
                gridView_key.ColumnHeaderContextMenu.Opened += new RoutedEventHandler(gridViewSelector.ContextMenuOpening);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        public void RefreshMenu()
        {
            mBinds.ResetInputBindings(this, listView_key);
            mm.CtxmGenerateContextMenu(listView_key.ContextMenu, CtxmCode.EpgAutoAddView, true);
        }

        public void SaveSize()
        {
            gridViewSelector.SaveSize();
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

        /// <summary>情報の更新通知</summary>
        public void UpdateInfo()
        {
            ReloadInfo = true;
            if (ReloadInfo == true && this.IsVisible == true) ReloadInfo = !ReloadInfoData();
        }
        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true) ReloadInfo = !ReloadInfoData();
        }
        private void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (ReloadInfo == true && this.IsVisible == true) ReloadInfo = !ReloadInfoData();
        }
        private bool ReloadInfoData()
        {
            try
            {
                //更新前の選択情報の保存
                var oldItems = new ListViewSelectedKeeper(listView_key, true);

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
                return true;
            }
            catch (Exception ex)
            {
                this.Dispatcher.BeginInvoke(new Action(() =>
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }), null);
                return false;
            }
        }

        private List<EpgAutoDataItem> GetSelectedItemsList()
        {
            return listView_key.SelectedItems.Cast<EpgAutoDataItem>().ToList();
        }

        private EpgAutoDataItem SelectSingleItem(bool notSelectionChange = false)
        {
            return mutil.SelectSingleItem(listView_key, notSelectionChange) as EpgAutoDataItem;
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
                EpgCmds.ShowDialog.Execute(sender, this);
            }
        }

        bool _ItemOrderNotSaved = false;
        bool ItemOrderNotSaved
        {
            get { return this._ItemOrderNotSaved; }
            set
            {
                this._ItemOrderNotSaved = value;
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

        private void button_saveItemOrder_Click(object sender, ExecutedRoutedEventArgs e)
        {
            if (CmdExeUtil.IsDisplayKgMessage(e) == true)
            {
                if(MessageBox.Show("並びの変更を保存します。\r\nよろしいですか？", "保存の確認", 
                    MessageBoxButton.OKCancel) != MessageBoxResult.OK) return;
            }

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

        private void button_reloadItem_Click(object sender, ExecutedRoutedEventArgs e)
        {
            if (CmdExeUtil.IsDisplayKgMessage(e) == true)
            {
                if(MessageBox.Show("元の並びに復元します。\r\nよろしいですか？", "復元の確認", 
                    MessageBoxButton.OKCancel) != MessageBoxResult.OK) return;
            }

            this.ReloadInfoData();
            this.ItemOrderNotSaved = false;
            this.gridViewSorter.ResetSortParams();
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
                    this.dragItems = mutil.ToList(this.dragItem);
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

        private void button_up_Click(object sender, ExecutedRoutedEventArgs e)
        {
            this.moveItem(itemMoveDirections.up);
        }

        private void button_down_Click(object sender, ExecutedRoutedEventArgs e)
        {
            this.moveItem(itemMoveDirections.down);
        }

        private void myPopup_MouseLeave(object sender, MouseEventArgs e)
        {
            this.myPopup.IsOpen = false;
        }

    }
}

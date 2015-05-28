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
    /// ManualAutoAddView.xaml の相互作用ロジック
    /// </summary>
    public partial class ManualAutoAddView : UserControl
    {
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private ViewUtil vutil = CommonManager.Instance.VUtil;
        private MenuManager mm = CommonManager.Instance.MM;
        private List<ManualAutoAddDataItem> resultList = new List<ManualAutoAddDataItem>();
        private bool ReloadInfo = true;

        private CmdExeManualAutoAdd mc;
        private MenuBinds mBinds = new MenuBinds();

        private GridViewSelector gridViewSelector = null;
        private RoutedEventHandler headerSelect_Click = null;
        private GridViewSorter<ManualAutoAddDataItem> gridViewSorter = new GridViewSorter<ManualAutoAddDataItem>("RecFolder");

        public ManualAutoAddView()
        {
            InitializeComponent();
            try
            {
                //最初にコマンド集の初期化
                mc = new CmdExeManualAutoAdd(this);
                mc.SetFuncGetDataList(isAll => (isAll == true ? resultList : this.GetSelectedItemsList()).ManualAutoAddInfoList());
                mc.SetFuncSelectSingleData((noChange) =>
                {
                    var item = this.SelectSingleItem(noChange);
                    return item == null ? null : item.ManualAutoAddInfo;
                });
                mc.SetFuncReleaseSelectedData(() => listView_key.UnselectAll());

                //コマンドをコマンド集から登録
                mc.ResetCommandBindings(this, listView_key.ContextMenu);

                //コンテキストメニューを開く時の設定
                listView_key.ContextMenu.Opened += new RoutedEventHandler(mc.SupportContextMenuLoading);

                //ボタンの設定
                mBinds.View = CtxmCode.ManualAutoAddView;
                mBinds.SetCommandToButton(button_add, EpgCmds.ShowAddDialog);
                mBinds.SetCommandToButton(button_change, EpgCmds.ShowDialog);
                mBinds.SetCommandToButton(button_del, EpgCmds.Delete);
                mBinds.SetCommandToButton(button_del2, EpgCmds.Delete2);

                //メニューの作成、ショートカットの登録
                RefreshMenu();

                //グリッドビュー関連の設定
                gridViewSelector = new GridViewSelector(gridView_key, Settings.Instance.AutoAddManualColumn);
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
            mm.CtxmGenerateContextMenu(listView_key.ContextMenu, CtxmCode.ManualAutoAddView, true);
        }

        public void SaveViewData()
        {
            gridViewSelector.SaveSize(Settings.Instance.AutoAddManualColumn);
        }

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            vutil.GridViewHeaderClickSort<ManualAutoAddDataItem>(e, gridViewSorter, resultList, listView_key);
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

                ErrCode err = CommonManager.Instance.DB.ReloadManualAutoAddInfo();
                if (CommonManager.CmdErrMsgTypical(err, "情報の取得", this) == false) return false;

                foreach (ManualAutoAddData info in CommonManager.Instance.DB.ManualAutoAddList.Values)
                {
                    resultList.Add(new ManualAutoAddDataItem(info));
                }
                listView_key.DataContext = resultList;

                this.gridViewSorter.ResetSortParams();

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

        private List<ManualAutoAddDataItem> GetSelectedItemsList()
        {
            return listView_key.SelectedItems.Cast<ManualAutoAddDataItem>().ToList();
        }

        private ManualAutoAddDataItem SelectSingleItem(bool notSelectionChange = false)
        {
            return mutil.SelectSingleItem(listView_key, notSelectionChange) as ManualAutoAddDataItem;
        }

        private void listView_key_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            EpgCmds.ShowDialog.Execute(sender, this);
        }

    }
}

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
    /// RecInfoView.xaml の相互作用ロジック
    /// </summary>
    public partial class RecInfoView : UserControl
    {
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private MenuManager mm = CommonManager.Instance.MM;
        private List<RecInfoItem> resultList = new List<RecInfoItem>();
        private bool ReloadInfo = true;

        private CmdExeRecinfo mc;
        private MenuBinds mBinds = new MenuBinds();

        private GridViewSelector gridViewSelector = null;
        private RoutedEventHandler headerSelect_Click = null;
        private GridViewSorter<RecInfoItem> gridViewSorter = new GridViewSorter<RecInfoItem>();

        public RecInfoView()
        {
            InitializeComponent();

            try
            {
                //最初にコマンド集の初期化
                mc = new CmdExeRecinfo(this);
                mc.SetFuncGetDataList(isAll => (isAll == true ? resultList : this.GetSelectedItemsList()).RecInfoList());
                mc.SetFuncSelectSingleData((noChange) =>
                {
                    var item = this.SelectSingleItem(noChange);
                    return item == null ? null : item.RecInfo;
                });
                mc.SetFuncReleaseSelectedData(() => listView_recinfo.UnselectAll());

                //コマンド集からコマンドを登録
                mc.ResetCommandBindings(this, listView_recinfo.ContextMenu);

                //コンテキストメニューを開く時の設定
                listView_recinfo.ContextMenu.Opened += new RoutedEventHandler(mc.SupportContextMenuLoading);

                //ボタンの設定
                mBinds.View = CtxmCode.RecInfoView;
                mBinds.SetCommandToButton(button_del, EpgCmds.Delete);
                mBinds.SetCommandToButton(button_delAll, EpgCmds.DeleteAll);
                mBinds.SetCommandToButton(button_play, EpgCmds.Play);

                //メニューの作成、ショートカットの登録
                RefreshMenu();

                //グリッドビュー関連の設定
                gridViewSelector = new GridViewSelector(gridView_recinfo, Settings.Instance.RecInfoListColumn);
                headerSelect_Click += new RoutedEventHandler(gridViewSelector.HeaderSelectClick);
                gridView_recinfo.ColumnHeaderContextMenu.Opened += new RoutedEventHandler(gridViewSelector.ContextMenuOpening);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        public void RefreshMenu()
        {
            mBinds.ResetInputBindings(this, listView_recinfo);
            mm.CtxmGenerateContextMenu(listView_recinfo.ContextMenu, CtxmCode.RecInfoView, true);
        }
        public void SaveSize()
        {
            gridViewSelector.SaveSize();
        }
        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;
            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    this.gridViewSorter.SortByMultiHeader(this.resultList, headerClicked);
                    listView_recinfo.Items.Refresh();
                    Settings.Instance.RecInfoColumnHead=this.gridViewSorter.LastHeader;
                    Settings.Instance.RecInfoSortDirection = this.gridViewSorter.LastDirection;
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
        public bool ReloadInfoData()
        {
            try
            {
                //更新前の選択情報の保存
                var oldItems = new ListViewSelectedKeeper(listView_recinfo, true);

                listView_recinfo.DataContext = null;
                resultList.Clear();

                if (CommonManager.Instance.VUtil.EpgTimerNWNotConnect() == true) return false;

                ErrCode err = CommonManager.Instance.DB.ReloadrecFileInfo();
                if (CommonManager.CmdErrMsgTypical(err, "録画情報の取得", this) == false) return false;

                foreach (RecFileInfo info in CommonManager.Instance.DB.RecFileInfo.Values)
                {
                    resultList.Add(new RecInfoItem(info));
                }

                if (this.gridViewSorter.IsExistSortParams)
                {
                    this.gridViewSorter.SortByMultiHeader(this.resultList);
                }
                else
                {
                    this.gridViewSorter.ResetSortParams();
                    this.gridViewSorter.SortByMultiHeaderWithKey(this.resultList, gridView_recinfo.Columns,
                        Settings.Instance.RecInfoColumnHead, true, Settings.Instance.RecInfoSortDirection);
                }
                listView_recinfo.DataContext = resultList;

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
        private RecInfoItem SelectSingleItem(bool notSelectionChange = false)
        {
            return mutil.SelectSingleItem(listView_recinfo, notSelectionChange) as RecInfoItem;
        }
        private List<RecInfoItem> GetSelectedItemsList()
        {
            return listView_recinfo.SelectedItems.Cast<RecInfoItem>().ToList();
        }
        private void listView_recinfo_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (Settings.Instance.PlayDClick == true)
            {
                EpgCmds.Play.Execute(sender, this);
            }
            else
            {
                EpgCmds.ShowDialog.Execute(sender, this);
            }
        }
        //リストのカギマークからの呼び出し
        public bool ChgProtectRecInfoForMark(RecInfoItem hitItem)
        {
            if (listView_recinfo.SelectedItems.Contains(hitItem) == true)
            {
                EpgCmds.ProtectChange.Execute(listView_recinfo, this);
                return true;
            }
            return false;
        }
    }
}

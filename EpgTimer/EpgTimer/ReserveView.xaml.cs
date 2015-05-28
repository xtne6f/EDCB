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
    /// ReserveView.xaml の相互作用ロジック
    /// </summary>
    public partial class ReserveView : UserControl
    {
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private ViewUtil vutil = CommonManager.Instance.VUtil;
        private MenuManager mm = CommonManager.Instance.MM;
        private List<ReserveItem> reserveList = new List<ReserveItem>();
        private bool ReloadInfo = true;

        private CmdExeReserve mc; //予約系コマンド集
        private MenuBinds mBinds = new MenuBinds();

        private GridViewSelector gridViewSelector = null;
        private RoutedEventHandler headerSelect_Click = null;
        private GridViewSorter<ReserveItem> gridViewSorter = null;

        public ReserveView()
        {
            InitializeComponent();

            try
            {
                //最初にコマンド集の初期化
                mc = new CmdExeReserve(this);
                mc.SetFuncGetDataList(isAll => (isAll == true ? reserveList : this.GetSelectedItemsList()).ReserveInfoList());
                mc.SetFuncSelectSingleData((noChange) =>
                {
                    var item = this.SelectSingleItem(noChange);
                    return item == null ? null : item.ReserveInfo;
                });
                mc.SetFuncReleaseSelectedData(() => listView_reserve.UnselectAll());

                //コマンド集からコマンドを登録。多少冗長だが、持っているコマンドは全部登録してしまう。
                //フォーカスによってコンテキストメニューからウィンドウにコマンドが繋がらない場合があるので、
                //コンテキストメニューにもコマンドを登録する。
                mc.ResetCommandBindings(this, listView_reserve.ContextMenu);

                //ボタンの設定。XML側でコマンド指定しておけば、ループでまとめ処理できるけど、
                //インテリセンス効かないし(一応エラーチェックは入る)、コード側に一覧として書き出す。
                mBinds.View = CtxmCode.ReserveView;
                mBinds.SetCommandToButton(button_on_off, EpgCmds.ChgOnOff);
                mBinds.SetCommandToButton(button_change, EpgCmds.ShowDialog);
                mBinds.SetCommandToButton(button_del, EpgCmds.Delete);
                mBinds.SetCommandToButton(button_add_manual, EpgCmds.ShowAddDialog);
                mBinds.SetCommandToButton(button_timeShiftPlay, EpgCmds.Play);

                //メニューの作成、ショートカットの登録
                RefreshMenu();

                //コンテキストメニューを開く時の設定
                listView_reserve.ContextMenu.Opened += new RoutedEventHandler(mc.SupportContextMenuLoading);

                //グリッドビュー関連の設定
                gridViewSelector = new GridViewSelector(gridView_reserve, Settings.Instance.ReserveListColumn);
                headerSelect_Click += new RoutedEventHandler(gridViewSelector.HeaderSelectClick);
                gridView_reserve.ColumnHeaderContextMenu.Opened += new RoutedEventHandler(gridViewSelector.ContextMenuOpening);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        public void RefreshMenu()
        {
            mm.CtxmGenerateContextMenu(listView_reserve.ContextMenu, CtxmCode.ReserveView, true);
            mBinds.ResetInputBindings(this, listView_reserve);
            //mBinds.SetCommandBindings(mc.CommandBindings(), this, listView_reserve.ContextMenu);//やめ。あるだけ全部最初に登録することにする。
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
                var oldItems = new ListViewSelectedKeeper(listView_reserve, true);

                listView_reserve.DataContext = null;
                reserveList.Clear();

                if (vutil.ReloadReserveData(this) == false) return false;

                foreach (ReserveData info in CommonManager.Instance.DB.ReserveList.Values)
                {
                    reserveList.Add(new ReserveItem(info));
                }

                // 枠線表示用
                CommonManager.Instance.DB.ReloadEpgData();

                if (this.gridViewSorter != null)
                {
                    this.gridViewSorter.SortByMultiHeader(this.reserveList);
                }
                else
                {
                    this.gridViewSorter = new GridViewSorter<ReserveItem>(new string[] { "RecFileName", "RecFolder" });
                    this.gridViewSorter.SortByMultiHeaderWithKey(this.reserveList, gridView_reserve.Columns,
                        Settings.Instance.ResColumnHead, true, Settings.Instance.ResSortDirection);
                }

                listView_reserve.DataContext = reserveList;

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
        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            vutil.GridViewHeaderClickSort<ReserveItem>(e, gridViewSorter, reserveList, listView_reserve);
        }
        public void SaveViewData()
        {
            gridViewSelector.SaveSize(Settings.Instance.ReserveListColumn);
            if (gridViewSorter != null)
            {
                Settings.Instance.ResColumnHead = this.gridViewSorter.LastHeader;
                Settings.Instance.ResSortDirection = this.gridViewSorter.LastDirection;
            }
        }
        private ReserveItem SelectSingleItem(bool notSelectionChange = false)
        {
            return mutil.SelectSingleItem(listView_reserve, notSelectionChange) as ReserveItem;
        }
        private List<ReserveItem> GetSelectedItemsList()
        {
            return listView_reserve.SelectedItems.Cast<ReserveItem>().ToList();
        }
        private void listView_reserve_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            //DoubleClickのジェスチャうまく反応してくれない‥。
            EpgCmds.ShowDialog.Execute(sender, this);
        }
    }
}

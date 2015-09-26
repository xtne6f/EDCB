using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;
using EpgTimer.UserCtrlView;

namespace EpgTimer
{
    /// <summary>
    /// EpgAutoAddView.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgAutoAddView : DataViewBase
    {
        private ListViewController<EpgAutoDataItem> lstCtrl;
        private CmdExeEpgAutoAdd mc;

        //ドラッグ移動ビュー用の設定
        private MouseButtonEventHandler listViewItem_PreviewMouseLeftButtonDown;
        private MouseEventHandler listViewItem_MouseEnter;
        class lvDragData : ListBoxDragMoverView.LVDMHelper
        {
            private EpgAutoAddView View;
            public lvDragData(EpgAutoAddView view) { View = view; }
            public override uint GetID(object data) { return (data as EpgAutoDataItem).EpgAutoAddInfo.dataID; }
            public override void SetID(object data, uint ID) { (data as EpgAutoDataItem).EpgAutoAddInfo.dataID = ID; }
            public override bool SaveChange() { return View.mutil.EpgAutoAddChange(View.lstCtrl.dataList.EpgAutoAddInfoList(), false); }
            public override bool RestoreOrder() { return View.ReloadInfoData(); }
            public override void ItemMoved() { View.lstCtrl.gvSorter.ResetSortParams(); }
        }

        public EpgAutoAddView()
        {
            InitializeComponent();

            try
            {
                //リストビュー関連の設定
                lstCtrl = new ListViewController<EpgAutoDataItem>(this);
                lstCtrl.SetSavePath(mutil.GetMemberName(() => Settings.Instance.AutoAddEpgColumn));
                lstCtrl.SetViewSetting(listView_key, gridView_key, false, null
                    , (sender, e) => dragMover.NotSaved |= lstCtrl.GridViewHeaderClickSort(e));

                //ドラッグ移動関係、イベント追加はdragMoverでやりたいが、あまり綺麗にならないのでこっちに並べる
                this.dragMover.SetData(this, listView_key, lstCtrl.dataList, new lvDragData(this));
                listView_key.PreviewMouseLeftButtonUp += new MouseButtonEventHandler(dragMover.listBox_PreviewMouseLeftButtonUp);
                listViewItem_PreviewMouseLeftButtonDown += new MouseButtonEventHandler(dragMover.listBoxItem_PreviewMouseLeftButtonDown);
                listViewItem_MouseEnter += new MouseEventHandler(dragMover.listBoxItem_MouseEnter);

                //最初にコマンド集の初期化
                mc = new CmdExeEpgAutoAdd(this);
                mc.SetFuncGetDataList(isAll => (isAll == true ? lstCtrl.dataList : lstCtrl.GetSelectedItemsList()).EpgAutoAddInfoList());
                mc.SetFuncSelectSingleData((noChange) =>
                {
                    var item = lstCtrl.SelectSingleItem(noChange);
                    return item == null ? null : item.EpgAutoAddInfo;
                });
                mc.SetFuncReleaseSelectedData(() => listView_key.UnselectAll());

                //コマンドをコマンド集から登録
                mc.ResetCommandBindings(this, listView_key.ContextMenu);

                //コンテキストメニューを開く時の設定
                lstCtrl.SetCtxmTargetSave(listView_key.ContextMenu);//こっちが先
                listView_key.ContextMenu.Opened += new RoutedEventHandler(mc.SupportContextMenuLoading);

                //ボタンの設定
                mBinds.View = CtxmCode.EpgAutoAddView;
                mBinds.SetCommandToButton(button_add, EpgCmds.ShowAddDialog);
                mBinds.SetCommandToButton(button_change, EpgCmds.ShowDialog);
                mBinds.SetCommandToButton(button_del, EpgCmds.Delete);
                mBinds.SetCommandToButton(button_del2, EpgCmds.Delete2);

                //メニューの作成、ショートカットの登録
                RefreshMenu();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        public void RefreshMenu()
        {
            mBinds.ResetInputBindings(this, listView_key);
            mm.CtxmGenerateContextMenu(listView_key.ContextMenu, CtxmCode.EpgAutoAddView, true);
        }
        public void SaveViewData()
        {
            lstCtrl.SaveViewDataToSettings();
        }
        protected override bool ReloadInfoData()
        {
            EpgCmds.DragCancel.Execute(null, this);

            return lstCtrl.ReloadInfoData(dataList =>
            {
                ErrCode err = CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                if (CommonManager.CmdErrMsgTypical(err, "情報の取得", this) == false) return false;

                foreach (EpgAutoAddData info in CommonManager.Instance.DB.EpgAutoAddList.Values)
                {
                    dataList.Add(new EpgAutoDataItem(info));
                }
                dragMover.NotSaved = false;
                return true;
            });
        }
        //SearchWindowからのリスト選択状態の変更を優先するために、MouseUpイベントによる
        //listViewによるアイテム選択処理より後でダイアログを出すようにする。
        private bool doubleClicked = false;
        private void listView_key_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            doubleClicked = true;
        }
        private void listView_key_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            if (doubleClicked == true)
            {
                doubleClicked = false;
                EpgCmds.ShowDialog.Execute(sender, this);
            }
        }
        public void UpdateListViewSelection(uint autoAddID)
        {
            if (this.IsVisible == true)
            {
                listView_key.UnselectAll();

                if (autoAddID == 0) return;//無くても結果は同じ

                var target = listView_key.Items.OfType<EpgAutoDataItem>()
                    .FirstOrDefault(item => item.EpgAutoAddInfo.dataID == autoAddID);

                if (target != null)
                {
                    listView_key.SelectedItem = target;
                }
            }
        }
        //リストのチェックボックスからの呼び出し
        public void ChgKeyEnabledFromCheckbox(EpgAutoDataItem hitItem)
        {
            if (listView_key.SelectedItems.Contains(hitItem) == false)
            {
                listView_key.SelectedItem = hitItem;
            }
            EpgCmds.ChgOnOffKeyEnabled.Execute(listView_key, this);
        }
    }
}

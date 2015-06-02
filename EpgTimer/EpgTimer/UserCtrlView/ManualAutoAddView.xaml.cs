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
    /// ManualAutoAddView.xaml の相互作用ロジック
    /// </summary>
    public partial class ManualAutoAddView : DataViewBase
    {
        private ListViewController<ManualAutoAddDataItem> lstCtrl;
        private CmdExeManualAutoAdd mc;

        //ドラッグ移動ビュー用の設定
        private MouseButtonEventHandler listViewItem_PreviewMouseLeftButtonDown;
        private MouseButtonEventHandler listViewItem_PreviewMouseLeftButtonUp;
        private MouseEventHandler listViewItem_MouseEnter;
        class lvDragData : ListBoxDragMoverView.LVDMHelper
        {
            private ManualAutoAddView View;
            public lvDragData(ManualAutoAddView view) { View = view; }
            public override uint GetID(object data) { return (data as ManualAutoAddDataItem).ManualAutoAddInfo.dataID; }
            public override void SetID(object data, uint ID) { (data as ManualAutoAddDataItem).ManualAutoAddInfo.dataID = ID; }
            public override bool SaveChange() { return View.mutil.ManualAutoAddChange(View.lstCtrl.dataList.ManualAutoAddInfoList(), false); }
            public override bool RestoreOrder() { return View.ReloadInfoData(); }
            public override void ItemMoved() { View.lstCtrl.gvSorter.ResetSortParams(); }
        }

        public ManualAutoAddView()
        {
            InitializeComponent();

            try
            {
                //リストビュー関連の設定
                lstCtrl = new ListViewController<ManualAutoAddDataItem>(this);
                lstCtrl.SetSavePath(mutil.GetMemberName(() => Settings.Instance.AutoAddManualColumn));
                lstCtrl.SetViewSetting(listView_key, gridView_key, false, new string[] { "RecFolder" }
                    , (sender, e) => dragMover.NotSaved |= lstCtrl.GridViewHeaderClickSort(e));

                //ドラッグ移動関係
                this.dragMover.SetData(this, listView_key, lstCtrl.dataList, new lvDragData(this));
                listView_key.PreviewMouseLeftButtonUp += new MouseButtonEventHandler(dragMover.listBox_PreviewMouseLeftButtonUp);
                listViewItem_PreviewMouseLeftButtonDown += new MouseButtonEventHandler(dragMover.listBoxItem_PreviewMouseLeftButtonDown);
                listViewItem_PreviewMouseLeftButtonUp += new MouseButtonEventHandler(dragMover.listBoxItem_PreviewMouseLeftButtonUp);
                listViewItem_MouseEnter += new MouseEventHandler(dragMover.listBoxItem_MouseEnter);

                //最初にコマンド集の初期化
                mc = new CmdExeManualAutoAdd(this);
                mc.SetFuncGetDataList(isAll => (isAll == true ? lstCtrl.dataList : lstCtrl.GetSelectedItemsList()).ManualAutoAddInfoList());
                mc.SetFuncSelectSingleData((noChange) =>
                {
                    var item = lstCtrl.SelectSingleItem(noChange);
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
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        public void RefreshMenu()
        {
            mBinds.ResetInputBindings(this, listView_key);
            mm.CtxmGenerateContextMenu(listView_key.ContextMenu, CtxmCode.ManualAutoAddView, true);
        }
        public void SaveViewData()
        {
            lstCtrl.SaveViewDataToSettings();
        }
        protected override bool ReloadInfoData()
        {
            return lstCtrl.ReloadInfoData(dataList =>
            {
                ErrCode err = CommonManager.Instance.DB.ReloadManualAutoAddInfo();
                if (CommonManager.CmdErrMsgTypical(err, "情報の取得", this) == false) return false;

                foreach (ManualAutoAddData info in CommonManager.Instance.DB.ManualAutoAddList.Values)
                {
                    dataList.Add(new ManualAutoAddDataItem(info));
                }
                dragMover.NotSaved = false;
                return true;
            });
        }
        private void listView_key_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            EpgCmds.ShowDialog.Execute(sender, this);
        }
    }
}

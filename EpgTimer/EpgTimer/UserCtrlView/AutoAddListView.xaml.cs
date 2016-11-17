﻿using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

using EpgTimer.UserCtrlView;

namespace EpgTimer
{
    /// <summary>
    /// EpgAutoAddView.xaml の相互作用ロジック
    /// </summary>
    public partial class AutoAddListView : DataViewBase
    {
        public AutoAddListView()
        {
            InitializeComponent();
        }
    }
    public class AutoAddViewBaseT<T, S> : AutoAddListView
        where T : AutoAddData, new()
        where S : AutoAddDataItemT<T>
    {
        protected CtxmCode viewCode;
        protected string ColumnSavePath;
        protected List<GridViewColumn> ColumnList;

        protected ListViewController<S> lstCtrl;
        protected CmdExeAutoAdd<T> mc;

        protected virtual void PostProcSaveOrder(Dictionary<uint, uint> changeIDTable) { }

        //ドラッグ移動ビュー用の設定
        class lvDragData : ListBoxDragMoverView.LVDMHelper
        {
            private AutoAddViewBaseT<T, S> View;
            public lvDragData(AutoAddViewBaseT<T, S> view) { View = view; }
            public override uint GetID(object data) { return (data as S).Data.DataID; }
            public override bool SaveChange(Dictionary<uint, uint> changeIDTable)
            {
                var newList = View.lstCtrl.dataList.Select(item => item.Data.CloneObj() as T).ToList();
                newList.ForEach(item => item.DataID = changeIDTable[item.DataID]);

                bool ret = MenuUtil.AutoAddChange(newList, false, false, false, false);
                StatusManager.StatusNotifySet(ret, "並べ替えを保存");
                if (ret == true)
                {
                    //dataListと検索ダイアログへのIDの反映。dataListは既にコピーだが、SaveChange成功後に行う
                    View.lstCtrl.dataList.ForEach(item => item.Data.DataID = changeIDTable[item.Data.DataID]);
                    View.PostProcSaveOrder(changeIDTable);
                }
                return ret;
            }
            public override bool RestoreOrder()
            {
                bool ret = View.ReloadInfoData();
                StatusManager.StatusNotifySet(ret, "並べ替えを復元");
                return ret;
            }
            public override void StatusChanged()
            {
                //とりあえず今はこれで
                var tab = View.Parent as TabItem;
                tab.Header = (tab.Header as string).TrimEnd('*') + (View.dragMover.NotSaved == true ? "*" : "");
            }
            public override void ItemMoved() { View.lstCtrl.gvSorter.ResetSortParams(); }
        }

        public AutoAddViewBaseT()
        {
            //リストビューデータ差し込み
            ColumnList = Resources[this.GetType().Name] as GridViewColumnList;
            ColumnList.AddRange(Resources["CommonColumns"] as GridViewColumnList);
            ColumnList.AddRange(Resources["RecSettingViewColumns"] as GridViewColumnList);

            InitAutoAddView();
        }

        public virtual void InitAutoAddView()
        {
            try
            {
                //リストビュー関連の設定
                lstCtrl = new ListViewController<S>(this);
                lstCtrl.SetSavePath(ColumnSavePath);
                lstCtrl.SetViewSetting(listView_key, gridView_key, true, false
                    , ColumnList, (sender, e) => dragMover.NotSaved |= lstCtrl.GridViewHeaderClickSort(e));
                ColumnList = null;
                lstCtrl.SetSelectedItemDoubleClick(EpgCmds.ShowDialog);

                //ステータス変更の設定
                lstCtrl.SetSelectionChangedEventHandler((sender, e) => this.UpdateStatus(1));

                //ドラッグ移動関係
                this.dragMover.SetData(this, listView_key, lstCtrl.dataList, new lvDragData(this));

                //最初にコマンド集の初期化
                //mc = (R)Activator.CreateInstance(typeof(R), this);
                mc.SetFuncGetDataList(isAll => (isAll == true ? lstCtrl.dataList : lstCtrl.GetSelectedItemsList()).AutoAddInfoList());
                mc.SetFuncSelectSingleData((noChange) =>
                {
                    var item = lstCtrl.SelectSingleItem(noChange);
                    return item == null ? null : item.Data as T;
                });
                mc.SetFuncReleaseSelectedData(() => listView_key.UnselectAll());

                //コマンド集に無いもの
                mc.AddReplaceCommand(EpgCmds.ChgOnOffCheck, (sender, e) => lstCtrl.ChgOnOffFromCheckbox(e.Parameter, EpgCmds.ChgOnOffKeyEnabled));

                //コマンドをコマンド集から登録
                mc.ResetCommandBindings(this, listView_key.ContextMenu);

                //コンテキストメニューを開く時の設定
                listView_key.ContextMenu.Opened += new RoutedEventHandler(mc.SupportContextMenuLoading);

                //ボタンの設定
                mBinds.View = viewCode;
                mBinds.SetCommandToButton(button_add, EpgCmds.ShowAddDialog);
                mBinds.SetCommandToButton(button_change, EpgCmds.ShowDialog);
                mBinds.SetCommandToButton(button_del, EpgCmds.Delete);
                mBinds.SetCommandToButton(button_del2, EpgCmds.Delete2);

                //メニューの作成、ショートカットの登録
                //RefreshMenu();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        public void RefreshMenu()
        {
            mBinds.ResetInputBindings(this, listView_key);
            mm.CtxmGenerateContextMenu(listView_key.ContextMenu, viewCode, true);
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
                ErrCode err = AutoAddData.ReloadDBManagerList(typeof(T));
                if (CommonManager.CmdErrMsgTypical(err, "情報の取得") == false) return false;

                dataList.AddRange(AutoAddData.GetDBManagerList(typeof(T)).Select(info => (S)Activator.CreateInstance(typeof(S), info.CloneObj())));

                dragMover.NotSaved = false;
                return true;
            });
        }
        protected override void SelectViewItemData(UInt64 id)
        {
            ViewUtil.JumpToListItem(id, listView_key, false);
        }
        protected override void UpdateStatusData(int mode = 0)
        {
            if (mode == 0) this.status[1] = ViewUtil.ConvertAutoAddStatus(lstCtrl.dataList, "自動予約登録数");
            List<S> sList = lstCtrl.GetSelectedItemsList();
            this.status[2] = sList.Count == 0 ? "" : ViewUtil.ConvertAutoAddStatus(sList, "　選択中");
        }
        protected override void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            base.UserControl_IsVisibleChanged(sender, e);

            if (this.IsVisible == false) return;

            ViewUtil.JumpToListItem(BlackoutWindow.SelectedData, listView_key, BlackoutWindow.NowJumpTable);
            BlackoutWindow.Clear();
        }
    }

    public class EpgAutoAddView : AutoAddViewBaseT<EpgAutoAddData, EpgAutoDataItem>
    {
        public override void InitAutoAddView()
        {
            //固有設定
            mc = new CmdExeEpgAutoAdd(this);//ジェネリックでも処理できるが‥
            viewCode = CtxmCode.EpgAutoAddView;
            ColumnSavePath = CommonUtil.NameOf(() => Settings.Instance.AutoAddEpgColumn);

            //初期化の続き
            base.InitAutoAddView();
        }
        protected override void UpdateSelectViewItem()
        {
            SearchWindow.UpdatesViewSelection();
        }
        protected override void PostProcSaveOrder(Dictionary<uint, uint> changeIDTable)
        {
            SearchWindow.UpdatesAutoAddViewOrderChanged(changeIDTable);
        }
    }

    public class ManualAutoAddView : AutoAddViewBaseT<ManualAutoAddData, ManualAutoAddDataItem>
    {
        public override void InitAutoAddView()
        {
            //固有設定
            mc = new CmdExeManualAutoAdd(this);
            viewCode = CtxmCode.ManualAutoAddView;
            ColumnSavePath = CommonUtil.NameOf(() => Settings.Instance.AutoAddManualColumn);

            //録画設定の表示項目を調整
            ColumnList.Remove(ColumnList.Find(data => (data.Header as GridViewColumnHeader).Uid == "Tuijyu"));
            ColumnList.Remove(ColumnList.Find(data => (data.Header as GridViewColumnHeader).Uid == "Pittari"));

            //初期化の続き
            base.InitAutoAddView();
        }
        protected override void UpdateSelectViewItem()
        {
            AddManualAutoAddWindow.UpdatesViewSelection();
        }
        protected override void PostProcSaveOrder(Dictionary<uint, uint> changeIDTable)
        {
            AddManualAutoAddWindow.UpdatesAutoAddViewOrderChanged(changeIDTable);
        }
    }

}

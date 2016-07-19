using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    //キーワード予約、プログラム自動登録の共通メソッド
    public class CmdExeAutoAdd<T> : CmdExe<T> where T : AutoAddData, new()
    {
        public CmdExeAutoAdd(UIElement owner) : base(owner) { }
        protected override void mc_ChangeKeyEnabled(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = MenuUtil.AutoAddChangeKeyEnabled(dataList, (byte)CmdExeUtil.ReadIdData(e, 0, 1) == 0);
        }
        protected override void mc_ChangeOnOffKeyEnabled(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = MenuUtil.AutoAddChangeOnOffKeyEnabled(dataList);
        }
        protected override void mc_ChangeRecSetting(object sender, ExecutedRoutedEventArgs e)
        {
            if (mcc_chgRecSetting(e) == false) return;
            IsCommandExecuted = MenuUtil.AutoAddChange(dataList);
        }
        protected override void mc_ChgBulkRecSet(object sender, ExecutedRoutedEventArgs e)
        {
            if (MenuUtil.ChangeBulkSet(dataList.RecSettingList(), this.Owner, typeof(T) == typeof(ManualAutoAddData)) == false) return;
            IsCommandExecuted = MenuUtil.AutoAddChange(dataList);
        }
        protected override void mc_Delete(object sender, ExecutedRoutedEventArgs e)
        {
            if (mcs_DeleteCheck(e) == false) return;
            IsCommandExecuted = MenuUtil.AutoAddDelete(dataList);
        }
        protected override void mc_Delete2(object sender, ExecutedRoutedEventArgs e)
        {
            if (CmdExeUtil.CheckAllProcCancel(e, dataList, true) == true) return;
            IsCommandExecuted = MenuUtil.AutoAddDelete(dataList, true, true);
        }
        protected override void mc_AdjustReserve(object sender, ExecutedRoutedEventArgs e)
        {
            if (CmdExeUtil.CheckAllProcCancel(e, dataList, false) == true) return;
            IsCommandExecuted = MenuUtil.AutoAddChangeSyncReserve(dataList);
        }
        protected override ReserveData mcs_GetNextReserve()
        {
            if (dataList.Count == 0) return null;

            ReserveData resinfo = dataList[0].GetNextReserve();
            return resinfo != null ? resinfo : dataList[0].GetReserveList().GetNextReserve(true);
        }
        protected override void mcs_ctxmLoading_switch(ContextMenu ctxm, MenuItem menu)
        {
            if (menu.Tag == EpgCmdsEx.ChgMenu)
            {
                mcs_chgMenuOpening(menu);
            }
            else if (menu.Tag == EpgCmds.JumpReserve || menu.Tag == EpgCmds.JumpTuner || menu.Tag == EpgCmds.JumpTable)
            {
                mcs_jumpTabMenuOpening(menu, "次の無効予約へジャンプ");
            }
            else if (menu.Tag == EpgCmdsEx.OpenFolderMenu)
            {
                mm.CtxmGenerateOpenFolderItems(menu, this.ItemCount == 0 ? null : dataList[0].RecSettingInfo);
            }
        }
    }

    //プログラム自動登録の固有メソッド
    public class CmdExeManualAutoAdd : CmdExeAutoAdd<ManualAutoAddData>
    {
        public CmdExeManualAutoAdd(UIElement owner) : base(owner) { _copyItemData = ManualAutoAddDataEx.CopyTo; }
        protected override void mc_ShowDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == MenuUtil.OpenChangeManualAutoAddDialog(dataList[0], this.Owner);
        }
        protected override void mc_ShowAddDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == MenuUtil.OpenAddManualAutoAddDialog(Owner);
        }
    }

    //キーワード予約の固有メソッド
    public class CmdExeEpgAutoAdd : CmdExeAutoAdd<EpgAutoAddData>
    {
        public CmdExeEpgAutoAdd(UIElement owner) : base(owner) { _copyItemData = EpgAutoAddDataEx.CopyTo; }
        protected override void mc_ShowDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == MenuUtil.OpenChangeEpgAutoAddDialog(dataList[0]);
        }
        protected override void mc_ShowAddDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == MenuUtil.OpenAddEpgAutoAddDialog();
        }
        protected override void mc_ChgGenre(object sender, ExecutedRoutedEventArgs e)
        {
            if (MenuUtil.ChgGenre(dataList.RecSearchKeyList(), this.Owner) == false) return;
            IsCommandExecuted = MenuUtil.AutoAddChange(dataList);
        }
        protected override void mc_CopyNotKey(object sender, ExecutedRoutedEventArgs e)
        {
            Clipboard.SetDataObject(dataList[0].searchInfo.notKey);
            IsCommandExecuted = true;
        }
        protected override void mc_SetNotKey(object sender, ExecutedRoutedEventArgs e)
        {
            if (CmdExeUtil.IsMessageBeforeCommand(e) == true)
            {
                var text = string.Format("Notキーを変更してよろしいですか?\r\n\r\n"
                    + "[変更項目数: {0}]\r\n[貼り付けテキスト: \"{1}\"]\r\n\r\n", dataList.Count, Clipboard.GetText())
                    + CmdExeUtil.FormatTitleListForDialog(dataList.Select(info => info.searchInfo.andKey).ToList());

                if (MessageBox.Show(text.ToString(), "[Notキーワード変更]の確認", MessageBoxButton.OKCancel,
                                    MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK)
                { return; }
            }

            IsCommandExecuted = MenuUtil.EpgAutoAddChangeNotKey(dataList);
        }
    }
}

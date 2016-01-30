using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    //キーワード予約、プログラム自動登録の共通メソッド
    public class CmdExeAutoAdd<T> : CmdExe<T> where T : AutoAddData, new()
    {
        public CmdExeAutoAdd(Control owner) : base(owner) { }
        protected override void mc_ChangeKeyEnabled(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = mutil.AutoAddChangeKeyEnabled(dataList, (byte)CmdExeUtil.ReadIdData(e, 0, 1));
        }
        protected override void mc_ChangeOnOffKeyEnabled(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = mutil.AutoAddChangeOnOffKeyEnabled(dataList);
        }
        protected override void mc_ChangeRecSetting(object sender, ExecutedRoutedEventArgs e)
        {
            if (mcc_chgRecSetting(dataList.RecSettingList(), e, this.Owner) == false) return;
            IsCommandExecuted = mutil.AutoAddChange(dataList);
        }
        protected override void mc_ChgBulkRecSet(object sender, ExecutedRoutedEventArgs e)
        {
            if (mutil.ChangeBulkSet(dataList.RecSettingList(), this.Owner, typeof(T) == typeof(ManualAutoAddData)) == false) return;
            IsCommandExecuted = mutil.AutoAddChange(dataList);
        }
        protected override void mc_Delete(object sender, ExecutedRoutedEventArgs e)
        {
            if (e.Command == EpgCmds.DeleteAll)
            {
                if (CmdExeUtil.CheckAllDeleteCancel(e, dataList.Count) == true)
                { return; }
            }
            else
            {
                if (CmdExeUtil.CheckKeyboardDeleteCancel(e, dataList.Select(data => data.DataTitle).ToList()) == true)
                { return; }
            }
            IsCommandExecuted = mutil.AutoAddDelete(dataList);
        }
        protected override void mc_Delete2(object sender, ExecutedRoutedEventArgs e)
        {
            if (CmdExeUtil.CheckAllProcCancel(e, dataList, true) == true) return;
            IsCommandExecuted = mutil.AutoAddDelete(dataList, true, true, true);
        }
        protected override void mc_AdjustReserve(object sender, ExecutedRoutedEventArgs e)
        {
            if (CmdExeUtil.CheckAllProcCancel(e, dataList, false) == true) return;
            IsCommandExecuted = mutil.AutoAddChangeSyncReserve(dataList, true, false, true);
        }
        protected override void mcs_ctxmLoading_switch(ContextMenu ctxm, MenuItem menu)
        {
            if (menu.Tag == EpgCmdsEx.ChgMenu)
            {
                mcs_chgMenuOpening(menu, dataList.RecSettingList(), typeof(T) == typeof(ManualAutoAddData));
            }
            else if (menu.Tag == EpgCmdsEx.OpenFolderMenu)
            {
                mm.CtxmGenerateOpenFolderItems(menu, this.itemCount == 0 ? null : dataList[0].RecSettingInfo);
            }
        }
    }

    //プログラム自動登録の固有メソッド
    public class CmdExeManualAutoAdd : CmdExeAutoAdd<ManualAutoAddData>
    {
        public CmdExeManualAutoAdd(Control owner) : base(owner) { _copyItemData = ManualAutoAddDataEx.CopyTo; }
        protected override void mc_ShowDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == mutil.OpenChangeManualAutoAddDialog(dataList[0], this.Owner);
        }
        protected override void mc_ShowAddDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == mutil.OpenAddManualAutoAddDialog(Owner);
        }
    }

    //キーワード予約の固有メソッド
    public class CmdExeEpgAutoAdd : CmdExeAutoAdd<EpgAutoAddData>
    {
        public CmdExeEpgAutoAdd(Control owner) : base(owner) { _copyItemData = EpgAutoAddDataEx.CopyTo; }
        protected override void mc_ShowDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == mutil.OpenChangeEpgAutoAddDialog(dataList[0], this.Owner);
        }
        protected override void mc_ShowAddDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == mutil.OpenAddEpgAutoAddDialog(Owner);
        }
        protected override void mc_ChgGenre(object sender, ExecutedRoutedEventArgs e)
        {
            if (mutil.ChgGenre(dataList.RecSearchKeyList(), this.Owner) == false) return;
            IsCommandExecuted = mutil.AutoAddChange(dataList);
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

            IsCommandExecuted = mutil.EpgAutoAddChangeNotKey(dataList);
        }
    }
}

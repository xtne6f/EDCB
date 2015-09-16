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
    public class CmdExeManualAutoAdd : CmdExe<ManualAutoAddData>
    {
        public CmdExeManualAutoAdd(Control owner)
            : base(owner)
        {
            _copyItemData = CtrlCmdDefEx.CopyTo;
        }
        protected override void mc_ShowDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == mutil.OpenChangeManualAutoAddDialog(dataList[0], this.Owner);
        }
        protected override void mc_ShowAddDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == mutil.OpenAddManualAutoAddDialog(Owner);
        }
        protected override void mc_ChangeRecSetting(object sender, ExecutedRoutedEventArgs e)
        {
            if (mcc_chgRecSetting(dataList.RecSettingList(), e, this.Owner) == false) return;
            IsCommandExecuted = mutil.ManualAutoAddChange(dataList);
        }
        protected override void mc_ChgBulkRecSet(object sender, ExecutedRoutedEventArgs e)
        {
            if (mutil.ChangeBulkSet(dataList.RecSettingList(), this.Owner, true) == false) return;
            IsCommandExecuted = mutil.ManualAutoAddChange(dataList);
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
                if (CmdExeUtil.CheckKeyboardDeleteCancel(e, dataList.Select(data => data.title).ToList()) == true)
                { return; }
            }
            IsCommandExecuted = mutil.ManualAutoAddDelete(dataList);
        }
        protected override void mc_Delete2(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = mcs_Delete2_3(sender, e, true);
        }
        protected override void mc_Delete3(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = mcs_Delete2_3(sender, e, false);
        }
        protected bool mcs_Delete2_3(object sender, ExecutedRoutedEventArgs e, bool deleteAutoAddItem)
        {
            if (CmdExeUtil.IsMessageBeforeCommand(e) == true)
            {
                string msg1 = deleteAutoAddItem == true ? "予約ごと削除" : "予約のみ削除";
                string msg2 = deleteAutoAddItem == true ? "削除項目数" : "処理項目数";

                string text = string.Format(msg1 + "してよろしいですか?\r\n\r\n"
                    + "[" + msg2 + ": {0}]\r\n[削除される予約数: {1}]\r\n\r\n", dataList.Count, dataList.GetReserveList().Count)
                    + CmdExeUtil.FormatTitleListForDialog(dataList.Select(info => info.title).ToList());

                if (MessageBox.Show(text, "[" + msg1 + "]の確認", MessageBoxButton.OKCancel,
                                    MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK)
                { return false; }
            }

            if (deleteAutoAddItem == true)
            {
                //EpgTimerSrvでの自動予約登録の実行タイミングに左右されず確実に予約を削除するため、先に削除
                if (mutil.ManualAutoAddDelete(dataList) == false) return false;
            }

            //配下の予約の削除、再収集する
            return mutil.ReserveDelete(dataList.GetReserveList());
        }
        protected override void mc_AdjustReserve(object sender, ExecutedRoutedEventArgs e)
        {
            if (CmdExeUtil.IsMessageBeforeCommand(e) == true)
            {
                string text = string.Format("予約の録画設定を自動登録の録画設定に合わせてもよろしいですか?\r\n\r\n"
                    + "[処理項目数: {0}]\r\n[対象予約数: {1}]\r\n\r\n", dataList.Count, dataList.GetReserveList().Count)
                    + CmdExeUtil.FormatTitleListForDialog(dataList.Select(info => info.title).ToList());

                if (MessageBox.Show(text, "[予約の録画設定変更]の確認", MessageBoxButton.OKCancel,
                                    MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK)
                { return; }
            }
            IsCommandExecuted = mutil.ReserveAdjustChange(dataList);
        }
        protected override void mc_CopyTitle(object sender, ExecutedRoutedEventArgs e)
        {
            mutil.CopyTitle2Clipboard(dataList[0].title, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected override void mc_SearchTitle(object sender, ExecutedRoutedEventArgs e)
        {
            mutil.SearchText(dataList[0].title, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected override void mcs_ctxmLoading_switch(ContextMenu ctxm, MenuItem menu)
        {
            if (menu.Tag == EpgCmdsEx.ChgMenu)
            {
                mcs_chgMenuOpening(menu, dataList.RecSettingList(), true, true);
            }
            else if (menu.Tag == EpgCmdsEx.OpenFolderMenu)
            {
                mm.CtxmGenerateOpenFolderItems(menu, this.itemCount == 0 ? null : dataList[0].recSetting);
            }
        }
    }
}

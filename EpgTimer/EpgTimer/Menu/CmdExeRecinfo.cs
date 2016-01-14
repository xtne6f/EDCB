using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    public class CmdExeRecinfo : CmdExe<RecFileInfo>
    {
        public CmdExeRecinfo(Control owner)
            : base(owner)
        {
            _copyItemData = RecFileInfoEx.CopyTo;
        }
        protected override void mc_ShowDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == mutil.OpenRecInfoDialog(dataList[0], this.Owner);
        }
        protected override void mc_ProtectChange(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = mutil.RecinfoChgProtect(dataList);
        }
        protected override void mc_Delete(object sender, ExecutedRoutedEventArgs e)
        {
            dataList = dataList.GetNoProtectedList();
            if (dataList.Count == 0) return;

            if (e.Command == EpgCmds.DeleteAll)
            {
                if (CmdExeUtil.CheckAllDeleteCancel(e, dataList.Count) == true)
                { return; }
            }
            else
            {
                if (CmdExeUtil.CheckKeyboardDeleteCancel(e, dataList.Select(data => data.Title).ToList()) == true)
                { return; }
            }

            if (IniFileHandler.GetPrivateProfileInt("SET", "RecInfoDelFile", 0, SettingPath.CommonIniPath) == 1)
            {
                if (MessageBox.Show("録画ファイルが存在する場合は一緒に削除されます。\r\nよろしいですか？",
                    "ファイル削除", MessageBoxButton.OKCancel) != MessageBoxResult.OK)
                { return; }
            }

            IsCommandExecuted = mutil.RecinfoDelete(dataList);
        }
        protected override void mc_ToAutoadd(object sender, ExecutedRoutedEventArgs e)
        {
            mutil.SendAutoAdd(dataList[0], this.Owner, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected override void mc_Play(object sender, ExecutedRoutedEventArgs e)
        {
            CommonManager.Instance.FilePlay(dataList[0].RecFilePath);
            IsCommandExecuted = true;
        }
        protected override void mc_OpenFolder(object sender, ExecutedRoutedEventArgs e)
        {
            CommonManager.Instance.OpenFolder(dataList[0].RecFilePath);
            IsCommandExecuted = true;
        }
        protected override void mc_CopyTitle(object sender, ExecutedRoutedEventArgs e)
        {
            mutil.CopyTitle2Clipboard(dataList[0].Title, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected override void mc_CopyContent(object sender, ExecutedRoutedEventArgs e)
        {
            mutil.CopyContent2Clipboard(dataList[0], CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected override void mc_SearchTitle(object sender, ExecutedRoutedEventArgs e)
        {
            mutil.SearchTextWeb(dataList[0].Title, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected override void mcs_ctxmLoading_switch(ContextMenu ctxm, MenuItem menu)
        {
            if (menu.Tag == EpgCmds.Delete || menu.Tag == EpgCmds.DeleteAll)
            {
                menu.IsEnabled = dataList.HasNoProtected();
            }
        }
    }
}

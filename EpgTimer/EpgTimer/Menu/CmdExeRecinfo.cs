using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    public class CmdExeRecinfo : CmdExe<RecFileInfo>
    {
        public CmdExeRecinfo(UIElement owner)
            : base(owner)
        {
            _copyItemData = RecFileInfoEx.CopyTo;
        }
        protected override void mc_ShowDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == MenuUtil.OpenRecInfoDialog(dataList[0], this.Owner);
        }
        protected override void mc_ProtectChange(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = MenuUtil.RecinfoChgProtect(dataList);
        }
        protected override void mc_Delete(object sender, ExecutedRoutedEventArgs e)
        {
            dataList = dataList.GetNoProtectedList();
            if (mcs_DeleteCheck(e) == false) return;

            if (IniFileHandler.GetPrivateProfileInt("SET", "RecInfoDelFile", 0, SettingPath.CommonIniPath) == 1)
            {
                if (MessageBox.Show("録画ファイルが存在する場合は一緒に削除されます。\r\nよろしいですか？",
                    "ファイル削除", MessageBoxButton.OKCancel) != MessageBoxResult.OK)
                { return; }
            }

            IsCommandExecuted = MenuUtil.RecinfoDelete(dataList);
        }
        protected override void mc_Play(object sender, ExecutedRoutedEventArgs e)
        {
            CommonManager.Instance.FilePlay(dataList[0].RecFilePath);
            IsCommandExecuted = true;
        }
        protected override void mc_CopyContent(object sender, ExecutedRoutedEventArgs e)
        {
            MenuUtil.CopyContent2Clipboard(dataList[0], CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected override SearchItem mcs_GetSearchItem()
        {
            if (dataList.Count == 0) return null;

            EpgEventInfo info = dataList[0].SearchEventInfo();
            return info == null ? null : new SearchItem(info);
        }
        protected override void mcs_ctxmLoading_switch(ContextMenu ctxm, MenuItem menu)
        {
            if (menu.Tag == EpgCmds.Delete || menu.Tag == EpgCmds.DeleteAll)
            {
                menu.IsEnabled = dataList.HasNoProtected();
            }
            else if (menu.Tag == EpgCmds.JumpTable)
            {
                menu.IsEnabled = mcs_GetSearchItem() != null;
            }
            else if (menu.Tag == EpgCmdsEx.ShowAutoAddDialogMenu)
            {
                menu.IsEnabled = mm.CtxmGenerateChgAutoAdd(menu, dataList.Count != 0 ? dataList[0] : null);
            }
            else if (menu.Tag == EpgCmds.OpenFolder)
            {
                string path = (dataList.Count == 0 ? null : dataList[0].RecFilePath);
                (menu.CommandParameter as EpgCmdParam).Data = path;
                menu.ToolTip = path;
            }
        }
    }
}

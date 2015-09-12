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
    public class CmdExeEpgAutoAdd : CmdExe<EpgAutoAddData>
    {
        public CmdExeEpgAutoAdd(Control owner)
            : base(owner)
        {
            _copyItemData = CtrlCmdDefEx.CopyTo;
        }
        protected override void mc_ShowDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == mutil.OpenChangeEpgAutoAddDialog(dataList[0], this.Owner);
        }
        protected override void mc_ShowAddDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == mutil.OpenAddEpgAutoAddDialog(Owner);
        }
        protected override void mc_ChangeRecSetting(object sender, ExecutedRoutedEventArgs e)
        {
            if (mcc_chgRecSetting(dataList.RecSettingList(), e, this.Owner) == false) return;
            IsCommandExecuted = mutil.EpgAutoAddChange(dataList);
        }
        protected override void mc_ChgBulkRecSet(object sender, ExecutedRoutedEventArgs e)
        {
            if (mutil.ChangeBulkSet(dataList.RecSettingList(), this.Owner) == false) return;
            IsCommandExecuted = mutil.EpgAutoAddChange(dataList);
        }
        protected override void mc_ChgGenre(object sender, ExecutedRoutedEventArgs e)
        {
            if (mutil.ChgGenre(dataList.RecSearchKeyList(), this.Owner) == false) return;
            IsCommandExecuted = mutil.EpgAutoAddChange(dataList);
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
                if (CmdExeUtil.CheckKeyboardDeleteCancel(e, 
                    dataList.Select(data => new EpgAutoDataItem(data).AndKey).ToList()) == true) return;
            }
            IsCommandExecuted = mutil.EpgAutoAddDelete(dataList);
        }
        protected override void mc_Delete2(object sender, ExecutedRoutedEventArgs e)
        {
            var delList = new List<ReserveData>();

            if (CmdExeUtil.IsMessageBeforeCommand(e) == true)
            {
                dataList.ForEach(info => delList.AddRange(info.SearchItemList().ReserveInfoList()));
                delList = delList.Distinct().ToList();
                int DisplayNum = Settings.Instance.KeyDeleteDisplayItemNum;

                var text = new StringBuilder(
                    string.Format("予約項目ごと削除してよろしいですか?\r\n"
                                + "(無効の「自動予約登録項目」による予約も削除されます。)\r\n\r\n"
                                + "[削除項目数: {0}]\r\n[削除される予約数: {1}]\r\n\r\n"
                                , dataList.Count, delList.Count));
                foreach (var info in dataList.Take(DisplayNum)) { text.AppendFormat(" ・ {0}\r\n", new EpgAutoDataItem(info).AndKey); }
                if (dataList.Count > DisplayNum) text.AppendFormat("\r\n　　ほか {0} 項目", dataList.Count - DisplayNum);

                if (MessageBox.Show(text.ToString(), "[予約ごと削除]の確認", MessageBoxButton.OKCancel,
                                    MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK)
                { return; }
            }

            //EpgTimerSrvでの自動予約登録の実行タイミングに左右されず確実に予約を削除するため、
            //先に自動予約登録項目を削除する。
            if (mutil.EpgAutoAddDelete(dataList) == false) return;

            //配下の予約の削除、リストはAutoAddAppendにあるが、きちんと読み直す
            var keyList = new List<EpgSearchKeyInfo>();
            var list = new List<EpgEventInfo>();

            dataList.ForEach(item =>
            {
                EpgSearchKeyInfo key = item.searchInfo;
                key.keyDisabledFlag = 0; //無効解除
                keyList.Add(key);
            });

            cmd.SendSearchPg(keyList, ref list);

            delList.Clear();
            foreach (EpgEventInfo info in list)
            {
                if (info.start_time.AddSeconds(info.DurationFlag == 0 ? 0 : info.durationSec) > DateTime.Now) 
                {
                    foreach (ReserveData info2 in CommonManager.Instance.DB.ReserveList.Values)
                    {
                        if (CommonManager.EqualsPg(info, info2) == true)
                        {
                            //重複したEpgEventInfoは送られてこないので、登録時の重複チェックは不要
                            delList.Add(info2);
                            break;
                        }
                    }
                }
            }

            IsCommandExecuted = mutil.ReserveDelete(delList);
        }
        protected override void mc_CopyTitle(object sender, ExecutedRoutedEventArgs e)
        {
            mutil.CopyTitle2Clipboard(new EpgAutoDataItem(dataList[0]).AndKey, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected override void mc_SearchTitle(object sender, ExecutedRoutedEventArgs e)
        {
            mutil.SearchText(new EpgAutoDataItem(dataList[0]).AndKey, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
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
                int DisplayNum = Settings.Instance.KeyDeleteDisplayItemNum;
                var text = new StringBuilder(string.Format("Notキーを変更してよろしいですか?\r\n\r\n"
                    + "[変更項目数: {0}]\r\n[貼り付けテキスト: \"{1}\"]\r\n\r\n", dataList.Count, Clipboard.GetText()));
                foreach (var info in dataList.Take(DisplayNum)) { text.AppendFormat(" ・ {0}\r\n", new EpgAutoDataItem(info).AndKey); }
                if (dataList.Count > DisplayNum) text.AppendFormat("\r\n　　ほか {0} 項目", dataList.Count - DisplayNum);

                if (MessageBox.Show(text.ToString(), "[Notキーワード変更]の確認", MessageBoxButton.OKCancel,
                                    MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK)
                { return; }
            }

            IsCommandExecuted = mutil.EpgAutoAddChangeNotKey(dataList);
        }
        protected override void mcs_ctxmLoading_switch(ContextMenu ctxm, MenuItem menu)
        {
            if (menu.Tag == EpgCmdsEx.ChgMenu)
            {
                mcs_chgMenuOpening(menu, dataList.RecSettingList(), false, false);
            }
            else if (menu.Tag == EpgCmdsEx.OpenFolderMenu)
            {
                mm.CtxmGenerateOpenFolderItems(menu, this.itemCount == 0 ? null : dataList[0].recSetting);
            }
        }
    }
}

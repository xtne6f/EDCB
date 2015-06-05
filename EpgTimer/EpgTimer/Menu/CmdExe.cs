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
    public class CmdExe<T> where T : class, new()
    {
        public struct cmdOption
        {
            public ExecutedRoutedEventHandler Exe;
            public CanExecuteRoutedEventHandler CanExe;
            public cmdExeType ExeType;
            public bool IsNeedClone;//データをコピーする。変更系コマンドで通信エラーなどあった場合に問題が起きないようにする。
            public bool IsNeedItem;//コマンド実行に対象が必要。ビュー切り替えなどはこれがfalse。
            public bool IsReleaseItem;//コマンド実行前に選択アイテムを解除する。
            public bool IsChangeDB;//コマンドにより、DBの状態が変化するもの。

            public cmdOption(ExecutedRoutedEventHandler exe
                , CanExecuteRoutedEventHandler canExe = null
                , cmdExeType exeType = cmdExeType.NoSetItem
                , bool needClone = false
                , bool needItem = true
                , bool releaseItem = false
                , bool changeDB = false
                )
            {
                Exe = exe;
                CanExe = canExe;
                ExeType = exeType;
                IsNeedClone = needClone;
                IsNeedItem = needItem;
                IsReleaseItem = releaseItem;
                IsChangeDB = changeDB;
            }
        }
        public enum cmdExeType
        {
            MultiItem,//複数選択対象
            SingleItem,//単一アイテム対象
            NoSetItem,//アイテム不要か自力で収集、IsCommandCancelは使う
            AllItem,//全アイテム対象
            Direct//完全に独立して実行、IsCommandCancelも使わない
        }

        protected CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        protected MenuUtil mutil = CommonManager.Instance.MUtil;
        protected MenuManager mm = CommonManager.Instance.MM;

        protected Control Owner;

        protected Dictionary<ICommand, cmdOption> cmdList = new Dictionary<ICommand, cmdOption>();

        protected Func<bool, List<T>> _getDataList = null;
        protected Func<bool, T> _selectSingleData = null;
        protected Action _releaseSelectedData = null;
        protected Action<object, ExecutedRoutedEventArgs, cmdOption> _postProc = null;
        protected Action<T, T> _copyItemData = null;

        public void SetFuncGetDataList(Func<bool, List<T>> f) { _getDataList = f; }
        public void SetFuncSelectSingleData(Func<bool, T> f) { _selectSingleData = f; }
        public void SetFuncReleaseSelectedData(Action f) { _releaseSelectedData = f; }
        public void SetFuncPostProc(Action<object, ExecutedRoutedEventArgs, cmdOption> f) { _postProc = f; }

        protected List<T> dataList = new List<T>();
        public bool IsCommandExecuted { get; protected set; }

        public CmdExe() { }
        public CmdExe(Control owner)
        {
            this.Owner = owner;
            cmdList.Add(EpgCmds.Add, new cmdOption(mc_Add, null, cmdExeType.MultiItem, changeDB: true));
            cmdList.Add(EpgCmds.AddOnPreset, new cmdOption(mc_AddOnPreset, null, cmdExeType.MultiItem, changeDB: true));
            cmdList.Add(EpgCmds.ChgOnOff, new cmdOption(mc_ChangeOnOff, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgOnPreset, new cmdOption(mc_ChangeRecSetting, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgBulkRecSet, new cmdOption(mc_ChgBulkRecSet, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgGenre, new cmdOption(mc_ChgGenre, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgRecmode, new cmdOption(mc_ChangeRecSetting, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgPriority, new cmdOption(mc_ChangeRecSetting, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgRelay, new cmdOption(mc_ChangeRecSetting, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgPittari, new cmdOption(mc_ChangeRecSetting, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgTuner, new cmdOption(mc_ChangeRecSetting, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgMarginStart, new cmdOption(mc_ChangeRecSetting, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgMarginStartValue, new cmdOption(mc_ChangeRecSetting, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgMarginEnd, new cmdOption(mc_ChangeRecSetting, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgMarginEndValue, new cmdOption(mc_ChangeRecSetting, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.Delete, new cmdOption(mc_Delete, null, cmdExeType.MultiItem, changeDB: true));
            cmdList.Add(EpgCmds.Delete2, new cmdOption(mc_Delete2, null, cmdExeType.MultiItem, changeDB: true));
            cmdList.Add(EpgCmds.DeleteAll, new cmdOption(mc_Delete, null, cmdExeType.AllItem, changeDB: true));
            cmdList.Add(EpgCmds.ShowDialog, new cmdOption(mc_ShowDialog, null, cmdExeType.SingleItem, changeDB: true));
            cmdList.Add(EpgCmds.ShowAddDialog, new cmdOption(mc_ShowAddDialog, null, cmdExeType.NoSetItem, false, false, true, changeDB: true));
            cmdList.Add(EpgCmds.JumpTable, new cmdOption(mc_JumpTable, null, cmdExeType.SingleItem));
            cmdList.Add(EpgCmds.ToAutoadd, new cmdOption(mc_ToAutoadd, null, cmdExeType.SingleItem));
            cmdList.Add(EpgCmds.ReSearch, new cmdOption(null, null, cmdExeType.Direct));//個別に指定
            cmdList.Add(EpgCmds.ReSearch2, new cmdOption(null, null, cmdExeType.Direct));//個別に指定
            cmdList.Add(EpgCmds.Play, new cmdOption(mc_Play, null, cmdExeType.SingleItem));
            cmdList.Add(EpgCmds.OpenFolder, new cmdOption(mc_OpenFolder, null, cmdExeType.SingleItem));
            cmdList.Add(EpgCmds.CopyTitle, new cmdOption(mc_CopyTitle, null, cmdExeType.SingleItem));
            cmdList.Add(EpgCmds.CopyContent, new cmdOption(mc_CopyContent, null, cmdExeType.SingleItem));
            cmdList.Add(EpgCmds.SearchTitle, new cmdOption(mc_SearchTitle, null, cmdExeType.SingleItem));
            cmdList.Add(EpgCmds.CopyNotKey, new cmdOption(mc_CopyNotKey, null, cmdExeType.SingleItem));
            cmdList.Add(EpgCmds.SetNotKey, new cmdOption(mc_SetNotKey, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ProtectChange, new cmdOption(mc_ProtectChange, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ViewChgSet, new cmdOption(null, null, cmdExeType.Direct, false, false));//個別に指定
            cmdList.Add(EpgCmds.ViewChgMode, new cmdOption(null, null, cmdExeType.Direct, false, false));//個別に指定
            cmdList.Add(EpgCmds.MenuSetting, new cmdOption(mc_MenuSetting, null, cmdExeType.Direct, false, false));

            cmdList.Add(EpgCmdsEx.AddMenu, new cmdOption(null, null, cmdExeType.MultiItem));//メニュー用
            cmdList.Add(EpgCmdsEx.ChgMenu, new cmdOption(null, null, cmdExeType.MultiItem));//メニュー用
            cmdList.Add(EpgCmdsEx.OpenFolderMenu, new cmdOption(null, null, cmdExeType.SingleItem));//メニュー用
            cmdList.Add(EpgCmdsEx.ViewMenu, new cmdOption(null, null, cmdExeType.NoSetItem, false, false));//メニュー用
        }
        protected virtual void SetData(bool IsAllData = false)
        {
            dataList = _getDataList == null ? null : _getDataList(IsAllData);
            dataList = dataList == null ? new List<T>() : dataList.OfType<T>().ToList();
            OrderAdjust<T>(dataList, _selectSingleData);
        }
        protected void OrderAdjust<S>(List<S> list, Func<bool, S> _getSingle) where S : class
        {
            if (list.Count >= 2 && _getSingle != null)
            {
                S single = _getSingle(true);
                if (list.Contains(single))
                {
                    list.Remove(single);
                    list.Insert(0, single);
                }
            }
        }
        protected virtual void ClearData()
        {
            dataList.Clear();
        }
        protected virtual int itemCount { get { return dataList == null ? 0 : dataList.Count; } }
        protected virtual void SelectSingleData()
        {
            if (_selectSingleData != null) _selectSingleData(false);
        }
        protected virtual void ReleaseSelectedData()
        {
            if (_releaseSelectedData != null) _releaseSelectedData();
        }
        protected virtual void CopyDataList()
        {
            if (_copyItemData != null) dataList = CopyObj.Clone(dataList, _copyItemData);
        }
        protected void PostProc(object sender, ExecutedRoutedEventArgs e)
        {
            if (_postProc != null) _postProc(sender, e, GetCmdParam(e.Command));
        }
        protected cmdOption GetCmdParam(ICommand icmd)
        {
            cmdOption cmdPrm;
            cmdList.TryGetValue(icmd, out cmdPrm);//無ければnullメンバのparamが返る。
            return cmdPrm;
        }
        public void AddReplaceCommand(ICommand icmd, ExecutedRoutedEventHandler exe, CanExecuteRoutedEventHandler canExe = null)
        {
            if (icmd == null) return;

            cmdOption cmdPrm = GetCmdParam(icmd);
            cmdPrm.Exe = exe;
            cmdPrm.CanExe = canExe;
            cmdPrm.ExeType = cmdExeType.Direct;

            if (cmdList.ContainsKey(icmd) == true)
            {
                cmdList[icmd] = cmdPrm;
            }
            else
            {
                cmdList.Add(icmd, cmdPrm);
            }
        }
        /// <summary>持っているコマンドを登録する。</summary>
        public void ResetCommandBindings(UIElement cTrgView, UIElement cTrgMenu = null)
        {
            try
            {
                var cBinds = new CommandBindingCollection();
                foreach (var item in cmdList)
                {
                    //Exeがあるものを処理する。
                    ExecutedRoutedEventHandler exeh = GetExecute(item.Key);
                    if (exeh != null)
                    {
                        foreach (var cTrg in new List<UIElement> { cTrgView, cTrgMenu }.OfType<UIElement>())
                        {
                            //古いものは削除
                            var delList = cTrg.CommandBindings.OfType<CommandBinding>().Where(bind => bind.Command == item.Key).ToList();
                            delList.ForEach(delItem => cTrg.CommandBindings.Remove(delItem));
                            cTrg.CommandBindings.Add(new CommandBinding(item.Key, exeh, GetCanExecute(item.Key)));
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        public ExecutedRoutedEventHandler GetExecute(ICommand icmd)
        {
            cmdOption cmdPrm = GetCmdParam(icmd);
            return cmdPrm.ExeType == cmdExeType.Direct ? cmdPrm.Exe : GetExecute(cmdPrm);
        }
        protected ExecutedRoutedEventHandler GetExecute(cmdOption cmdPrm)
        {
            return new ExecutedRoutedEventHandler((sender, e) =>
            {
                try
                {
                    IsCommandExecuted = false;

                    if (cmdPrm.ExeType == cmdExeType.SingleItem) SelectSingleData();
                    if (cmdPrm.ExeType != cmdExeType.NoSetItem) SetData(cmdPrm.ExeType == cmdExeType.AllItem);
                    if (cmdPrm.IsNeedClone == true) CopyDataList();
                    if (cmdPrm.IsReleaseItem == true) ReleaseSelectedData();

                    if (cmdPrm.Exe != null && (cmdPrm.IsNeedItem == false || this.itemCount != 0))
                    {
                        cmdPrm.Exe(sender, e);
                        PostProc(sender, e);//後処理があれば実施する
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }
                ClearData();
            });
        }
        public CanExecuteRoutedEventHandler GetCanExecute(ICommand icmd)
        {
            return GetCmdParam(icmd).CanExe;
        }

        protected virtual void mc_Add(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_AddOnPreset(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ChangeOnOff(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ChangeRecSetting(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ChgBulkRecSet(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ChgGenre(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_Delete(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_Delete2(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ShowDialog(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ShowAddDialog(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_JumpTable(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ToAutoadd(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_Play(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_OpenFolder(object sender, ExecutedRoutedEventArgs e)
        {
            CommonManager.Instance.OpenFolder(CmdExeUtil.ReadObjData(e) as string);
        }
        protected virtual void mc_CopyTitle(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_CopyContent(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_SearchTitle(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_CopyNotKey(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_SetNotKey(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ProtectChange(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_MenuSetting(object sender, ExecutedRoutedEventArgs e)
        {
            SetContextMenuWindow dlg = new SetContextMenuWindow();
            dlg.info = Settings.Instance.MenuSet.Clone();
            dlg.Owner = (Window)PresentationSource.FromVisual(Owner).RootVisual;

            if (dlg.ShowDialog() == true)
            {
                Settings.Instance.MenuSet = dlg.info.Clone();

                var mainWindow = (MainWindow)Application.Current.MainWindow;
                mainWindow.RefreshMenu(true);

                if (dlg.Owner is SearchWindow)
                {
                    (dlg.Owner as SearchWindow).RefreshMenu();
                }
            }
        }
        protected bool mcc_chgRecSetting(List<RecSettingData> infoList, ExecutedRoutedEventArgs e, Control owner = null)
        {
            if (e.Command == EpgCmds.ChgOnPreset)
            {
                return mutil.ChangeOnPreset(infoList, (uint)CmdExeUtil.ReadIdData(e, 0, 0xFE));
            }
            else if (e.Command == EpgCmds.ChgRecmode)
            {
                return mutil.ChangeRecmode(infoList, (byte)CmdExeUtil.ReadIdData(e, 0, 5));
            }
            else if (e.Command == EpgCmds.ChgPriority)
            {
                return mutil.ChangePriority(infoList, (byte)CmdExeUtil.ReadIdData(e, 1, 5));
            }
            else if (e.Command == EpgCmds.ChgRelay)
            {
                return mutil.ChangeRelay(infoList, (byte)CmdExeUtil.ReadIdData(e, 0, 1));
            }
            else if (e.Command == EpgCmds.ChgPittari)
            {
                return mutil.ChangePittari(infoList, (byte)CmdExeUtil.ReadIdData(e, 0, 1));
            }
            else if (e.Command == EpgCmds.ChgTuner)
            {
                return mutil.ChangeTuner(infoList, (uint)CmdExeUtil.ReadIdData(e, 0, 0xFE));
            }
            else if (e.Command == EpgCmds.ChgMarginStart)
            {
                return mutil.ChangeMargin(infoList, CmdExeUtil.ReadIdData(e), true);
            }
            else if (e.Command == EpgCmds.ChgMarginStartValue)
            {
                return mutil.ChangeMarginValue(infoList, true, owner);
            }
            else if (e.Command == EpgCmds.ChgMarginEnd)
            {
                return mutil.ChangeMargin(infoList, CmdExeUtil.ReadIdData(e), false);
            }
            else if (e.Command == EpgCmds.ChgMarginEndValue)
            {
                return mutil.ChangeMarginValue(infoList, false, owner);
            }
            return false;
        }
        public virtual void SupportContextMenuLoading(object sender, RoutedEventArgs e)
        {
            try
            {
                IsCommandExecuted = false;
                SetData();

                var ctxm = sender as ContextMenu;

                ctxm.IsOpen = ctxm.Items.Count != 0;
                if (ctxm.IsOpen == false)
                {
                    return;
                }

                if (e != null)
                {
                    //リストビューの場合は、アイテムの無いところではデータ選択してないものと見なす。
                    if (ctxm.PlacementTarget is ListBoxItem == false)
                    {
                        ClearData();
                    }
                }

                foreach (var menu in ctxm.Items.OfType<MenuItem>())
                {
                    //有効無効制御。ボタンをあまりグレーアウトしたくないのでCanExecuteを使わずここで実施する
                    menu.IsEnabled = this.itemCount != 0;
                    var icmd = menu.Tag as ICommand;
                    if (icmd != null && cmdList.ContainsKey(icmd) == true)
                    {
                        if (GetCmdParam(icmd).IsNeedItem == false)
                        {
                            menu.IsEnabled = true;
                        }
                    }

                    //コマンド集に応じた処理
                    mcs_ctxmLoading_switch(ctxm, menu);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            ClearData();
        }
        protected virtual void mcs_ctxmLoading_switch(ContextMenu ctxm, MenuItem menu) { }
        protected void mcs_chgMenuOpening(MenuItem menu, List<RecSettingData> recSettings, bool pg1 = false, bool pgAll = false)
        {
            if (menu.IsEnabled == false) return;

            var view = (menu.CommandParameter as EpgCmdParam).Code;
            pg1 |= pgAll;

            Action<MenuItem, int> SetCheckmarkSubMenus = (subMenu, value) =>
            {
                foreach (var item in subMenu.Items.OfType<MenuItem>())
                {
                    item.IsChecked = ((item.CommandParameter as EpgCmdParam).ID == value);
                }
            };

            //選択アイテムが全て同じ設定の場合だけチェックを表示する
            foreach (var subMenu in menu.Items.OfType<MenuItem>())
            {
                if (subMenu.Tag == EpgCmdsEx.ChgOnPresetMenu)
                {
                    mm.CtxmGenerateChgOnPresetItems(subMenu);
                }
                else if (subMenu.Tag == EpgCmds.ChgBulkRecSet)
                {
                    subMenu.Visibility = (recSettings.Count < 2 ? Visibility.Collapsed : Visibility.Visible);
                    subMenu.ToolTip = (pg1 == true ?
                        "プログラム予約は「イベントリレー追従」「ぴったり(？)録画」のオプションは設定対象外" : null);
                }
                else if (subMenu.Tag == EpgCmds.ChgGenre)
                {
                    if (view != CtxmCode.EpgAutoAddView)
                    {
                        subMenu.Visibility = Visibility.Collapsed;
                        continue;
                    }

                    subMenu.Visibility = (recSettings.Count < 2 ? Visibility.Collapsed : Visibility.Visible);
                }
                else if (subMenu.Tag == EpgCmdsEx.ChgRecmodeMenu)
                {
                    byte value = recSettings.All(info => info.RecMode == recSettings[0].RecMode) ? recSettings[0].RecMode : byte.MaxValue;
                    subMenu.Header = string.Format("録画モード : {0}", value == byte.MaxValue ? "*" : CommonManager.Instance.ConvertRecModeText(value));
                    SetCheckmarkSubMenus(subMenu, value);
                }
                else if (subMenu.Tag == EpgCmdsEx.ChgPriorityMenu)
                {
                    byte value = recSettings.All(info => info.Priority == recSettings[0].Priority) ? recSettings[0].Priority : byte.MaxValue;
                    subMenu.Header = string.Format("優先度 : {0}", value == byte.MaxValue ? "*" : value.ToString());
                    SetCheckmarkSubMenus(subMenu, value);
                }
                else if (subMenu.Tag == EpgCmdsEx.ChgRelayMenu || subMenu.Tag == EpgCmdsEx.ChgPittariMenu)
                {
                    if (view == CtxmCode.ManualAutoAddView)
                    {
                        subMenu.Visibility = Visibility.Collapsed;
                        continue;
                    }

                    byte value;
                    string format;
                    if (subMenu.Tag == EpgCmdsEx.ChgRelayMenu)
                    {
                        value = recSettings.All(info => info.TuijyuuFlag == recSettings[0].TuijyuuFlag) ? recSettings[0].TuijyuuFlag : byte.MaxValue;
                        format = "イベントリレー追従 : {0}";
                    }
                    else
                    {
                        value = recSettings.All(info => info.PittariFlag == recSettings[0].PittariFlag) ? recSettings[0].PittariFlag : byte.MaxValue;
                        format = "ぴったり（？）録画 : {0}";
                    }
                    value = (pg1 == true ? byte.MaxValue : value);
                    subMenu.Header = string.Format(format, value == byte.MaxValue ? "*" : CommonManager.Instance.YesNoDictionary[value].DisplayName);
                    SetCheckmarkSubMenus(subMenu, value);
                    subMenu.ToolTip = (pg1 == true ? "プログラム予約は対象外" : null);
                    subMenu.IsEnabled = (pgAll == false);
                }
                else if (subMenu.Tag == EpgCmdsEx.ChgTunerMenu)
                {
                    uint tunerID = recSettings.All(info => info.TunerID == recSettings[0].TunerID) ? recSettings[0].TunerID : uint.MaxValue;
                    mm.CtxmGenerateTunerMenuItems(subMenu);
                    subMenu.Header = string.Format("チューナー : {0}", tunerID == uint.MaxValue ? "*" : CommonManager.Instance.ConvertTunerText(tunerID));
                    SetCheckmarkSubMenus(subMenu, (int)tunerID);
                }
                else if (subMenu.Tag == EpgCmdsEx.ChgMarginStartMenu)
                {
                    int value = recSettings.All(info => mutil.GetMargin(info, true) == mutil.GetMargin(recSettings[0], true)) ? mutil.GetMargin(recSettings[0], true) : int.MaxValue;
                    subMenu.Header = string.Format("開始マージン : {0} 秒", value == int.MaxValue ? "*" : value.ToString());
                }
                else if (subMenu.Tag == EpgCmdsEx.ChgMarginEndMenu)
                {
                    int value = recSettings.All(info => mutil.GetMargin(info, false) == mutil.GetMargin(recSettings[0], false)) ? mutil.GetMargin(recSettings[0], false) : int.MaxValue;
                    subMenu.Header = string.Format("終了マージン : {0} 秒", value == int.MaxValue ? "*" : value.ToString());
                }
            }
        }
    }

    public class CmdExeUtil
    {
        public static bool CheckAllDeleteCancel(ExecutedRoutedEventArgs e, int Count)
        {
            if (CmdExeUtil.IsMessageBeforeCommand(e) == false) return false;

            return (MessageBox.Show(string.Format(
                "全て削除しますか?\r\n" + "[削除アイテム数: {0}]", Count)
                , "[全削除]の確認", MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK);
        }
        public static bool CheckKeyboardDeleteCancel(ExecutedRoutedEventArgs e, List<string> list)
        {
            if (IsDisplayKgMessage(e) == false) return false;
            if (list == null || list.Count == 0) return false;

            int DisplayNum = Settings.Instance.KeyDeleteDisplayItemNum;
            var text = new StringBuilder(string.Format("削除しますか?\r\n\r\n"
                + "[削除アイテム数: {0}]\r\n\r\n", list.Count));
            foreach (var info in list.Take(DisplayNum)) { text.AppendFormat(" ・ {0}\r\n", info); }
            if (list.Count > DisplayNum) text.AppendFormat("\r\n　　ほか {0} アイテム", list.Count - DisplayNum);

            return (MessageBox.Show(text.ToString(), "削除の確認", MessageBoxButton.OKCancel,
                MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK);
        }
        public static bool IsMessageBeforeCommand(ExecutedRoutedEventArgs e)
        {
            if (HasCommandParameter(e) == false) return false;
            //コマンド側のオプションに変更可能なようにまとめておく
            bool NoMessage = false;
            if (e.Command == EpgCmds.DeleteAll) NoMessage = Settings.Instance.MenuSet.NoMessageDeleteAll;
            else if (e.Command == EpgCmds.Delete2) NoMessage = Settings.Instance.MenuSet.NoMessageDelete2;
            else if (e.Command == EpgCmds.SetNotKey) NoMessage = Settings.Instance.MenuSet.NoMessageNotKEY;

            return NoMessage == false || IsDisplayKgMessage(e);
        }
        public static bool IsDisplayKgMessage(ExecutedRoutedEventArgs e)
        {
            return Settings.Instance.MenuSet.NoMessageKeyGesture == false && IsKeyGesture(e);
        }
        public static bool IsKeyGesture(ExecutedRoutedEventArgs e)
        {
            if (HasCommandParameter(e) == false) return false;
            return (e.Parameter as EpgCmdParam).SourceType == typeof(KeyGesture);
        }
        public static int ReadIdData(ExecutedRoutedEventArgs e, int min = int.MinValue, int max = int.MaxValue)
        {
            if (HasCommandParameter(e) == false) return min;
            return Math.Max(Math.Min((int)((e.Parameter as EpgCmdParam).ID), max), min);
        }
        public static object ReadObjData(ExecutedRoutedEventArgs e)
        {
            if (HasCommandParameter(e) == false) return null;
            return (e.Parameter as EpgCmdParam).Data;
        }
        public static bool HasCommandParameter(ExecutedRoutedEventArgs e)
        {
            return (e != null && e.Parameter is EpgCmdParam);
        }

    }
}
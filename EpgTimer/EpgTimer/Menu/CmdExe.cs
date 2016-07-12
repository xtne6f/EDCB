using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    public class CmdExe<T> where T : class, IRecWorkMainData, new()
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

        protected static CtrlCmdUtil cmd { get { return CommonManager.Instance.CtrlCmd; } }
        protected static MenuManager mm { get { return CommonManager.Instance.MM; } }

        protected Control Owner;
        protected static MainWindow mainWindow { get { return ViewUtil.MainWindow; } }

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

        protected Dictionary<ICommand, string> cmdMessage = new Dictionary<ICommand, string>();

        public CmdExe(Control owner)
        {
            this.Owner = owner;
            cmdList.Add(EpgCmds.Add, new cmdOption(mc_Add, null, cmdExeType.MultiItem, changeDB: true));
            cmdList.Add(EpgCmds.AddOnPreset, new cmdOption(mc_AddOnPreset, null, cmdExeType.MultiItem, changeDB: true));
            cmdList.Add(EpgCmds.ChgOnOff, new cmdOption(mc_ChangeOnOff, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgOnPreset, new cmdOption(mc_ChangeRecSetting, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgResMode, new cmdOption(mc_ChgResMode, null, cmdExeType.MultiItem, changeDB: true));
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
            cmdList.Add(EpgCmds.ChgKeyEnabled, new cmdOption(mc_ChangeKeyEnabled, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.ChgOnOffKeyEnabled, new cmdOption(mc_ChangeOnOffKeyEnabled, null, cmdExeType.MultiItem, true, changeDB: true));
            cmdList.Add(EpgCmds.Delete, new cmdOption(mc_Delete, null, cmdExeType.MultiItem, changeDB: true));
            cmdList.Add(EpgCmds.Delete2, new cmdOption(mc_Delete2, null, cmdExeType.MultiItem, changeDB: true));
            cmdList.Add(EpgCmds.DeleteAll, new cmdOption(mc_Delete, null, cmdExeType.AllItem, changeDB: true));
            cmdList.Add(EpgCmds.AdjustReserve, new cmdOption(mc_AdjustReserve, null, cmdExeType.MultiItem, changeDB: true));
            cmdList.Add(EpgCmds.ShowDialog, new cmdOption(mc_ShowDialog, null, cmdExeType.SingleItem, changeDB: true));
            cmdList.Add(EpgCmds.ShowAddDialog, new cmdOption(mc_ShowAddDialog, null, cmdExeType.NoSetItem, false, false, true, changeDB: true));
            cmdList.Add(EpgCmds.JumpReserve, new cmdOption(mc_JumpReserve, null, cmdExeType.SingleItem));
            cmdList.Add(EpgCmds.JumpTuner, new cmdOption(mc_JumpTuner, null, cmdExeType.SingleItem));
            cmdList.Add(EpgCmds.JumpTable, new cmdOption(mc_JumpTable, null, cmdExeType.SingleItem));
            cmdList.Add(EpgCmds.ShowAutoAddDialog, new cmdOption(mc_ShowAutoAddDialog, null, cmdExeType.SingleItem, changeDB: true));
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

            SetCmdMessage();
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
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
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
                        if (Settings.Instance.DisplayStatus == true && Settings.Instance.DisplayStatusNotify == true &&
                            e != null && e.Command != null)
                        {
                            CommonManager.Instance.StatusNotifySet(IsCommandExecuted, GetCmdMessage(e.Command), this.Owner);
                        }
                    }
                }
                catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
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
        protected virtual void mc_ChgResMode(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ChgBulkRecSet(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ChgGenre(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ChangeKeyEnabled(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ChangeOnOffKeyEnabled(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_Delete(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_Delete2(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_AdjustReserve(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ShowDialog(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ShowAddDialog(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_JumpReserve(object sender, ExecutedRoutedEventArgs e)
        {
            mcs_JumpTab(CtxmCode.ReserveView, true);
        }
        protected virtual void mc_JumpTuner(object sender, ExecutedRoutedEventArgs e)
        {
            mcs_JumpTab(CtxmCode.TunerReserveView, true, Settings.Instance.TunerDisplayOffReserve == false);
        }
        protected virtual void mc_JumpTable(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mcs_JumpTab(CtxmCode code, bool reserveOnly = false, bool onReserveOnly = false)
        {
            ReserveData resinfo = mcs_GetNextReserve();
            reserveOnly |= onReserveOnly;
            if (reserveOnly && resinfo == null) return;
            if (onReserveOnly && resinfo.IsEnabled == false) return;

            mcs_SetBlackoutWindow(new ReserveItem(resinfo));
            mainWindow.moveTo_tabItem(code);
            IsCommandExecuted = true;
        }
        protected virtual void mcs_SetBlackoutWindow(SearchItem item = null)
        {
            BlackoutWindow.SelectedData = item;
        }
        protected virtual ReserveData mcs_GetNextReserve() { return new ReserveData(); }
        protected virtual void mc_ShowAutoAddDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == MenuUtil.OpenChangeAutoAddDialog(CmdExeUtil.ReadObjData(e) as Type, (uint)CmdExeUtil.ReadIdData(e), this.Owner);
        }
        protected virtual void mc_ToAutoadd(object sender, ExecutedRoutedEventArgs e)
        {
            MenuUtil.SendAutoAdd(dataList[0] as IBasicPgInfo, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected virtual void mc_Play(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_OpenFolder(object sender, ExecutedRoutedEventArgs e)
        {
            CommonManager.Instance.OpenFolder(CmdExeUtil.ReadObjData(e) as string, "録画フォルダを開く");
        }
        protected virtual void mc_CopyTitle(object sender, ExecutedRoutedEventArgs e)
        {
            MenuUtil.CopyTitle2Clipboard(dataList[0].DataTitle, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected virtual void mc_CopyContent(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_SearchTitle(object sender, ExecutedRoutedEventArgs e)
        {
            MenuUtil.SearchTextWeb(dataList[0].DataTitle, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected virtual void mc_CopyNotKey(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_SetNotKey(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_ProtectChange(object sender, ExecutedRoutedEventArgs e) { }
        protected virtual void mc_MenuSetting(object sender, ExecutedRoutedEventArgs e)
        {
            var dlg = new SetContextMenuWindow();
            dlg.Owner = CommonUtil.GetTopWindow(Owner);
            dlg.info = Settings.Instance.MenuSet.Clone();

            if (dlg.ShowDialog() == true)
            {
                Settings.Instance.MenuSet = dlg.info.Clone();
                Settings.SaveToXmlFile();//メニュー設定の保存
                mainWindow.RefreshMenu();
            }
        }
        protected bool mcc_chgRecSetting(ExecutedRoutedEventArgs e)
        {
            List<RecSettingData> infoList = dataList.OfType<IRecSetttingData>().RecSettingList();

            if (e.Command == EpgCmds.ChgOnPreset)
            {
                return MenuUtil.ChangeOnPreset(infoList, (uint)CmdExeUtil.ReadIdData(e, 0, 0xFE));
            }
            else if (e.Command == EpgCmds.ChgRecmode)
            {
                return MenuUtil.ChangeRecmode(infoList, (byte)CmdExeUtil.ReadIdData(e, 0, 5));
            }
            else if (e.Command == EpgCmds.ChgPriority)
            {
                return MenuUtil.ChangePriority(infoList, (byte)CmdExeUtil.ReadIdData(e, 1, 5));
            }
            else if (e.Command == EpgCmds.ChgRelay)
            {
                return MenuUtil.ChangeRelay(infoList, (byte)CmdExeUtil.ReadIdData(e, 0, 1));
            }
            else if (e.Command == EpgCmds.ChgPittari)
            {
                return MenuUtil.ChangePittari(infoList, (byte)CmdExeUtil.ReadIdData(e, 0, 1));
            }
            else if (e.Command == EpgCmds.ChgTuner)
            {
                return MenuUtil.ChangeTuner(infoList, (uint)CmdExeUtil.ReadIdData(e, 0, int.MaxValue - 1));
            }
            else if (e.Command == EpgCmds.ChgMarginStart)
            {
                return MenuUtil.ChangeMargin(infoList, CmdExeUtil.ReadIdData(e), true);
            }
            else if (e.Command == EpgCmds.ChgMarginStartValue)
            {
                return MenuUtil.ChangeMarginValue(infoList, true, this.Owner);
            }
            else if (e.Command == EpgCmds.ChgMarginEnd)
            {
                return MenuUtil.ChangeMargin(infoList, CmdExeUtil.ReadIdData(e), false);
            }
            else if (e.Command == EpgCmds.ChgMarginEndValue)
            {
                return MenuUtil.ChangeMarginValue(infoList, false, this.Owner);
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

                if (ctxm.PlacementTarget is ListBox && e != null)
                {
                    //リストビューの場合は、アイテムの無いところではデータ選択してないものと見なす。
                    if ((ctxm.PlacementTarget as ListBox).GetPlacementItem() == null)
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
        protected virtual void mcs_jumpTabMenuOpening(MenuItem menu, string offres_tooltip = null)
        {
            //メニュー実行時に選択されるアイテムが予約でないとき、または予約が無いときは無効
            ReserveData resinfo = mcs_GetNextReserve();
            menu.IsEnabled = (resinfo != null);
            menu.ToolTip = null;
            if (resinfo == null) return;

            if (resinfo.IsEnabled == false)
            {
                menu.ToolTip = offres_tooltip;

                if (menu.Tag == EpgCmds.JumpTuner && Settings.Instance.TunerDisplayOffReserve == false)
                {
                    //無効予約を回避
                    menu.IsEnabled = false;
                    menu.ToolTip = "無効予約は使用予定チューナー画面に表示されない設定になっています。";
                }
            }
        }
        protected void mcs_chgMenuOpening(MenuItem menu)
        {
            if (menu.IsEnabled == false) return;

            var listr = dataList.OfType<IRecSetttingData>().ToList();
            List<RecSettingData> recSettings = listr.RecSettingList();
            var view = (menu.CommandParameter as EpgCmdParam).Code;

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
                if (subMenu.Tag == EpgCmdsEx.ChgKeyEnabledMenu)
                {
                    if (typeof(T).IsSubclassOf(typeof(AutoAddData)) == false)
                    {
                        subMenu.Visibility = Visibility.Collapsed;
                        continue;
                    }
                    subMenu.Visibility = Visibility.Visible;

                    var list = dataList.OfType<AutoAddData>().ToList();
                    bool? value = list.All(info => info.IsEnabled == list[0].IsEnabled) ? (bool?)list[0].IsEnabled : null;
                    subMenu.Header = string.Format("自動登録有効 : {0}", value == null ? "*" : (value == true ? "有効" : "無効"));
                    SetCheckmarkSubMenus(subMenu, value == true ? 0 : value == false ? 1 : int.MinValue);
                }
                else if (subMenu.Tag == EpgCmdsEx.ChgOnPresetMenu)
                {
                    mm.CtxmGenerateChgOnPresetItems(subMenu);

                    RecPresetItem pre_0 = listr[0].RecSettingInfo.LookUpPreset(listr[0].IsManual);
                    RecPresetItem value = listr.All(data => data.RecSettingInfo.LookUpPreset(data.IsManual).ID == pre_0.ID) ? pre_0 : null;
                    subMenu.Header = string.Format("プリセット : {0}", value == null ? "*" : value.DisplayName);
                    SetCheckmarkSubMenus(subMenu, value == null ? int.MinValue : (int)value.ID);
                }
                else if (subMenu.Tag == EpgCmdsEx.ChgResModeMenu)
                {
                    mm.CtxmGenerateChgResModeAutoAddItems(subMenu, itemCount == 1 ? dataList[0] as ReserveData : null);

                    if (typeof(T) != typeof(ReserveData))
                    {
                        subMenu.Visibility = Visibility.Collapsed;
                        continue;
                    }
                    subMenu.Visibility = Visibility.Visible;

                    var list = dataList.OfType<ReserveData>().ToList();
                    ReserveMode? resMode_0 = list[0].ReserveMode;
                    ReserveMode? value = list.All(info => info.ReserveMode == resMode_0) ? resMode_0 : null;
                    subMenu.Header = string.Format("予約モード : {0}", value == null ? "*" : CommonManager.ConvertResModeText(value));
                    SetCheckmarkSubMenus(subMenu, value == ReserveMode.EPG ? 0 : value == ReserveMode.Program ? 1 : int.MinValue);

                    if (list[0].IsAutoAdded == false) continue;

                    foreach (var item in subMenu.Items.OfType<MenuItem>())
                    {
                        Type type = (item.CommandParameter as EpgCmdParam).Data as Type;
                        int id = (item.CommandParameter as EpgCmdParam).ID;
                        AutoAddData autoAdd = AutoAddData.AutoAddList(type, (uint)id);
                        if (autoAdd != null)
                        {
                            item.IsChecked = autoAdd.GetReserveList().Any(info => info.ReserveID == list[0].ReserveID);
                        }
                    }
                }
                else if (subMenu.Tag == EpgCmds.ChgBulkRecSet)
                {
                    subMenu.Visibility = (recSettings.Count < 2 ? Visibility.Collapsed : Visibility.Visible);
                }
                else if (subMenu.Tag == EpgCmds.ChgGenre)
                {
                    if (typeof(T) != typeof(EpgAutoAddData))
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
                    if (typeof(T) != typeof(ReserveData) && typeof(T) != typeof(EpgAutoAddData))
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
                    subMenu.Header = string.Format(format, value == byte.MaxValue ? "*" : CommonManager.Instance.YesNoDictionary[value].DisplayName);
                    SetCheckmarkSubMenus(subMenu, value);
                    subMenu.IsEnabled = listr.Any(info => info.IsManual == false);
                    subMenu.ToolTip = (subMenu.IsEnabled != true ? "プログラム予約は対象外" : null);
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
                    int value = recSettings.All(info => info.GetTrueMargin(true) == recSettings[0].GetTrueMargin(true)) ? recSettings[0].GetTrueMargin(true) : int.MaxValue;
                    subMenu.Header = string.Format("開始マージン : {0} 秒", value == int.MaxValue ? "*" : value.ToString());
                }
                else if (subMenu.Tag == EpgCmdsEx.ChgMarginEndMenu)
                {
                    int value = recSettings.All(info => info.GetTrueMargin(false) == recSettings[0].GetTrueMargin(false)) ? recSettings[0].GetTrueMargin(false) : int.MaxValue;
                    subMenu.Header = string.Format("終了マージン : {0} 秒", value == int.MaxValue ? "*" : value.ToString());
                }
            }
        }

        protected virtual string GetCmdMessage(ICommand icmd)
        {
            string cmdMsg = null;
            cmdMessage.TryGetValue(icmd, out cmdMsg);
            return GetCmdMessageFormat(cmdMsg, this.itemCount);
        }
        protected string GetCmdMessageFormat(string cmdMsg, int Count)
        {
            if (string.IsNullOrEmpty(cmdMsg) == true) return null;
            return string.Format("{0}(処理数:{1})", cmdMsg, Count);
        }
        protected virtual void SetCmdMessage()
        {
            cmdMessage.Add(EpgCmds.Add, "予約を追加");
            //cmdMessage.Add(EpgCmds.ShowAddDialog, "");
            cmdMessage.Add(EpgCmds.AddOnPreset, "指定プリセットで予約を追加");
            cmdMessage.Add(EpgCmds.ChgOnOff, "簡易予約/有効・無効切替を実行");
            cmdMessage.Add(EpgCmds.ChgOnPreset, "録画プリセットを変更");
            cmdMessage.Add(EpgCmds.ChgResMode, "予約モードを変更");
            cmdMessage.Add(EpgCmds.ChgBulkRecSet, "録画設定を変更");
            cmdMessage.Add(EpgCmds.ChgGenre, "ジャンル絞り込みを変更");
            cmdMessage.Add(EpgCmds.ChgRecmode, "録画モードを変更");
            cmdMessage.Add(EpgCmds.ChgPriority, "優先度を変更");
            cmdMessage.Add(EpgCmds.ChgRelay, "イベントリレー追従設定を変更");
            cmdMessage.Add(EpgCmds.ChgPittari, "ぴったり録画設定を変更");
            cmdMessage.Add(EpgCmds.ChgTuner, "チューナー指定を変更");
            cmdMessage.Add(EpgCmds.ChgMarginStart, "録画マージンを変更");
            cmdMessage.Add(EpgCmds.ChgMarginStartValue, "録画マージンを変更");
            cmdMessage.Add(EpgCmds.ChgMarginEnd, "録画マージンを変更");
            cmdMessage.Add(EpgCmds.ChgMarginEndValue, "録画マージンを変更");
            cmdMessage.Add(EpgCmds.ChgKeyEnabled, "有効/無効を変更");
            cmdMessage.Add(EpgCmds.ChgOnOffKeyEnabled, "有効/無効切替を実行");
            cmdMessage.Add(EpgCmds.Delete, "削除を実行");
            cmdMessage.Add(EpgCmds.Delete2, "予約ごと削除を実行");
            cmdMessage.Add(EpgCmds.DeleteAll, "全て削除を実行");
            cmdMessage.Add(EpgCmds.AdjustReserve, "自動予約登録に予約を合わせる");
            //cmdMessage.Add(EpgCmds.ShowDialog, "");
            //cmdMessage.Add(EpgCmds.JumpReserve, "予約一覧へジャンプ");
            //cmdMessage.Add(EpgCmds.JumpTuner, "チューナー画面へジャンプ");
            //cmdMessage.Add(EpgCmds.JumpTable, "番組表へジャンプ");
            //cmdMessage.Add(EpgCmds.ShowAutoAddDialog, "");
            //cmdMessage.Add(EpgCmds.ToAutoadd, "");
            //cmdMessage.Add(EpgCmds.ReSearch, "再検索を実行");
            //cmdMessage.Add(EpgCmds.ReSearch2, "再検索を実行");
            //cmdMessage.Add(EpgCmds.Play, "再生/追いかけ再生を実行");
            //cmdMessage.Add(EpgCmds.OpenFolder, "フォルダーを開く");
            //cmdMessage.Add(EpgCmds.CopyTitle, "名前をコピー");
            //cmdMessage.Add(EpgCmds.CopyContent, "番組情報をコピー");
            //cmdMessage.Add(EpgCmds.SearchTitle, "名前をネットで検索");
            //cmdMessage.Add(EpgCmds.CopyNotKey, "Notキーをコピー");
            //cmdMessage.Add(EpgCmds.SetNotKey, "Notキーに貼り付け");
            cmdMessage.Add(EpgCmds.ProtectChange, "プロテクト切替を実行");
            //cmdMessage.Add(EpgCmds.ViewChgSet, "");
            //cmdMessage.Add(EpgCmds.ViewChgMode, "");
            //cmdMessage.Add(EpgCmds.MenuSetting, "");

            //cmdMessage.Add(EpgCmds.AddInDialog, "追加を実行");
            //cmdMessage.Add(EpgCmds.ChangeInDialog, "変更を実行");
            //cmdMessage.Add(EpgCmds.DeleteInDialog, "削除を実行");
            //cmdMessage.Add(EpgCmds.Delete2InDialog, "全て削除を実行");
            //cmdMessage.Add(EpgCmds.Search, "");
            //cmdMessage.Add(EpgCmds.TopItem, "");
            //cmdMessage.Add(EpgCmds.UpItem, "");
            //cmdMessage.Add(EpgCmds.DownItem, "");
            //cmdMessage.Add(EpgCmds.BottomItem, "");
            //cmdMessage.Add(EpgCmds.SaveOrder, "");
            //cmdMessage.Add(EpgCmds.RestoreOrder, "");
            //cmdMessage.Add(EpgCmds.DragCancel, "");
            //cmdMessage.Add(EpgCmds.Cancel, "");
            //cmdMessage.Add(EpgCmds.ChgOnOffCheck, "");//一覧画面のチェックボックス用
        }
    }

    public class CmdExeUtil
    {
        public static bool CheckAllDeleteCancel(ExecutedRoutedEventArgs e, int Count)
        {
            if (CmdExeUtil.IsMessageBeforeCommand(e) == false) return false;

            return (MessageBox.Show(string.Format(
                "全て削除しますか?\r\n" + "[削除項目数: {0}]", Count)
                , "[全削除]の確認", MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK);
        }
        public static bool CheckKeyboardDeleteCancel(ExecutedRoutedEventArgs e, List<string> list)
        {
            if (IsDisplayKgMessage(e) == false) return false;
            if (list == null || list.Count == 0) return false;

            return (MessageBox.Show(
                string.Format("削除しますか?\r\n\r\n" + "[削除項目数: {0}]\r\n\r\n", list.Count) + FormatTitleListForDialog(list)
                , "削除の確認", MessageBoxButton.OKCancel,
                MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK);
        }
        public static bool CheckAllProcCancel(ExecutedRoutedEventArgs e, IEnumerable<AutoAddData> dataList, bool IsDelete)
        {
            if (CmdExeUtil.IsMessageBeforeCommand(e) == false) return false;

            List<string> titleList = dataList.Select(info => info.DataTitle).ToList();
            if (titleList.Count == 0) return false;

            var s = IsDelete == true
                ? new string[] { "予約ごと削除して", "削除", "削除される予約数", "予約ごと削除" }
                : new string[] { "予約の録画設定を自動登録の録画設定に合わせても", "処理", "対象予約数", "予約の録画設定変更" };

            var text = string.Format("{0}よろしいですか?\r\n"
                                        + "(個別予約も処理の対象となります。)\r\n\r\n"
                                        + "[{1}項目数: {2}]\r\n"
                                        + "[{3}: {4}]\r\n\r\n", s[0], s[1], titleList.Count, s[2], dataList.GetReserveList().Count)
                + CmdExeUtil.FormatTitleListForDialog(titleList);

            return (MessageBox.Show(text, "[" + s[3] + "]の確認", MessageBoxButton.OKCancel,
                                MessageBoxImage.Exclamation, MessageBoxResult.OK) != MessageBoxResult.OK);
        }
        public static string FormatTitleListForDialog(ICollection<string> list)
        {
            int DisplayNum = MenuSettingData.CautionDisplayItemNum;
            var text = new StringBuilder();
            foreach (var info in list.Take(DisplayNum)) { text.AppendFormat(" ・ {0}\r\n", info); }
            if (list.Count > DisplayNum) text.AppendFormat("\r\n　　ほか {0} 項目", list.Count - DisplayNum);
            return text.ToString();
        }
        public static bool IsMessageBeforeCommand(ExecutedRoutedEventArgs e)
        {
            if (HasCommandParameter(e) == false) return false;
            //コマンド側のオプションに変更可能なようにまとめておく
            bool NoMessage = false;
            if (e.Command == EpgCmds.DeleteAll) NoMessage = Settings.Instance.MenuSet.NoMessageDeleteAll;
            else if (e.Command == EpgCmds.Delete2) NoMessage = Settings.Instance.MenuSet.NoMessageDelete2;
            else if (e.Command == EpgCmds.SetNotKey) NoMessage = Settings.Instance.MenuSet.NoMessageNotKEY;
            else if (e.Command == EpgCmds.AdjustReserve) NoMessage = Settings.Instance.MenuSet.NoMessageAdjustRes;

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
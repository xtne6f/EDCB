﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
    
namespace EpgTimer
{
    public class CmdExeReserve : CmdExe<ReserveData>
    {
        protected Func<bool, List<SearchItem>> _getSearchList = null;
        protected Func<List<EpgEventInfo>> _getEpgEventList = null;
        protected Func<bool, SearchItem> _selectSingleSearchData = null;

        public void SetFuncGetSearchList(Func<bool, List<SearchItem>> f) { _getSearchList = f; }
        public void SetFuncGetEpgEventList(Func<List<EpgEventInfo>> f) { _getEpgEventList = f; }
        public void SetFuncSelectSingleSearchData(Func<bool, SearchItem> f) { _selectSingleSearchData = f; }

        public byte EpgInfoOpenMode { get; set; }
        public RecSettingView recSettingView { get; set; }

        protected override int ItemCount
        {
            get { return (dataList == null ? 0 : dataList.Count) + (eventListEx == null ? 0 : eventListEx.Count); }
        }
        protected IAutoAddTargetData headData = null;//メニューオープン時に使用
        protected IAutoAddTargetData headDataEv = null;//番組情報優先先頭データ。headDataは予約情報優先。
        protected List<EpgEventInfo> eventList = new List<EpgEventInfo>();
        protected List<EpgEventInfo> eventListEx = new List<EpgEventInfo>();//reserveData(dataList)とかぶらないもの

        public CmdExeReserve(UIElement owner)
            : base(owner)
        {
            _copyItemData = ReserveDataEx.CopyTo;
        }
        protected override void SetData(bool IsAllData = false  )
        {
            base.SetData(IsAllData);
            if (_getSearchList != null)//SearchItemリストがある場合
            {
                List<SearchItem> searchList = _getSearchList(IsAllData);
                searchList = searchList == null ? new List<SearchItem>() : searchList.OfType<SearchItem>().ToList();//無くても大丈夫なはずだが一応
                OrderAdjust<SearchItem>(searchList, _selectSingleSearchData);
                dataList = searchList.GetReserveList();
                eventList = searchList.GetEventList();
                eventListEx = searchList.GetNoReserveList();
                headData = searchList.Count == 0 ? null : searchList[0].IsReserved == true ? searchList[0].ReserveInfo as IAutoAddTargetData : searchList[0].EventInfo;
                headDataEv = searchList.Count == 0 ? null : searchList[0].EventInfo;
            }
            else
            {
                eventList = _getEpgEventList == null ? null : _getEpgEventList();
                eventList = eventList == null ? new List<EpgEventInfo>() : eventList.OfType<EpgEventInfo>().ToList();
                eventListEx = new List<EpgEventInfo>();
                eventList.ForEach(epg => 
                {
                    if (dataList.All(res => epg.CurrentPgUID() != res.CurrentPgUID()))
                    {
                        eventListEx.Add(epg);
                    }
                });
                headData = dataList.Count != 0 ? dataList[0] as IAutoAddTargetData : eventList.Count != 0 ? eventList[0] : null;
                headDataEv = eventList.Count != 0 ? eventList[0] as IAutoAddTargetData : dataList.Count != 0 ? dataList[0] : null;
            }
        }
        protected override void ClearData()
        {
            base.ClearData();
            headData = null;
            headDataEv = null;
            eventList.Clear();
            eventListEx.Clear();
        }
        protected override void SelectSingleData()
        {
            base.SelectSingleData();
            if (_selectSingleSearchData != null) _selectSingleSearchData(false);
        }
        //以下個別コマンド対応
        protected override void mc_Add(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = MenuUtil.ReserveAdd(eventListEx, this.recSettingView, 0);
        }
        protected override void mc_AddOnPreset(object sender, ExecutedRoutedEventArgs e)
        {
            uint presetID = (uint)CmdExeUtil.ReadIdData(e, 0, 0xFE);
            IsCommandExecuted = MenuUtil.ReserveAdd(eventListEx, null, presetID);
        }
        protected override void mc_ShowDialog(object sender, ExecutedRoutedEventArgs e)
        {
            if (dataList.Count != 0)//予約情報優先
            {
                IsCommandExecuted = true == MenuUtil.OpenChangeReserveDialog(dataList[0], this.Owner, EpgInfoOpenMode);
            }
            else if (eventListEx.Count != 0)
            {
                IsCommandExecuted = true == MenuUtil.OpenEpgReserveDialog(eventListEx[0], this.Owner, EpgInfoOpenMode);
            }
        }
        protected override void mc_ShowAddDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == MenuUtil.OpenManualReserveDialog(Owner);
        }
        protected override void mc_ChangeOnOff(object sender, ExecutedRoutedEventArgs e)
        {
            //多数アイテム処理の警告。合計数に対して出すので、結構扱いづらい。
            if (MenuUtil.CautionManyMessage(this.ItemCount, "簡易予約/有効←→無効") == false) return;

            bool ret1 = MenuUtil.ReserveChangeOnOff(dataList, this.recSettingView, false);
            var eList = dataList.Count == 0 ? eventListEx : eventListEx.FindAll(data => data.IsReservable == true);
            bool ret2 = MenuUtil.ReserveAdd(eList, this.recSettingView, 0, false);
            IsCommandExecuted = !(ret1 == false && ret2 == false || dataList.Count == 0 && ret2 == false || eventListEx.Count == 0 && ret1 == false);
        }
        protected override void mc_ChangeRecSetting(object sender, ExecutedRoutedEventArgs e)
        {
            if (mcc_chgRecSetting(e) == false) return;
            IsCommandExecuted = MenuUtil.ReserveChange(dataList);
        }
        protected override void mc_ChgResMode(object sender, ExecutedRoutedEventArgs e)
        {
            if (dataList.Count == 0) return;

            var data = CmdExeUtil.ReadObjData(e) as Type;
            uint id = (uint)CmdExeUtil.ReadIdData(e);

            if (data == null)
            {
                //通常の変更
                IsCommandExecuted = MenuUtil.ReserveChangeResMode(dataList, id);
            }

            if (dataList.Count != 1) return;//通常はここに引っかかることは無いはず

            AutoAddData autoAdd = AutoAddData.AutoAddList(data, id);
            if (autoAdd != null)
            {
                IsCommandExecuted = MenuUtil.ReserveChangeResModeAutoAdded(dataList, autoAdd);
            }
        }
        protected override void mc_ChgBulkRecSet(object sender, ExecutedRoutedEventArgs e)
        {
            var mList = dataList.FindAll(info => info.IsEpgReserve == false);
            if (MenuUtil.ChangeBulkSet(dataList.RecSettingList(), this.Owner, mList.Count == dataList.Count) == false) return;
            IsCommandExecuted = MenuUtil.ReserveChange(dataList);
        }
        protected override void mc_Delete(object sender, ExecutedRoutedEventArgs e)
        {
            if (mcs_DeleteCheck(e) == false) return;
            IsCommandExecuted = MenuUtil.ReserveDelete(dataList);
        }
        protected override SearchItem mcs_GetSearchItem()
        {
            if (dataList.Count != 0)//予約情報優先
            {
                return new ReserveItem(dataList[0]);
            }
            else if (eventList.Count != 0)
            {
                return new SearchItem(eventList[0]);
            }
            return null;
        }
        protected override ReserveData mcs_GetNextReserve()
        {
            return headData as ReserveData;
        }
        protected override void mc_ToAutoadd(object sender, ExecutedRoutedEventArgs e)
        {
            ReserveData resData = null;
            if (eventList.Count != 0)
            {
                resData = CtrlCmdDefEx.ConvertEpgToReserveData(eventList[0]);
                if (dataList.Count != 0)
                {
                    resData.RecSetting = dataList[0].RecSetting.Clone();
                }
                else
                {
                    resData.RecSetting = Settings.Instance.RecPresetList[0].RecPresetData.Clone();
                }
            }
            else if (dataList.Count != 0)
            {
                resData = dataList[0];
            }
            MenuUtil.SendAutoAdd(resData, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected override void mc_Play(object sender, ExecutedRoutedEventArgs e)
        {
            if (dataList.Count != 0)
            {
                CommonManager.Instance.FilePlay(dataList[0]);
                IsCommandExecuted = true;
            }
        }
        protected override void mc_CopyTitle(object sender, ExecutedRoutedEventArgs e)
        {
            //番組情報優先
            MenuUtil.CopyTitle2Clipboard(headDataEv.DataTitle, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true; //itemCount!=0 だが、この条件はこの位置では常に満たされている。
        }
        protected override void mc_CopyContent(object sender, ExecutedRoutedEventArgs e)
        {
            if (eventList.Count != 0)//番組情報優先
            {
                MenuUtil.CopyContent2Clipboard(eventList[0], CmdExeUtil.IsKeyGesture(e));
            }
            else if (dataList.Count != 0)
            {
                MenuUtil.CopyContent2Clipboard(dataList[0], CmdExeUtil.IsKeyGesture(e));
            }
            IsCommandExecuted = true;
        }
        protected override void mc_InfoSearchTitle(object sender, ExecutedRoutedEventArgs e)
        {
            //番組情報優先
            IsCommandExecuted = true == MenuUtil.OpenInfoSearchDialog(headDataEv.DataTitle, CmdExeUtil.IsKeyGesture(e));
        }
        protected override void mc_SearchTitle(object sender, ExecutedRoutedEventArgs e)
        {
            //番組情報優先
            MenuUtil.SearchTextWeb(headDataEv.DataTitle, CmdExeUtil.IsKeyGesture(e));
            IsCommandExecuted = true;
        }
        protected override void mcs_ctxmLoading_switch(ContextMenu ctxm, MenuItem menu)
        {
            var view = (menu.CommandParameter as EpgCmdParam).Code;

            //有効無効制御の追加分。予約データが無ければ無効
            new List<ICommand> { EpgCmdsEx.ChgMenu, EpgCmds.Delete, EpgCmds.DeleteAll, EpgCmds.Play }.ForEach(icmd =>
            {
                if (menu.Tag == icmd) menu.IsEnabled = dataList.Count != 0;
            });

            var CheckReservableEpg = new Func<MenuItem, bool>(mi =>
            {
                if (eventListEx.Count != 0 && eventListEx.Count(data => data.IsReservable == true) == 0)
                {
                    mi.IsEnabled = false;
                    mi.ToolTip = "放映終了";
                }
                return mi.IsEnabled;
            });

            //switch使えないのでifで回す。
            if (menu.Tag == EpgCmds.ChgOnOff)
            {
                if (dataList.Count == 0)
                {
                    menu.Header = "簡易予約";
                    //予約データの有無で切り替える。

                    if (CheckReservableEpg(menu) == true)
                    {
                        if (view == CtxmCode.SearchWindow)
                        {
                            RecPresetItem preset = (this.Owner as SearchWindow).GetRecSetting().LookUpPreset();
                            string text = preset.IsCustom == true ? "カスタム設定" : string.Format("プリセット'{0}'", preset.DisplayName);
                            menu.ToolTip = string.Format("このダイアログの録画設定({0})で予約する", text);
                        }
                        else
                        {
                            menu.ToolTip = "プリセット'デフォルト'で予約する";
                        }
                    }
                }
                else
                {
                    menu.Header = "予約←→無効";
                    menu.ToolTip = null;
                    menu.Visibility = Visibility.Visible;
                    if (view == CtxmCode.TunerReserveView && Settings.Instance.MenuSet.IsManualAssign.Contains(view) == false)
                    {
                        //簡易メニュー時は、無効列非表示のとき表示しない。
                        if( Settings.Instance.TunerDisplayOffReserve == false)
                        {
                            menu.Visibility = Visibility.Collapsed;
                        }
                    }
                }
            }
            else if (menu.Tag == EpgCmdsEx.AddMenu)
            {
                if (CheckReservableEpg(menu) == true)
                {
                    menu.IsEnabled = eventListEx.Count != 0;//未予約アイテムがあれば有効
                    mm.CtxmGenerateAddOnPresetItems(menu);
                }
            }
            else if (menu.Tag == EpgCmdsEx.ChgMenu)
            {
                mcs_chgMenuOpening(menu);
            }
            else if (menu.Tag == EpgCmds.JumpReserve || menu.Tag == EpgCmds.JumpTuner)
            {
                mcs_jumpTabMenuOpening(menu);
            }
            else if (menu.Tag == EpgCmds.JumpTable)
            {
                if (view != CtxmCode.EpgView) return;

                //標準モードでは非表示。
                if ((int)ctxm.Tag == 0)
                {
                    menu.Visibility = Visibility.Collapsed;
                }
            }
            else if (menu.Tag == EpgCmdsEx.ShowAutoAddDialogMenu)
            {
                menu.IsEnabled = mm.CtxmGenerateChgAutoAdd(menu, headData);
            }
            else if (menu.Tag == EpgCmds.Play)
            {
                menu.IsEnabled = false;
                menu.ToolTip = null;
                var info = headData as ReserveData;
                if (info != null && info.IsEnabled == true)
                {
                    if (info.IsOnRec() == true)
                    {
                        menu.IsEnabled = true;
                    }
                    else
                    {
                        menu.ToolTip = "まだ録画が開始されていません。";
                    }
                }
            }
            else if (menu.Tag == EpgCmdsEx.OpenFolderMenu)
            {
                mm.CtxmGenerateOpenFolderItems(menu, headData as ReserveData == null ? null : dataList[0].RecSetting);
            }
            else if (menu.Tag == EpgCmdsEx.ViewMenu)
            {
                foreach (var item in menu.Items.OfType<MenuItem>().Where(item => item.Tag == EpgCmds.ViewChgMode))
                {
                    item.IsChecked = ((item.CommandParameter as EpgCmdParam).ID == (int)ctxm.Tag);
                }
            }
        }
        protected override string GetCmdMessage(ICommand icmd)
        {
            if (icmd != EpgCmds.Add && icmd != EpgCmds.AddOnPreset && icmd != EpgCmds.ChgOnOff)
            {
                return base.GetCmdMessage(icmd);
            }

            string cmdMsg = cmdMessage[icmd];
            if (icmd == EpgCmds.Add && eventListEx.Count == 0)
            {
                return null;
            }
            if (icmd == EpgCmds.ChgOnOff)
            {
                if (eventListEx.Count == 0) cmdMsg = "有効・無効切替を実行";
                else if (dataList.Count == 0) cmdMsg = "簡易予約を実行";
            }
            return GetCmdMessageFormat(cmdMsg, this.ItemCount);
        }
    }
}

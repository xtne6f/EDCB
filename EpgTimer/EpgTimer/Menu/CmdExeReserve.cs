using System;
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

        protected object headData = null;//メニューオープン時に使用
        protected List<EpgEventInfo> eventList = new List<EpgEventInfo>();
        protected List<EpgEventInfo> eventListEx = new List<EpgEventInfo>();//reserveData(dataList)とかぶらないもの

        public CmdExeReserve(Control owner)
            : base(owner)
        {
            _copyItemData = CtrlCmdDefEx.CopyTo;
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
                headData = searchList.Count == 0 ? null : searchList[0].IsReserved == true ? searchList[0].ReserveInfo as object : searchList[0].EventInfo as object;
            }
            else
            {
                eventList = _getEpgEventList == null ? null : _getEpgEventList();
                eventList = eventList == null ? new List<EpgEventInfo>() : eventList.OfType<EpgEventInfo>().ToList();
                eventListEx = new List<EpgEventInfo>();
                eventList.ForEach(epg => 
                {
                    if (dataList.All(res => CommonManager.EqualsPg(epg, res) == false))
                    {
                        eventListEx.Add(epg);
                    }
                });
                headData = dataList.Count != 0 ? dataList[0] as object : eventList.Count != 0 ? eventList[0] as object : null;
            }
        }
        protected override void ClearData()
        {
            base.ClearData();
            headData = null;
            eventList.Clear();
            eventListEx.Clear();
        }
        protected override int itemCount
        { 
            get 
            {
                return (dataList == null ? 0 : dataList.Count) + (eventListEx == null ? 0 : eventListEx.Count); 
            } 
        }
        protected override void SelectSingleData()
        {
            base.SelectSingleData();
            if (_selectSingleSearchData != null) _selectSingleSearchData(false);
        }
        //以下個別コマンド対応
        protected override void mc_Add(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = mutil.ReserveAdd(eventListEx, this.recSettingView, 0);
        }
        protected override void mc_AddOnPreset(object sender, ExecutedRoutedEventArgs e)
        {
            uint presetID = (uint)CmdExeUtil.ReadIdData(e, 0, 0xFE);
            IsCommandExecuted = mutil.ReserveAdd(eventListEx, null, presetID);
        }
        protected override void mc_ShowDialog(object sender, ExecutedRoutedEventArgs e)
        {
            if (dataList.Count != 0)//予約情報優先
            {
                IsCommandExecuted = true == mutil.OpenChangeReserveDialog(dataList[0], this.Owner, EpgInfoOpenMode);
            }
            else if (eventListEx.Count != 0)
            {
                IsCommandExecuted = true == mutil.OpenEpgReserveDialog(eventListEx[0], this.Owner, EpgInfoOpenMode);
            }
        }
        protected override void mc_ShowAddDialog(object sender, ExecutedRoutedEventArgs e)
        {
            IsCommandExecuted = true == mutil.OpenManualReserveDialog(Owner);
        }
        protected override void mc_ChangeOnOff(object sender, ExecutedRoutedEventArgs e)
        {
            //多数アイテム処理の警告。合計数に対して出すので、結構扱いづらい。
            if (mutil.CautionManyMessage(this.itemCount, "簡易予約/有効←→無効") == false) return;

            if (mutil.ReserveChangeOnOff(dataList, this.recSettingView, false) == false) return;
            IsCommandExecuted = mutil.ReserveAdd(eventListEx, this.recSettingView, 0, false);
        }
        protected override void mc_ChangeRecSetting(object sender, ExecutedRoutedEventArgs e)
        {
            if (mcc_chgRecSetting(dataList.RecSettingList(), e, this.Owner) == false) return;
            IsCommandExecuted = mutil.ReserveChange(dataList);
        }
        protected override void mc_ChgBulkRecSet(object sender, ExecutedRoutedEventArgs e)
        {
            var mList = dataList.Where(info => info.EventID == 0xFFFF).ToList();
            if (mutil.ChangeBulkSet(dataList.RecSettingList(), this.Owner, mList.Count == dataList.Count) == false) return;
            IsCommandExecuted = mutil.ReserveChange(dataList);
        }
        protected override void mc_Delete(object sender, ExecutedRoutedEventArgs e)
        {
            if (dataList.Count != 0)
            {
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
                IsCommandExecuted = mutil.ReserveDelete(dataList);
            }
        }
        protected override void mc_JumpReserve(object sender, ExecutedRoutedEventArgs e)
        {
            mcs_JumpTab(CtxmCode.ReserveView, true);
        }
        protected override void mc_JumpTuner(object sender, ExecutedRoutedEventArgs e)
        {
            mcs_JumpTab(CtxmCode.TunerReserveView, true, Settings.Instance.TunerDisplayOffReserve == false);
        }
        protected override void mc_JumpTable(object sender, ExecutedRoutedEventArgs e)
        {
            var param = e.Parameter as EpgCmdParam;
            if (param == null) return;

            if (param.Code == CtxmCode.EpgView)
            {
                param.ID = 0;//実際は設定するまでもなく、初期値0。
                BlackoutWindow.NowJumpTable = true;
                var mainWindow = Application.Current.MainWindow as MainWindow;
                new BlackoutWindow(mainWindow).showWindow(mainWindow.tabItem_epg.Header.ToString());

                EpgCmds.ViewChgMode.Execute(e.Parameter, (IInputElement)sender);
                IsCommandExecuted = true;
            }
            else
            {
                mcs_JumpTab(CtxmCode.EpgView);
            }
        }
        protected void mcs_JumpTab(CtxmCode code, bool reserveOnly = false, bool onReserveOnly = false)
        {
            if (reserveOnly && dataList.Count == 0) return;
            if (onReserveOnly && dataList[0].RecSetting.RecMode == 5) return;

            mcs_SetBlackoutWindow();
            var mainWindow = Application.Current.MainWindow as MainWindow;
            mainWindow.moveTo_tabItem(code);
            IsCommandExecuted = true;
        }
        protected void mcs_SetBlackoutWindow()
        {
            if (dataList.Count != 0)//予約情報優先
            {
                BlackoutWindow.SelectedItem = new ReserveItem(dataList[0]);
            }
            else if (eventList.Count != 0)
            {
                BlackoutWindow.SelectedItem = new SearchItem(eventList[0]);
            }
        }
        protected override void mc_ToAutoadd(object sender, ExecutedRoutedEventArgs e)
        {
            if (eventList.Count != 0)//番組情報優先
            {
                mutil.SendAutoAdd(eventList[0], this.Owner, CmdExeUtil.IsKeyGesture(e));
            }
            else if (dataList.Count != 0)
            {
                mutil.SendAutoAdd(dataList[0], this.Owner, CmdExeUtil.IsKeyGesture(e));
            }
            IsCommandExecuted = true;
        }
        protected override void mc_Play(object sender, ExecutedRoutedEventArgs e)
        {
            if (dataList.Count != 0)
            {
                if (CommonManager.Instance.NWMode == false && Settings.Instance.FilePlayOnAirWithExe == true
                    && dataList[0].RecSetting.RecMode != 4)//視聴モードは録画ファイル無いので対象外
                {
                    mutil.FilePlay(dataList[0]);
                }
                else
                {
                    CommonManager.Instance.TVTestCtrl.StartTimeShift(dataList[0].ReserveID);
                }
                IsCommandExecuted = true;
            }
        }
        protected override void mc_CopyTitle(object sender, ExecutedRoutedEventArgs e)
        {
            if (eventList.Count != 0)//番組情報優先
            {
                mutil.CopyTitle2Clipboard(eventList[0].Title(), CmdExeUtil.IsKeyGesture(e));
            }
            else if (dataList.Count != 0)
            {
                mutil.CopyTitle2Clipboard(dataList[0].Title, CmdExeUtil.IsKeyGesture(e));
            }
            IsCommandExecuted = true; //itemCount!=0 だが、この条件はこの位置では常に満たされている。
        }
        protected override void mc_CopyContent(object sender, ExecutedRoutedEventArgs e)
        {
            if (eventList.Count != 0)//番組情報優先
            {
                mutil.CopyContent2Clipboard(eventList[0], CmdExeUtil.IsKeyGesture(e));
            }
            else if (dataList.Count != 0)
            {
                mutil.CopyContent2Clipboard(dataList[0], CmdExeUtil.IsKeyGesture(e));
            }
            IsCommandExecuted = true;
        }
        protected override void mc_SearchTitle(object sender, ExecutedRoutedEventArgs e)
        {
            if (eventList.Count != 0)//番組情報優先
            {
                mutil.SearchText(eventList[0].Title(), CmdExeUtil.IsKeyGesture(e));
            }
            else if (dataList.Count != 0)
            {
                mutil.SearchText(dataList[0].Title, CmdExeUtil.IsKeyGesture(e));
            }
            IsCommandExecuted = true;
        }
        public void ViewChangeModeSupport()
        {
            var cmdPrm = new cmdOption((s, e) => mcs_SetBlackoutWindow(), null, cmdExeType.SingleItem, false, false);
            GetExecute(cmdPrm)(null, null);
        }
        protected override void mcs_ctxmLoading_switch(ContextMenu ctxm, MenuItem menu)
        {
            var view = (menu.CommandParameter as EpgCmdParam).Code;

            //有効無効制御の追加分。予約データが無ければ無効
            new List<ICommand> { EpgCmdsEx.ChgMenu, EpgCmds.Delete, EpgCmds.DeleteAll, EpgCmds.Play }.ForEach(icmd =>
            {
                if (menu.Tag == icmd) menu.IsEnabled = dataList.Count != 0;
            });

            //switch使えないのでifで回す。
            if (menu.Tag == EpgCmds.ChgOnOff)
            {
                if (dataList.Count == 0)
                {
                    menu.Header = "簡易予約";
                    //予約データの有無で切り替える。

                    if (view == CtxmCode.SearchWindow)
                    {
                        RecPresetItem preset = (this.Owner as SearchWindow).GetRecSetting().LookUpPreset();
                        string text = preset.ID == 0xFFFFFFFF ? "カスタム設定" : string.Format("プリセット'{0}'", preset.DisplayName);
                        menu.ToolTip = string.Format("このダイアログの録画設定({0})で予約する", text);
                    }
                    else
                    {
                        menu.ToolTip = "プリセット'デフォルト'で予約する";
                    }
                }
                else
                {
                    menu.Header = "予約←→無効";
                    menu.ToolTip = null;
                    menu.Visibility = Visibility.Visible;
                    if (view == CtxmCode.TunerReserveView && Settings.Instance.MenuSet.IsManualMenu == false)
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
                menu.IsEnabled = eventListEx.Count != 0;//未予約アイテムがあれば有効
                mm.CtxmGenerateAddOnPresetItems(menu);
            }
            else if (menu.Tag == EpgCmdsEx.ChgMenu)
            {
                List<int> mList = dataList.Select(info => info.EventID == 0xFFFF ? 1 : 0).ToList();
                mcs_chgMenuOpening(menu, dataList.RecSettingList(), mList.Sum() == dataList.Count, mList);
            }
            else if (menu.Tag == EpgCmds.JumpReserve || menu.Tag == EpgCmds.JumpTuner)
            {
                //先にサブウィンドウ関係をチェック
                if (mcs_ctxmLoading_CheckSearchSubWindow(menu) == true) return;

                //メニュー実行時に選択されるアイテムが予約でないときは無効
                menu.IsEnabled = (headData as ReserveData != null);
                menu.ToolTip = null;
 
                if (menu.Tag == EpgCmds.JumpTuner && Settings.Instance.TunerDisplayOffReserve == false && menu.IsEnabled == true)
                {
                    //無効予約を回避
                    menu.IsEnabled = dataList[0].RecSetting.RecMode != 5;
                    menu.ToolTip = "無効予約は使用予定チューナー画面に表示されない設定になっています。";
                }
            }
            else if (menu.Tag == EpgCmds.JumpTable)
            {
                if (mcs_ctxmLoading_CheckSearchSubWindow(menu) == true) return;
                if (view != CtxmCode.EpgView) return;

                //標準モードでは非表示。
                if ((int)ctxm.Tag == 0)
                {
                    menu.Visibility = Visibility.Collapsed;
                }
            }
            else if (menu.Tag == EpgCmds.ReSearch2)
            {
                mcs_ctxmLoading_CheckSearchSubWindow(menu);
            }
            else if (menu.Tag == EpgCmds.Play)
            {
                menu.IsEnabled = false;
                menu.ToolTip = null;
                var info = headData as ReserveData;
                if (info != null && info.RecSetting.RecMode != 5)
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
        protected bool mcs_ctxmLoading_CheckSearchSubWindow(MenuItem menu)
        {
            bool isHit = (this.Owner is SearchWindow) == true && ((SearchWindow)this.Owner).IsThisSubWindow == true;
            if (isHit == true)
            {
                menu.IsEnabled = false;
                menu.Header += menu.Header.ToString().EndsWith("(無効)") ? "" : "(無効)";
                menu.ToolTip = "サブウィンドウでは無効になります。";
            }
            return isHit;
        }
    }
}

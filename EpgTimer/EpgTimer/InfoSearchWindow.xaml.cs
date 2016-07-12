using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    using UserCtrlView;

    /// <summary>
    /// SearchWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class InfoSearchWindow : InfoSearchWindowBase
    {
        private ListViewController<InfoSearchItem> lstCtrl;
        private CmdExe<InfoSearchItem> mc;

        private bool startSearch = false;

        public InfoSearchWindow()
        {
            InitializeComponent();

            chkboxPinned = this.checkBox_windowPinned;

            try
            {
                //リストビュー関連の設定
                lstCtrl = new ListViewController<InfoSearchItem>(this);
                lstCtrl.SetSavePath(CommonUtil.NameOf(() => Settings.Instance.InfoSearchWndColumn)
                    , CommonUtil.NameOf(() => Settings.Instance.InfoSearchColumnHead)
                    , CommonUtil.NameOf(() => Settings.Instance.InfoSearchSortDirection));
                lstCtrl.SetViewSetting(listView_result, gridView_result, true, true);
                lstCtrl.SetSelectedItemDoubleClick(EpgCmds.ShowDialog);

                //ステータス変更の設定
                lstCtrl.SetSelectionChangedEventHandler((sender, e) => this.UpdateStatus(1));

                //最初にコマンド集の初期化
                mc = new CmdExe<InfoSearchItem>(this);
                mc.SetFuncGetDataList(isAll => isAll == true ? lstCtrl.dataList : lstCtrl.GetSelectedItemsList());
                mc.SetFuncSelectSingleData((noChange) => lstCtrl.SelectSingleItem(noChange));
                mc.SetFuncReleaseSelectedData(() => listView_result.UnselectAll());
                
                //コマンド集に無いもの
                mc.AddReplaceCommand(EpgCmds.Search, mc_Search);
                mc.AddReplaceCommand(EpgCmds.JumpList, mc_JumpTab);
                mc.AddReplaceCommand(EpgCmds.ReSearch, mc_ReSearch);
                mc.AddReplaceCommand(EpgCmds.ReSearch2, mc_ReSearch);
                mc.AddReplaceCommand(EpgCmds.Cancel, (sender, e) => this.Close());

                //コマンド集を振り替えるもの
                mc.AddReplaceCommand(EpgCmds.ShowDialog, mc_ShowDialog);
                mc.AddReplaceCommand(EpgCmds.ChgOnOff, mc_ChgOnOff);
                mc.AddReplaceCommand(EpgCmds.Delete, mc_Delete);

                //コマンド集からコマンドを登録。
                mc.ResetCommandBindings(this, listView_result.ContextMenu);

                //コンテキストメニューを開く時の設定
                listView_result.ContextMenu.Opened += new RoutedEventHandler(mc.SupportContextMenuLoading);

                //ボタンの設定
                mBinds.View = CtxmCode.InfoSearchWindow;
                mBinds.SetCommandToButton(button_search, EpgCmds.Search);
                mBinds.AddInputCommand(EpgCmds.Cancel);//ショートカット登録

                //メニューの作成、ショートカットの登録
                this.RefreshMenu();

                //その他設定
                TextBox_SearchWord.Text = Settings.Instance.InfoSearchLastWord;
                checkBox_TitleOnly.IsChecked = Settings.Instance.InfoSearchTitleOnly;
                checkBox_ShowToolTip.IsChecked = Settings.Instance.InfoSearchItemTooltip;
                checkBox_ReserveInfo.IsChecked = Settings.Instance.InfoSearchReserveInfo;
                checkBox_RecInfo.IsChecked = Settings.Instance.InfoSearchRecInfo;
                checkBox_EpgAutoAddInfo.IsChecked = Settings.Instance.InfoSearchEpgAutoAddInfo;
                checkBox_ManualAutoAddInfo.IsChecked = Settings.Instance.InfoSearchManualAutoAddInfo;

                //ツールチップオプションの設定の登録
                InfoSearchItem.RegisterTooltipOption(() => this.checkBox_ShowToolTip.IsChecked == true, this);

                //ステータスバーの登録
                StatusManager.RegisterStatusbar(this.statusBar, this);

                //ウインドウ位置の復元
                Settings.Instance.InfoSearchWndSet.SetToWindow(this);

            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        public override void RefreshMenu()
        {
            mBinds.ResetInputBindings(this, listView_result);
            mm.CtxmGenerateContextMenu(listView_result.ContextMenu, CtxmCode.InfoSearchWindow, true);
        }

        public void SetSearchWord(string word)
        {
            if (word != null)
            {
                startSearch = true;
                TextBox_SearchWord.Text = word;
            }
        }
        
        private void mc_Search(object sender, ExecutedRoutedEventArgs e)
        {
            Search();
            StatusManager.StatusNotifySet(true, "検索を実行");
        }
        private void Search()
        {
            try
            {
                lstCtrl.ReloadInfoData(dataList =>
                {
                    var targetItems = new List<IRecWorkMainData>();
                    if (checkBox_ReserveInfo.IsChecked == true)
                    {
                        targetItems.AddRange(CommonManager.Instance.DB.ReserveList.Values);
                    }
                    if (checkBox_RecInfo.IsChecked == true)
                    {
                        //起動直後は読み込んでない場合がある。
                        CommonManager.Instance.DB.ReloadrecFileInfo();

                        //詳細情報が必要な場合はあらかじめ読込んでおく。
                        if (checkBox_TitleOnly.IsChecked  == false)
                        {
                            CommonManager.Instance.DB.ReadRecFileAppend();
                        }

                        targetItems.AddRange(CommonManager.Instance.DB.RecFileInfo.Values);
                    }
                    if (checkBox_EpgAutoAddInfo.IsChecked == true)
                    {
                        targetItems.AddRange(CommonManager.Instance.DB.EpgAutoAddList.Values);
                    }
                    if (checkBox_ManualAutoAddInfo.IsChecked == true)
                    {
                        targetItems.AddRange(CommonManager.Instance.DB.ManualAutoAddList.Values);
                    }

                    var hitItems = targetItems.Select(data =>
                    {
                        string s = new InfoSearchItem(data).GetSearchText(checkBox_TitleOnly.IsChecked == true);
                        return new Tuple<IRecWorkMainData, string>(data, CommonManager.AdjustSearchText(s).Replace(" ", ""));
                    }).ToList();

                    string[] sWords = CommonManager.AdjustSearchText(TextBox_SearchWord.Text).Split(new char[] { ' ' }, StringSplitOptions.RemoveEmptyEntries);
                    foreach (string sWord in sWords)
                    {
                        hitItems = hitItems.FindAll(s => s.Item2.Contains(sWord));
                    }

                    lstCtrl.dataList.AddRange(InfoSearchItem.Items(hitItems.Select(item => item.Item1)));
                    return true;
                });
                UpdateStatus();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        private void UpdateStatus(int mode = 0)
        {
            string s1 = null;
            string s2 = "";
            if (mode == 0) s1 = ViewUtil.ConvertInfoSearchItemStatus(lstCtrl.dataList, "検索数");
            if (Settings.Instance.DisplayStatus == true)
            {
                List<InfoSearchItem> sList = lstCtrl.GetSelectedItemsList();
                s2 = sList.Count == 0 ? "" : ViewUtil.ConvertInfoSearchItemStatus(sList, "　選択中");
            }
            this.statusBar.SetText(s1, s2);
        }
        private void checkBox_ShowToolTip_Checked(object sender, RoutedEventArgs e)
        {
            listView_result.Items.Refresh();
        }
        private void mc_ShowDialog(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem == null) return;
            //
            object data = lstCtrl.SelectSingleItem().Data;

            if      (data is ReserveData)       MenuUtil.OpenChangeReserveDialog((ReserveData)data, this);
            else if (data is RecFileInfo)       MenuUtil.OpenRecInfoDialog((RecFileInfo)data, this);
            else if (data is EpgAutoAddData)    MenuUtil.OpenChangeEpgAutoAddDialog((EpgAutoAddData)data);
            else if (data is ManualAutoAddData) MenuUtil.OpenChangeManualAutoAddDialog((ManualAutoAddData)data, this);
        }
        private void mc_ChgOnOff(object sender, ExecutedRoutedEventArgs e)
        {
            if (listView_result.SelectedItem == null) return;
            //
            List<IRecWorkMainData> dataList = lstCtrl.GetSelectedItemsList().Select(data => data.Data).ToList();

            if (MenuUtil.CautionManyMessage(dataList.Count, "変更の確認") == false)
            { return; }

            MenuUtil.ReserveChangeOnOff(dataList.OfType<ReserveData>().Clone(), null, false);
            MenuUtil.RecinfoChgProtect(dataList.OfType<RecFileInfo>().Clone());
            MenuUtil.AutoAddChangeOnOffKeyEnabled(dataList.OfType<AutoAddData>().Clone());

            StatusManager.StatusNotifySet(true, mc.GetCmdMessageFormat("状態切替を実行", dataList.Count));
        }
        private void mc_Delete(object sender, ExecutedRoutedEventArgs e)
        {
            if (listView_result.SelectedItem == null) return;
            //
            List<IRecWorkMainData> dataList = lstCtrl.GetSelectedItemsList().Select(data => data.Data).ToList();

            if (CmdExeUtil.CheckKeyboardDeleteCancel(e, dataList.Select(data => data.DataTitle).ToList()) == true)
            { return; }

            if (MenuUtil.CautionManyMessage(dataList.Count, "削除の確認") == false)
            { return; }

            MenuUtil.ReserveDelete(dataList.OfType<ReserveData>().ToList());
            MenuUtil.RecinfoDelete(dataList.OfType<RecFileInfo>().ToList());
            MenuUtil.AutoAddDelete(dataList.OfType<AutoAddData>().ToList());

            StatusManager.StatusNotifySet(true, mc.GetCmdMessageFormat("削除を実行", dataList.Count));
        }
        private void mc_JumpTab(object sender, ExecutedRoutedEventArgs e)
        {
            if (listView_result.SelectedItem == null) return;
            //
            object vItem = lstCtrl.SelectSingleItem().ViewItem;

            CtxmCode code = CtxmCode.EtcWindow;
            if      (vItem is ReserveItem)              code = CtxmCode.ReserveView;
            else if (vItem is RecInfoItem)              code = CtxmCode.RecInfoView;
            else if (vItem is EpgAutoDataItem)          code = CtxmCode.EpgAutoAddView;
            else if (vItem is ManualAutoAddDataItem)    code = CtxmCode.ManualAutoAddView;

            JumpTabAndHide(code, vItem);
        }
        private void mc_ReSearch(object sender, ExecutedRoutedEventArgs e)
        {
            if (listView_result.SelectedItem == null) return;
            //
            string word = MenuUtil.TrimEpgKeyword(lstCtrl.SelectSingleItem().EventName, CmdExeUtil.IsKeyGesture(e));
            if (e.Command == EpgCmds.ReSearch)
            {
                TextBox_SearchWord.Text = word;
                mc_Search(sender, e);
            }
            else
            {
                var dlg = new InfoSearchWindow();
                dlg.SetSearchWord(word);
                dlg.checkBox_TitleOnly.IsChecked = this.checkBox_TitleOnly.IsChecked;
                dlg.checkBox_ShowToolTip.IsChecked = this.checkBox_ShowToolTip.IsChecked;
                dlg.checkBox_ReserveInfo.IsChecked = this.checkBox_ReserveInfo.IsChecked;
                dlg.checkBox_RecInfo.IsChecked = this.checkBox_RecInfo.IsChecked;
                dlg.checkBox_EpgAutoAddInfo.IsChecked = this.checkBox_EpgAutoAddInfo.IsChecked;
                dlg.checkBox_ManualAutoAddInfo.IsChecked = this.checkBox_ManualAutoAddInfo.IsChecked;
                dlg.Left = this.Left + 50;
                dlg.Top = this.Top + 25;
                dlg.checkBox_windowPinned.IsChecked = checkBox_windowPinned.IsChecked;
                dlg.Show();
            }
        }
        
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            TextBox_SearchWord.Focus();
            if (startSearch == true)
            {
                Search();
            }
        }
        protected override void Window_Closed(object sender, EventArgs e)
        {
            Settings.Instance.InfoSearchWndSet.GetFromWindow(this);
            Settings.Instance.InfoSearchLastWord = TextBox_SearchWord.Text;
            Settings.Instance.InfoSearchTitleOnly = checkBox_TitleOnly.IsChecked == true;
            Settings.Instance.InfoSearchItemTooltip = checkBox_ShowToolTip.IsChecked == true;
            Settings.Instance.InfoSearchReserveInfo = checkBox_ReserveInfo.IsChecked == true;
            Settings.Instance.InfoSearchRecInfo = checkBox_RecInfo.IsChecked == true;
            Settings.Instance.InfoSearchEpgAutoAddInfo = checkBox_EpgAutoAddInfo.IsChecked == true;
            Settings.Instance.InfoSearchManualAutoAddInfo = checkBox_ManualAutoAddInfo.IsChecked == true;

            lstCtrl.SaveViewDataToSettings();

            base.Window_Closed(sender, e);
        }

        protected override void ReloadInfoData()
        {
            if (ReloadInfo == true && this.IsVisible == true)
            {
                Search();
                ReloadInfo = false;
            }
        }
    }
    //ジェネリックパラメータTはstatic関係の分割用なので何でもいい
    public class InfoSearchWindowBase : HideableWindow<InfoSearchWindow>
    {
        static InfoSearchWindowBase()
        {
            buttonID = "予約簡易検索";
        }
    }
}

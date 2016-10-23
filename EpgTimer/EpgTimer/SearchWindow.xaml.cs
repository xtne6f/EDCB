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
    public partial class SearchWindow : SearchWindowBase
    {
        private ListViewController<SearchItem> lstCtrl;
        private CmdExeReserve mc; //予約系コマンド集

        private bool ReloadReserveInfo = false;

        public enum SearchMode { Find, NewAdd, Change }
        private SearchMode winMode = SearchMode.Find;

        private static EpgAutoAddView autoAddView { get { return ViewUtil.MainWindow.autoAddView.epgAutoAddView; } }
        private UInt32 autoAddID = 0;

        public SearchWindow()
        {
            InitializeComponent();

            chkboxPinned = this.checkBox_windowPinned;

            try
            {
                //リストビュー関連の設定
                var list_columns = Resources["ReserveItemViewColumns"] as GridViewColumnList;
                list_columns.AddRange(Resources["RecSettingViewColumns"] as GridViewColumnList);

                lstCtrl = new ListViewController<SearchItem>(this);
                lstCtrl.SetSavePath(CommonUtil.NameOf(() => Settings.Instance.SearchWndColumn)
                    , CommonUtil.NameOf(() => Settings.Instance.SearchColumnHead)
                    , CommonUtil.NameOf(() => Settings.Instance.SearchSortDirection));
                lstCtrl.SetViewSetting(listView_result, gridView_result, true, true, list_columns);
                lstCtrl.SetSelectedItemDoubleClick(EpgCmds.ShowDialog);

                //ステータス変更の設定
                lstCtrl.SetSelectionChangedEventHandler((sender, e) => this.UpdateStatus(1));

                //最初にコマンド集の初期化
                mc = new CmdExeReserve(this);
                mc.SetFuncGetSearchList(isAll => (isAll == true ? lstCtrl.dataList.ToList() : lstCtrl.GetSelectedItemsList()));
                mc.SetFuncSelectSingleSearchData(lstCtrl.SelectSingleItem);
                mc.SetFuncReleaseSelectedData(() => listView_result.UnselectAll());
                mc.recSettingView = this.recSettingView;
                
                //コマンド集に無いもの
                mc.AddReplaceCommand(EpgCmds.ReSearch, mc_Research);
                mc.AddReplaceCommand(EpgCmds.ReSearch2, mc_Research);
                mc.AddReplaceCommand(EpgCmds.Search, button_search_Click);
                mc.AddReplaceCommand(EpgCmds.AddInDialog, button_add_epgAutoAdd_Click);
                mc.AddReplaceCommand(EpgCmds.ChangeInDialog, button_chg_epgAutoAdd_Click, (sender, e) => e.CanExecute = winMode == SearchMode.Change);
                mc.AddReplaceCommand(EpgCmds.DeleteInDialog, button_del_epgAutoAdd_Click, (sender, e) => e.CanExecute = winMode == SearchMode.Change);
                mc.AddReplaceCommand(EpgCmds.Delete2InDialog, button_del2_epgAutoAdd_Click, (sender, e) => e.CanExecute = winMode == SearchMode.Change);
                mc.AddReplaceCommand(EpgCmds.UpItem, (sender, e) => button_up_down_Click(-1));
                mc.AddReplaceCommand(EpgCmds.DownItem, (sender, e) => button_up_down_Click(1));
                mc.AddReplaceCommand(EpgCmds.Cancel, (sender, e) => this.Close());
                mc.AddReplaceCommand(EpgCmds.ChgOnOffCheck, (sender, e) => lstCtrl.ChgOnOffFromCheckbox(e.Parameter, EpgCmds.ChgOnOff));

                //コマンド集を振り替えるもの
                mc.AddReplaceCommand(EpgCmds.JumpReserve, (sender, e) => mc_JumpTab(CtxmCode.ReserveView));
                mc.AddReplaceCommand(EpgCmds.JumpTuner, (sender, e) => mc_JumpTab(CtxmCode.TunerReserveView));
                mc.AddReplaceCommand(EpgCmds.JumpTable, (sender, e) => mc_JumpTab(CtxmCode.EpgView));

                //コマンド集からコマンドを登録。
                mc.ResetCommandBindings(this, listView_result.ContextMenu);

                //コンテキストメニューを開く時の設定
                listView_result.ContextMenu.Opened += new RoutedEventHandler(mc.SupportContextMenuLoading);

                //ボタンの設定
                mBinds.View = CtxmCode.SearchWindow;
                mBinds.SetCommandToButton(button_search, EpgCmds.Search);
                mBinds.SetCommandToButton(button_add_reserve, EpgCmds.Add);
                mBinds.SetCommandToButton(button_delall_reserve, EpgCmds.DeleteAll);
                mBinds.SetCommandToButton(button_add_epgAutoAdd, EpgCmds.AddInDialog);
                mBinds.SetCommandToButton(button_chg_epgAutoAdd, EpgCmds.ChangeInDialog);
                mBinds.SetCommandToButton(button_del_epgAutoAdd, EpgCmds.DeleteInDialog);
                mBinds.SetCommandToButton(button_del2_epgAutoAdd, EpgCmds.Delete2InDialog);
                mBinds.SetCommandToButton(button_up_epgAutoAdd, EpgCmds.UpItem);
                mBinds.SetCommandToButton(button_down_epgAutoAdd, EpgCmds.DownItem);
                mBinds.AddInputCommand(EpgCmds.Cancel);//ショートカット登録

                //メニューの作成、ショートカットの登録
                RefreshMenu();

                //その他のショートカット(検索ダイアログ固有の設定)
                searchKeyView.InputBindings.Add(new InputBinding(EpgCmds.Search, new KeyGesture(Key.Enter)));

                //録画プリセット変更時の対応
                recSettingView.SelectedPresetChanged += new EventHandler(SetRecSettingTabHeader);

                //ステータスバーの登録
                StatusManager.RegisterStatusbar(this.statusBar, this);

                //ウインドウ位置の復元
                Settings.Instance.SearchWndSet.SetToWindow(this);

                SetSearchKey(Settings.Instance.DefSearchKey);
                SetRecSetting(Settings.Instance.RecPresetList[0].RecPresetData);

                //notify残ってれば更新。通常残ってないはず。
                ViewUtil.ReloadReserveData();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        public override void RefreshMenu()
        {
            mBinds.ResetInputBindings(this, listView_result);
            mm.CtxmGenerateContextMenu(listView_result.ContextMenu, CtxmCode.SearchWindow, true);
        }

        public EpgSearchKeyInfo GetSearchKey()
        {
            return searchKeyView.GetSearchKey();
        }
        public void SetSearchKey(EpgSearchKeyInfo key)
        {
            searchKeyView.SetSearchKey(key);
        }
        public RecSettingData GetRecSetting()
        {
            return recSettingView.GetRecSetting();
        }
        public void SetRecSetting(RecSettingData set)
        {
            recSettingView.SetDefSetting(set);
        }
        public EpgAutoAddData GetAutoAddData()
        {
            var data = new EpgAutoAddData();
            data.dataID = autoAddID;
            data.searchInfo = GetSearchKey();
            data.recSetting = GetRecSetting();
            return data;
        }
        public void SetAutoAddData(EpgAutoAddData data)
        {
            autoAddID = data.dataID;
            SetSearchKey(data.searchInfo);
            SetRecSetting(data.recSetting);
        }
        private void ChangeAutoAddData(EpgAutoAddData data, bool refresh = true)
        {
            this.SetViewMode(SearchMode.Change);
            this.SetAutoAddData(data);
            this.UpdateEpgAutoAddViewSelection();
            if (refresh == true) UpdateInfo();
        }
        public void SetViewMode(SearchMode md)
        {
            winMode = md;
            WindowTitleSet();
        }
        public void WindowTitleSet()
        {
            string s = (winMode == SearchMode.Find ? "検索" : "キーワード自動予約登録");
            if (searchKeyView != null && string.IsNullOrEmpty(searchKeyView.ComboBox_andKey.Text) == false)
            {
                s = searchKeyView.ComboBox_andKey.Text + " - " + s;
            }
            this.Title = s;
        }
        public void SetRecSettingTabHeader(object sender, EventArgs e)
        {
            string preset_str = "";
            if (Settings.Instance.DisplayPresetOnSearch == true)
            {
                RecPresetItem preset = recSettingView.SelectedPreset(sender == null);
                if (preset != null && string.IsNullOrEmpty(preset.DisplayName) == false)
                {
                    preset_str = string.Format(" - {0}", preset.DisplayName);
                }
            }
            tabItem2.Header = "録画設定" + preset_str;
        }

        private void button_search_Click(object sender, ExecutedRoutedEventArgs e)
        {
            SearchPg(true);
            StatusManager.StatusNotifySet(true, "検索を実行");
        }
        private void SearchPg(bool addSearchLog = false)
        {
            if (addSearchLog == true) searchKeyView.AddSearchLog();

            lstCtrl.ReloadInfoData(dataList =>
            {
                EpgSearchKeyInfo key = GetSearchKey();
                key.keyDisabledFlag = 0; //無効解除
                var list = new List<EpgEventInfo>();

                CommonManager.Instance.CtrlCmd.SendSearchPg(CommonUtil.ToList(key), ref list);

                lstCtrl.dataList.AddFromEventList(list, false, true);
                return true;
            });

            UpdateStatus();
            SetRecSettingTabHeader(null, null);
            WindowTitleSet();
        }

        private void UpdateStatus(int mode = 0)
        {
            string s1 = null;
            string s2 = "";
            if (mode == 0) s1 = ViewUtil.ConvertSearchItemStatus(lstCtrl.dataList, "検索数");
            if (Settings.Instance.DisplayStatus == true)
            {
                List<SearchItem> sList = lstCtrl.GetSelectedItemsList();
                s2 = sList.Count == 0 ? "" : ViewUtil.ConvertSearchItemStatus(sList, "　選択中");
            }
            this.statusBar.SetText(s1, s2);
        }
        private void ReloadReserveData()
        {
            lstCtrl.dataList.SetReserveData();
            lstCtrl.RefreshListView(true);
            UpdateStatus();
        }

        private void button_add_epgAutoAdd_Click(object sender, ExecutedRoutedEventArgs e)
        {
            bool ret = false;
            try
            {
                ret = CheckAutoAddChange(e, 0);
                if (ret == true)
                {
                    //一覧画面非表示の状態から実施する場合のためのコード
                    if (autoAddView.IsVisible == false && CommonManager.Instance.DB.EpgAutoAddList.Count == 0)
                    {
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                        CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                    }

                    List<uint> oldlist = CommonManager.Instance.DB.EpgAutoAddList.Keys.ToList();

                    ret = MenuUtil.AutoAddAdd(CommonUtil.ToList(this.GetAutoAddData()));
                    if (ret == true)
                    {
                        //以降の処理をEpgTimerSrvからの更新通知後に実行すればReload減らせるが、トラブル増えそうなのでこのまま。
                        CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                        CommonManager.Instance.DB.ReloadEpgAutoAddInfo();

                        List<uint> newlist = CommonManager.Instance.DB.EpgAutoAddList.Keys.ToList();
                        List<uint> diflist = newlist.Except(oldlist).ToList();

                        if (diflist.Count == 1)
                        {
                            ChangeAutoAddData(CommonManager.Instance.DB.EpgAutoAddList[diflist[0]], false);
                        }
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            StatusManager.StatusNotifySet(ret, "キーワード予約を追加");
        }
        private void button_chg_epgAutoAdd_Click(object sender, ExecutedRoutedEventArgs e)
        {
            bool ret = false;
            try
            {
                ret = CheckAutoAddChange(e, 1);
                if (ret == true)
                {
                    ret = MenuUtil.AutoAddChange(CommonUtil.ToList(this.GetAutoAddData()));
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            StatusManager.StatusNotifySet(ret, "キーワード予約を変更");
        }
        private void button_del_epgAutoAdd_Click(object sender, ExecutedRoutedEventArgs e)
        {
            bool ret = false;
            try
            {
                ret = CheckAutoAddChange(e, 2);
                if (ret == true)
                {
                    ret = MenuUtil.AutoAddDelete(CommonUtil.ToList(CommonManager.Instance.DB.EpgAutoAddList[autoAddID]));
                    if (ret == true)
                    {
                        SetViewMode(SearchMode.NewAdd);
                        this.autoAddID = 0;
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            StatusManager.StatusNotifySet(ret, "キーワード予約を削除");
        }
        private void button_del2_epgAutoAdd_Click(object sender, ExecutedRoutedEventArgs e)
        {
            bool ret = false;
            try
            {
                ret = CheckAutoAddChange(e, 3);
                if (ret == true)
                {
                    ret = MenuUtil.AutoAddDelete(CommonUtil.ToList(CommonManager.Instance.DB.EpgAutoAddList[autoAddID]), true, true);
                    if (ret == true)
                    {
                        SetViewMode(SearchMode.NewAdd);
                        this.autoAddID = 0;
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            StatusManager.StatusNotifySet(ret, "キーワード予約を予約ごと削除");
        }
        //proc 0:追加、1:変更、2:削除、3:予約ごと削除
        private bool CheckAutoAddChange(ExecutedRoutedEventArgs e, int proc)
        {
            if (proc != 3)
            {
                if (CmdExeUtil.IsDisplayKgMessage(e) == true)
                {
                    var strMode = new string[] { "追加", "変更", "削除" }[proc];
                    if (MessageBox.Show("キーワード予約登録を" + strMode + "します。\r\nよろしいですか？", strMode + "の確認", MessageBoxButton.OKCancel) != MessageBoxResult.OK)
                    { return false; }
                }
            }
            else
            {
                if (CmdExeUtil.CheckAllProcCancel(e, CommonUtil.ToList(CommonManager.Instance.DB.EpgAutoAddList[autoAddID]), true) == true)
                { return false; }
            }

            //データの更新。最初のキャンセルを過ぎたら画面を更新する。
            SearchPg();

            if (proc != 0)
            {
                if (CommonManager.Instance.DB.EpgAutoAddList.ContainsKey(this.autoAddID) == false)
                {
                    MessageBox.Show("項目がありません。\r\n" + "既に削除されています。\r\n" + "(別のEpgtimerによる操作など)", "データエラー", MessageBoxButton.OK, MessageBoxImage.Exclamation);
                    SetViewMode(SearchMode.NewAdd);
                    this.autoAddID = 0;
                    return false;
                }
            }

            if (proc < 2 && Settings.Instance.CautionManyChange == true && searchKeyView.searchKeyDescView.checkBox_keyDisabled.IsChecked != true)
            {
                if (MenuUtil.CautionManyMessage(lstCtrl.dataList.GetNoReserveList().Count, "予約追加の確認") == false)
                { return false; }
            }

            return true;
        }
        private void button_up_down_Click(int direction)
        {
            EpgAutoAddData newItem;

            Func<int, int, int> GetNextIdx = (oldIdx, listCount) =>
            {
                if (oldIdx == -1)
                {
                    return direction >= 0 ? 0 : listCount - 1;
                }
                else
                {
                    return ((oldIdx + direction) % listCount + listCount) % listCount;
                }
            };

            if(autoAddView.IsVisible == true)
            {
                ListView list = autoAddView.listView_key;
                if (list.Items.Count == 0) return;

                this.UpdateEpgAutoAddViewSelection();
                list.SelectedIndex = GetNextIdx(list.SelectedIndex, list.Items.Count);
                newItem = (list.SelectedItem as EpgAutoDataItem).EpgAutoAddInfo;
            }
            else
            {
                //並べ替え中など、一覧画面と順番が異なる場合もある。
                CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                Dictionary<uint, EpgAutoAddData> dict = CommonManager.Instance.DB.EpgAutoAddList;
                if (dict.Count == 0) return;
                
                EpgAutoAddData oldItem;
                List<EpgAutoAddData> list = dict.Values.ToList();
                if (this.autoAddID == 0 || dict.TryGetValue(this.autoAddID, out oldItem) == false)
                {
                    newItem = list[GetNextIdx(-1, list.Count)];
                }
                else
                {
                    newItem = list[GetNextIdx(list.IndexOf(oldItem), list.Count)];
                }
            }

            ChangeAutoAddData(newItem);
        }
        private void mc_JumpTab(CtxmCode trg_code)
        {
            lstCtrl.SelectSingleItem();
            JumpTabAndHide(trg_code, mc.GetJumpTabItem(trg_code));
        }
        private void mc_Research(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                if (listView_result.SelectedItem != null)
                {
                    SearchItem item = lstCtrl.SelectSingleItem();

                    EpgSearchKeyInfo defKey = GetSearchKey();
                    defKey.andKey = MenuUtil.TrimEpgKeyword(item.EventName, CmdExeUtil.IsKeyGesture(e));
                    defKey.regExpFlag = 0;
                    defKey.serviceList.Clear();
                    UInt64 sidKey = item.EventInfo.Create64Key();
                    defKey.serviceList.Add((Int64)sidKey);

                    if (e.Command == EpgCmds.ReSearch)
                    {
                        SetSearchKey(defKey);
                        SearchPg();
                    }
                    else
                    {
                        var dlg = new SearchWindow();
                        dlg.SetViewMode(winMode == SearchMode.Change ? SearchMode.NewAdd : winMode);
                        if (Settings.Instance.MenuSet.CancelAutoAddOff == true)
                        {
                            defKey.keyDisabledFlag = 0;
                        }
                        dlg.SetSearchKey(defKey);
                        dlg.SetRecSetting(this.GetRecSetting());
                        dlg.Left = this.Left + 50;
                        dlg.Top = this.Top + 25;
                        dlg.checkBox_windowPinned.IsChecked = checkBox_windowPinned.IsChecked;
                        dlg.Show();
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if ((winMode == SearchMode.Find || winMode == SearchMode.NewAdd) && string.IsNullOrEmpty(searchKeyView.ComboBox_andKey.Text))
            {
                this.searchKeyView.ComboBox_andKey.Focus();
            }
            else
            {
                this.SearchPg();
            }
        }
        protected override void Window_Closed(object sender, EventArgs e)
        {
            Settings.Instance.SearchWndSet.GetFromWindow(this);
            lstCtrl.SaveViewDataToSettings();
            base.Window_Closed(sender, e);
        }

        public override void UpdateInfo(bool reload = true)
        {
            ReloadReserveInfo = true;
            base.UpdateInfo(reload);
        }
        protected override void Window_Activated(object sender, EventArgs e)
        {
            UpdateEpgAutoAddViewSelection();
            base.Window_Activated(sender, e);
        }
        protected override void ReloadInfoData()
        {
            //再検索はCtrlCmdを使うので、アクティブウィンドウでだけ実行させる。
            if (this.IsActive == true)
            {
                if (ReloadInfo == true)
                {
                    SearchPg();
                    ReloadInfo = false;
                    ReloadReserveInfo = false;
                }
            }
            //表示の更新は見えてれば実行する。
            if (this.IsVisible == true)
            {
                if (ReloadReserveInfo == true)
                {
                    ReloadReserveData();
                    ReloadReserveInfo = false;
                }
            }
        }
        public static bool UpdatesEpgAutoAddViewSelection()
        {
            foreach (var win in Application.Current.Windows.OfType<SearchWindow>())
            {
                if (win.IsActive == true)
                {
                    win.UpdateEpgAutoAddViewSelection();
                    return true;
                }
            }
            return false;
        }
        private void UpdateEpgAutoAddViewSelection()
        {
            autoAddView.UpdateListViewSelection(this.autoAddID);
        }

        public static void UpdatesEpgAutoAddViewOrderChanged(Dictionary<uint, uint> changeIDTable)
        {
            foreach (var win in Application.Current.Windows.OfType<SearchWindow>())
            {
                win.UpdateEpgAutoAddViewOrderChanged(changeIDTable);
            }
        }
        private void UpdateEpgAutoAddViewOrderChanged(Dictionary<uint, uint> changeIDTable)
        {
            if (this.autoAddID == 0) return;

            if (changeIDTable.ContainsKey(this.autoAddID) == false)
            {
                //ID無くなった
                this.autoAddID = 0;
                SetViewMode(SearchMode.NewAdd);
            }
            else
            {
                //新しいIDに変更
                this.autoAddID = changeIDTable[this.autoAddID];
            }
        }
    }
    public class SearchWindowBase : HideableWindow<SearchWindow>
    {
        static SearchWindowBase()
        {
            buttonID = "検索";
        }
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    /// <summary>
    /// SearchWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SearchWindow : Window
    {
        //よく使うので
        static MainWindow mainWindow { get { return (MainWindow)Application.Current.MainWindow; } }
        static EpgAutoAddView autoAddView { get { return mainWindow.autoAddView.epgAutoAddView; } }

        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private ViewUtil vutil = CommonManager.Instance.VUtil;
        private MenuManager mm = CommonManager.Instance.MM;

        private CmdExeReserve mc; //予約系コマンド集
        private MenuBinds mBinds = new MenuBinds();

        private ListViewController<SearchItem> lstCtrl;

        public enum SearchMode { Find, NewAdd, Change }
        private SearchMode winMode = SearchMode.Find;
        
        private UInt32 autoAddID = 0;

        private bool ReloadInfo = false;
        private bool ReloadReserveInfo = false;

        public SearchWindow()
        {
            InitializeComponent();

            try
            {
                //リストビュー関連の設定
                var list_columns = Resources["ReserveItemViewColumns"] as GridViewColumnList;
                list_columns.AddRange(Resources["RecSettingViewColumns"] as GridViewColumnList);

                lstCtrl = new ListViewController<SearchItem>(this);
                lstCtrl.SetSavePath(CommonUtil.GetMemberName(() => Settings.Instance.SearchWndColumn)
                    , CommonUtil.GetMemberName(() => Settings.Instance.SearchColumnHead)
                    , CommonUtil.GetMemberName(() => Settings.Instance.SearchSortDirection));
                lstCtrl.SetViewSetting(listView_result, gridView_result, true, true, list_columns);
                lstCtrl.SetSelectedItemDoubleClick(EpgCmds.ShowDialog);

                //最初にコマンド集の初期化
                mc = new CmdExeReserve(this);
                mc.SetFuncGetSearchList(isAll => (isAll == true ? lstCtrl.dataList.ToList() : lstCtrl.GetSelectedItemsList()));
                mc.SetFuncSelectSingleSearchData(lstCtrl.SelectSingleItem);
                mc.SetFuncReleaseSelectedData(() => listView_result.UnselectAll());
                mc.recSettingView = this.recSettingView;
                
                //コマンド集に無いもの
                mc.AddReplaceCommand(EpgCmds.ReSearch, mc_Research);
                mc.AddReplaceCommand(EpgCmds.ReSearch2, mc_Research);
                mc.AddReplaceCommand(EpgCmds.Search, (sender, e) => SearchPg());
                mc.AddReplaceCommand(EpgCmds.AddInDialog, button_add_epgAutoAdd_Click);
                mc.AddReplaceCommand(EpgCmds.ChangeInDialog, button_chg_epgAutoAdd_Click, (sender, e) => e.CanExecute = winMode == SearchMode.Change);
                mc.AddReplaceCommand(EpgCmds.DeleteInDialog, button_del_epgAutoAdd_Click, (sender, e) => e.CanExecute = winMode == SearchMode.Change);
                mc.AddReplaceCommand(EpgCmds.UpItem, (sender, e) => button_up_down_Click(-1));
                mc.AddReplaceCommand(EpgCmds.DownItem, (sender, e) => button_up_down_Click(1));
                mc.AddReplaceCommand(EpgCmds.Cancel, (sender, e) => this.Close());
                mc.AddReplaceCommand(EpgCmds.ChgOnOffCheck, (sender, e) => lstCtrl.ChgOnOffFromCheckbox(e.Parameter, EpgCmds.ChgOnOff));

                //コマンド集を振り替えるもの
                mc.AddReplaceCommand(EpgCmds.JumpReserve, (sender, e) => mc_JumpTab(CtxmCode.ReserveView, true));
                mc.AddReplaceCommand(EpgCmds.JumpTuner, (sender, e) => mc_JumpTab(CtxmCode.TunerReserveView, true, Settings.Instance.TunerDisplayOffReserve == false));
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
                mBinds.SetCommandToButton(button_up_epgAutoAdd, EpgCmds.UpItem);
                mBinds.SetCommandToButton(button_down_epgAutoAdd, EpgCmds.DownItem);
                mBinds.AddInputCommand(EpgCmds.Cancel);//ショートカット登録

                //メニューの作成、ショートカットの登録
                RefreshMenu();

                //その他のショートカット(検索ダイアログ固有の設定)
                searchKeyView.InputBindings.Add(new InputBinding(EpgCmds.Search, new KeyGesture(Key.Enter)));

                //録画プリセット変更時の対応
                recSettingView.SelectedPresetChanged += new EventHandler(SetRecSettingTabHeader);

                //ウインドウ位置の復元
                if (Settings.Instance.SearchWndTop != -100)
                {
                    this.Top = Settings.Instance.SearchWndTop;
                }
                if (Settings.Instance.SearchWndLeft != -100)
                {
                    this.Left = Settings.Instance.SearchWndLeft;
                }
                if (Settings.Instance.SearchWndWidth > 0)
                {
                    this.Width = Settings.Instance.SearchWndWidth;
                }
                if (Settings.Instance.SearchWndHeight > 0)
                {
                    this.Height = Settings.Instance.SearchWndHeight;
                }
                checkBox_windowPinned.IsChecked = Settings.Instance.SearchWndPinned;

                SetSearchKey(Settings.Instance.DefSearchKey);
                SetRecSetting(Settings.Instance.RecPresetList[0].RecPresetData);

                //notify残ってれば更新。通常残ってないはず。
                vutil.ReloadReserveData();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        public static void RefreshMenu(Window owner_win)
        {
            foreach (SearchWindow win in owner_win.OwnedWindows.OfType<SearchWindow>())
            {
                SearchWindow.RefreshMenu(win);
                win.RefreshMenu();
            }
        }
        public void RefreshMenu()
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
            button_chg_epgAutoAdd.Visibility = (winMode == SearchMode.Change ? Visibility.Visible : Visibility.Hidden);
            button_del_epgAutoAdd.Visibility = button_chg_epgAutoAdd.Visibility;
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

        private void SearchPg()
        {
            lstCtrl.ReloadInfoData(dataList =>
            {
                EpgSearchKeyInfo key = GetSearchKey();
                key.keyDisabledFlag = 0; //無効解除
                var list = new List<EpgEventInfo>();

                cmd.SendSearchPg(CommonUtil.ToList(key), ref list);

                lstCtrl.dataList.AddFromEventList(list, false, true);

                searchKeyView.AddSearchLog();
                return true;
            });

            RefreshStatus();
            SetRecSettingTabHeader(null, null);
            WindowTitleSet();
        }

        private void RefreshStatus()
        {
            text_result.Text = string.Format("検索数:{0}  ", lstCtrl.dataList.Count);
            List<ReserveData> rlist = lstCtrl.dataList.GetReserveList();
            if (rlist.Count != 0)
            {
                int OnCount = rlist.Count(data => data.IsEnabled == true);
                int OffCount = rlist.Count - OnCount;
                text_result.Text += string.Format("予約数:{0} ( 有効 {1} / 無効 {2} )", rlist.Count, OnCount, OffCount);
            }
        }

        private void ReloadReserveData()
        {
            lstCtrl.dataList.SetReserveData();
            this.listView_result.Items.Refresh();
            RefreshStatus();
        }

        private void button_add_epgAutoAdd_Click(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                if (CheckAutoAddChange(e, 0) == false) return;

                //一覧画面非表示の状態から実施する場合のためのコード
                if (autoAddView.IsVisible == false && CommonManager.Instance.DB.EpgAutoAddList.Count == 0)
                {
                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                    CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                }

                List<uint> oldlist = CommonManager.Instance.DB.EpgAutoAddList.Keys.ToList();

                if (mutil.AutoAddAdd(CommonUtil.ToList(this.GetAutoAddData())) == true)
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
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_chg_epgAutoAdd_Click(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                if (CheckAutoAddChange(e, 1) == false) return;

                mutil.AutoAddChange(CommonUtil.ToList(this.GetAutoAddData()));
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_del_epgAutoAdd_Click(object sender, ExecutedRoutedEventArgs e)
        {
            if (CheckAutoAddChange(e, 2) == false) return;

            if (mutil.AutoAddDelete(CommonUtil.ToList(CommonManager.Instance.DB.EpgAutoAddList[autoAddID])) == true)
            {
                SetViewMode(SearchMode.NewAdd);
                this.autoAddID = 0;
            }
        }

        //proc 0:追加、1:変更、2:削除
        private bool CheckAutoAddChange(ExecutedRoutedEventArgs e, int proc)
        {
            if (CmdExeUtil.IsDisplayKgMessage(e) == true)
            {
                var strMode = new string[] { "追加", "変更", "削除" }[proc];
                if (MessageBox.Show("自動予約登録を" + strMode + "します。\r\nよろしいですか？", strMode + "の確認", MessageBoxButton.OKCancel) != MessageBoxResult.OK)
                { return false; }
            }

            //データの更新
            SearchPg();

            if (proc != 0)
            {
                if (CommonManager.Instance.DB.EpgAutoAddList.ContainsKey(this.autoAddID) == false)
                {
                    MessageBox.Show("項目がありません。\r\n" + "既に削除されています。\r\n");
                    SetViewMode(SearchMode.NewAdd);
                    this.autoAddID = 0;
                    return false;
                }
            }

            if (proc != 2 && Settings.Instance.CautionManyChange == true && searchKeyView.searchKeyDescView.checkBox_keyDisabled.IsChecked != true)
            {
                if (mutil.CautionManyMessage(lstCtrl.dataList.GetNoReserveList().Count, "予約追加の確認") == false)
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
                if (list.Items.Count == 0) return;//ここには引っかからないはずだけど一応チェック入れておく。

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

        private void mc_JumpTab(CtxmCode code, bool reserveOnly = false, bool onReserveOnly = false)
        {
            if (listView_result.SelectedItem != null)
            {
                SearchItem item = lstCtrl.SelectSingleItem();

                reserveOnly |= onReserveOnly;
                if (reserveOnly && item.IsReserved == false) return;
                if (onReserveOnly && item.ReserveInfo.IsEnabled == false) return;

                BlackoutWindow.SelectedItem = item;

                SetHideSearchWindow(this);
                SearchWindow.MinimizeWindows();

                mainWindow.moveTo_tabItem(code);
            }
        }

        private void mc_Research(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                if (listView_result.SelectedItem != null)
                {
                    SearchItem item = lstCtrl.SelectSingleItem();

                    EpgSearchKeyInfo defKey = GetSearchKey();
                    defKey.andKey = mutil.TrimEpgKeyword(item.EventName, CmdExeUtil.IsKeyGesture(e));
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
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public static void MinimizeWindows()
        {
            foreach (SearchWindow win in Application.Current.Windows.OfType<SearchWindow>())
            {
                win.WindowState = WindowState.Minimized;
            }
        }

        private void Window_StateChanged(object sender, EventArgs e)
        {
            if (this.WindowState != WindowState.Minimized)
            {
                if (hideSearchWindow == this) SearchWindow.SetHideSearchWindow(null);
            }
        }

        private void Window_Closed(object sender, System.EventArgs e)
        {
            if (this.Visibility == Visibility.Visible && this.WindowState == WindowState.Normal)
            {
                Settings.Instance.SearchWndWidth = this.Width;
                Settings.Instance.SearchWndHeight = this.Height;
                Settings.Instance.SearchWndTop = this.Top;
                Settings.Instance.SearchWndLeft = this.Left;
            }

            Settings.Instance.SearchWndPinned = checkBox_windowPinned.IsChecked == true;
            lstCtrl.SaveViewDataToSettings();
            if (hideSearchWindow == this) SearchWindow.SetHideSearchWindow(null);

            if (AllClosing == false)
            {
                Settings.SaveToXmlFile();//検索ワード、ウィンドウ位置の保存
                if (Application.Current.Windows.OfType<SearchWindow>().Count(win => win.IsActive == true) == 0)
                {
                    mainWindow.ListFoucsOnVisibleChanged();
                }
            }
        }

        private static bool AllClosing = false;
        public static void CloseWindows(bool IsSave = false)
        {
            AllClosing = true;

            foreach (SearchWindow win in Application.Current.Windows.OfType<SearchWindow>())
            {
                win.Close();
            }

            if (IsSave == true) Settings.SaveToXmlFile();

            AllClosing = false;
        }

        public static void UpdatesInfo(bool refreshOnly = false)
        {
            foreach (SearchWindow win in Application.Current.Windows.OfType<SearchWindow>())
            {
                win.UpdateInfo(refreshOnly);
            }
        }
        public void UpdateInfo(bool refreshOnly = false)
        {
            if (refreshOnly == false)
            {
                ReloadInfo = true;
            }
            ReloadReserveInfo = true;
            ReloadInfoData();
        }
        private void Window_Activated(object sender, EventArgs e)
        {
            UpdateEpgAutoAddViewSelection();
            ReloadInfoData();
        }
        private void ReloadInfoData()
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
            foreach (SearchWindow win in Application.Current.Windows.OfType<SearchWindow>())
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

        /// <summary>番組表などへジャンプした際に最小化したSearchWindow</summary>
        private static SearchWindow hideSearchWindow = null;
        public static bool HasHideSearchWindow { get { return hideSearchWindow != null; } }
        private static void SetHideSearchWindow(SearchWindow win)
        {
            // 情報を保持は最新のもの1つだけ
            hideSearchWindow = win;
            mainWindow.EmphasizeSearchButton(SearchWindow.HasHideSearchWindow);
        }

        public static void RestoreMinimizedWindow()
        {
            // 最小化したSearchWindowを復帰
            if (SearchWindow.HasHideSearchWindow == true)
            {
                hideSearchWindow.WindowState = WindowState.Normal;
            }
        }

        private void checkBox_windowPinned_Checked(object sender, RoutedEventArgs e)
        {
            this.Owner = ((sender as CheckBox).IsChecked == true) ? mainWindow : null;
        }
    }
}

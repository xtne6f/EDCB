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
    public partial class SearchWindow : SearchWindowBase
    {
        protected override UserCtrlView.DataViewBase DataView { get { return ViewUtil.MainWindow.autoAddView.epgAutoAddView; } }
        protected override string AutoAddString { get { return "キーワード予約"; } }

        private ListViewController<SearchItem> lstCtrl;
        private CmdExeReserve mc; //予約系コマンド集

        public SearchWindow()
        {
            InitializeComponent();

            try
            {
                buttonID = "検索";
                base.SetParam(true, checkBox_windowPinned, checkBox_dataReplace);

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
                mc.AddReplaceCommand(EpgCmds.AddInDialog, autoadd_add);
                mc.AddReplaceCommand(EpgCmds.ChangeInDialog, autoadd_chg, (sender, e) => e.CanExecute = winMode == AutoAddMode.Change);
                mc.AddReplaceCommand(EpgCmds.DeleteInDialog, autoadd_del1, (sender, e) => e.CanExecute = winMode == AutoAddMode.Change);
                mc.AddReplaceCommand(EpgCmds.Delete2InDialog, autoadd_del2, (sender, e) => e.CanExecute = winMode == AutoAddMode.Change);
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
        public override AutoAddData GetAutoAddData()
        {
            var data = new EpgAutoAddData();
            data.dataID = autoAddID;
            data.searchInfo = GetSearchKey();
            data.recSetting = GetRecSetting();
            return data;
        }
        public override bool SetAutoAddData(AutoAddData setdata)
        {
            if (setdata as EpgAutoAddData == null) return false;
            var data = setdata as EpgAutoAddData;

            autoAddID = data.dataID;
            SetSearchKey(data.searchInfo);
            SetRecSetting(data.recSetting);
            return true;
        }
        public override void ChangeAutoAddData(AutoAddData data, bool refresh = true)
        {
            base.ChangeAutoAddData(data);
            if (refresh == true) SearchPg();
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

                dataList.AddFromEventList(list, false, true);
                return true;
            });

            UpdateStatus();
            SetRecSettingTabHeader(null, null);
            SetWindowTitle();
        }
        public override void SetWindowTitle()
        {
            this.Title = ViewUtil.WindowTitleText(searchKeyView.ComboBox_andKey.Text, (winMode == AutoAddMode.Find ? "検索" : "キーワード自動予約登録"));
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
        private void RefreshReserveInfo()
        {
            lstCtrl.dataList.SetReserveData();
            lstCtrl.RefreshListView(true);
            UpdateStatus();
        }

        //proc 0:追加、1:変更、2:削除、3:予約ごと削除
        protected override int CheckAutoAddChange(ExecutedRoutedEventArgs e, int proc)
        {
            int ret = base.CheckAutoAddChange(e, proc);

            //データの更新。最初のキャンセルを過ぎていたら画面を更新する。
            if (ret >= 0) SearchPg();

            if (ret != 0) return ret;

            if (proc < 2 && Settings.Instance.CautionManyChange == true && searchKeyView.searchKeyDescView.checkBox_keyDisabled.IsChecked != true)
            {
                if (MenuUtil.CautionManyMessage(lstCtrl.dataList.GetNoReserveList().Count, "予約追加の確認") == false)
                { return 2; }
            }

            return 0;
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
                        WriteWindowSaveData();

                        var dlg = new SearchWindow();
                        dlg.SetViewMode(winMode == AutoAddMode.Change ? AutoAddMode.NewAdd : winMode);
                        if (Settings.Instance.MenuSet.CancelAutoAddOff == true)
                        {
                            defKey.keyDisabledFlag = 0;
                        }
                        dlg.SetSearchKey(defKey);
                        dlg.SetRecSetting(this.GetRecSetting());
                        dlg.Show();
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if ((winMode == AutoAddMode.Find || winMode == AutoAddMode.NewAdd) && string.IsNullOrEmpty(searchKeyView.ComboBox_andKey.Text))
            {
                this.searchKeyView.ComboBox_andKey.Focus();
            }
            else
            {
                this.SearchPg();
            }
        }

        protected override void WriteWindowSaveData()
        {
            lstCtrl.SaveViewDataToSettings();
            base.WriteWindowSaveData();
        }

        private bool RefreshReserveInfoFlg = false;
        public override void UpdateInfo(bool reload = true)
        {
            RefreshReserveInfoFlg = true;
            base.UpdateInfo(reload);
        }
        protected override void ReloadInfo()
        {
            //再検索はCtrlCmdを使うので、アクティブウィンドウでだけ実行させる。
            if (ReloadInfoFlg == true && this.IsActive == true)
            {
                SearchPg();
                ReloadInfoFlg = false;
                RefreshReserveInfoFlg = false;
            }
            //表示の更新は見えてれば実行する。
            if (RefreshReserveInfoFlg == true && this.IsVisible == true && this.WindowState != WindowState.Minimized)
            {
                RefreshReserveInfo();
                RefreshReserveInfoFlg = false;
            }
        }
    }
    public class SearchWindowBase : AutoAddWindow<SearchWindow, EpgAutoAddData> { }

    public enum AutoAddMode { Find, NewAdd, Change }
    public class AutoAddWindow<T, S> : HideableWindow<T> where S : AutoAddData
    {
        protected override UInt64 DataID { get { return autoAddID; } }
        protected virtual AutoAddListView AutoAddView { get { return DataView as AutoAddListView; } }
        protected virtual string AutoAddString { get { return ""; } }
        protected UInt32 autoAddID = 0;
        protected AutoAddData autoAddData { get { return AutoAddData.AutoAddList(typeof(S), autoAddID); } }
        protected virtual IEnumerable<AutoAddData> autoAddDBList { get { return AutoAddData.GetDBManagerList(typeof(S)); } }
        protected virtual ErrCode ReloadAutoAddDBList(bool notify = false) { return AutoAddData.ReloadDBManagerList(typeof(S), notify); }

        protected AutoAddMode winMode = AutoAddMode.Find;
        public virtual void SetViewMode(AutoAddMode md)
        { 
            winMode = md;
            SetWindowTitle();
            if (md != AutoAddMode.Change) autoAddID = 0;
        }
        public virtual void SetWindowTitle() { }

        public virtual AutoAddData GetAutoAddData() { return null; }
        public virtual bool SetAutoAddData(AutoAddData data) { return false; }
        public virtual void ChangeAutoAddData(AutoAddData data, bool refresh = true)
        {
            if (data as S == null) return;
            if (SetAutoAddData(data) == false) return;
            SetViewMode(AutoAddMode.Change);
            UpdateViewSelection();
        }

        //proc 0:追加、1:変更、2:削除、3:予約ごと削除
        static string[] cmdMsg = new string[] { "追加", "変更", "削除", "予約ごと削除" };
        protected virtual int CheckAutoAddChange(ExecutedRoutedEventArgs e, int proc)
        {
            if (proc != 3)
            {
                if (CmdExeUtil.IsDisplayKgMessage(e) == true)
                {
                    if (MessageBox.Show(AutoAddString + "を" + cmdMsg[proc] + "します。\r\nよろしいですか？", cmdMsg[proc] + "の確認", MessageBoxButton.OKCancel) != MessageBoxResult.OK)
                    { return -2; }
                }
            }
            else
            {
                if (CmdExeUtil.CheckAllProcCancel(e, CommonUtil.ToList(autoAddData), true) == true)
                { return -1; }
            }

            if (proc != 0)
            {
                if (autoAddData == null)
                {
                    MessageBox.Show("項目がありません。\r\n" + "既に削除されています。", "データエラー", MessageBoxButton.OK, MessageBoxImage.Exclamation);
                    SetViewMode(AutoAddMode.NewAdd);
                    return 1;
                }
            }

            return 0;
        }

        protected void autoadd_add(object sender, ExecutedRoutedEventArgs e) { autoadd_add_chg(e, 0); }
        protected void autoadd_chg(object sender, ExecutedRoutedEventArgs e) { autoadd_add_chg(e, 1); }
        protected void autoadd_add_chg(ExecutedRoutedEventArgs e, int code)
        {
            bool ret = false;
            try
            {
                AutoAddData data = GetAutoAddData();
                if (data != null && CheckAutoAddChange(e, code) == 0)
                {
                    if (code == 0)
                    {
                        //一覧画面非表示の状態から実施する場合のためのコード
                        if (AutoAddView.IsVisible == false && autoAddDBList.Count() == 0)
                        {
                            ReloadAutoAddDBList(true);
                        }

                        List<uint> oldlist = autoAddDBList.Select(item => item.DataID).ToList();

                        ret = MenuUtil.AutoAddAdd(CommonUtil.ToList(data));
                        if (ret == true)
                        {
                            //以降の処理をEpgTimerSrvからの更新通知後に実行すればReload減らせるが、トラブル増えそうなのでこのまま。
                            ReloadAutoAddDBList(true);

                            List<uint> newlist = autoAddDBList.Select(item => item.DataID).ToList();
                            List<uint> diflist = newlist.Except(oldlist).ToList();

                            if (diflist.Count == 1)
                            {
                                ChangeAutoAddData(AutoAddData.AutoAddList(typeof(S), diflist[0]), false);
                            }
                        }
                    }
                    else
                    {
                        ret = MenuUtil.AutoAddChange(CommonUtil.ToList(data));
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            StatusManager.StatusNotifySet(ret, AutoAddString + "を" + cmdMsg[code]);
        }

        protected void autoadd_del1(object sender, ExecutedRoutedEventArgs e) { autoadd_del(e, 2); }
        protected void autoadd_del2(object sender, ExecutedRoutedEventArgs e) { autoadd_del(e, 3); }
        protected void autoadd_del(ExecutedRoutedEventArgs e, int code)
        {
            bool ret = false;
            try
            {
                if (CheckAutoAddChange(e, code) == 0)
                {
                    if (code == 2)
                    {
                        ret = MenuUtil.AutoAddDelete(CommonUtil.ToList(autoAddData));
                    }
                    else
                    {
                        ret = MenuUtil.AutoAddDelete(CommonUtil.ToList(autoAddData), true, true);
                    }

                    if (ret == true)
                    {
                        SetViewMode(AutoAddMode.NewAdd);
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            StatusManager.StatusNotifySet(ret, AutoAddString + "を" + cmdMsg[code]);
        }

        protected virtual void button_up_down_Click(int direction)
        {
            AutoAddData newItem;

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

            if (AutoAddView.IsVisible == true)
            {
                ListView list = AutoAddView.listView_key;
                if (list.Items.Count == 0) return;

                UpdateViewSelection();//Activate()で移動しているはずだが、一応再確定させておく
                list.SelectedIndex = GetNextIdx(list.SelectedIndex, list.Items.Count);
                newItem = (list.SelectedItem as AutoAddDataItem).Data;
            }
            else
            {
                //並べ替え中など、一覧画面と順番が異なる場合もある。
                ReloadAutoAddDBList();
                List<AutoAddData> list = autoAddDBList.ToList();
                if (list.Count == 0) return;

                AutoAddData oldItem = autoAddData;
                if (oldItem == null)
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
        
        public static void UpdatesAutoAddViewOrderChanged(Dictionary<uint, uint> changeIDTable)
        {
            foreach (var win in Application.Current.Windows.OfType<AutoAddWindow<T, S>>())
            {
                win.UpdateAutoAddViewOrderChanged(changeIDTable);
            }
        }
        protected void UpdateAutoAddViewOrderChanged(Dictionary<uint, uint> changeIDTable)
        {
            if (autoAddID == 0) return;

            if (changeIDTable.ContainsKey(autoAddID) == false)
            {
                //ID無くなった
                SetViewMode(AutoAddMode.NewAdd);
            }
            else
            {
                //新しいIDに変更
                autoAddID = changeIDTable[autoAddID];
            }
        }
    }
}

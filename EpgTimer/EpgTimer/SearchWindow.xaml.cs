using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;
using System.ComponentModel;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// SearchWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SearchWindow : Window
    {
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private ViewUtil vutil = CommonManager.Instance.VUtil;
        private MenuManager mm = CommonManager.Instance.MM;

        private CmdExeReserve mc; //予約系コマンド集
        private MenuBinds mBinds = new MenuBinds();

        private ListViewController<SearchItem> lstCtrl;

        public enum SearchMode { Find, NewAdd, Change }
        private SearchMode winMode = SearchMode.Find;
        private bool subWindow = false;
        private static string subWindowString = "(サブウィンドウ)";
        
        private UInt32 autoAddID = 0;

        MainWindow mainWindow = null;
        private DateTime? lastSettingTime = null;

        public SearchWindow()
        {
            InitializeComponent();

            try
            {
                //比較その他でよく使う
                mainWindow = (MainWindow)Application.Current.MainWindow;

                //リストビュー関連の設定
                lstCtrl = new ListViewController<SearchItem>(this);
                lstCtrl.SetInitialSortKey("StartTime");
                lstCtrl.SetViewSetting(listView_result, gridView_result, true);

                //最初にコマンド集の初期化
                mc = new CmdExeReserve(this);
                mc.SetFuncGetSearchList(isAll => (isAll == true ? lstCtrl.dataList.ToList() : lstCtrl.GetSelectedItemsList()));
                mc.SetFuncSelectSingleSearchData(lstCtrl.SelectSingleItem);
                mc.SetFuncReleaseSelectedData(() => listView_result.UnselectAll());
                mc.SetFuncPostProc((sender, e, cmdOpt) =>
                {
                    if (mc.IsCommandExecuted == true && cmdOpt.IsChangeDB == true)
                    {
                        this.RefreshAndSearch();
                    }
                });
                mc.recSettingView = this.recSettingView;
                
                //コマンド集に無いもの
                mc.AddReplaceCommand(EpgCmds.ReSearch, mc_Research);
                mc.AddReplaceCommand(EpgCmds.ReSearch2, mc_Research);
                mc.AddReplaceCommand(EpgCmds.Search, (sender, e) => SearchPg());
                mc.AddReplaceCommand(EpgCmds.AddInDialog, button_add_epgAutoAdd_Click);
                mc.AddReplaceCommand(EpgCmds.ChangeInDialog, button_chg_epgAutoAdd_Click, (sender, e) => e.CanExecute = winMode == SearchMode.Change);
                mc.AddReplaceCommand(EpgCmds.DeleteInDialog, button_del_epgAutoAdd_Click, (sender, e) => e.CanExecute = winMode == SearchMode.Change);
                mc.AddReplaceCommand(EpgCmds.UpItem, button_up_epgAutoAdd_Click, (sender, e) => e.CanExecute = winMode == SearchMode.Change && mainWindow.autoAddView.epgAutoAddView.IsVisible);
                mc.AddReplaceCommand(EpgCmds.DownItem, button_down_epgAutoAdd_Click, (sender, e) => e.CanExecute = winMode == SearchMode.Change && mainWindow.autoAddView.epgAutoAddView.IsVisible);
                mc.AddReplaceCommand(EpgCmds.Cancel, (sender, e) => this.Close());

                //コマンド集を振り替えるもの
                mc.AddReplaceCommand(EpgCmds.JumpTable, mc_JumpTable);

                //コマンド集からコマンドを登録。
                mc.ResetCommandBindings(this, listView_result.ContextMenu);

                //コンテキストメニューを開く時の設定
                lstCtrl.SetCtxmTargetSave(listView_result.ContextMenu);//こっちが先
                listView_result.ContextMenu.Tag = this; //情報を付与
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

                //ウインドウ位置の復元
                if (Settings.Instance.SearchWndTop != 0)
                {
                    this.Top = Settings.Instance.SearchWndTop;
                }
                if (Settings.Instance.SearchWndLeft != 0)
                {
                    this.Left = Settings.Instance.SearchWndLeft;
                }
                if (Settings.Instance.SearchWndWidth != 0)
                {
                    this.Width = Settings.Instance.SearchWndWidth;
                }
                if (Settings.Instance.SearchWndHeight != 0)
                {
                    this.Height = Settings.Instance.SearchWndHeight;
                }

                searchKeyView.SetSearchKey(Settings.Instance.DefSearchKey);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        public void RefreshMenu()
        {
            if (this.lastSettingTime == mm.LastMenuSettingTime) return;
            this.lastSettingTime = mm.LastMenuSettingTime;

            mBinds.ResetInputBindings(this, listView_result);
            mm.CtxmGenerateContextMenu(listView_result.ContextMenu, CtxmCode.SearchWindow, true);
        }

        public void GetSearchKey(ref EpgSearchKeyInfo key)
        {
            searchKeyView.GetSearchKey(ref key);
        }

        public void SetSearchDefKey(EpgSearchKeyInfo key)
        {
            searchKeyView.SetSearchKey(key);
        }

        public void SetRecInfoDef(RecSettingData set)
        {
            recSettingView.SetDefSetting(set);
        }

        public void SetViewMode(SearchMode md)
        {
            winMode = md;
            if (winMode == SearchMode.Find)
            {
                Title = "検索";
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_del_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_up_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_down_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
            }
            else if (winMode == SearchMode.NewAdd)
            {
                Title = "EPG予約条件";
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_del_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_up_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_down_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
            }
            else if (winMode == SearchMode.Change)
            {
                Title = "EPG予約条件";
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
                button_del_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
                button_up_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
                button_down_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
            }
            SetSubWindowTitle();
        }
        public void SetSubWindow()
        {
            subWindow = true;
            SetSubWindowTitle();
        }
        public void SetSubWindowTitle()
        {
            if (subWindow == true)
            {
                Title += (Title.EndsWith(subWindowString) == true ? "" : subWindowString);
            }
        }

        public void SetChgAutoAddID(UInt32 id)
        {
            autoAddID = id;
        }

        private void RefreshAndSearch()
        {
            CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
            CommonManager.Instance.DB.ReloadReserveInfo();
            SearchPg();
        }

        private void SearchPg()
        {
            lstCtrl.ReloadInfoData(dataList =>
            {
                EpgSearchKeyInfo key = new EpgSearchKeyInfo();
                searchKeyView.GetSearchKey(ref key);
                key.keyDisabledFlag = 0; //無効解除
                List<EpgEventInfo> list = new List<EpgEventInfo>();

                cmd.SendSearchPg(mutil.ToList(key), ref list);

                lstCtrl.dataList.AddFromEventList(list, false, true);

                searchKeyView.AddSearchLog();
                return true;
            });
        }

        private bool CheckCautionMany()
        {
            if (Settings.Instance.CautionManyChange == true && searchKeyView.searchKeyDescView.checkBox_keyDisabled.IsChecked != true)
            {
                SearchPg();

                if (mutil.CautionManyMessage(lstCtrl.dataList.GetNoReserveList().Count, "予約追加の確認") == false)
                {
                    return false;
                }
            }
            return true;
        }

        private void button_add_epgAutoAdd_Click(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                if (CmdExeUtil.IsDisplayKgMessage(e) == true)
                {
                    if (MessageBox.Show("自動予約登録を追加します。\r\nよろしいですか？", "追加の確認", MessageBoxButton.OKCancel) != MessageBoxResult.OK)
                    { return; }
                }
                if (CheckCautionMany() == false) return;

                var addItem = new EpgAutoAddData();
                var searchKey = new EpgSearchKeyInfo();
                searchKeyView.GetSearchKey(ref searchKey);

                var recSetKey = new RecSettingData();
                recSettingView.GetRecSetting(ref recSetKey);

                addItem.searchInfo = searchKey;
                addItem.recSetting = recSetKey;

                //一覧画面非表示の状態から実施する場合のためのコード
                if (CommonManager.Instance.DB.EpgAutoAddList.Count == 0)
                {
                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                    CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                }

                if (mutil.EpgAutoAddAdd(mutil.ToList(addItem)) == true)
                {
                    List<uint> oldlist = CommonManager.Instance.DB.EpgAutoAddList.Keys.ToList();

                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                    CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                    
                    List<uint> newlist = CommonManager.Instance.DB.EpgAutoAddList.Keys.ToList();
                    List<uint> diflist = newlist.Except(oldlist).ToList();

                    if (diflist.Count == 1)
                    {
                        EpgAutoAddData newinfo = CommonManager.Instance.DB.EpgAutoAddList[diflist[0]];
                        this.SetViewMode(SearchMode.Change);
                        this.SetChgAutoAddID(newinfo.dataID);
                        
                        //情報の再読み込みは不要なはずだが、安全のため実行しておく
                        this.SetSearchDefKey(newinfo.searchInfo);
                        this.SetRecInfoDef(newinfo.recSetting);

                        mainWindow.autoAddView.epgAutoAddView.UpdateInfo();
                        UpdateEpgAutoAddViewSelection();
                    }
                    
                    RefreshAndSearch();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private bool CheckExistAutoAddItem()
        {
            bool retval = CommonManager.Instance.DB.EpgAutoAddList.ContainsKey(this.autoAddID);
            if (retval == false)
            {
                MessageBox.Show("項目がありません。\r\n" + "既に削除されています。\r\n" + "(別のEpgtimerによる操作など)");
                SetViewMode(SearchMode.NewAdd);
                this.autoAddID = 0;
            }
            return retval;
        }

        private void button_chg_epgAutoAdd_Click(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                if (this.autoAddID == 0) return;
                if (CmdExeUtil.IsDisplayKgMessage(e) == true)
                {
                    if (MessageBox.Show("自動予約登録を変更します。\r\nよろしいですか？", "変更の確認", MessageBoxButton.OKCancel) != MessageBoxResult.OK)
                    { return; }
                }
                if (CheckExistAutoAddItem() == false) return;
                if (CheckCautionMany() == false) return;

                var addItem = new EpgAutoAddData();
                addItem.dataID = autoAddID;
                var searchKey = new EpgSearchKeyInfo();
                searchKeyView.GetSearchKey(ref searchKey);

                var recSetKey = new RecSettingData();
                recSettingView.GetRecSetting(ref recSetKey);

                addItem.searchInfo = searchKey;
                addItem.recSetting = recSetKey;

                if (mutil.EpgAutoAddChange(mutil.ToList(addItem)) == true)
                {
                    RefreshAndSearch();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_del_epgAutoAdd_Click(object sender, ExecutedRoutedEventArgs e)
        {
            if (this.autoAddID == 0) return;
            if (CmdExeUtil.IsDisplayKgMessage(e) == true)
            {
                if (MessageBox.Show("この自動予約登録を削除します。\r\nよろしいですか？", "削除の確認", MessageBoxButton.OKCancel) != MessageBoxResult.OK)
                { return; }
            }
            if (CheckExistAutoAddItem() == false) return;

            if (mutil.EpgAutoAddDelete(mutil.ToList(autoAddID)) == true)
            {
                SetViewMode(SearchMode.NewAdd);
                this.autoAddID = 0;
            }
        }

        private void button_up_epgAutoAdd_Click(object sender, ExecutedRoutedEventArgs e)
        {
            Move_epgAutoAdd(-1);
        }
        private void button_down_epgAutoAdd_Click(object sender, ExecutedRoutedEventArgs e)
        {
            Move_epgAutoAdd(1);
        }
        private void Move_epgAutoAdd(int direction)
        {
            if (this.autoAddID == 0) return;

            ListView epglist = mainWindow.autoAddView.epgAutoAddView.listView_key;

            //念のため
            UpdateEpgAutoAddViewSelection();
            if (epglist.SelectedIndex == -1) return;

            epglist.SelectedIndex = ((epglist.SelectedIndex + direction) % epglist.Items.Count + epglist.Items.Count) % epglist.Items.Count;
            EpgAutoAddData newinfo = (epglist.SelectedItem as EpgAutoDataItem).EpgAutoAddInfo;

            this.SetChgAutoAddID(newinfo.dataID);
            this.SetSearchDefKey(newinfo.searchInfo);
            this.SetRecInfoDef(newinfo.recSetting);

            SearchPg();
        }

        private void listView_result_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            EpgCmds.ShowDialog.Execute(sender, null);
        }

        private void Window_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (this.WindowState == WindowState.Normal)
            {
                if (this.Visibility == System.Windows.Visibility.Visible && this.Width > 0 && this.Height > 0)
                {
                    Settings.Instance.SearchWndWidth = this.Width;
                    Settings.Instance.SearchWndHeight = this.Height;
                }
            }
        }

        private void Window_LocationChanged(object sender, EventArgs e)
        {
            if (this.WindowState == WindowState.Normal)
            {
                if (this.Visibility == System.Windows.Visibility.Visible && this.Top > 0 && this.Left > 0)
                {
                    Settings.Instance.SearchWndTop = this.Top;
                    Settings.Instance.SearchWndLeft = this.Left;
                }
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if ((winMode == SearchMode.Find || winMode == SearchMode.NewAdd) && 
                (searchKeyView.ComboBox_andKey.Text == null || searchKeyView.ComboBox_andKey.Text == ""))
            {
                this.searchKeyView.ComboBox_andKey.Focus();
            }
            else
            {
                this.SearchPg();
            }
        }

        private void mc_JumpTable(object sender, ExecutedRoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                BlackoutWindow.SelectedSearchItem = lstCtrl.SelectSingleItem();
                var mainWindow1 = this.Owner as MainWindow;
                if (mainWindow1 != null)
                {
                    if (BlackoutWindow.unvisibleSearchWindow != null)
                    {
                        // 非表示で保存するSearchWindowを1つに限定するため
                        this.Close();
                    }
                    else
                    {
                        this.Hide();
                        mainWindow1.EmphasizeSearchButton(true);
                        BlackoutWindow.unvisibleSearchWindow = this;
                    }
                    mainWindow1.moveTo_tabItem_epg();
                    mainWindow1.Hide(); // EpgDataView.UserControl_IsVisibleChangedイベントを発生させる
                    mainWindow1.Show();
                }
            }
        }

        private void Window_IsVisibleChanged_1(object sender, DependencyPropertyChangedEventArgs e)
        {
            var mainWindow1 = this.Owner as MainWindow;
            if (this.IsVisible)
            {
                if (BlackoutWindow.unvisibleSearchWindow == this)
                {
                    mainWindow1.EmphasizeSearchButton(false);
                    BlackoutWindow.unvisibleSearchWindow = null;
                }
            }
        }

        private void mc_Research(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                if (listView_result.SelectedItem != null)
                {
                    SearchItem item = lstCtrl.SelectSingleItem();

                    EpgSearchKeyInfo defKey = new EpgSearchKeyInfo();
                    searchKeyView.GetSearchKey(ref defKey);
                    defKey.andKey = mutil.TrimEpgKeyword(item.EventName, CmdExeUtil.IsKeyGesture(e));
                    defKey.regExpFlag = 0;
                    defKey.serviceList.Clear();
                    UInt64 sidKey = item.EventInfo.Create64Key();
                    defKey.serviceList.Add((Int64)sidKey);

                    if (e.Command == EpgCmds.ReSearch)
                    {
                        searchKeyView.SetSearchKey(defKey);
                        SearchPg();
                    }
                    else
                    {
                        //「番組名で再検索」と比べてどうなのという感じだが、元の検索を残したまま作業できる
                        //新番組チェックなんかには向いてるかもしれないが、機能としては微妙なところ。

                        var setInfo = new RecSettingData();
                        recSettingView.GetRecSetting(ref setInfo);

                        var dlg = new SearchWindow();
                        //SearchWindowからの呼び出しを記録する。表示制御などでも使う。
                        dlg.Owner = this;
                        dlg.SetViewMode(winMode == SearchMode.Change ? SearchMode.NewAdd : winMode);
                        dlg.SetSubWindow();
                        dlg.SetSearchDefKey(defKey);
                        dlg.SetRecInfoDef(setInfo);
                        //dlg.Left += 50;//なぜか動かせない‥
                        //dlg.Top += 50;
                        dlg.ShowDialog();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void Window_Closed(object sender, System.EventArgs e)
        {
            Settings.SaveToXmlFile();//検索ワードリストの保存
            if (this.Owner as SearchWindow != null)
            {
                (this.Owner as SearchWindow).SearchPg();
            }
            else
            {
                mainWindow.ListFoucsOnVisibleChanged();
            }
        }
        
        private void Window_Activated(object sender, EventArgs e)
        {
            UpdateEpgAutoAddViewSelection();
            RefreshMenu();
        }

        private void UpdateEpgAutoAddViewSelection()
        {
            mainWindow.autoAddView.epgAutoAddView.UpdateListViewSelection(this.autoAddID);
        }

    }
}

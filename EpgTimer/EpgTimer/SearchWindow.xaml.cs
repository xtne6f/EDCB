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
        private List<SearchItem> resultList = new List<SearchItem>();
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;

        private UInt32 autoAddID = 0;

        public SearchWindow()
        {
            InitializeComponent();

            try
            {
                if (Settings.Instance.NoStyle == 0)
                {
                    ResourceDictionary rd = new ResourceDictionary();
                    rd.MergedDictionaries.Add(
                        Application.LoadComponent(new Uri("/PresentationFramework.Aero, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aero.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                        //Application.LoadComponent(new Uri("/PresentationFramework.Classic, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35, ProcessorArchitecture=MSIL;component/themes/Classic.xaml", UriKind.Relative)) as ResourceDictionary
                        );
                    this.Resources = rd;
                }
                else
                {
                    button_search.Style = null;
                    button_add_reserve.Style = null;
                    button_delall_reserve.Style = null;
                    button_add_epgAutoAdd.Style = null;
                    button_chg_epgAutoAdd.Style = null;
                    button_del_epgAutoAdd.Style = null;
                    button_up_epgAutoAdd.Style = null;
                    button_down_epgAutoAdd.Style = null;
                }

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

                EpgSearchKeyInfo defKey = new EpgSearchKeyInfo();
                Settings.GetDefSearchSetting(ref defKey);

                searchKeyView.SetSearchKey(defKey);
            }
            catch
            {
            }
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

        public void SetViewMode(UInt16 mode)
        {
            if (mode == 1)//新規
            {
                Title = "EPG予約条件";
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_del_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_up_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_down_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
            }
            else if (mode == 2)//変更
            {
                Title = "EPG予約条件";
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
                button_del_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
                button_up_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
                button_down_epgAutoAdd.Visibility = System.Windows.Visibility.Visible;
            }
            else
            {
                Title = "検索";
                button_chg_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_del_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_up_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
                button_down_epgAutoAdd.Visibility = System.Windows.Visibility.Hidden;
            }
        }

        private void SetMode_UpDownButtons()
        {
            MainWindow mainWindow = (MainWindow)Application.Current.MainWindow;
            button_up_epgAutoAdd.IsEnabled = mainWindow.autoAddView.epgAutoAddView.IsVisible;
            button_down_epgAutoAdd.IsEnabled = mainWindow.autoAddView.epgAutoAddView.IsVisible;
        }

        public void SetChgAutoAddID(UInt32 id)
        {
            autoAddID = id;
        }

        private void tabItem_searchKey_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Enter)
            {
                SearchPg();
            }
        }

        private void button_search_Click(object sender, RoutedEventArgs e)
        {
            SearchPg();
        }

        private void RefreshAndSearch()
        {
            CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.ReserveInfo);
            CommonManager.Instance.DB.ReloadReserveInfo();
            SearchPg();
        }

        private void SearchPg()
        {
            try
            {
                //更新前の選択情報の保存
                var oldItems = new ListViewSelectedKeeper<SearchItem>(listView_result, true);

                listView_result.DataContext = null;
                resultList.Clear();

                EpgSearchKeyInfo key = new EpgSearchKeyInfo();
                searchKeyView.GetSearchKey(ref key);
                key.andKey = key.andKey.Substring(key.andKey.StartsWith("^!{999}") ? 7 : 0);
                List<EpgEventInfo> list = new List<EpgEventInfo>();

                cmd.SendSearchPg(mutil.GetList(key), ref list);

                foreach (EpgEventInfo info in list)
                {
                    if (info.start_time.AddSeconds(info.durationSec) > DateTime.Now)
                    {
                        resultList.Add(new SearchItem(info));
                    }
                }
                mutil.SetSearchItemReserved(resultList);

                if (this.gridViewSorter.IsExistSortParams)
                {
                    this.gridViewSorter.SortByMultiHeader(this.resultList);
                }
                else
                {
                    this.gridViewSorter.ResetSortParams();
                    this.gridViewSorter.SortByMultiHeaderWithKey(this.resultList, gridView_result.Columns, "StartTime");
                }
                listView_result.DataContext = resultList;

                searchKeyView.SaveSearchLog();

                //選択情報の復元
                oldItems.RestoreListViewSelected();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private List<SearchItem> GetSelectedItemsList()
        {
            return listView_result.SelectedItems.Cast<SearchItem>().ToList();
        }

        private void button_add_reserve_Click(object sender, RoutedEventArgs e)
        {
            add_reserve(recSettingView, null);//現在の設定で予約、簡易予約と同等
        }

        private void MenuItem_Click_AddReservePreset(object sender, RoutedEventArgs e)
        {
            add_reserve(null, sender);
        }

        private void add_reserve(RecSettingView localSetting = null, object sender = null)
        {
            mutil.ReserveAdd(GetSelectedItemsList(), localSetting, sender);
            RefreshAndSearch();
        }

        private void button_delall_reserve_Click(object sender, RoutedEventArgs e)
        {
            string text1 = "予約されている全ての項目を削除しますか?";
            string caption1 = "予約全削除の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) == MessageBoxResult.OK)
            {
                //更新前の選択情報の保存
                var oldItems = new ListViewSelectedKeeper<SearchItem>(listView_result, true);

                mutil.ReserveDelete(resultList);
                RefreshAndSearch();

                oldItems.RestoreListViewSelected();
            }
        }

        private void button_add_epgAutoAdd_Click(object sender, RoutedEventArgs e)
        {
            try
            {
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

                if (mutil.EpgAutoAddAdd(addItem) == true)
                {
                    List<uint> oldlist = CommonManager.Instance.DB.EpgAutoAddList.Keys.ToList();

                    CommonManager.Instance.DB.SetUpdateNotify((UInt32)UpdateNotifyItem.AutoAddEpgInfo);
                    CommonManager.Instance.DB.ReloadEpgAutoAddInfo();
                    
                    List<uint> newlist = CommonManager.Instance.DB.EpgAutoAddList.Keys.ToList();
                    List<uint> diflist = newlist.Except(oldlist).ToList();

                    if (diflist.Count == 1)
                    {
                        EpgAutoAddData newinfo = CommonManager.Instance.DB.EpgAutoAddList[diflist[0]];
                        this.SetViewMode(2);
                        this.SetChgAutoAddID(newinfo.dataID);
                        
                        //情報の再読み込みは不要なはずだが、安全のため実行しておく
                        this.SetSearchDefKey(newinfo.searchInfo);
                        this.SetRecInfoDef(newinfo.recSetting);

                        MainWindow mainWindow = (MainWindow)Application.Current.MainWindow;
                        mainWindow.autoAddView.epgAutoAddView.UpdateInfo();
                        UpdateEpgAutoAddViewSelection();
                    }
                    //

                    RefreshAndSearch();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_up_epgAutoAdd_Click(object sender, RoutedEventArgs e)
        {
            Move_epgAutoAdd(-1);
        }

        private void button_down_epgAutoAdd_Click(object sender, RoutedEventArgs e)
        {
            Move_epgAutoAdd(1);
        }

        private void Move_epgAutoAdd(int direction)
        {
            if (this.autoAddID == 0) return;

            MainWindow mainWindow = (MainWindow)Application.Current.MainWindow;
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

        private bool CheckExistAutoAddItem()
        {
            bool retval = CommonManager.Instance.DB.EpgAutoAddList.ContainsKey(this.autoAddID);
            if (retval == false)
            {
                MessageBox.Show("項目がありません。\r\n" +
                    "既に削除されています。\r\n" +
                    "(別のEpgtimerによる操作など)");
                SetViewMode(1);
                this.autoAddID = 0;
            }
            return retval;
        }

        private void button_chg_epgAutoAdd_Click(object sender, RoutedEventArgs e)
        {
            if (this.autoAddID == 0) return;
            if (CheckExistAutoAddItem() == false) return;

            try
            {
                var addItem = new EpgAutoAddData();
                addItem.dataID = autoAddID;
                var searchKey = new EpgSearchKeyInfo();
                searchKeyView.GetSearchKey(ref searchKey);

                var recSetKey = new RecSettingData();
                recSettingView.GetRecSetting(ref recSetKey);

                addItem.searchInfo = searchKey;
                addItem.recSetting = recSetKey;

                if (mutil.EpgAutoAddChange(addItem) == true)
                {
                    RefreshAndSearch();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_del_epgAutoAdd_Click(object sender, RoutedEventArgs e)
        {
            if (this.autoAddID == 0) return;
            if (CheckExistAutoAddItem() == false) return;

            if (mutil.EpgAutoAddDelete(autoAddID) == true)
            {
                SetViewMode(1);
                this.autoAddID = 0;
            }
        }

        private void listView_result_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            this.MenuItem_Click_ShowDialog(listView_result.SelectedItem, new RoutedEventArgs());
        }

        GridViewSorter<SearchItem> gridViewSorter = new GridViewSorter<SearchItem>();

        private void GridViewColumnHeader_Click(object sender, RoutedEventArgs e)
        {
            GridViewColumnHeader headerClicked = e.OriginalSource as GridViewColumnHeader;
            if (headerClicked != null)
            {
                if (headerClicked.Role != GridViewColumnHeaderRole.Padding)
                {
                    this.gridViewSorter.SortByMultiHeader(this.resultList, headerClicked);
                    listView_result.Items.Refresh();
                }
            }
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

        protected override void OnPreviewKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.Up:
                        //this.autoAddIDが有効なIDを持っていてもボタン不可の場合がある
                        if (this.button_up_epgAutoAdd.IsVisible == true && this.button_up_epgAutoAdd.IsEnabled == true)
                        {
                            this.button_up_epgAutoAdd.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        }
                        e.Handled = true;
                        break;
                    case Key.Down:
                        if (this.button_down_epgAutoAdd.IsVisible == true && this.button_down_epgAutoAdd.IsEnabled == true)
                        {
                            this.button_down_epgAutoAdd.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        }
                        e.Handled = true;
                        break;
                }
            }
            base.OnPreviewKeyDown(e);
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control) && Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
            {
                switch (e.Key)
                {
                    case Key.A:
                        if (MessageBox.Show("自動予約登録を追加します。\r\nよろしいですか？", "追加の確認", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                        {
                            this.button_add_epgAutoAdd.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        }
                        e.Handled = true;
                        break;
                    case Key.C:
                        if (this.button_chg_epgAutoAdd.IsVisible == true && this.button_chg_epgAutoAdd.IsEnabled == true)
                        {
                            if (MessageBox.Show("自動予約登録を変更します。\r\nよろしいですか？", "変更の確認", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                            {
                                this.button_chg_epgAutoAdd.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                            }
                        }
                        e.Handled = true;
                        break;
                    case Key.X:
                        if (this.button_del_epgAutoAdd.IsVisible == true && this.button_del_epgAutoAdd.IsEnabled == true)
                        {
                            if (MessageBox.Show("この自動予約登録を削除します。\r\nよろしいですか？", "削除の確認", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                            {
                                this.button_del_epgAutoAdd.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                            }
                        }
                        e.Handled = true;
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.F:
                        this.button_search.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Escape:
                        this.Close();
                        break;
                }
            }
            base.OnKeyDown(e);
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if (Title == "検索")
            {
                this.searchKeyView.ComboBox_andKey.Focus();
            }
            else
            {
                this.SearchPg();
            }
        }

        void listView_result_PreviewKeyDown(object sender, KeyEventArgs e)
        {
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control) && Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
            {
                switch (e.Key)
                {
                    case Key.D:
                        this.button_delall_reserve.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.Control)
            {
                switch (e.Key)
                {
                    case Key.D:
                        this.deleteItem();
                        break;
                    case Key.S:
                        this.MenuItem_Click_ChangeOnOff(this, new RoutedEventArgs(Button.ClickEvent));
                        break;
                    case Key.C:
                        this.CopyTitle2Clipboard();
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.F3:
                        this.MenuItem_Click_ProgramTable(this, new RoutedEventArgs(Button.ClickEvent));
                        e.Handled = true;
                        break;
                    case Key.Enter:
                        this.MenuItem_Click_ShowDialog(this, new RoutedEventArgs(Button.ClickEvent));
                        e.Handled = true;
                        break;
                    case Key.Delete:
                        this.deleteItem();
                        e.Handled = true;
                        break;
                }
            }
        }

        void deleteItem()
        {
            var delList = GetSelectedItemsList().ReserveInfoList();
            if (delList.Count == 0) { return; }

            string text1 = "削除しますか?　[削除アイテム数: " + delList.Count + "]" + "\r\n\r\n";
            delList.ForEach(item => text1 += " ・ " + item.Title + "\r\n");

            string caption1 = "登録項目削除の確認";
            if (MessageBox.Show(text1, caption1, MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.OK) == MessageBoxResult.OK)
            {
                this.MenuItem_Click_DeleteItem(this.listView_result.SelectedItem, new RoutedEventArgs(Button.ClickEvent));
            }
        }

        private void MenuItem_Click_DeleteItem(object sender, RoutedEventArgs e)
        {
            mutil.ReserveDelete(GetSelectedItemsList());
            RefreshAndSearch();
        }

        private void MenuItem_Click_ChangeOnOff(object sender, RoutedEventArgs e)
        {
            mutil.ReserveChangeOnOff(GetSelectedItemsList(), recSettingView);
            RefreshAndSearch();
        }

        private SearchItem SelectSingleItem()
        {
            return mutil.SelectSingleItem<SearchItem>(listView_result);
        }

        private void MenuItem_Click_ShowDialog(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                if (mutil.OpenSearchItemWithWindow(SelectSingleItem(), this) == true)
                {
                    RefreshAndSearch();
                }
            }
        }

        private void MenuItem_Click_RecMode(object sender, RoutedEventArgs e)
        {
            mutil.ReserveChangeRecmode(GetSelectedItemsList(), sender);
            RefreshAndSearch();
        }

        private void MenuItem_Click_Priority(object sender, RoutedEventArgs e)
        {
            mutil.ReserveChangePriority(GetSelectedItemsList(), sender);
            RefreshAndSearch();
        }

        private void MenuItem_Click_ProgramTable(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                SearchItem item = SelectSingleItem();
                BlackoutWindow.SelectedSearchItem = item;
                MainWindow mainWindow1 = this.Owner as MainWindow;
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
            MainWindow mainWindow1 = this.Owner as MainWindow;
            if (this.IsVisible)
            {
                if (BlackoutWindow.unvisibleSearchWindow == this)
                {
                    mainWindow1.EmphasizeSearchButton(false);
                    BlackoutWindow.unvisibleSearchWindow = null;
                }
            }
        }

        private void MenuItem_Click_Research(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listView_result.SelectedItem != null)
                {
                    SearchItem item = SelectSingleItem();

                    EpgSearchKeyInfo defKey = new EpgSearchKeyInfo();
                    searchKeyView.GetSearchKey(ref defKey);
                    defKey.andKey = mutil.TrimEpgKeyword(item.EventName);
                    defKey.regExpFlag = 0;
                    defKey.serviceList.Clear();
                    UInt64 sidKey = item.EventInfo.Create64Key();
                    defKey.serviceList.Add((Int64)sidKey);
                    searchKeyView.SetSearchKey(defKey);

                    SearchPg();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void MenuItem_Click_Research2(object sender, RoutedEventArgs e)
        {
            try
            {
                //「番組名で再検索」と比べてどうなのという感じだが、元の検索を残したまま作業できる
                //新番組チェックなんかには向いてるかもしれないが、機能としては微妙なところ。
                if (listView_result.SelectedItem != null)
                {
                    SearchItem item = SelectSingleItem();

                    EpgSearchKeyInfo defKey = new EpgSearchKeyInfo();
                    searchKeyView.GetSearchKey(ref defKey);
                    defKey.andKey = mutil.TrimEpgKeyword(item.EventName);
                    defKey.regExpFlag = 0;
                    defKey.serviceList.Clear();
                    UInt64 sidKey = item.EventInfo.Create64Key();
                    defKey.serviceList.Add((Int64)sidKey);

                    RecSettingData setInfo = new RecSettingData();
                    recSettingView.GetRecSetting(ref setInfo);

                    SearchWindow dlg = new SearchWindow();
                    //SearchWindowからの呼び出しを記録する。表示制御などでも使う。
                    dlg.Owner = this;
                    dlg.SetViewMode((ushort)(Title == "検索" ? 3 : 1));
                    dlg.Title += "(サブウィンドウ)";
                    dlg.SetSearchDefKey(defKey);
                    dlg.SetRecInfoDef(setInfo);
                    //dlg.Left += 50;//なぜか動かせない‥
                    //dlg.Top += 50;
                    dlg.ShowDialog();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void MenuItem_Click_CopyTitle(object sender, RoutedEventArgs e)
        {
            CopyTitle2Clipboard();
        }

        private void CopyTitle2Clipboard()
        {
            if (listView_result.SelectedItem != null)
            {
                SearchItem item = SelectSingleItem();
                mutil.CopyTitle2Clipboard(item.EventName);
            }
        }

        private void MenuItem_Click_CopyContent(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                SearchItem item = SelectSingleItem();
                mutil.CopyContent2Clipboard(item.EventInfo);
            }
        }

        private void MenuItem_Click_SearchTitle(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                SearchItem item = SelectSingleItem();
                mutil.SearchText(item.EventName);
            }
        }

        private void cmdMenu_Loaded(object sender, RoutedEventArgs e)
        {
            if (listView_result.SelectedItem != null)
            {
                try
                {
                    List<SearchItem> list = GetSelectedItemsList();
                    bool hasReserved = list.HasReserved();
                    bool hasNoReserved = list.HasNoReserved();

                    foreach (object item in ((ContextMenu)sender).Items)
                    {
                        //孫ウィンドウは禁止。番組表表示とも相性悪いのでキャンセル。
                        if (item is MenuItem && ((((MenuItem)item).Name == "cmdResearch2") || (((MenuItem)item).Name == "cmdProgramTable")))
                        {
                            if (this.Owner as SearchWindow != null)
                            {
                                ((MenuItem)item).IsEnabled = false;
                                ((MenuItem)item).Header += ((MenuItem)item).Header.ToString().EndsWith("(無効)") ? "" : "(無効)";
                            }
                        }

                        if (item is MenuItem && (((MenuItem)item).Name == "cmdDlt"))
                        {
                            ((MenuItem)item).IsEnabled = hasReserved;
                        }
                        else if (item is MenuItem && (((MenuItem)item).Name == "cmdAdd"))
                        {
                            ((MenuItem)item).IsEnabled = hasNoReserved;
                            if (hasNoReserved)
                            {
                                mutil.ExpandPresetItems((MenuItem)item, MenuItem_Click_AddReservePreset);
                            }
                        }
                        else if (item is MenuItem && ((MenuItem)item).Name == "cmdChg")
                        {
                            ((MenuItem)item).IsEnabled = hasReserved;
                            if (hasReserved)
                            {
                                mutil.CheckChgItems((MenuItem)item, list);//現在の状態(録画モード、優先度)にチェックを入れる
                            }
                        }
                        else if (mutil.AppendMenuVisibleControl(item)) { }
                        else { }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }
            }
        }

        private void Window_Closed(object sender, System.EventArgs e)
        {
            if (this.Owner as SearchWindow != null)
            {
                (this.Owner as SearchWindow).SearchPg();
            }
            else
            {
                MainWindow mainWindow = (MainWindow)Application.Current.MainWindow;
                mainWindow.ListFoucsOnVisibleChanged();
            }
        }
        
        private void Window_Activated(object sender, EventArgs e)
        {
            UpdateEpgAutoAddViewSelection();
            SetMode_UpDownButtons();
        }

        private void UpdateEpgAutoAddViewSelection()
        {
            MainWindow mainWindow = (MainWindow)Application.Current.MainWindow;
            mainWindow.autoAddView.epgAutoAddView.UpdateListViewSelection(this.autoAddID);
        }
    }
}

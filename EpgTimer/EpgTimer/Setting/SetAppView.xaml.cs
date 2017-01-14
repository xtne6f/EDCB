using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Interop;
using System.IO;
using System.Reflection;

namespace EpgTimer.Setting
{
    /// <summary>
    /// SetAppView.xaml の相互作用ロジック
    /// </summary>
    public partial class SetAppView : UserControl
    {
        private EpgSearchKeyInfo defSearchKey = new EpgSearchKeyInfo();

        private List<ViewMenuItem> buttonItem = new List<ViewMenuItem>();
        private List<ViewMenuItem> taskItem = new List<ViewMenuItem>();

        private Dictionary<UInt64, ServiceViewItem> serviceList = new Dictionary<UInt64, ServiceViewItem>();
        private List<IEPGStationInfo> stationList = new List<IEPGStationInfo>();

        public SetAppView()
        {
            InitializeComponent();

            if (CommonManager.Instance.NWMode == true)
            {
                checkBox_wakeReconnect.IsEnabled = true;
                checkBox_suspendClose.IsEnabled = true;
                checkBox_ngAutoEpgLoad.IsEnabled = true;
                button_srvSetting.IsEnabled = false;
            }

            try
            {
                try
                {
                    checkBox_closeMin.IsChecked = Settings.Instance.CloseMin;
                    checkBox_minWake.IsChecked = Settings.Instance.WakeMin;
                    checkBox_noToolTips.IsChecked = Settings.Instance.NoToolTip;
                    checkBox_noBallonTips.IsChecked = Settings.Instance.NoBallonTips;
                    checkBox_playDClick.IsChecked = Settings.Instance.PlayDClick;
                    checkBox_showTray.IsChecked = Settings.Instance.ShowTray;
                    checkBox_minHide.IsChecked = Settings.Instance.MinHide;

                    checkBox_wakeReconnect.IsChecked = Settings.Instance.WakeReconnectNW;
                    checkBox_suspendClose.IsChecked = Settings.Instance.SuspendCloseNW;
                    checkBox_ngAutoEpgLoad.IsChecked = Settings.Instance.NgAutoEpgLoadNW;
                }
                catch
                {
                }

                button_shortCutAdd.Visibility = File.Exists(System.IO.Path.Combine(
                    Environment.GetFolderPath(Environment.SpecialFolder.Startup), "EpgTime.lnk")) ? Visibility.Hidden : Visibility.Visible;
                button_shortCutDel.Visibility = button_shortCutAdd.Visibility == Visibility.Visible ? Visibility.Hidden : Visibility.Visible;

                Settings.GetDefSearchSetting(ref defSearchKey);

                checkBox_showAsTab.IsChecked = Settings.Instance.ViewButtonShowAsTab;

                buttonItem.Add(new ViewMenuItem("（空白）", false));
                buttonItem.Add(new ViewMenuItem("設定", false));
                if (CommonManager.Instance.NWMode == true)
                {
                    buttonItem.Add(new ViewMenuItem("再接続", false));
                }
                buttonItem.Add(new ViewMenuItem("検索", false));
                buttonItem.Add(new ViewMenuItem("スタンバイ", false));
                buttonItem.Add(new ViewMenuItem("休止", false));
                buttonItem.Add(new ViewMenuItem("EPG取得", false));
                buttonItem.Add(new ViewMenuItem("EPG再読み込み", false));
                buttonItem.Add(new ViewMenuItem("終了", false));
                buttonItem.Add(new ViewMenuItem("カスタム１", false));
                buttonItem.Add(new ViewMenuItem("カスタム２", false));
                buttonItem.Add(new ViewMenuItem("NetworkTV終了", false));
                buttonItem.Add(new ViewMenuItem("情報通知ログ", false));

                taskItem.Add(new ViewMenuItem("（セパレータ）", false));
                taskItem.Add(new ViewMenuItem("設定", false));
                if (CommonManager.Instance.NWMode == true)
                {
                    taskItem.Add(new ViewMenuItem("再接続", false));
                }
                taskItem.Add(new ViewMenuItem("スタンバイ", false));
                taskItem.Add(new ViewMenuItem("休止", false));
                taskItem.Add(new ViewMenuItem("EPG取得", false));
                taskItem.Add(new ViewMenuItem("終了", false));

                foreach (String info in Settings.Instance.ViewButtonList)
                {
                    //リストが空であることを示す特殊なアイテムを無視
                    if (String.Compare(info, "（なし）") == 0)
                    {
                        continue;
                    }
                    //.NET的に同一文字列のStringを入れると選択動作がおかしくなるみたいなので毎回作成しておく
                    listBox_viewBtn.Items.Add(new ViewMenuItem(info, true));
                    if (String.Compare(info, "（空白）") != 0)
                    {
                        foreach (ViewMenuItem item in buttonItem)
                        {
                            if (String.Compare(info, item.MenuName) == 0)
                            {
                                item.IsSelected = true;
                                break;
                            }
                        }
                    }
                }
                foreach (String info in Settings.Instance.TaskMenuList)
                {
                    //.NET的に同一文字列のStringを入れると選択動作がおかしくなるみたいなので毎回作成しておく
                    listBox_viewTask.Items.Add(new ViewMenuItem(info, true));
                    if (String.Compare(info, "（セパレータ）") != 0)
                    {
                        foreach (ViewMenuItem item in taskItem)
                        {
                            if (String.Compare(info, item.MenuName) == 0)
                            {
                                item.IsSelected = true;
                                break;
                            }
                        }
                    }
                }

                ReLoadButtonItem();
                ReLoadTaskItem();

                textBox_name1.Text = Settings.Instance.Cust1BtnName;
                textBox_exe1.Text = Settings.Instance.Cust1BtnCmd;
                textBox_opt1.Text = Settings.Instance.Cust1BtnCmdOpt;

                textBox_name2.Text = Settings.Instance.Cust2BtnName;
                textBox_exe2.Text = Settings.Instance.Cust2BtnCmd;
                textBox_opt2.Text = Settings.Instance.Cust2BtnCmdOpt;

                foreach (ChSet5Item info in ChSet5.Instance.ChList.Values)
                {
                    ServiceViewItem item = new ServiceViewItem(info);
                    serviceList.Add(item.Key, item);
                }
                listBox_service.ItemsSource = serviceList.Values;

                stationList = Settings.Instance.IEpgStationList.ToList();
                ReLoadStation();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void ReLoadButtonItem()
        {
            listBox_itemBtn.Items.Clear();
            foreach (ViewMenuItem info in buttonItem)
            {
                if (info.IsSelected == false)
                {
                    listBox_itemBtn.Items.Add(info);
                }
            }
        }

        private void ReLoadTaskItem()
        {
            listBox_itemTask.Items.Clear();
            foreach (ViewMenuItem info in taskItem)
            {
                if (info.IsSelected == false)
                {
                    listBox_itemTask.Items.Add(info);
                }
            }
        }

        public void SaveSetting()
        {
            Settings.Instance.CloseMin = (bool)checkBox_closeMin.IsChecked;
            Settings.Instance.WakeMin = (bool)checkBox_minWake.IsChecked;
            Settings.Instance.ShowTray = (bool)checkBox_showTray.IsChecked;
            Settings.Instance.MinHide = (bool)checkBox_minHide.IsChecked;

            if (checkBox_noToolTips.IsChecked == true)
            {
                Settings.Instance.NoToolTip = true;
            }
            else
            {
                Settings.Instance.NoToolTip = false;
            }

            if (checkBox_noBallonTips.IsChecked == true)
            {
                Settings.Instance.NoBallonTips = true;
            }
            else
            {
                Settings.Instance.NoBallonTips = false;
            }
            if (checkBox_playDClick.IsChecked == true)
            {
                Settings.Instance.PlayDClick = true;
            }
            else
            {
                Settings.Instance.PlayDClick = false;
            }

            if (checkBox_wakeReconnect.IsChecked == true)
            {
                Settings.Instance.WakeReconnectNW = true;
            }
            else
            {
                Settings.Instance.WakeReconnectNW = false;
            }
            if (checkBox_suspendClose.IsChecked == true)
            {
                Settings.Instance.SuspendCloseNW = true;
            }
            else
            {
                Settings.Instance.SuspendCloseNW = false;
            }
            if (checkBox_ngAutoEpgLoad.IsChecked == true)
            {
                Settings.Instance.NgAutoEpgLoadNW = true;
            }
            else
            {
                Settings.Instance.NgAutoEpgLoadNW = false;
            }


            if (defSearchKey.regExpFlag == 0)
            {
                Settings.Instance.SearchKeyRegExp = false;
            }
            else
            {
                Settings.Instance.SearchKeyRegExp = true;
            }
            if (defSearchKey.aimaiFlag == 0)
            {
                Settings.Instance.SearchKeyAimaiFlag = false;
            }
            else
            {
                Settings.Instance.SearchKeyAimaiFlag = true;
            }
            if (defSearchKey.titleOnlyFlag == 0)
            {
                Settings.Instance.SearchKeyTitleOnly = false;
            }
            else
            {
                Settings.Instance.SearchKeyTitleOnly = true;
            }
            if (defSearchKey.titleOnlyFlag == 0)
            {
                Settings.Instance.SearchKeyTitleOnly = false;
            }
            else
            {
                Settings.Instance.SearchKeyTitleOnly = true;
            }
            Settings.Instance.SearchKeyContentList.Clear();
            foreach (EpgContentData info in defSearchKey.contentList)
            {
                ContentKindInfo item = new ContentKindInfo();
                item.Nibble1 = info.content_nibble_level_1;
                item.Nibble2 = info.content_nibble_level_2;
                Settings.Instance.SearchKeyContentList.Add(item);
            }
            Settings.Instance.SearchKeyDateItemList.Clear();
            foreach (EpgSearchDateInfo info in defSearchKey.dateList)
            {
                DateItem item = new DateItem();
                item.DateInfo = info;
                Settings.Instance.SearchKeyDateItemList.Add(item);
            }
            Settings.Instance.SearchKeyServiceList.Clear();
            foreach (Int64 info in defSearchKey.serviceList)
            {
                Settings.Instance.SearchKeyServiceList.Add(info);
            }

            if (defSearchKey.notContetFlag == 0)
            {
                Settings.Instance.SearchKeyNotContent = false;
            }
            else
            {
                Settings.Instance.SearchKeyNotContent = true;
            }
            if (defSearchKey.notDateFlag == 0)
            {
                Settings.Instance.SearchKeyNotDate = false;
            }
            else
            {
                Settings.Instance.SearchKeyNotDate = true;
            }
            Settings.Instance.SearchKeyFreeCA = defSearchKey.freeCAFlag;
            Settings.Instance.SearchKeyChkRecEnd = defSearchKey.chkRecEnd;
            Settings.Instance.SearchKeyChkRecDay = defSearchKey.chkRecDay;

            Settings.Instance.ViewButtonShowAsTab = checkBox_showAsTab.IsChecked == true;
            Settings.Instance.ViewButtonList.Clear();
            foreach (ViewMenuItem info in listBox_viewBtn.Items)
            {
                Settings.Instance.ViewButtonList.Add(info.MenuName);
            }
            if (Settings.Instance.ViewButtonList.Count == 0)
            {
                //リストが空であることを示す特殊なアイテムを追加
                Settings.Instance.ViewButtonList.Add("（なし）");
            }

            Settings.Instance.TaskMenuList.Clear();
            foreach (ViewMenuItem info in listBox_viewTask.Items)
            {
                Settings.Instance.TaskMenuList.Add(info.MenuName);
            }

            Settings.Instance.Cust1BtnName = textBox_name1.Text;
            Settings.Instance.Cust1BtnCmd = textBox_exe1.Text;
            Settings.Instance.Cust1BtnCmdOpt = textBox_opt1.Text;

            Settings.Instance.Cust2BtnName = textBox_name2.Text;
            Settings.Instance.Cust2BtnCmd = textBox_exe2.Text;
            Settings.Instance.Cust2BtnCmdOpt = textBox_opt2.Text;

            Settings.Instance.IEpgStationList = stationList.ToList();
        }

        private void button_btnUp_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_viewBtn.SelectedItem != null)
            {
                if (listBox_viewBtn.SelectedIndex >= 1)
                {
                    object temp = listBox_viewBtn.SelectedItem;
                    int index = listBox_viewBtn.SelectedIndex;
                    listBox_viewBtn.Items.RemoveAt(listBox_viewBtn.SelectedIndex);
                    listBox_viewBtn.Items.Insert(index - 1, temp);
                    listBox_viewBtn.SelectedIndex = index - 1;
                }
            }
        }

        private void button_btnDel_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_viewBtn.SelectedItem != null)
            {
                ViewMenuItem info = listBox_viewBtn.SelectedItem as ViewMenuItem;
                if (String.Compare(info.MenuName, "設定") == 0)
                {
                    bool found = false;
                    foreach (ViewMenuItem item in listBox_viewTask.Items)
                    {
                        if ((found = item.MenuName == "設定") != false)
                        {
                            break;
                        }
                    }
                    if (!found)
                    {
                        MessageBox.Show("設定は上部表示ボタンか右クリック表示項目のどちらかに必要です");
                        return;
                    }
                }
                if (String.Compare(info.MenuName, "（空白）") != 0)
                {
                    foreach (ViewMenuItem item in buttonItem)
                    {
                        if (String.Compare(info.MenuName, item.MenuName) == 0)
                        {
                            item.IsSelected = false;
                            break;
                        }
                    }
                }
                listBox_viewBtn.Items.Remove(info);
                ReLoadButtonItem();
            }
        }

        private void button_btnAdd_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_itemBtn.SelectedItem != null)
            {
                ViewMenuItem info = listBox_itemBtn.SelectedItem as ViewMenuItem;
                if (String.Compare(info.MenuName, "（空白）") != 0)
                {
                    info.IsSelected = true;
                }
                listBox_viewBtn.Items.Add(new ViewMenuItem(info.MenuName, true));
                ReLoadButtonItem();
            }
        }

        private void button_btnDown_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_viewBtn.SelectedItem != null)
            {
                if (listBox_viewBtn.SelectedIndex < listBox_viewBtn.Items.Count - 1)
                {
                    object temp = listBox_viewBtn.SelectedItem;
                    int index = listBox_viewBtn.SelectedIndex;
                    listBox_viewBtn.Items.RemoveAt(listBox_viewBtn.SelectedIndex);
                    listBox_viewBtn.Items.Insert(index + 1, temp);
                    listBox_viewBtn.SelectedIndex = index + 1;
                }
            }
        }

        private void button_taskUp_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_viewTask.SelectedItem != null)
            {
                if (listBox_viewTask.SelectedIndex >= 1)
                {
                    object temp = listBox_viewTask.SelectedItem;
                    int index = listBox_viewTask.SelectedIndex;
                    listBox_viewTask.Items.RemoveAt(listBox_viewTask.SelectedIndex);
                    listBox_viewTask.Items.Insert(index - 1, temp);
                    listBox_viewTask.SelectedIndex = index - 1;
                }
            }
        }

        private void button_taskDel_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_viewTask.SelectedItem != null)
            {
                ViewMenuItem info = listBox_viewTask.SelectedItem as ViewMenuItem;
                if (String.Compare(info.MenuName, "設定") == 0)
                {
                    bool found = false;
                    foreach (ViewMenuItem item in listBox_viewBtn.Items)
                    {
                        if ((found = item.MenuName == "設定") != false)
                        {
                            break;
                        }
                    }
                    if (!found)
                    {
                        MessageBox.Show("設定は上部表示ボタンか右クリック表示項目のどちらかに必要です");
                        return;
                    }
                }
                if (String.Compare(info.MenuName, "（セパレータ）") != 0)
                {
                    foreach (ViewMenuItem item in taskItem)
                    {
                        if (String.Compare(info.MenuName, item.MenuName) == 0)
                        {
                            item.IsSelected = false;
                            break;
                        }
                    }
                }
                listBox_viewTask.Items.Remove(info);
                ReLoadTaskItem();
            }
        }

        private void button_taskAdd_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_itemTask.SelectedItem != null)
            {
                ViewMenuItem info = listBox_itemTask.SelectedItem as ViewMenuItem;
                if (String.Compare(info.MenuName, "（セパレータ）") != 0)
                {
                    info.IsSelected = true;
                }
                listBox_viewTask.Items.Add(new ViewMenuItem(info.MenuName, true));
                ReLoadTaskItem();
            }
        }

        private void button_taskDown_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_viewTask.SelectedItem != null)
            {
                if (listBox_viewTask.SelectedIndex < listBox_viewTask.Items.Count - 1)
                {
                    object temp = listBox_viewTask.SelectedItem;
                    int index = listBox_viewTask.SelectedIndex;
                    listBox_viewTask.Items.RemoveAt(listBox_viewTask.SelectedIndex);
                    listBox_viewTask.Items.Insert(index + 1, temp);
                    listBox_viewTask.SelectedIndex = index + 1;
                }
            }
        }

        private void button_recDef_Click(object sender, RoutedEventArgs e)
        {
            SetDefRecSettingWindow dlg = new SetDefRecSettingWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            dlg.ShowDialog();
        }

        private void button_searchDef_Click(object sender, RoutedEventArgs e)
        {
            SetDefSearchSettingWindow dlg = new SetDefSearchSettingWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            dlg.SetDefSetting(defSearchKey);

            if (dlg.ShowDialog() == true)
            {
                dlg.GetSetting(ref defSearchKey);
            }
        }

        private void button_shortCutAdd_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                string shortcutPath = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Startup), "EpgTime.lnk");
                if (File.Exists(shortcutPath) == false)
                {
                    CreateShortCut(shortcutPath, Assembly.GetEntryAssembly().Location, "");
                }
                button_shortCutAdd.Visibility = Visibility.Hidden;
                button_shortCutDel.Visibility = Visibility.Visible;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_shortCutDel_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                string shortcutPath = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Startup), "EpgTime.lnk");
                if (File.Exists(shortcutPath))
                {
                    File.Delete(shortcutPath);
                }
                button_shortCutAdd.Visibility = Visibility.Visible;
                button_shortCutDel.Visibility = Visibility.Hidden;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        [System.Runtime.InteropServices.DllImport("user32.dll")]
        private static extern bool SetForegroundWindow(IntPtr hWnd);

        private void button_srvSetting_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (CommonManager.Instance.SrvSettingProcess == null || CommonManager.Instance.SrvSettingProcess.HasExited)
                {
                    CommonManager.Instance.SrvSettingProcess =
                        System.Diagnostics.Process.Start(System.IO.Path.Combine(SettingPath.ModulePath, "EpgTimerSrv.exe"), "/setting");
                }
                else
                {
                    SetForegroundWindow(CommonManager.Instance.SrvSettingProcess.MainWindowHandle);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        /// <summary>
        /// ショートカットの作成
        /// </summary>
        /// <remarks>WSHを使用して、ショートカット(lnkファイル)を作成します。(遅延バインディング)</remarks>
        /// <param name="path">出力先のファイル名(*.lnk)</param>
        /// <param name="targetPath">対象のアセンブリ(*.exe)</param>
        /// <param name="description">説明</param>
        private void CreateShortCut(String path, String targetPath, String description)
        {
            //using System.Reflection;

            // WSHオブジェクトを作成し、CreateShortcutメソッドを実行する
            Type shellType = Type.GetTypeFromProgID("WScript.Shell");
            object shell = Activator.CreateInstance(shellType);
            object shortCut = shellType.InvokeMember("CreateShortcut", BindingFlags.InvokeMethod, null, shell, new object[] { path });

            Type shortcutType = shell.GetType();
            // TargetPathプロパティをセットする
            shortcutType.InvokeMember("TargetPath", BindingFlags.SetProperty, null, shortCut, new object[] { targetPath });
            shortcutType.InvokeMember("WorkingDirectory", BindingFlags.SetProperty, null, shortCut, new object[] { System.IO.Path.GetDirectoryName(targetPath) });
            // Descriptionプロパティをセットする
            shortcutType.InvokeMember("Description", BindingFlags.SetProperty, null, shortCut, new object[] { description });
            // Saveメソッドを実行する
            shortcutType.InvokeMember("Save", BindingFlags.InvokeMethod, null, shortCut, null);
        }

        private void button_exe1_Click(object sender, RoutedEventArgs e)
        {
            Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();
            dlg.DefaultExt = ".exe";
            dlg.Filter = "exe Files (.exe)|*.exe;|all Files(*.*)|*.*";

            Nullable<bool> result = dlg.ShowDialog();
            if (result == true)
            {
                textBox_exe1.Text = dlg.FileName;
            }
        }

        private void button_exe2_Click(object sender, RoutedEventArgs e)
        {
            Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();
            dlg.DefaultExt = ".exe";
            dlg.Filter = "exe Files (.exe)|*.exe;|all Files(*.*)|*.*";

            Nullable<bool> result = dlg.ShowDialog();
            if (result == true)
            {
                textBox_exe2.Text = dlg.FileName;
            }
        }

        private void ReLoadStation()
        {
            listBox_iEPG.Items.Clear();
            if (listBox_service.SelectedItem != null)
            {
                ServiceViewItem item = listBox_service.SelectedItem as ServiceViewItem;
                foreach (IEPGStationInfo info in stationList)
                {
                    if (info.Key == item.Key)
                    {
                        listBox_iEPG.Items.Add(info);
                    }
                }
            }
        }

        private void button_add_iepg_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_service.SelectedItem != null)
            {
                ServiceViewItem item = listBox_service.SelectedItem as ServiceViewItem;
                foreach (IEPGStationInfo info in stationList)
                {
                    if (String.Compare(info.StationName, textBox_station.Text) == 0)
                    {
                        MessageBox.Show("すでに登録済みです");
                        return;
                    }
                }
                IEPGStationInfo addItem = new IEPGStationInfo();
                addItem.StationName = textBox_station.Text;
                addItem.Key = item.Key;

                stationList.Add(addItem);

                ReLoadStation();
            }
        }

        private void button_del_iepg_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_iEPG.SelectedItem != null)
            {
                IEPGStationInfo item = listBox_iEPG.SelectedItem as IEPGStationInfo;
                stationList.Remove(item);
                ReLoadStation();
            }
        }

        private void listBox_service_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ReLoadStation();
        }
    }
}

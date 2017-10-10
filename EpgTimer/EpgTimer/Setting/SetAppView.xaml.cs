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
        public SetAppView()
        {
            InitializeComponent();
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            checkBox_wakeReconnect.IsEnabled = CommonManager.Instance.NWMode;
            checkBox_suspendClose.IsEnabled = CommonManager.Instance.NWMode;
            checkBox_ngAutoEpgLoad.IsEnabled = CommonManager.Instance.NWMode;
            button_srvSetting.IsEnabled = CommonManager.Instance.NWMode == false;

            button_shortCutAdd.Visibility = File.Exists(System.IO.Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.Startup), "EpgTime.lnk")) ? Visibility.Hidden : Visibility.Visible;
            button_shortCutDel.Visibility = button_shortCutAdd.Visibility == Visibility.Visible ? Visibility.Hidden : Visibility.Visible;

            var settings = (Settings)DataContext;
            if (settings != null)
            {
                listBox_viewBtn.Items.Clear();
                foreach (string item in settings.ViewButtonList)
                {
                    // リストが空であることを示す特殊なアイテムを無視
                    if (item != "（なし）")
                    {
                        listBox_viewBtn.Items.Add(item);
                    }
                }
                OnUpdateViewButtonListBox(true);

                listBox_viewTask.Items.Clear();
                foreach (string item in settings.TaskMenuList)
                {
                    listBox_viewTask.Items.Add(item);
                }
                OnUpdateViewTaskListBox(true);
            }

            listBox_service.ItemsSource = ChSet5.Instance.ChListSelected.Select(a => new ServiceViewItem(a));
        }

        public void SaveSetting()
        {
        }

        private void OnUpdateViewButtonListBox(bool updateAll)
        {
            var itemList = new List<string> {
                "（空白）", "設定", "検索", "スタンバイ", "休止", "EPG取得", "EPG再読み込み", "終了",
                "カスタム１" , "カスタム２", "NetworkTV終了", "情報通知ログ"
            };
            if (CommonManager.Instance.NWMode)
            {
                itemList.Add("再接続");
            }
            // コンテキストを更新する
            var viewButtonList = ((Settings)DataContext).ViewButtonList;
            viewButtonList.Clear();
            viewButtonList.AddRange(listBox_viewBtn.Items.OfType<string>());
            if (viewButtonList.Count == 0)
            {
                // リストが空であることを示す特殊なアイテムを追加
                viewButtonList.Add("（なし）");
            }
            if (updateAll)
            {
                listBox_itemBtn.Items.Clear();
                foreach (string item in itemList)
                {
                    // 表示項目にないものだけ追加
                    if (item == "（空白）" || viewButtonList.IndexOf(item) < 0)
                    {
                        listBox_itemBtn.Items.Add(item);
                    }
                }
            }
        }

        private void OnUpdateViewTaskListBox(bool updateAll)
        {
            var itemList = new List<string> {
                "（セパレータ）", "設定", "スタンバイ", "休止", "EPG取得", "終了"
            };
            if (CommonManager.Instance.NWMode)
            {
                itemList.Add("再接続");
            }
            // コンテキストを更新する
            var taskMenuList = ((Settings)DataContext).TaskMenuList;
            taskMenuList.Clear();
            taskMenuList.AddRange(listBox_viewTask.Items.OfType<string>());
            if (updateAll)
            {
                listBox_itemTask.Items.Clear();
                foreach (string item in itemList)
                {
                    // 表示項目にないものだけ追加
                    if (item == "（セパレータ）" || taskMenuList.IndexOf(item) < 0)
                    {
                        listBox_itemTask.Items.Add(item);
                    }
                }
            }
        }

        private void button_up_Click(object sender, RoutedEventArgs e)
        {
            var listBox = (ListBox)((Button)sender).Tag;
            int index = listBox.SelectedIndex;
            if (index >= 1)
            {
                listBox.Items.Insert(index - 1, listBox.SelectedItem);
                listBox.Items.RemoveAt(index + 1);
                listBox.SelectedIndex = index - 1;
                OnUpdateViewButtonListBox(false);
                OnUpdateViewTaskListBox(false);
            }
        }

        private void button_down_Click(object sender, RoutedEventArgs e)
        {
            var listBox = (ListBox)((Button)sender).Tag;
            int index = listBox.SelectedIndex;
            if (0 <= index && index < listBox.Items.Count - 1)
            {
                listBox.Items.Insert(index + 2, listBox.SelectedItem);
                listBox.Items.RemoveAt(index);
                listBox.SelectedIndex = index + 1;
                OnUpdateViewButtonListBox(false);
                OnUpdateViewTaskListBox(false);
            }
        }

        private void button_btnDel_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_viewBtn.SelectedItem != null)
            {
                if ((string)listBox_viewBtn.SelectedItem == "設定")
                {
                    if (listBox_viewTask.Items.OfType<string>().All(a => a != "設定"))
                    {
                        MessageBox.Show("設定は上部表示ボタンか右クリック表示項目のどちらかに必要です");
                        return;
                    }
                }
                listBox_viewBtn.Items.RemoveAt(listBox_viewBtn.SelectedIndex);
                OnUpdateViewButtonListBox(true);
            }
        }

        private void button_btnAdd_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_itemBtn.SelectedItem != null)
            {
                listBox_viewBtn.Items.Add(listBox_itemBtn.SelectedItem);
                OnUpdateViewButtonListBox(true);
            }
        }

        private void button_taskDel_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_viewTask.SelectedItem != null)
            {
                if ((string)listBox_viewTask.SelectedItem == "設定")
                {
                    if (listBox_viewBtn.Items.OfType<string>().All(a => a != "設定"))
                    {
                        MessageBox.Show("設定は上部表示ボタンか右クリック表示項目のどちらかに必要です");
                        return;
                    }
                }
                listBox_viewTask.Items.RemoveAt(listBox_viewTask.SelectedIndex);
                OnUpdateViewTaskListBox(true);
            }
        }

        private void button_taskAdd_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_itemTask.SelectedItem != null)
            {
                listBox_viewTask.Items.Add(listBox_itemTask.SelectedItem);
                OnUpdateViewTaskListBox(true);
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
            var defSearchKey = new EpgSearchKeyInfo();
            ((Settings)DataContext).GetDefSearchSetting(defSearchKey);
            dlg.SetDefSetting(defSearchKey);

            if (dlg.ShowDialog() == true)
            {
                dlg.GetSetting(ref defSearchKey);
                var settings = (Settings)DataContext;
                settings.SearchKeyRegExp = defSearchKey.regExpFlag != 0;
                settings.SearchKeyAimaiFlag = defSearchKey.aimaiFlag != 0;
                settings.SearchKeyTitleOnly = defSearchKey.titleOnlyFlag != 0;
                settings.SearchKeyContentList.Clear();
                foreach (EpgContentData info in defSearchKey.contentList)
                {
                    var item = new ContentKindInfo();
                    item.Nibble1 = info.content_nibble_level_1;
                    item.Nibble2 = info.content_nibble_level_2;
                    settings.SearchKeyContentList.Add(item);
                }
                settings.SearchKeyDateItemList.Clear();
                foreach (EpgSearchDateInfo info in defSearchKey.dateList)
                {
                    var item = new DateItem();
                    item.DateInfo = info;
                    settings.SearchKeyDateItemList.Add(item);
                }
                settings.SearchKeyServiceList.Clear();
                settings.SearchKeyServiceList.AddRange(defSearchKey.serviceList);
                settings.SearchKeyNotContent = defSearchKey.notContetFlag != 0;
                settings.SearchKeyNotDate = defSearchKey.notDateFlag != 0;
                settings.SearchKeyFreeCA = defSearchKey.freeCAFlag;
                settings.SearchKeyChkRecEnd = defSearchKey.chkRecEnd;
                settings.SearchKeyChkRecDay = defSearchKey.chkRecDay;
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
                textBox_exe1.Focus();
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
                textBox_exe2.Focus();
                textBox_exe2.Text = dlg.FileName;
            }
        }

        private void ReLoadStation()
        {
            listBox_iEPG.Items.Clear();
            if (listBox_service.SelectedItem != null)
            {
                ServiceViewItem item = listBox_service.SelectedItem as ServiceViewItem;
                foreach (IEPGStationInfo info in ((Settings)DataContext).IEpgStationList)
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
                foreach (IEPGStationInfo info in ((Settings)DataContext).IEpgStationList)
                {
                    if (string.Compare(info.StationName, textBox_station.Text, new System.Globalization.CultureInfo("ja-JP"),
                                       System.Globalization.CompareOptions.IgnoreWidth | System.Globalization.CompareOptions.IgnoreCase) == 0)
                    {
                        MessageBox.Show("すでに登録済みです");
                        return;
                    }
                }
                IEPGStationInfo addItem = new IEPGStationInfo();
                addItem.StationName = textBox_station.Text;
                addItem.Key = item.Key;

                ((Settings)DataContext).IEpgStationList.Add(addItem);

                ReLoadStation();
            }
        }

        private void button_del_iepg_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_iEPG.SelectedItem != null)
            {
                IEPGStationInfo item = listBox_iEPG.SelectedItem as IEPGStationInfo;
                ((Settings)DataContext).IEpgStationList.Remove(item);
                ReLoadStation();
            }
        }

        private void listBox_service_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            ReLoadStation();
        }
    }
}

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
using System.IO;
using System.Collections.ObjectModel;
using System.Reflection;
using System.Runtime.InteropServices;

namespace EpgTimer.Setting
{
    /// <summary>
    /// SetBasicView.xaml の相互作用ロジック
    /// </summary>
    public partial class SetBasicView : UserControl
    {
        private ObservableCollection<EpgCaptime> timeList = new ObservableCollection<EpgCaptime>();
        private List<ServiceViewItem> serviceList;

        public bool IsChangeSettingPath { get; private set; }

        public SetBasicView()
        {
            InitializeComponent();

            IsChangeSettingPath = false;

            if (CommonManager.Instance.NWMode == true)
            {
                CommonManager.Instance.VUtil.ChangeChildren(grid_folder, false);
                label1.IsEnabled = true;
                textBox_setPath.IsEnabled = true;
                button_setPath.IsEnabled = true;
                label3.IsEnabled = true;
                listBox_recFolder.IsEnabled = true;
                label4.IsEnabled = true;
                button_shortCut.IsEnabled = true;
                label5.IsEnabled = true;
                CommonManager.Instance.VUtil.DisableControlChildren(tabItem2);
                grid_tuner.IsEnabled = true;
                CommonManager.Instance.VUtil.ChangeChildren(grid_tuner, false);
                listBox_bon.IsEnabled = true;
                CommonManager.Instance.VUtil.DisableControlChildren(tabItem3);
            }

            listBox_Button_Set();

            try
            {
                textBox_setPath.Text = SettingPath.SettingFolderPath;
                textBox_exe.Text = SettingPath.EdcbExePath;

                string viewAppIniPath = SettingPath.ModulePath.TrimEnd('\\') + "\\ViewApp.ini";
                textBox_cmdBon.Text = IniFileHandler.GetPrivateProfileString("APP_CMD_OPT", "Bon", "-d", viewAppIniPath);
                textBox_cmdMin.Text = IniFileHandler.GetPrivateProfileString("APP_CMD_OPT", "Min", "-min", viewAppIniPath);
                textBox_cmdViewOff.Text = IniFileHandler.GetPrivateProfileString("APP_CMD_OPT", "ViewOff", "-noview", viewAppIniPath);

                Settings.Instance.DefRecFolders.ForEach(folder => listBox_recFolder.Items.Add(folder));
                textBox_recInfoFolder.Text = IniFileHandler.GetPrivateProfileString("SET", "RecInfoFolder", "", SettingPath.CommonIniPath);

                button_shortCut.Content = SettingPath.ModuleName + ".exe" + (File.Exists(
                    System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Startup), SettingPath.ModuleName + ".lnk")) ? "を解除" : "");
                button_shortCutSrv.Content = (string)button_shortCutSrv.Content + (File.Exists(
                    System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Startup), "EpgTimerSrv.lnk")) ? "を解除" : "");

                SortedList<Int32, TunerInfo> tunerInfo = new SortedList<Int32, TunerInfo>();
                foreach (string fileName in CommonManager.Instance.GetBonFileList())
                {
                    try
                    {
                        TunerInfo item = new TunerInfo(fileName);
                        item.TunerNum = IniFileHandler.GetPrivateProfileInt(item.BonDriver, "Count", 0, SettingPath.TimerSrvIniPath).ToString();
                        item.IsEpgCap = (IniFileHandler.GetPrivateProfileInt(item.BonDriver, "GetEpg", 1, SettingPath.TimerSrvIniPath) != 0);
                        item.EPGNum = IniFileHandler.GetPrivateProfileInt(item.BonDriver, "EPGCount", 0, SettingPath.TimerSrvIniPath).ToString();
                        int priority = IniFileHandler.GetPrivateProfileInt(item.BonDriver, "Priority", 0xFFFF, SettingPath.TimerSrvIniPath);
                        while (true)
                        {
                            if (tunerInfo.ContainsKey(priority) == true)
                            {
                                priority++;
                            }
                            else
                            {
                                tunerInfo.Add(priority, item);
                                break;
                            }
                        }
                    }
                    catch (Exception ex)
                    {
                        MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                    }
                }
                foreach (TunerInfo info in tunerInfo.Values)
                {
                    listBox_bon.Items.Add(info);
                }
                if (listBox_bon.Items.Count > 0)
                {
                    listBox_bon.SelectedIndex = 0;
                }

                comboBox_HH.DataContext = CommonManager.Instance.HourDictionary.Values;
                comboBox_HH.SelectedIndex = 0;
                comboBox_MM.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_MM.SelectedIndex = 0;

                serviceList = new List<ServiceViewItem>();
                try
                {
                    foreach (ChSet5Item info in ChSet5.Instance.ChList.Values)
                    {
                        ServiceViewItem item = new ServiceViewItem(info);
                        if (info.EpgCapFlag == 1)
                        {
                            item.IsSelected = true;
                        }
                        else
                        {
                            item.IsSelected = false;
                        }
                        serviceList.Add(item);
                    }
                }
                catch
                {
                }
                listView_service.DataContext = serviceList;

                if (IniFileHandler.GetPrivateProfileInt("SET", "BSBasicOnly", 1, SettingPath.CommonIniPath) == 1)
                {
                    checkBox_bs.IsChecked = true;
                }
                else
                {
                    checkBox_bs.IsChecked = false;
                }
                if (IniFileHandler.GetPrivateProfileInt("SET", "CS1BasicOnly", 1, SettingPath.CommonIniPath) == 1)
                {
                    checkBox_cs1.IsChecked = true;
                }
                else
                {
                    checkBox_cs1.IsChecked = false;
                }
                if (IniFileHandler.GetPrivateProfileInt("SET", "CS2BasicOnly", 1, SettingPath.CommonIniPath) == 1)
                {
                    checkBox_cs2.IsChecked = true;
                }
                else
                {
                    checkBox_cs2.IsChecked = false;
                }
                if (IniFileHandler.GetPrivateProfileInt("SET", "CS3BasicOnly", 0, SettingPath.CommonIniPath) == 1)
                {
                    checkBox_cs3.IsChecked = true;
                }
                else
                {
                    checkBox_cs3.IsChecked = false;
                }

                int capCount = IniFileHandler.GetPrivateProfileInt("EPG_CAP", "Count", 0, SettingPath.TimerSrvIniPath);
                if (capCount == 0)
                {
                    EpgCaptime item = new EpgCaptime();
                    item.IsSelected = true;
                    item.Time = "23:00";
                    item.BSBasicOnly = checkBox_bs.IsChecked == true;
                    item.CS1BasicOnly = checkBox_cs1.IsChecked == true;
                    item.CS2BasicOnly = checkBox_cs2.IsChecked == true;
                    item.CS3BasicOnly = checkBox_cs3.IsChecked == true;
                    timeList.Add(item);
                }
                else
                {
                    for (int i = 0; i < capCount; i++)
                    {
                        EpgCaptime item = new EpgCaptime();
                        item.Time = IniFileHandler.GetPrivateProfileString("EPG_CAP", i.ToString(), "", SettingPath.TimerSrvIniPath);
                        if (IniFileHandler.GetPrivateProfileInt("EPG_CAP", i.ToString() + "Select", 0, SettingPath.TimerSrvIniPath) == 1)
                        {
                            item.IsSelected = true;
                        }
                        else
                        {
                            item.IsSelected = false;
                        }
                        // 取得種別(bit0(LSB)=BS,bit1=CS1,bit2=CS2,bit3=CS3)。負値のときは共通設定に従う
                        int flags = IniFileHandler.GetPrivateProfileInt("EPG_CAP", i.ToString() + "BasicOnlyFlags", -1, SettingPath.TimerSrvIniPath);
                        if (flags >= 0)
                        {
                            item.BSBasicOnly = (flags & 1) != 0;
                            item.CS1BasicOnly = (flags & 2) != 0;
                            item.CS2BasicOnly = (flags & 4) != 0;
                            item.CS3BasicOnly = (flags & 8) != 0;
                        }
                        else
                        {
                            item.BSBasicOnly = checkBox_bs.IsChecked == true;
                            item.CS1BasicOnly = checkBox_cs1.IsChecked == true;
                            item.CS2BasicOnly = checkBox_cs2.IsChecked == true;
                            item.CS3BasicOnly = checkBox_cs3.IsChecked == true;
                        }
                        timeList.Add(item);
                    }
                }
                ListView_time.DataContext = timeList;

                textBox_ngCapMin.Text = IniFileHandler.GetPrivateProfileInt("SET", "NGEpgCapTime", 20, SettingPath.TimerSrvIniPath).ToString();
                textBox_ngTunerMin.Text = IniFileHandler.GetPrivateProfileInt("SET", "NGEpgCapTunerTime", 20, SettingPath.TimerSrvIniPath).ToString();

            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void SaveSetting()
        {
            try
            {
                string setPath = textBox_setPath.Text.Trim();
                setPath = setPath == "" ? SettingPath.DefSettingFolderPath : setPath;
                System.IO.Directory.CreateDirectory(setPath);

                IsChangeSettingPath = SettingPath.SettingFolderPath.TrimEnd('\\') != setPath.TrimEnd('\\');
                SettingPath.SettingFolderPath = setPath;

                IniFileHandler.WritePrivateProfileString("SET", "RecExePath",
                    string.Compare(textBox_exe.Text, SettingPath.ModulePath.TrimEnd('\\') + "\\EpgDataCap_Bon.exe", true) == 0 ? null : textBox_exe.Text, SettingPath.CommonIniPath);

                string viewAppIniPath = SettingPath.ModulePath.TrimEnd('\\') + "\\ViewApp.ini";
                if (IniFileHandler.GetPrivateProfileString("APP_CMD_OPT", "Bon", "-d", viewAppIniPath) != textBox_cmdBon.Text)
                {
                    IniFileHandler.WritePrivateProfileString("APP_CMD_OPT", "Bon", textBox_cmdBon.Text, viewAppIniPath);
                }
                if (IniFileHandler.GetPrivateProfileString("APP_CMD_OPT", "Min", "-min", viewAppIniPath) != textBox_cmdMin.Text)
                {
                    IniFileHandler.WritePrivateProfileString("APP_CMD_OPT", "Min", textBox_cmdMin.Text, viewAppIniPath);
                }
                if (IniFileHandler.GetPrivateProfileString("APP_CMD_OPT", "ViewOff", "-noview", viewAppIniPath) != textBox_cmdViewOff.Text)
                {
                    IniFileHandler.WritePrivateProfileString("APP_CMD_OPT", "ViewOff", textBox_cmdViewOff.Text, viewAppIniPath);
                }

                int recFolderCount = listBox_recFolder.Items.Count == 1 &&
                    string.Compare(((string)listBox_recFolder.Items[0]).TrimEnd('\\'), textBox_setPath.Text.TrimEnd('\\'), true) == 0 ? 0 : listBox_recFolder.Items.Count;
                IniFileHandler.WritePrivateProfileString("SET", "RecFolderNum", recFolderCount.ToString(), SettingPath.CommonIniPath);
                for (int i = 0; i < recFolderCount; i++)
                {
                    string key = "RecFolderPath" + i.ToString();
                    string val = listBox_recFolder.Items[i] as string;
                    IniFileHandler.WritePrivateProfileString("SET", key, val, SettingPath.CommonIniPath);
                }

                IniFileHandler.WritePrivateProfileString("SET", "RecInfoFolder",
                    textBox_recInfoFolder.Text.Trim() == "" ? null : textBox_recInfoFolder.Text, SettingPath.CommonIniPath);

                for (int i = 0; i < listBox_bon.Items.Count; i++)
                {
                    TunerInfo info = listBox_bon.Items[i] as TunerInfo;

                    IniFileHandler.WritePrivateProfileString(info.BonDriver, "Count", info.TunerNum, SettingPath.TimerSrvIniPath);
                    if (info.IsEpgCap == true)
                    {
                        IniFileHandler.WritePrivateProfileString(info.BonDriver, "GetEpg", "1", SettingPath.TimerSrvIniPath);
                    }
                    else
                    {
                        IniFileHandler.WritePrivateProfileString(info.BonDriver, "GetEpg", "0", SettingPath.TimerSrvIniPath);
                    }
                    IniFileHandler.WritePrivateProfileString(info.BonDriver, "EPGCount", info.EPGNum, SettingPath.TimerSrvIniPath);
                    IniFileHandler.WritePrivateProfileString(info.BonDriver, "Priority", i.ToString(), SettingPath.TimerSrvIniPath);
                }

                if (checkBox_bs.IsChecked == true)
                {
                    IniFileHandler.WritePrivateProfileString("SET", "BSBasicOnly", "1", SettingPath.CommonIniPath);
                }
                else
                {
                    IniFileHandler.WritePrivateProfileString("SET", "BSBasicOnly", "0", SettingPath.CommonIniPath);
                }
                if (checkBox_cs1.IsChecked == true)
                {
                    IniFileHandler.WritePrivateProfileString("SET", "CS1BasicOnly", "1", SettingPath.CommonIniPath);
                }
                else
                {
                    IniFileHandler.WritePrivateProfileString("SET", "CS1BasicOnly", "0", SettingPath.CommonIniPath);
                }
                if (checkBox_cs2.IsChecked == true)
                {
                    IniFileHandler.WritePrivateProfileString("SET", "CS2BasicOnly", "1", SettingPath.CommonIniPath);
                }
                else
                {
                    IniFileHandler.WritePrivateProfileString("SET", "CS2BasicOnly", "0", SettingPath.CommonIniPath);
                }
                if (checkBox_cs3.IsChecked == true)
                {
                    IniFileHandler.WritePrivateProfileString("SET", "CS3BasicOnly", "1", SettingPath.CommonIniPath);
                }
                else
                {
                    IniFileHandler.WritePrivateProfileString("SET", "CS3BasicOnly", "0", SettingPath.CommonIniPath);
                }

                foreach (ServiceViewItem info in serviceList)
                {
                    UInt64 key = info.ServiceInfo.Key;
                    try
                    {
                        if (info.IsSelected == true)
                        {
                            ChSet5.Instance.ChList[key].EpgCapFlag = 1;
                        }
                        else
                        {
                            ChSet5.Instance.ChList[key].EpgCapFlag = 0;
                        }
                    }
                    catch
                    {
                    }
                }

                IniFileHandler.WritePrivateProfileString("EPG_CAP", "Count", timeList.Count.ToString(), SettingPath.TimerSrvIniPath);
                for (int i = 0; i < timeList.Count; i++)
                {
                    EpgCaptime item = timeList[i] as EpgCaptime;
                    IniFileHandler.WritePrivateProfileString("EPG_CAP", i.ToString(), item.Time, SettingPath.TimerSrvIniPath);
                    if (item.IsSelected == true)
                    {
                        IniFileHandler.WritePrivateProfileString("EPG_CAP", i.ToString() + "Select", "1", SettingPath.TimerSrvIniPath);
                    }
                    else
                    {
                        IniFileHandler.WritePrivateProfileString("EPG_CAP", i.ToString() + "Select", "0", SettingPath.TimerSrvIniPath);
                    }
                    int flags = (item.BSBasicOnly ? 1 : 0) | (item.CS1BasicOnly ? 2 : 0) | (item.CS2BasicOnly ? 4 : 0) | (item.CS3BasicOnly ? 8 : 0);
                    IniFileHandler.WritePrivateProfileString("EPG_CAP", i.ToString() + "BasicOnlyFlags", flags.ToString(), SettingPath.TimerSrvIniPath);
                }


                IniFileHandler.WritePrivateProfileString("SET", "NGEpgCapTime", textBox_ngCapMin.Text, SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString("SET", "NGEpgCapTunerTime", textBox_ngTunerMin.Text, SettingPath.TimerSrvIniPath);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_setPath_Click(object sender, RoutedEventArgs e)
        {
            CommonManager.GetFolderNameByDialog(textBox_setPath, "設定関係保存フォルダの選択");
        }
        private void button_exe_Click(object sender, RoutedEventArgs e)
        {
            CommonManager.GetFileNameByDialog(textBox_exe, false, "", ".exe");
        }
        private void button_recInfoFolder_Click(object sender, RoutedEventArgs e)
        {
            CommonManager.GetFolderNameByDialog(textBox_recInfoFolder, "録画情報保存フォルダの選択");
        }

        private void listBox_Button_Set()
        {
            //エスケープキャンセルだけは常に有効にする。
            var bxr = new BoxExchangeEdit.BoxExchangeEditor(null, this.listBox_recFolder, true);
            var bxb = new BoxExchangeEdit.BoxExchangeEditor(null, this.listBox_bon, true);
            var bxt = new BoxExchangeEdit.BoxExchangeEditor(null, this.ListView_time, true);

            if (CommonManager.Instance.NWMode == false)
            {
                //録画設定関係
                bxr.AllowDragDrop();
                bxr.AllowKeyAction();
                bxr.targetBoxAllowDoubleClick(bxr.TargetBox, (sender, e) => button_rec_open.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)));
                button_rec_up.Click += new RoutedEventHandler(bxr.button_Up_Click);
                button_rec_down.Click += new RoutedEventHandler(bxr.button_Down_Click);
                button_rec_del.Click += new RoutedEventHandler(bxr.button_Delete_Click);

                //チューナ関係関係
                bxb.AllowDragDrop();
                button_bon_up.Click += new RoutedEventHandler(bxb.button_Up_Click);
                button_bon_down.Click += new RoutedEventHandler(bxb.button_Down_Click);

                //EPG取得関係
                bxt.TargetItemsSource = timeList;
                bxt.AllowDragDrop();
                bxt.AllowKeyAction();
                button_delTime.Click += new RoutedEventHandler(bxt.button_Delete_Click);
            }
        }

        private void listBox_recFolder_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (listBox_recFolder.SelectedItem is string)
            {
                textBox_recFolder.Text = listBox_recFolder.SelectedItem as string;
            }
        }

        private void button_rec_open_Click(object sender, RoutedEventArgs e)
        {
            CommonManager.GetFolderNameByDialog(textBox_recFolder, "録画フォルダの選択");
        }

        private void button_rec_add_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (String.IsNullOrEmpty(textBox_recFolder.Text) == false)
                {
                    foreach (String info in listBox_recFolder.Items)
                    {
                        if (String.Compare(textBox_recFolder.Text, info, true) == 0)
                        {
                            MessageBox.Show("すでに追加されています");
                            return;
                        }
                    }
                    listBox_recFolder.Items.Add(textBox_recFolder.Text);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_shortCut_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                string shortcutPath = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Startup), SettingPath.ModuleName + ".lnk");
                if (File.Exists(shortcutPath))
                {
                    File.Delete(shortcutPath);
                }
                else
                {
                    CreateShortCut(shortcutPath, Assembly.GetEntryAssembly().Location, "");
                }
                button_shortCut.Content = ((string)button_shortCut.Content).Replace("を解除", "") + (File.Exists(shortcutPath) ? "を解除" : "");
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_shortCutSrv_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                string shortcutPath = System.IO.Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Startup), "EpgTimerSrv.lnk");
                if (File.Exists(shortcutPath))
                {
                    File.Delete(shortcutPath);
                }
                else
                {
                    CreateShortCut(shortcutPath, System.IO.Path.Combine(SettingPath.ModulePath, "EpgTimerSrv.exe"), "");
                }
                button_shortCutSrv.Content = ((string)button_shortCutSrv.Content).Replace("を解除", "") + (File.Exists(shortcutPath) ? "を解除" : "");
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

        private void listBox_bon_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (listBox_bon.SelectedItem != null)
                {
                    TunerInfo info = listBox_bon.SelectedItem as TunerInfo;
                    textBox_bon_num.DataContext = info;
                    checkBox_bon_epg.DataContext = info;
                    textBox_bon_epgnum.DataContext = info;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_allChk_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                foreach (ServiceViewItem info in this.serviceList)
                {
                    info.IsSelected = true;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_videoChk_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                foreach (ServiceViewItem info in this.serviceList)
                {
                    info.IsSelected = (ChSet5.IsVideo(info.ServiceInfo.ServiceType) == true);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_allClear_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                foreach (ServiceViewItem info in this.serviceList)
                {
                    info.IsSelected = false;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_addTime_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (comboBox_HH.SelectedItem != null && comboBox_MM.SelectedItem != null)
                {
                    UInt16 hh = (UInt16)comboBox_HH.SelectedItem;
                    UInt16 mm = (UInt16)comboBox_MM.SelectedItem;
                    String time = hh.ToString("D2") + ":" + mm.ToString("D2");
                    int wday = comboBox_wday.SelectedIndex;
                    if (1 <= wday && wday <= 7)
                    {
                        // 曜日指定接尾辞(w1=Mon,...,w7=Sun)
                        time += "w" + ((wday + 5) % 7 + 1);
                    }

                    foreach (EpgCaptime info in timeList)
                    {
                        if (String.Compare(info.Time, time, true) == 0)
                        {
                            MessageBox.Show("すでに登録済みです");
                            return;
                        }
                    }
                    EpgCaptime item = new EpgCaptime();
                    item.IsSelected = true;
                    item.Time = time;
                    item.BSBasicOnly = checkBox_bs.IsChecked == true;
                    item.CS1BasicOnly = checkBox_cs1.IsChecked == true;
                    item.CS2BasicOnly = checkBox_cs2.IsChecked == true;
                    item.CS3BasicOnly = checkBox_cs3.IsChecked == true;
                    timeList.Add(item);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
    }

    //BonDriver一覧の表示・設定用クラス
    public class TunerInfo
    {
        public TunerInfo(string bon) { BonDriver = bon; }
        public String BonDriver { get; set; }
        public String TunerNum { get; set; }
        public String EPGNum { get; set; }
        public bool IsEpgCap { get; set; }
        public override string ToString() { return BonDriver; }
    }
}

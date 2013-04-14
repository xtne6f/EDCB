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
        private ObservableCollection<EpgCaptime> timeList;
        private List<ServiceItem2> serviceList;

        public SetBasicView()
        {
            InitializeComponent();

            try
            {
                if (Settings.Instance.NoStyle == 1)
                {
                    button_setPath.Style = null;
                    button_exe.Style = null;
                    button_rec_up.Style = null;
                    button_rec_down.Style = null;
                    button_rec_del.Style = null;
                    button_rec_open.Style = null;
                    button_rec_add.Style = null;
                    button_shortCut.Style = null;
                    button_bon_up.Style = null;
                    button_bon_down.Style = null;
                    button_allChk.Style = null;
                    button_videoChk.Style = null;
                    button_allClear.Style = null;
                    button_addTime.Style = null;
                    button_delTime.Style = null;

                }


                StringBuilder buff = new StringBuilder(512);
                buff.Clear();
                IniFileHandler.GetPrivateProfileString("SET", "DataSavePath", SettingPath.DefSettingFolderPath, buff, 512, SettingPath.CommonIniPath);
                textBox_setPath.Text = buff.ToString();

                string defRecExe = System.IO.Path.GetDirectoryName(Environment.GetCommandLineArgs()[0]) + "\\EpgDataCap_Bon.exe";
                buff.Clear();
                IniFileHandler.GetPrivateProfileString("SET", "RecExePath", defRecExe, buff, 512, SettingPath.CommonIniPath);
                textBox_exe.Text = buff.ToString();

                int num = IniFileHandler.GetPrivateProfileInt("SET", "RecFolderNum", 0, SettingPath.CommonIniPath);
                if (num == 0)
                {
                    listBox_recFolder.Items.Add(SettingPath.DefSettingFolderPath);
                }
                else
                {
                    for (uint i = 0; i < num; i++)
                    {
                        string key = "RecFolderPath" + i.ToString();
                        buff.Clear();
                        IniFileHandler.GetPrivateProfileString("SET", key, "", buff, 512, SettingPath.CommonIniPath);
                        if (buff.Length > 0)
                        {
                            listBox_recFolder.Items.Add(buff.ToString());
                        }
                    }
                }

                string[] files = Directory.GetFiles(SettingPath.SettingFolderPath, "*.ChSet4.txt");
                SortedList<Int32, TunerInfo> tunerInfo = new SortedList<Int32, TunerInfo>();
                foreach (string info in files)
                {
                    try
                    {
                        TunerInfo item = new TunerInfo();
                        String fileName = System.IO.Path.GetFileName(info);
                        item.BonDriver = GetBonFileName(fileName);
                        item.BonDriver += ".dll";
                        item.TunerNum = IniFileHandler.GetPrivateProfileInt(item.BonDriver, "Count", 0, SettingPath.TimerSrvIniPath).ToString();
                        if (IniFileHandler.GetPrivateProfileInt(item.BonDriver, "GetEpg", 1, SettingPath.TimerSrvIniPath) == 0)
                        {
                            item.IsEpgCap = false;
                        }
                        else
                        {
                            item.IsEpgCap = true;
                        }
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

                chkEPGBasicOnly.IsChecked = false;

                serviceList = new List<ServiceItem2>();
                try
                {
                    foreach (ChSet5Item info in ChSet5.Instance.ChList.Values)
                    {
                        ServiceItem2 item = new ServiceItem2();
                        item.ServiceInfo = info;
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

                buff.Clear();
                timeList = new ObservableCollection<EpgCaptime>();
                int capCount = IniFileHandler.GetPrivateProfileInt("EPG_CAP", "Count", 0, SettingPath.TimerSrvIniPath);
                if (capCount == 0)
                {
                    EpgCaptime item = new EpgCaptime();
                    item.IsSelected = true;
                    item.IsBasicOnly = false;
                    item.Time = "23:00";
                    timeList.Add(item);
                }
                else
                {
                    for (int i = 0; i < capCount; i++)
                    {
                        buff.Clear();
                        EpgCaptime item = new EpgCaptime();
                        IniFileHandler.GetPrivateProfileString("EPG_CAP", i.ToString(), "", buff, 512, SettingPath.TimerSrvIniPath);
                        item.Time = buff.ToString();
                        if (IniFileHandler.GetPrivateProfileInt("EPG_CAP", i.ToString() + "Select", 0, SettingPath.TimerSrvIniPath) == 1)
                        {
                            item.IsSelected = true;
                        }
                        else
                        {
                            item.IsSelected = false;
                        }
                        if (IniFileHandler.GetPrivateProfileInt("EPG_CAP", i.ToString() + "Basic", 0, SettingPath.TimerSrvIniPath) == 1)
                        {
                            item.IsBasicOnly = true;
                        }
                        else
                        {
                            item.IsBasicOnly = false;
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

        private String GetBonFileName(String src)
        {
            int pos = src.LastIndexOf(")");
            if (pos < 1)
            {
                return src;
            }

            int count = 1;
            for (int i = pos - 1; i >= 0; i--)
            {
                if (src[i] == '(')
                {
                    count--;
                }
                else if (src[i] == ')')
                {
                    count++;
                }
                if (count == 0)
                {
                    return src.Substring(0, i);
                }
            }
            return src;
        }

        public void SaveSetting()
        {
            try
            {
                System.IO.Directory.CreateDirectory(textBox_setPath.Text);

                IniFileHandler.WritePrivateProfileString("SET", "DataSavePath", textBox_setPath.Text, SettingPath.CommonIniPath);
                IniFileHandler.WritePrivateProfileString("SET", "RecExePath", textBox_exe.Text, SettingPath.CommonIniPath);
                IniFileHandler.WritePrivateProfileString("SET", "RecFolderNum", listBox_recFolder.Items.Count.ToString(), SettingPath.CommonIniPath);
                for (int i = 0; i < listBox_recFolder.Items.Count; i++)
                {
                    string key = "RecFolderPath" + i.ToString();
                    string val = listBox_recFolder.Items[i] as string;
                    IniFileHandler.WritePrivateProfileString("SET", key, val, SettingPath.CommonIniPath);
                }

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

                foreach (ServiceItem2 info in serviceList)
                {
                    UInt64 key = ((UInt64)info.ServiceInfo.ONID) << 32 | ((UInt64)info.ServiceInfo.TSID) << 16 | ((UInt64)info.ServiceInfo.SID);
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
                    if (item.IsBasicOnly == true)
                    {
                        IniFileHandler.WritePrivateProfileString("EPG_CAP", i.ToString() + "Basic", "1", SettingPath.TimerSrvIniPath);
                    }
                    else
                    {
                        IniFileHandler.WritePrivateProfileString("EPG_CAP", i.ToString() + "Basic", "0", SettingPath.TimerSrvIniPath);
                    }
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
            try
            {
                System.Windows.Forms.FolderBrowserDialog dlg = new System.Windows.Forms.FolderBrowserDialog();
                dlg.Description = "設定関係保存フォルダ";
                if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                {
                    textBox_setPath.Text = dlg.SelectedPath;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_exe_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();
                dlg.DefaultExt = ".exe";
                dlg.Filter = "exe Files (.exe)|*.exe;|all Files(*.*)|*.*";

                Nullable<bool> result = dlg.ShowDialog();
                if (result == true)
                {
                    textBox_exe.Text = dlg.FileName;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_rec_up_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listBox_recFolder.SelectedItem != null)
                {
                    if (listBox_recFolder.SelectedIndex >= 1)
                    {
                        object temp = listBox_recFolder.SelectedItem;
                        int index = listBox_recFolder.SelectedIndex;
                        listBox_recFolder.Items.RemoveAt(listBox_recFolder.SelectedIndex);
                        listBox_recFolder.Items.Insert(index - 1, temp);
                        listBox_recFolder.SelectedIndex = index - 1;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_rec_down_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listBox_recFolder.SelectedItem != null)
                {
                    if (listBox_recFolder.SelectedIndex < listBox_recFolder.Items.Count - 1)
                    {
                        object temp = listBox_recFolder.SelectedItem;
                        int index = listBox_recFolder.SelectedIndex;
                        listBox_recFolder.Items.RemoveAt(listBox_recFolder.SelectedIndex);
                        listBox_recFolder.Items.Insert(index + 1, temp);
                        listBox_recFolder.SelectedIndex = index + 1;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_rec_del_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listBox_recFolder.SelectedItem != null)
                {
                    listBox_recFolder.Items.RemoveAt(listBox_recFolder.SelectedIndex);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_rec_open_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                System.Windows.Forms.FolderBrowserDialog dlg = new System.Windows.Forms.FolderBrowserDialog();
                dlg.Description = "録画フォルダ";
                if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
                {
                    textBox_recFolder.Text = dlg.SelectedPath;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
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
                Assembly myAssembly = Assembly.GetEntryAssembly();
                string targetPath = myAssembly.Location;
                string shortcutPath = System.IO.Path.Combine(
                    Environment.GetFolderPath(System.Environment.SpecialFolder.Startup),
                    @"EpgTime.lnk");

                CreateShortCut(shortcutPath, targetPath, "");
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
            // Descriptionプロパティをセットする
            shortcutType.InvokeMember("Description", BindingFlags.SetProperty, null, shortCut, new object[] { description });
            // Saveメソッドを実行する
            shortcutType.InvokeMember("Save", BindingFlags.InvokeMethod, null, shortCut, null);
        }

        private void button_bon_up_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listBox_bon.SelectedItem != null)
                {
                    if (listBox_bon.SelectedIndex >= 1)
                    {
                        object temp = listBox_bon.SelectedItem;
                        int index = listBox_bon.SelectedIndex;
                        listBox_bon.Items.RemoveAt(listBox_bon.SelectedIndex);
                        listBox_bon.Items.Insert(index - 1, temp);
                        listBox_bon.SelectedIndex = index - 1;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_bon_down_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (listBox_bon.SelectedItem != null)
                {
                    if (listBox_bon.SelectedIndex < listBox_bon.Items.Count - 1)
                    {
                        object temp = listBox_bon.SelectedItem;
                        int index = listBox_bon.SelectedIndex;
                        listBox_bon.Items.RemoveAt(listBox_bon.SelectedIndex);
                        listBox_bon.Items.Insert(index + 1, temp);
                        listBox_bon.SelectedIndex = index + 1;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
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
                foreach (ServiceItem2 info in this.serviceList)
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
                foreach (ServiceItem2 info in this.serviceList)
                {
                    if (info.ServiceInfo.ServiceType == 0x01 || info.ServiceInfo.ServiceType == 0xA5)
                    {
                        info.IsSelected = true;
                    }
                    else
                    {
                        info.IsSelected = false;
                    }
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
                foreach (ServiceItem2 info in this.serviceList)
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
                    if (chkEPGBasicOnly.IsChecked==true)
                    {
                        item.IsBasicOnly = true;
                    }
                    else
                    {
                        item.IsBasicOnly = false;
                    }
                    timeList.Add(item);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_delTime_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (ListView_time.SelectedItem != null)
                {
                    EpgCaptime item = ListView_time.SelectedItem as EpgCaptime;
                    timeList.Remove(item);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

    }
}

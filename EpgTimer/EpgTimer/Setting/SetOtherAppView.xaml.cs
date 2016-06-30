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

namespace EpgTimer.Setting
{
    /// <summary>
    /// SetOtherAppView.xaml の相互作用ロジック
    /// </summary>
    public partial class SetOtherAppView : UserControl
    {
        public SetOtherAppView()
        {
            InitializeComponent();

            if (CommonManager.Instance.NWMode == true)
            {
                label3.IsEnabled = false;
                listBox_bon.IsEnabled = false;
                button_del.IsEnabled = false;
                button_add.IsEnabled = false;
                comboBox_bon.IsEnabled = false;
            }

            try
            {
                textBox_exe.Text = Settings.Instance.TvTestExe;
                textBox_cmd.Text = Settings.Instance.TvTestCmd;
                checkBox_nwTvMode.IsChecked = Settings.Instance.NwTvMode;
                checkBox_nwUDP.IsChecked = Settings.Instance.NwTvModeUDP;
                checkBox_nwTCP.IsChecked = Settings.Instance.NwTvModeTCP;

                textBox_playExe.Text = Settings.Instance.FilePlayExe;
                textBox_playCmd.Text = Settings.Instance.FilePlayCmd;
                checkBox_playOnAirWithExe.IsChecked = Settings.Instance.FilePlayOnAirWithExe;

                string[] files = Directory.GetFiles(SettingPath.SettingFolderPath, "*.ChSet4.txt");
                SortedList<Int32, TunerInfo> tunerInfo = new SortedList<Int32, TunerInfo>();
                foreach (string info in files)
                {
                    try
                    {
                        String bonName = "";
                        String fileName = System.IO.Path.GetFileName(info);
                        bonName = GetBonFileName(fileName);
                        bonName += ".dll";
                        comboBox_bon.Items.Add(bonName);
                    }
                    catch
                    {
                    }
                }
                if (comboBox_bon.Items.Count > 0)
                {
                    comboBox_bon.SelectedIndex = 0;
                }

                int num = IniFileHandler.GetPrivateProfileInt("TVTEST", "Num", 0, SettingPath.TimerSrvIniPath);
                for (uint i = 0; i < num; i++)
                {
                    string item = IniFileHandler.GetPrivateProfileString("TVTEST", i.ToString(), "", SettingPath.TimerSrvIniPath);
                    if (item.Length > 0)
                    {
                        listBox_bon.Items.Add(item);
                    }
                }
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
            Settings.Instance.TvTestExe = textBox_exe.Text;
            Settings.Instance.TvTestCmd = textBox_cmd.Text;
            if (checkBox_nwTvMode.IsChecked == true)
            {
                Settings.Instance.NwTvMode = true;
            }
            else
            {
                Settings.Instance.NwTvMode = false;
            }
            if (checkBox_nwUDP.IsChecked == true)
            {
                Settings.Instance.NwTvModeUDP = true;
            }
            else
            {
                Settings.Instance.NwTvModeUDP = false;
            }

            if (checkBox_nwTCP.IsChecked == true)
            {
                Settings.Instance.NwTvModeTCP = true;
            }
            else
            {
                Settings.Instance.NwTvModeTCP = false;
            }

            IniFileHandler.WritePrivateProfileString("TVTEST", "Num", listBox_bon.Items.Count.ToString(), SettingPath.TimerSrvIniPath);
            for (int i = 0; i < listBox_bon.Items.Count; i++)
            {
                string val = listBox_bon.Items[i] as string;
                IniFileHandler.WritePrivateProfileString("TVTEST", i.ToString(), val, SettingPath.TimerSrvIniPath);
            }

            Settings.Instance.FilePlayExe = textBox_playExe.Text;
            Settings.Instance.FilePlayCmd = textBox_playCmd.Text;
            Settings.Instance.FilePlayOnAirWithExe = checkBox_playOnAirWithExe.IsChecked == true;
        }

        private void button_exe_Click(object sender, RoutedEventArgs e)
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

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_bon.SelectedItem != null)
            {
                listBox_bon.Items.RemoveAt(listBox_bon.SelectedIndex);
            }
        }

        private void button_add_Click(object sender, RoutedEventArgs e)
        {
            if (String.IsNullOrEmpty(comboBox_bon.Text) == false)
            {
                foreach (String info in listBox_bon.Items)
                {
                    if (String.Compare(comboBox_bon.Text, info, true) == 0)
                    {
                        MessageBox.Show("すでに追加されています");
                        return;
                    }
                }
                listBox_bon.Items.Add(comboBox_bon.Text);
            }
        }

        private void button_playExe_Click(object sender, RoutedEventArgs e)
        {
            Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();
            dlg.DefaultExt = ".exe";
            dlg.Filter = "exe Files (.exe)|*.exe;|all Files(*.*)|*.*";

            Nullable<bool> result = dlg.ShowDialog();
            if (result == true)
            {
                textBox_playExe.Text = dlg.FileName;
            }
        }
    }
}

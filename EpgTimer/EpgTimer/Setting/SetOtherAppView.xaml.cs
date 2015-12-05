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
                CommonManager.Instance.VUtil.DisableControlChildren(tabItem_play);
                label3.IsEnabled = false;
                button_del.IsEnabled = false;
                button_add.IsEnabled = false;
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

                comboBox_bon.ItemsSource = CommonManager.Instance.GetBonFileList();
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
            string path = CommonManager.Instance.GetFileNameByDialog(textBox_exe.Text, "", ".exe");
            if (path != null)
            {
                textBox_exe.Text = path;
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
            string path = CommonManager.Instance.GetFileNameByDialog(textBox_playExe.Text, "", ".exe");
            if (path != null)
            {
                textBox_playExe.Text = path;
            }
        }
    }
}

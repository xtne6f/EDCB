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
        private MenuUtil mutil = CommonManager.Instance.MUtil;

        public SetOtherAppView()
        {
            InitializeComponent();

            if (CommonManager.Instance.NWMode == true)
            {
                label3.IsEnabled = false;
                button_del.IsEnabled = false;
                button_add.IsEnabled = false;
                checkBox_playOnNwWithExe.IsEnabled = true;
            }

            //エスケープキャンセルだけは常に有効にする。
            var bx = new BoxExchangeEdit.BoxExchangeEditor(null, this.listBox_bon, true);
            if (CommonManager.Instance.NWMode == false)
            {
                bx.AllowDragDrop();
                bx.AllowKeyAction();
                button_del.Click += new RoutedEventHandler(bx.button_Delete_Click);
            }

            try
            {
                textBox_exe.Text = Settings.Instance.TvTestExe;
                textBox_cmd.Text = Settings.Instance.TvTestCmd;
                checkBox_nwTvMode.IsChecked = Settings.Instance.NwTvMode;
                checkBox_nwUDP.IsChecked = Settings.Instance.NwTvModeUDP;
                checkBox_nwTCP.IsChecked = Settings.Instance.NwTvModeTCP;
                textBox_TvTestOpenWait.Text = Settings.Instance.TvTestOpenWait.ToString();
                textBox_TvTestChgBonWait.Text = Settings.Instance.TvTestChgBonWait.ToString();

                textBox_playExe.Text = Settings.Instance.FilePlayExe;
                textBox_playCmd.Text = Settings.Instance.FilePlayCmd;
                checkBox_playOnAirWithExe.IsChecked = Settings.Instance.FilePlayOnAirWithExe;
                checkBox_playOnNwWithExe.IsChecked = Settings.Instance.FilePlayOnNwWithExe;

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
            Settings.Instance.NwTvMode = (checkBox_nwTvMode.IsChecked == true);
            Settings.Instance.NwTvModeUDP = (checkBox_nwUDP.IsChecked == true);
            Settings.Instance.NwTvModeTCP = (checkBox_nwTCP.IsChecked == true);
            Settings.Instance.TvTestOpenWait = mutil.MyToNumerical(textBox_TvTestOpenWait, Convert.ToInt32, 120000, 0, Settings.Instance.TvTestOpenWait);
            Settings.Instance.TvTestChgBonWait = mutil.MyToNumerical(textBox_TvTestChgBonWait, Convert.ToInt32, 120000, 0, Settings.Instance.TvTestChgBonWait);

            IniFileHandler.WritePrivateProfileString("TVTEST", "Num", listBox_bon.Items.Count.ToString(), SettingPath.TimerSrvIniPath);
            for (int i = 0; i < listBox_bon.Items.Count; i++)
            {
                string val = listBox_bon.Items[i] as string;
                IniFileHandler.WritePrivateProfileString("TVTEST", i.ToString(), val, SettingPath.TimerSrvIniPath);
            }

            Settings.Instance.FilePlayExe = textBox_playExe.Text;
            Settings.Instance.FilePlayCmd = textBox_playCmd.Text;
            Settings.Instance.FilePlayOnAirWithExe = checkBox_playOnAirWithExe.IsChecked == true;
            Settings.Instance.FilePlayOnNwWithExe = checkBox_playOnNwWithExe.IsChecked == true;
        }

        private void button_exe_Click(object sender, RoutedEventArgs e)
        {
            CommonManager.GetFileNameByDialog(textBox_exe, false, "", ".exe");
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
            CommonManager.GetFileNameByDialog(textBox_playExe, false, "", ".exe");
        }
    }
}

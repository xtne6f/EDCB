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

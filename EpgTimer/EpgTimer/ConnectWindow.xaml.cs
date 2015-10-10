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
using System.Windows.Shapes;

namespace EpgTimer
{
    /// <summary>
    /// ConnectWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class ConnectWindow : Window
    {
        public ConnectWindow()
        {
            InitializeComponent();

            try
            {
                textBox_srvIP.Text = Settings.Instance.NWServerIP.ToString();
                textBox_srvPort.Text = Settings.Instance.NWServerPort.ToString();
                checkBox_clientPort.IsChecked = Settings.Instance.NWWaitPort != 0;
                textBox_clientPort.Text = Settings.Instance.NWWaitPort == 0 ? "4520" : Settings.Instance.NWWaitPort.ToString();
                textBox_mac.Text = Settings.Instance.NWMacAdd.ToString();
            }
            catch
            {
            }
        }

        private void button_connect_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                Settings.Instance.NWServerIP = textBox_srvIP.Text;
                Settings.Instance.NWServerPort = Convert.ToUInt32(textBox_srvPort.Text);
                Settings.Instance.NWWaitPort = checkBox_clientPort.IsChecked == false ? 0 : Convert.ToUInt32(textBox_clientPort.Text);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                return;
            }

            DialogResult = true;
        }

        private void button_wake_Click(object sender, RoutedEventArgs e)
        {
            string[] mac = textBox_mac.Text.Split('-');
            if( mac.Length != 6 )
            {
                MessageBox.Show("書式は「xx-xx-xx-xx-xx-xx」です");
                return;
            }
            Settings.Instance.NWMacAdd = textBox_mac.Text;

            byte[] macAddress = new byte[6];
            for( int i=0; i<6; i++)
            {
                macAddress[i] = Convert.ToByte(mac[i],16);
            }
            NWConnect.SendMagicPacket(macAddress);
        }
    }
}

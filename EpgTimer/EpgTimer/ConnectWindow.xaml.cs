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
using EpgTimer;
using CtrlCmdCLI;

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
                button_connect.Style = null;
                button_wake.Style = null;
            }
            try
            {
                textBox_srvIP.Text = Settings.Instance.NWServerIP.ToString();
                textBox_srvPort.Text = Settings.Instance.NWServerPort.ToString();
                textBox_clientPort.Text = Settings.Instance.NWWaitPort.ToString();
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
                Settings.Instance.NWWaitPort = Convert.ToUInt32(textBox_clientPort.Text);
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

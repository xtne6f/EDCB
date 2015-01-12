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
    /// SetCoopSrvWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SetCoopSrvWindow : Window
    {
        public SetCoopSrvWindow()
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
                button_ok.Style = null;
                button_cancel.Style = null;
            }

            textBox_port.Text = "4510";
        }

        public void SetDefSetting(String ip, UInt32 port)
        {
            textBox_ip.Text = ip;
            textBox_port.Text = port.ToString();
        }

        public void GetSetting(ref String ip, ref UInt32 port)
        {
            try
            {
                ip = textBox_ip.Text;
                port = Convert.ToUInt32(textBox_port.Text);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_ok_Click(object sender, RoutedEventArgs e)
        {
            if (textBox_ip.Text.Length == 0)
            {
                MessageBox.Show("アドレスが入力されていません");
                return;
            }
            if (textBox_port.Text.Length == 0)
            {
                MessageBox.Show("ポートが入力されていません");
                return;
            }
            DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Escape:
                        this.button_cancel.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                }
            }
            base.OnKeyDown(e);
        }

    }
}

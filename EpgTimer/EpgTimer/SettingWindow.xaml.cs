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
    /// SettingWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SettingWindow : Window
    {
        public SettingWindow()
        {
            InitializeComponent();
        }

        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
            setBasicView.SaveSetting();
            setAppView.SaveSetting();
            setEpgView.SaveSetting();
            setOtherAppView.SaveSetting();

            Settings.SaveToXmlFile();
            if (CommonManager.Instance.NWMode == false)
            {
                ChSet5.SaveFile();
            }
            CommonManager.Instance.ReloadCustContentColorList();

            this.DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            this.DialogResult = false;
        }
    }
}

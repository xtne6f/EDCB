using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    /// <summary>
    /// SetDefSearchSettingWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SetDefSearchSettingWindow : Window
    {
        public SetDefSearchSettingWindow()
        {
            InitializeComponent();
        }

        public void SetDefSetting(EpgSearchKeyInfo key)
        {
            searchKey.SetSearchKey(key);
        }

        public EpgSearchKeyInfo GetSetting()
        {
            return searchKey.GetSearchKey();
        }

        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
    }
}

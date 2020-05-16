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
    /// EpgDataViewSettingWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgDataViewSettingWindow : Window
    {
        public EpgDataViewSettingWindow()
        {
            InitializeComponent();
        }

        /// <summary>
        /// デフォルト表示の設定値
        /// </summary>
        /// <param name="setInfo"></param>
        public void SetDefSetting(CustomEpgTabInfo setInfo)
        {
            epgDataViewSetting.SetSetting(setInfo);
        }

        /// <summary>
        /// 設定値の取得
        /// </summary>
        public CustomEpgTabInfo GetSetting()
        {
            return epgDataViewSetting.GetSetting();
        }


        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
        }
    }
}

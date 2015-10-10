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

namespace EpgTimer.TunerReserveViewCtrl
{
    /// <summary>
    /// TunerReserveNameView.xaml の相互作用ロジック
    /// </summary>
    public partial class TunerReserveNameView : UserControl
    {
        public TunerReserveNameView()
        {
            InitializeComponent();
        }

        public void ClearInfo()
        {
            stackPanel_tuner.Children.Clear();
        }

        public void SetTunerInfo(List<TunerNameViewItem> tunerInfo)
        {
            stackPanel_tuner.Children.Clear();
            foreach (TunerNameViewItem info in tunerInfo)
            {
                TextBlock item = new TextBlock();
                item.Text = info.TunerInfo.tunerName;
                if (info.TunerInfo.tunerID != 0xFFFFFFFF)
                {
                    item.Text += "\r\nID: " + info.TunerInfo.tunerID.ToString("X8");
                }
                item.Width = info.Width - 4;
                item.Margin = new Thickness(2, 2, 2, 2);
                item.Background = Brushes.AliceBlue;
                item.Foreground = Brushes.Black;
                item.TextAlignment = TextAlignment.Center;
                item.FontSize = 12;
                stackPanel_tuner.Children.Add(item);
            }
        }
    }

}

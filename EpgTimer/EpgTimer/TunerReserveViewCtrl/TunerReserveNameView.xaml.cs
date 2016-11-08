using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

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

        public void SetTunerInfo(List<PanelItem<TunerReserveInfo>> tunerInfo)
        {
            stackPanel_tuner.Children.Clear();
            foreach (var info in tunerInfo)
            {
                TextBlock item = new TextBlock();
                item.Text = info.Data.tunerName;
                if (info.Data.tunerID != 0xFFFFFFFF)
                {
                    item.Text += "\r\nID: " + info.Data.tunerID.ToString("X8");
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

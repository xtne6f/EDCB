using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;

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
                var item = new TextBlock();
                item.Text = info.Data.tunerName;
                if (info.Data.tunerID != 0xFFFFFFFF)
                {
                    item.Text += "\r\nID: " + info.Data.tunerID.ToString("X8");
                }
                item.Width = info.Width - 1;
                item.Padding = new Thickness(0, 0, 0, 2);
                item.Margin = new Thickness(0, 1, 1, 1);
                item.Background = CommonManager.Instance.TunerNameBackColor;
                item.Foreground = CommonManager.Instance.TunerNameFontColor;
                item.TextAlignment = TextAlignment.Center;
                item.FontSize = 12;
                stackPanel_tuner.Children.Add(item);
            }
        }
    }
}

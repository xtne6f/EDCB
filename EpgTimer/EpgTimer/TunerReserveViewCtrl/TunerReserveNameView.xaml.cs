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

        public void SetTunerInfo(List<TunerNameViewItem> tunerInfo, Brush backgroundBrush, bool isLight)
        {
            ClearInfo();
            foreach (TunerNameViewItem info in tunerInfo)
            {
                var item = new TextBlock()
                {
                    Style = (Style)FindResource(isLight ? "AppEpgServiceHeaderLightBackgroundTextBlockStyle" : "AppEpgServiceHeaderTextBlockStyle"),
                    Text = info.TunerInfo.tunerName + (info.TunerInfo.tunerID != 0xFFFFFFFF ? "\r\nID: " + info.TunerInfo.tunerID.ToString("X8") : "")
                };
                var grid = new Grid()
                {
                    Background = backgroundBrush,
                    Margin = new Thickness(1, 1, 1, 1),
                    Width = info.Width - 2
                };
                grid.Children.Add(item);
                stackPanel_tuner.Children.Add(grid);
            }
        }
    }
}

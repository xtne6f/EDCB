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
                var tuner1 = new StackPanel();
                tuner1.Width = info.Width - 1;
                tuner1.Margin = new Thickness(0, 1, 1, 1);
                tuner1.Background = CommonManager.Instance.TunerNameBackColor;

                var text = ViewUtil.GetPanelTextBlock(info.Data.tunerName);
                text.Margin = new Thickness(1, 0, 1, 0);
                text.Foreground = CommonManager.Instance.TunerNameFontColor;
                tuner1.Children.Add(text);

                text = ViewUtil.GetPanelTextBlock("ID: " + info.Data.tunerID.ToString("X8"));
                text.Margin = new Thickness(1, 0, 1, 2);
                text.Foreground = CommonManager.Instance.TunerNameFontColor;
                tuner1.Children.Add(text);

                stackPanel_tuner.Children.Add(tuner1);
            }
        }
    }
}

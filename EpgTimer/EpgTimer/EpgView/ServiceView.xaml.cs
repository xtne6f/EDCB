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

namespace EpgTimer.EpgView
{
    /// <summary>
    /// ServiceView.xaml の相互作用ロジック
    /// </summary>
    public partial class ServiceView : UserControl
    {
        public event Action<EpgServiceInfo> LeftDoubleClick;
        public event Action<EpgServiceInfo> RightClick;

        public ServiceView()
        {
            InitializeComponent();
        }

        public void ClearInfo()
        {
            stackPanel_service.Children.Clear();
        }

        public void SetService(List<EpgServiceInfo> serviceList, double serviceWidth, Brush serviceBrush, Brush textBrush)
        {
            stackPanel_service.Children.Clear();
            foreach (EpgServiceInfo info in serviceList)
            {
                TextBlock item = new TextBlock();
                item.Text = info.service_name;
                if (info.remote_control_key_id != 0)
                {
                    item.Text += "\r\n" + info.remote_control_key_id.ToString();
                }
                else if (info.ONID == 0x000A)
                {
                    item.Text += "\r\n" + info.network_name + " " + (info.SID & 0x3FF).ToString();
                }
                else
                {
                    item.Text += "\r\n" + info.network_name + " " + info.SID.ToString();
                }
                item.Width = serviceWidth - 2;
                item.TextAlignment = TextAlignment.Center;
                item.FontSize = 12;
                item.Foreground = textBrush;
                // 単にCenterだとやや重い感じになるので上げる
                item.Padding = new Thickness(0, 0, 0, 4);
                item.VerticalAlignment = VerticalAlignment.Center;
                var gridItem = new System.Windows.Controls.Primitives.UniformGrid();
                gridItem.Margin = new Thickness(1, 1, 1, 1);
                gridItem.Background = serviceBrush;
                gridItem.MouseLeftButtonUp += (sender, e) =>
                {
                    if (e.ClickCount == 2 && LeftDoubleClick != null)
                    {
                        LeftDoubleClick((EpgServiceInfo)((FrameworkElement)sender).Tag);
                    }
                };
                gridItem.MouseRightButtonUp += (sender, e) =>
                {
                    if (RightClick != null)
                    {
                        RightClick((EpgServiceInfo)((FrameworkElement)sender).Tag);
                    }
                };
                gridItem.Tag = info;
                gridItem.Children.Add(item);
                stackPanel_service.Children.Add(gridItem);
            }
        }
    }
}

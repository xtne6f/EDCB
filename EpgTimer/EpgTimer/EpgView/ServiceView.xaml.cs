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
                var stack = new StackPanel();
                stack.Orientation = Orientation.Horizontal;
                stack.Margin = new Thickness(1, 1, 1, 1);
                stack.Background = serviceBrush;
                stack.MouseLeftButtonDown += (sender, e) =>
                {
                    if (e.ClickCount == 2 && LeftDoubleClick != null)
                    {
                        LeftDoubleClick((EpgServiceInfo)((FrameworkElement)sender).Tag);
                    }
                };
                stack.MouseRightButtonUp += (sender, e) =>
                {
                    if (RightClick != null)
                    {
                        RightClick((EpgServiceInfo)((FrameworkElement)sender).Tag);
                    }
                };
                stack.Tag = info;
                stack.Children.Add(item);
                stackPanel_service.Children.Add(stack);
            }

            RefreshLogo();
        }

        public void RefreshLogo()
        {
            foreach (StackPanel stack in stackPanel_service.Children)
            {
                Image logoItem = stack.Children.OfType<Image>().FirstOrDefault();
                TextBlock item = stack.Children.OfType<TextBlock>().First();
                double serviceWidth = item.Width + (logoItem != null ? logoItem.Width + logoItem.Margin.Left : 0) + 2;
                if (logoItem != null)
                {
                    stack.Children.Remove(logoItem);
                }

                var info = (EpgServiceInfo)stack.Tag;
                ChSet5Item ch;
                if (ChSet5.Instance.ChList.TryGetValue(CommonManager.Create64Key(info.ONID, info.TSID, info.SID), out ch) &&
                    ch.Logo != null && serviceWidth >= 30 + 1 + 2)
                {
                    logoItem = new Image();
                    logoItem.Source = ch.Logo;
                    logoItem.Width = 30;
                    logoItem.VerticalAlignment = VerticalAlignment.Top;
                    logoItem.Margin = new Thickness(1, 2, 0, 0);
                    stack.Children.Insert(0, logoItem);
                    item.Width = serviceWidth - logoItem.Width - logoItem.Margin.Left - 2;
                }
                else
                {
                    item.Width = serviceWidth - 2;
                }
            }
        }
    }
}

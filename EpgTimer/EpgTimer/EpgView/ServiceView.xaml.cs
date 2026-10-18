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
using System.Windows.Threading;

namespace EpgTimer.EpgView
{
    /// <summary>
    /// ServiceView.xaml の相互作用ロジック
    /// </summary>
    public partial class ServiceView : UserControl
    {
        public event Action<EpgServiceInfo> LeftDoubleClick;
        public event Action<EpgServiceInfo> Click;

        public ServiceView()
        {
            InitializeComponent();
        }

        public void ClearInfo()
        {
            stackPanel_service.Children.Clear();
        }

        public void SetService(List<EpgServiceInfo> serviceList, double serviceWidth, Brush backgroundBrush, bool isLight, bool isClickLeft)
        {
            ClearInfo();
            uint tickCountToPreventAccidentalClick = (uint)Environment.TickCount;

            foreach (EpgServiceInfo info in serviceList)
            {
                var item = new TextBlock()
                {
                    Style = (Style)FindResource(isLight ? "AppEpgServiceHeaderLightBackgroundTextBlockStyle" : "AppEpgServiceHeaderTextBlockStyle"),
                    Text = info.service_name
                };
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
                Grid.SetColumn(item, 1);
                Grid.SetRowSpan(item, 2);
                var grid = new Grid()
                {
                    Background = backgroundBrush,
                    Margin = new Thickness(1, 1, 1, 1),
                    Width = serviceWidth - 2
                };
                grid.ColumnDefinitions.Add(new ColumnDefinition() { Width = GridLength.Auto });
                grid.ColumnDefinitions.Add(new ColumnDefinition());
                grid.RowDefinitions.Add(new RowDefinition());
                grid.RowDefinitions.Add(new RowDefinition());

                DispatcherTimer clickTimer = null;
                grid.MouseLeftButtonDown += (sender, e) =>
                {
                    if (e.ClickCount == 1 && clickTimer == null && isClickLeft && Click != null && (uint)e.Timestamp - tickCountToPreventAccidentalClick > 500)
                    {
                        // ダブルクリックと区別するため
                        clickTimer = new DispatcherTimer();
                        clickTimer.Interval = CommonUtil.GetDoubleClickTime() + TimeSpan.FromMilliseconds(100);
                        var tag = (EpgServiceInfo)((FrameworkElement)sender).Tag;
                        clickTimer.Tick += (sender2, e2) =>
                        {
                            clickTimer.Stop();
                            clickTimer = null;
                            if (Click != null)
                            {
                                Click(tag);
                            }
                        };
                        clickTimer.Start();
                    }
                    else if (clickTimer != null)
                    {
                        clickTimer.Stop();
                        clickTimer = null;
                    }
                    if (e.ClickCount == 2 && LeftDoubleClick != null)
                    {
                        LeftDoubleClick((EpgServiceInfo)((FrameworkElement)sender).Tag);
                    }
                };
                if (!isClickLeft)
                {
                    grid.MouseRightButtonUp += (sender, e) =>
                    {
                        if (Click != null && (uint)e.Timestamp - tickCountToPreventAccidentalClick > 500)
                        {
                            Click((EpgServiceInfo)((FrameworkElement)sender).Tag);
                        }
                    };
                }
                grid.Tag = info;
                grid.Children.Add(item);
                stackPanel_service.Children.Add(grid);
            }

            RefreshLogo();
        }

        public void RefreshLogo()
        {
            foreach (Grid grid in stackPanel_service.Children)
            {
                Image logoItem = grid.Children.OfType<Image>().FirstOrDefault();
                if (logoItem != null)
                {
                    grid.Children.Remove(logoItem);
                }
                var info = (EpgServiceInfo)grid.Tag;
                ChSet5Item ch;
                if (ChSet5.Instance.ChList.TryGetValue(CommonManager.Create64Key(info.ONID, info.TSID, info.SID), out ch) && ch.Logo != null)
                {
                    grid.Children.Insert(0, new Image()
                    {
                        Margin = new Thickness(1, 2, 0, 0),
                        Source = ch.Logo,
                        VerticalAlignment = VerticalAlignment.Top
                    });
                }
            }
        }
    }
}

using System.Collections.Generic;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer.EpgView
{
    /// <summary>
    /// ServiceView.xaml の相互作用ロジック
    /// </summary>
    public partial class ServiceView : UserControl
    {
        public ServiceView()
        {
            InitializeComponent();
            this.Background = CommonManager.Instance.EpgServiceBorderColor;
        }

        public void ClearInfo()
        {
            stackPanel_service.Children.Clear();
        }

        public void SetService(List<EpgServiceInfo> serviceList)
        {
            stackPanel_service.Children.Clear();
            foreach (EpgServiceInfo info in serviceList)
            {
                var service1 = new StackPanel();
                service1.Width = Settings.Instance.ServiceWidth - 1;
                service1.Margin = new Thickness(0, 1, 1, 1);
                service1.Background = CommonManager.Instance.EpgServiceBackColor;
                service1.MouseLeftButtonDown += new MouseButtonEventHandler(item_MouseLeftButtonDown);
                service1.DataContext = info;

                var text = ViewUtil.GetPanelTextBlock(info.service_name);
                text.Margin = new Thickness(1, 0, 1, 0);
                text.Foreground = CommonManager.Instance.EpgServiceFontColor;
                service1.Children.Add(text);

                text = ViewUtil.GetPanelTextBlock(info.remote_control_key_id != 0 ? info.remote_control_key_id.ToString() : info.network_name + " " + info.SID.ToString());
                text.Margin = new Thickness(1, 0, 1, 2);
                text.Foreground = CommonManager.Instance.EpgServiceFontColor;
                service1.Children.Add(text);

                stackPanel_service.Children.Add(service1);
            }
        }

        void item_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ClickCount == 2)
            {
                if (sender.GetType() == typeof(TextBlock))
                {
                    TextBlock item = sender as TextBlock;
                    EpgServiceInfo serviceInfo = item.DataContext as EpgServiceInfo;
                    CommonManager.Instance.TVTestCtrl.SetLiveCh(serviceInfo.ONID, serviceInfo.TSID, serviceInfo.SID);
                }
            }
        }
    }
}

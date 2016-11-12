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
                var item = new TextBlock();
                item.Text = info.service_name;
                if (info.remote_control_key_id != 0)
                {
                    item.Text += "\r\n" + info.remote_control_key_id.ToString();
                }
                else
                {
                    item.Text += "\r\n" + info.network_name + " " + info.SID.ToString();
                }
                item.Width = Settings.Instance.ServiceWidth - 1;
                item.Padding = new Thickness(0, 0, 0, 2);
                item.Margin = new Thickness(0, 1, 1, 1);
                item.Background = CommonManager.Instance.EpgServiceBackColor;
                item.Foreground = CommonManager.Instance.EpgServiceFontColor;
                item.TextAlignment = TextAlignment.Center;
                item.FontSize = 12;
                item.MouseLeftButtonDown += new MouseButtonEventHandler(item_MouseLeftButtonDown);
                item.DataContext = info;
                stackPanel_service.Children.Add(item);
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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel;

namespace EpgTimer
{
    public class ServiceItem2 : INotifyPropertyChanged
    {
        public event PropertyChangedEventHandler PropertyChanged;
        private bool selected = false;

        private void NotifyPropertyChanged(String info)
        {
            if (PropertyChanged != null)
            {
                PropertyChanged(this, new PropertyChangedEventArgs(info));
            }
        }


        public ChSet5Item ServiceInfo
        {
            get;
            set;
        }
        public bool IsSelected
        {
            get
            {
                return this.selected;
            }
            set
            {
                this.selected = value;
                NotifyPropertyChanged("IsSelected");
            }
        }
        public String ServiceName
        {
            get { return ServiceInfo.ServiceName; }
        }
        public String ToolTipView
        {
            get
            {
                if (Settings.Instance.NoToolTip == true)
                {
                    return null;
                }
                String viewTip = "";

                if (ServiceInfo != null)
                {
                    viewTip =
                        "service_name : " + ServiceInfo.ServiceName + "\r\n" +
                        "service_type : " + ServiceInfo.ServiceType.ToString() + "(0x" + ServiceInfo.ServiceType.ToString("X2") + ")" + "\r\n" +
                        "original_network_id : " + ServiceInfo.ONID.ToString() + "(0x" + ServiceInfo.ONID.ToString("X4") + ")" + "\r\n" +
                        "transport_stream_id : " + ServiceInfo.TSID.ToString() + "(0x" + ServiceInfo.TSID.ToString("X4") + ")" + "\r\n" +
                        "service_id : " + ServiceInfo.SID.ToString() + "(0x" + ServiceInfo.SID.ToString("X4") + ")" + "\r\n" +
                        "partial_reception : " + (ServiceInfo.PartialFlag ? 1 : 0);
                }
                return viewTip;
            }
        }
    }
}

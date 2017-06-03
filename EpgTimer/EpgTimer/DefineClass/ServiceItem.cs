using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.ComponentModel;

namespace EpgTimer
{
    public class ServiceItem : INotifyPropertyChanged
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

        public EpgServiceInfo ServiceInfo
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
        public UInt64 ID
        {
            get { return CommonManager.Create64Key(ServiceInfo.ONID, ServiceInfo.TSID, ServiceInfo.SID); }
        }
        public String ServiceName
        {
            get { return ServiceInfo.service_name; }
        }
        public String NetworkName
        {
            get { return CommonManager.ConvertNetworkNameText(ServiceInfo.ONID); }
        }
    }
}

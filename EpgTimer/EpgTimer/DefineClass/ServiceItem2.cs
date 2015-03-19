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
                return CommonManager.Instance.ConvertServiceItemText(ServiceInfo);
            }
        }
    }
}

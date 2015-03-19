using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class ServiceViewItem
    {
        public ServiceViewItem(ChSet5Item info)
        {
            ServiceInfo = info;
        }
        public ChSet5Item ServiceInfo
        {
            get;
            set;
        }
        public UInt64 Key
        {
            get
            {
                UInt64 key = ((UInt64)ServiceInfo.ONID) << 32 | ((UInt64)ServiceInfo.TSID) << 16 | (UInt64)ServiceInfo.SID;
                return key;
            }
        }
        public override string ToString()
        {
            if (ServiceInfo != null)
            {
                return ServiceInfo.ServiceName;
            }
            else
            {
                return "";
            }
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

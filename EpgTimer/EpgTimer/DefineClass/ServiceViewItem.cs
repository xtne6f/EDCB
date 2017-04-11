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

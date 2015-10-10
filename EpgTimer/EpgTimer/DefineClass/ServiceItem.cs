using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class ServiceItem : SelectableItem
    {
        public EpgServiceInfo ServiceInfo { get; set; }
        public UInt64 ID
        {
            get { return ServiceInfo.Create64Key(); }
        }
        public String ServiceName
        {
            get { return ServiceInfo.service_name; }
        }
        public String NetworkName
        {
            get
            {
                String name = "その他";
                if (ServiceInfo.ONID == 0x0004)
                {
                    name = "BS";
                }
                else if (ServiceInfo.ONID == 0x0006 || ServiceInfo.ONID == 0x0007)
                {
                    name = "CS";
                }
                else if (0x7880 <= ServiceInfo.ONID && ServiceInfo.ONID <= 0x7FE8)
                {
                    name = "地デジ";
                }
                return name;
            }
        }
    }
}

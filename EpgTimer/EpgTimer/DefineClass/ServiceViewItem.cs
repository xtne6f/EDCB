﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Controls;

namespace EpgTimer
{
    public class ServiceViewItem : SelectableItemNWMode
    {
        public ServiceViewItem(ChSet5Item info)
        {
            ServiceInfo = info;
        }
        public ChSet5Item ServiceInfo { get; set; }
        public UInt64 Key
        { 
            get { return ServiceInfo.Key; }
        }
        public String ServiceName
        { 
            get { return ServiceInfo.ServiceName; }
        }
        public TextBlock ToolTipView
        {
            get
            {
                if (Settings.Instance.NoToolTip == true) return null;

                return ViewUtil.GetTooltipBlockStandard(
                    "ServiceName : " + ServiceInfo.ServiceName + "\r\n" +
                    "ServiceType : " + ServiceInfo.ServiceType.ToString() + "(0x" + ServiceInfo.ServiceType.ToString("X2") + ")" + "\r\n" +
                    CommonManager.Convert64KeyString(ServiceInfo.Key) + "\r\n" +
                    "PartialReception : " + ServiceInfo.PartialFlag.ToString());
            }
        }
    }
}

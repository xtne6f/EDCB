using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class TunerNameViewItem
    {
        public TunerNameViewItem(TunerReserveInfo info, double width)
        {
            TunerInfo = info;
            Width = width;
        }
        public TunerReserveInfo TunerInfo
        {
            get;
            set;
        }
        public double Width
        {
            get;
            set;
        }
    }
}

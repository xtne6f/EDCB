using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace EpgTimer
{
    public class ReserveViewItem
    {
        public ReserveViewItem()
        {
            TitleDrawErr = false;
        }
        public ReserveViewItem(ReserveData info)
        {
            TitleDrawErr = false;
            ReserveInfo = info;
        }

        public ReserveData ReserveInfo
        {
            get;
            set;
        }
        public double Width
        {
            get;
            set;
        }

        public double Height
        {
            get;
            set;
        }

        public double LeftPos
        {
            get;
            set;
        }

        public double TopPos
        {
            get;
            set;
        }

        public bool TitleDrawErr
        {
            get;
            set;
        }
    }
}

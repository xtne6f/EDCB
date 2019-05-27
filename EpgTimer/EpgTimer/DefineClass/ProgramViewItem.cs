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
    public class ProgramViewItem
    {
        public ProgramViewItem(EpgEventInfo info, bool past, bool filtered)
        {
            EventInfo = info;
            Past = past;
            Filtered = filtered;
        }

        public EpgEventInfo EventInfo
        {
            get;
            private set;
        }

        public bool Past
        {
            get;
            private set;
        }

        public bool Filtered
        {
            get;
            private set;
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

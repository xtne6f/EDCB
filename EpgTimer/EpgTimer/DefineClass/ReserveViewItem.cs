using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class ReserveViewItem : ViewPanelItem<ReserveData>
    {
        public ReserveViewItem(ReserveData info) : base(info) { }
        public ReserveData ReserveInfo
        {
            get { return _Data; }
            set { _Data = value; }
        }
    }
}

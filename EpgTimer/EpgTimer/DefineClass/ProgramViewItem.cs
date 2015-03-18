using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class ProgramViewItem : EventItemBaseClass
    {
        public ProgramViewItem()
        {
            TitleDrawErr = false;
        }
        public ProgramViewItem(EpgEventInfo info)
        {
            base.EventInfo = info;
            TitleDrawErr = false;
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

        public Brush ContentColor
        {
            get
            {
                return base.BorderBrush;
            }
        }
    }
}

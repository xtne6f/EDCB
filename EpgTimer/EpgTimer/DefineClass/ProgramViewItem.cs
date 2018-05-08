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
        public ProgramViewItem(EpgEventInfo info, bool past)
        {
            EventInfo = info;
            Past = past;
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
                if (EventInfo.ContentInfo != null)
                {
                    foreach (EpgContentData info in EventInfo.ContentInfo.nibbleList)
                    {
                        if ((info.content_nibble_level_1 <= 0x0B || info.content_nibble_level_1 == 0x0F) &&
                            CommonManager.Instance.CustContentColorList.Count > info.content_nibble_level_1)
                        {
                            return CommonManager.Instance.CustContentColorList[info.content_nibble_level_1];
                        }
                    }
                }
                if (CommonManager.Instance.CustContentColorList.Count > 0x10)
                {
                    return CommonManager.Instance.CustContentColorList[0x10];
                }
                return Brushes.White;
            }
        }
    }
}

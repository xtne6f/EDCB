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
        public ProgramViewItem()
        {
            TitleDrawErr = false;
        }
        public ProgramViewItem(EpgEventInfo info)
        {
            EventInfo = info;
            TitleDrawErr = false;
        }

        public EpgEventInfo EventInfo
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

        public SolidColorBrush ContentColor
        {
            get
            {
                //return null;
                SolidColorBrush color = Brushes.White;
                if (EventInfo != null)
                {
                    if (EventInfo.ContentInfo != null)
                    {
                        if (EventInfo.ContentInfo.nibbleList.Count > 0)
                        {
                            try
                            {
                                foreach (EpgContentData info in EventInfo.ContentInfo.nibbleList)
                                {
                                    if (info.content_nibble_level_1 <= 0x0B || info.content_nibble_level_1 == 0x0F && Settings.Instance.ContentColorList.Count > info.content_nibble_level_1)
                                    {
                                        color = CommonManager.Instance.CustContentColorList[info.content_nibble_level_1];
                                        break;
                                    }
                                }
                            }
                            catch
                            {
                            }
                        }
                        else
                        {
                            color = CommonManager.Instance.CustContentColorList[0x10];
                        }
                    }
                    else
                    {
                        color = CommonManager.Instance.CustContentColorList[0x10];
                    }
                }

                return color;
            }
        }
    }
}

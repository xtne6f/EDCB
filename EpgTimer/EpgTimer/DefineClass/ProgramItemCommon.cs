using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class ProgramItemCommon : EventItemBaseClass
    {
        public ReserveData ReserveInfo { get; set; }

        public String JyanruKey
        {
            get
            {
                if (EventInfo == null) return "";
                //
                return CommonManager.Instance.ConvertJyanruText(EventInfo);
            }
        }

        public SolidColorBrush ForeColor
        {
            get
            {
                if (ReserveInfo == null) return CommonManager.Instance.ListDefForeColor;
                //
                return CommonManager.Instance.EventItemForeColor(ReserveInfo.RecSetting.RecMode);
            }
        }

        public SolidColorBrush BackColor
        {
            get
            {
                SolidColorBrush color = CommonManager.Instance.ResDefBackColor;
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.RecSetting.RecMode == 5)
                    {
                        color = CommonManager.Instance.ResNoBackColor;
                    }
                    else if (ReserveInfo.OverlapMode == 2)
                    {
                        color = CommonManager.Instance.ResErrBackColor;
                    }
                    else if (ReserveInfo.OverlapMode == 1)
                    {
                        color = CommonManager.Instance.ResWarBackColor;
                    }
                }
                return color;
            }
        }

    }

    public class EventItemBaseClass
    {
        public virtual EpgEventInfo EventInfo { get; set; }

        public Brush BorderBrush
        {
            get
            {
                Brush color1 = Brushes.White;
                if (EventInfo != null)
                {
                    if (EventInfo.ContentInfo != null)
                    {
                        if (EventInfo.ContentInfo.nibbleList.Count > 0)
                        {
                            try
                            {
                                foreach (EpgContentData info1 in EventInfo.ContentInfo.nibbleList)
                                {
                                    if (info1.content_nibble_level_1 <= 0x0B || info1.content_nibble_level_1 == 0x0F && Settings.Instance.ContentColorList.Count > info1.content_nibble_level_1)
                                    {
                                        color1 = CommonManager.Instance.CustContentColorList[info1.content_nibble_level_1];
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
                            color1 = CommonManager.Instance.CustContentColorList[0x10];
                        }
                    }
                    else
                    {
                        color1 = CommonManager.Instance.CustContentColorList[0x10];
                    }
                }

                return color1;
            }
        }
    }
 }

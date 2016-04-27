using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Media;
using System.Windows.Controls;
using System.Windows;

namespace EpgTimer
{
    public class ReserveItem : SearchItem
    {
        public ReserveItem() { }
        public ReserveItem(ReserveData item) { base.ReserveInfo = item; }

        public override EpgEventInfo EventInfo
        {
            get
            {
                if (eventInfo == null)
                {
                    if (ReserveInfo != null)
                    {
                        eventInfo = ReserveInfo.SearchEventInfo(false);
                    }
                }
                return eventInfo;
            }
        }

        public override String EventName
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return ReserveInfo.Title;
            }
        }
        public override String ServiceName
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return ReserveInfo.StationName;
            }
        }
        public override String NetworkName
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return CommonManager.ConvertNetworkNameText(ReserveInfo.OriginalNetworkID);
            }
        }
        public override String StartTime
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return CommonManager.ConvertTimeText(ReserveInfo.StartTime, ReserveInfo.DurationSecond, Settings.Instance.ResInfoNoYear, Settings.Instance.ResInfoNoSecond);
            }
        }
        public override long StartTimeValue
        {
            get
            {
                if (ReserveInfo == null) return long.MinValue;
                //
                return ReserveInfo.StartTime.Ticks;
            }
        }
        public String StartTimeShort
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return CommonManager.ConvertTimeText(ReserveInfo.StartTime, ReserveInfo.DurationSecond, true, true);
            }
        }
        public override String ProgramDuration
        {
            get
            {
                if (ReserveInfo == null) return "";
                //
                return CommonManager.ConvertDurationText(ReserveInfo.DurationSecond, Settings.Instance.ResInfoNoDurSecond);
            }
        }
        public override UInt32 ProgramDurationValue
        {
            get
            {
                if (ReserveInfo == null) return UInt32.MinValue;
                //
                return ReserveInfo.DurationSecond;
            }
        }
        public override TextBlock ToolTipView
        {
            get
            {
                if (Settings.Instance.NoToolTip == true) return null;
                //
                return mutil.GetTooltipBlockStandard(ConvertInfoText());
            }
        }
        public String ConvertInfoText()
        {
            if (ReserveInfo == null) return "";
            //
            String view = CommonManager.ConvertTimeText(ReserveInfo.StartTime, ReserveInfo.DurationSecond, false, false, false) + "\r\n";
            view += ServiceName + "(" + NetworkName + ")" + "\r\n";
            view += EventName + "\r\n\r\n";

            view += ConvertRecSettingText() + "\r\n";
            view += "予約状況 : " + Comment;
            view += "\r\n\r\n";

            view += "OriginalNetworkID : " + ReserveInfo.OriginalNetworkID.ToString() + " (0x" + ReserveInfo.OriginalNetworkID.ToString("X4") + ")\r\n";
            view += "TransportStreamID : " + ReserveInfo.TransportStreamID.ToString() + " (0x" + ReserveInfo.TransportStreamID.ToString("X4") + ")\r\n";
            view += "ServiceID : " + ReserveInfo.ServiceID.ToString() + " (0x" + ReserveInfo.ServiceID.ToString("X4") + ")\r\n";
            view += "EventID : " + ReserveInfo.EventID.ToString() + " (0x" + ReserveInfo.EventID.ToString("X4") + ")";

            return view;
        }

        public override String Status
        {
            get
            {
                String[] wiewString = { "", "無", "予+", "無+", "録*", "無*" };
                int index = 0;
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.IsOnAir() == true)
                    {
                        index = 2;
                    }
                    if (ReserveInfo.IsOnRec() == true)//マージンがあるので、IsOnAir==trueとは限らない
                    {
                        index = 4;
                    }
                    if (ReserveInfo.IsEnabled == false) //無効の判定
                    {
                        index += 1;
                    }
                }
                return wiewString[index];
            }
        }
        public override SolidColorBrush StatusColor
        {
            get
            {
                if (ReserveInfo != null)
                {
                    if (ReserveInfo.IsOnRec() == true)
                    {
                        return CommonManager.Instance.StatRecForeColor;
                    }
                    if (ReserveInfo.IsOnAir() == true)
                    {
                        return CommonManager.Instance.StatOnAirForeColor;
                    }
                }
                return CommonManager.Instance.StatResForeColor;
            }
        }
    }
}

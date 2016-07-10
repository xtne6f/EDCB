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

        public override ulong KeyID { get { return ReserveInfo == null ? 0 : ReserveInfo.ReserveID; } }

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
        public override String ConvertInfoText()
        {
            if (ReserveInfo == null) return "";
            //
            String view = CommonManager.ConvertTimeText(ReserveInfo.StartTime, ReserveInfo.DurationSecond, false, false, false) + "\r\n";
            view += ServiceName + "(" + NetworkName + ")" + "\r\n";
            view += EventName + "\r\n\r\n";

            view += ConvertRecSettingText() + "\r\n";
            view += "予約状況 : " + Comment + "\r\n\r\n";

            view += CommonManager.Convert64PGKeyString(ReserveInfo.Create64PgKey());

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
        public override Brush StatusColor
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

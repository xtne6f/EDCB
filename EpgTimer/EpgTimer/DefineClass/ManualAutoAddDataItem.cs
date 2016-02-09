using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Media;

namespace EpgTimer
{
    public class ManualAutoAddDataItem : AutoAddDataItemT<ManualAutoAddData>
    {
        public ManualAutoAddDataItem() { }
        public ManualAutoAddDataItem(ManualAutoAddData item) : base(item) { }

        public ManualAutoAddData ManualAutoAddInfo { get { return (ManualAutoAddData)_data; } set { _data = value; } }

        public static new string GetValuePropertyName(string key)
        {
            var obj = new ManualAutoAddDataItem();
            if (key == CommonUtil.GetMemberName(() => obj.StartTime))
            {
                return CommonUtil.GetMemberName(() => obj.StartTimeValue);
            }
            else if (key == CommonUtil.GetMemberName(() => obj.ProgramDuration))
            {
                return CommonUtil.GetMemberName(() => obj.ProgramDurationValue);
            }
            else if (key == CommonUtil.GetMemberName(() => obj.DayOfWeek))
            {
                return CommonUtil.GetMemberName(() => obj.DayOfWeekValue);
            }
            else
            {
                return AutoAddDataItem.GetValuePropertyName(key);
            }
        }

        public override bool IsManual { get { return true; } }

        public String DayOfWeek
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                String view = "";
                byte dayOfWeekFlag = GetWeekFlgMod();
                for (int i = 0; i < 7; i++)
                {
                    if ((dayOfWeekFlag & 0x01) != 0)
                    {
                        view += "日月火水木金土"[i];
                    }
                    dayOfWeekFlag >>= 1;
                }
                return view;
            }
        }
        public double DayOfWeekValue
        {
            get
            {
                if (ManualAutoAddInfo == null) return double.MinValue;
                //
                int ret = 0;
                byte dayOfWeekFlag = GetWeekFlgMod();
                for (int i = 1; i <= 7; i++)
                {
                    if ((dayOfWeekFlag & 0x01) != 0)
                    {
                        ret = 10 * ret + i;
                    }
                    dayOfWeekFlag >>= 1;
                }
                return ret * Math.Pow(10, (7 - ret.ToString().Length));
            }
        }
        private byte GetWeekFlgMod()
        {
            if (Settings.Instance.LaterTimeUse == true && DateTime28.IsLateHour(ManualAutoAddInfo.PgStartTime.Hour) == true)
            {
                return ManualAutoAddData.ShiftWeekFlag(ManualAutoAddInfo.dayOfWeekFlag, -1);
            }
            return ManualAutoAddInfo.dayOfWeekFlag;
        }
        public String StartTime
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                return CommonManager.ConvertTimeText(ManualAutoAddInfo.PgStartTime
                    , ManualAutoAddInfo.durationSecond, true, Settings.Instance.ResInfoNoSecond, true, true);
            }
        }
        public UInt32 StartTimeValue
        {
            get
            {
                if (ManualAutoAddInfo == null) return UInt32.MinValue;
                //
                return ManualAutoAddInfo.startTime;
            }
        }
        public String StartTimeShort
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                return CommonManager.ConvertTimeText(ManualAutoAddInfo.PgStartTime
                    , ManualAutoAddInfo.durationSecond, true, true, true, true);
            }
        }
        public String ProgramDuration
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                return CommonManager.ConvertDurationText(ManualAutoAddInfo.PgDurationSecond, Settings.Instance.ResInfoNoDurSecond);
            }
        }
        public UInt32 ProgramDurationValue
        {
            get
            {
                if (ManualAutoAddInfo == null) return UInt32.MinValue;
                //
                return ManualAutoAddInfo.PgDurationSecond;
            }
        }
        public override String NetworkName
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                return CommonManager.ConvertNetworkNameText(ManualAutoAddInfo.originalNetworkID);
            }
        }
        public override String ServiceName
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                return ManualAutoAddInfo.stationName;
            }
        }
    }

}

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
            else
            {
                return AutoAddDataItem.GetValuePropertyName(key);
            }
        }

        public String DayOfWeek
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                byte weekFlgMod = ManualAutoAddInfo.dayOfWeekFlag;
                if (Settings.Instance.LaterTimeUse == true && DateTime28.IsLateHour(ManualAutoAddInfo.PgStartTime.Hour) == true)
                {
                    weekFlgMod = ManualAutoAddData.ShiftWeekFlag(weekFlgMod, -1);
                }

                String view = "";
                for (int i = 0; i < 7; i++)
                {
                    if ((weekFlgMod & 0x01 << i) != 0)
                    {
                        view += "日月火水木金土"[i];
                    }
                }
                return view;
            }
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

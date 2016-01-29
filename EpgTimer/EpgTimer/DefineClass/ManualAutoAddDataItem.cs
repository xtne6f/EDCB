using System;
using System.Collections.Generic;
using System.Linq;
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
            if (key == CommonUtil.GetMemberName(() => obj.Time))
            {
                return CommonUtil.GetMemberName(() => obj.TimeValue);
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
                String view = "";
                String[] wiewString = { "日", "月", "火", "水", "木", "金", "土" };
                for (int i = 0; i < 7; i++)
                {
                    if ((ManualAutoAddInfo.dayOfWeekFlag & 0x01<<i) != 0)
                    {
                        view += wiewString[i];
                    }
                }
                return view;
            }
        }
        public String Time
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                return CommonManager.ConvertTimeText(ManualAutoAddInfo.PgStartTime
                    , ManualAutoAddInfo.durationSecond, true, Settings.Instance.ResInfoNoSecond, true, true);
            }
        }
        public UInt32 TimeValue
        {
            get
            {
                if (ManualAutoAddInfo == null) return UInt32.MinValue;
                //
                return ManualAutoAddInfo.startTime;
            }
        }
        public String TimeShort
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                return CommonManager.ConvertTimeText(ManualAutoAddInfo.PgStartTime
                    , ManualAutoAddInfo.durationSecond, true, true, true, true);
            }
        }
        public String StationName
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                return ManualAutoAddInfo.stationName;
            }
        }
        public SolidColorBrush ForeColor
        {
            get
            {
                return CommonManager.Instance.ListDefForeColor;
            }
        }
    }

}

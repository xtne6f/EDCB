using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Media;

namespace EpgTimer
{
    public class ManualAutoAddDataItem
    {
        public ManualAutoAddDataItem() { }
        public ManualAutoAddDataItem(ManualAutoAddData item)
        {
            this.ManualAutoAddInfo = item;
        }

        public ManualAutoAddData ManualAutoAddInfo { get; set; }

        public static string GetValuePropertyName(string key)
        {
            var obj = new ManualAutoAddDataItem();
            if (key == CommonUtil.GetMemberName(() => obj.Time))
            {
                return CommonUtil.GetMemberName(() => obj.TimeValue);
            }
            else
            {
                return key;
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
                return CommonManager.ConvertTimeText(new DateTime().AddSeconds(ManualAutoAddInfo.startTime)
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
                return CommonManager.ConvertTimeText(new DateTime().AddSeconds(ManualAutoAddInfo.startTime)
                    , ManualAutoAddInfo.durationSecond, true, true, true, true);
            }
        }
        public String Title
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                return ManualAutoAddInfo.title;
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
        public String RecMode
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                return CommonManager.Instance.ConvertRecModeText(ManualAutoAddInfo.recSetting.RecMode);
            }
        }
        public String Priority
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                return ManualAutoAddInfo.recSetting.Priority.ToString();
            }
        }
        public String ReserveCount
        {
            get
            {
                if (ManualAutoAddInfo == null) return "";
                //
                return ManualAutoAddInfo.GetReserveList().Count.ToString();
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

    public static class ManualAutoAddDataItemEx
    {
        public static List<ManualAutoAddData> ManualAutoAddInfoList(this ICollection<ManualAutoAddDataItem> itemlist)
        {
            return itemlist.Where(item => item != null).Select(item => item.ManualAutoAddInfo).ToList();
        }
    }
}

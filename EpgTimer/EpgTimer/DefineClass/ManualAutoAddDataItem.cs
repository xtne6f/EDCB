using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class ManualAutoAddDataItem
    {
        public ManualAutoAddDataItem(ManualAutoAddData item)
        {
            this.ManualAutoAddInfo = item;
        }

        public ManualAutoAddData ManualAutoAddInfo
        {
            get;
            private set;
        }

        public String DayOfWeek
        {
            get
            {
                String view = "";
                for (int i = 0; i < 7; i++)
                {
                    if ((ManualAutoAddInfo.dayOfWeekFlag & (0x01 << i)) != 0)
                    {
                        view += (new DateTime(2000, 1, 2 + i)).ToString("ddd");
                    }
                }
                return view;
            }
        }

        public String Time
        {
            get
            {
                return (new DateTime(2000, 1, 2)).AddSeconds(ManualAutoAddInfo.startTime).ToString("HH\\:mm\\:ss ～ ") +
                       (new DateTime(2000, 1, 2)).AddSeconds(ManualAutoAddInfo.startTime + ManualAutoAddInfo.durationSecond).ToString("HH\\:mm\\:ss");
            }
        }

        public String Title
        {
            get { return ManualAutoAddInfo.title; }
        }

        public String StationName
        {
            get { return ManualAutoAddInfo.stationName; }
        }

        public String RecMode
        {
            get
            {
                return CommonManager.Instance.RecModeList.Length > ManualAutoAddInfo.recSetting.RecMode ?
                       CommonManager.Instance.RecModeList[ManualAutoAddInfo.recSetting.RecMode] : "";
            }
        }

        public byte Priority
        {
            get { return ManualAutoAddInfo.recSetting.Priority; }
        }

        public uint ID
        {
            get { return ManualAutoAddInfo.dataID; }
        }
    }
}

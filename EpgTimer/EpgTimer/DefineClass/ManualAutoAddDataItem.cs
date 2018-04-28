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
                        view += CommonManager.Instance.DayOfWeekArray[i];
                    }
                }
                return view;
            }
        }

        public String Time
        {
            get
            {
                String view = "";
                {
                    UInt32 hh = ManualAutoAddInfo.startTime / (60 * 60);
                    UInt32 mm = (ManualAutoAddInfo.startTime % (60 * 60)) / 60;
                    UInt32 ss = ManualAutoAddInfo.startTime % 60;
                    view = hh.ToString() + ":" + mm.ToString() + ":" + ss.ToString();

                    UInt32 endTime = ManualAutoAddInfo.startTime + ManualAutoAddInfo.durationSecond;
                    if (endTime > 24 * 60 * 60)
                    {
                        endTime -= 24 * 60 * 60;
                    }
                    hh = endTime / (60 * 60);
                    mm = (endTime % (60 * 60)) / 60;
                    ss = endTime % 60;
                    view += " ～ " + hh.ToString() + ":" + mm.ToString() + ":" + ss.ToString();
                }
                return view;
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
    }
}

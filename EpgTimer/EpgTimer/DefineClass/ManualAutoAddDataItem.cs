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
            set;
        }

        public String DayOfWeek
        {
            get
            {
                String view = "";
                if (ManualAutoAddInfo != null)
                {
                    if ((ManualAutoAddInfo.dayOfWeekFlag & 0x01) != 0)
                    {
                        view += "日";
                    }
                    if ((ManualAutoAddInfo.dayOfWeekFlag & 0x02) != 0)
                    {
                        view += "月";
                    }
                    if ((ManualAutoAddInfo.dayOfWeekFlag & 0x04) != 0)
                    {
                        view += "火";
                    }
                    if ((ManualAutoAddInfo.dayOfWeekFlag & 0x08) != 0)
                    {
                        view += "水";
                    }
                    if ((ManualAutoAddInfo.dayOfWeekFlag & 0x10) != 0)
                    {
                        view += "木";
                    }
                    if ((ManualAutoAddInfo.dayOfWeekFlag & 0x20) != 0)
                    {
                        view += "金";
                    }
                    if ((ManualAutoAddInfo.dayOfWeekFlag & 0x40) != 0)
                    {
                        view += "土";
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
                if (ManualAutoAddInfo != null)
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
            get
            {
                String view = "";
                if (ManualAutoAddInfo != null)
                {
                    view = ManualAutoAddInfo.title;
                }
                return view;
            }
        }

        public String StationName
        {
            get
            {
                String view = "";
                if (ManualAutoAddInfo != null)
                {
                    view = ManualAutoAddInfo.stationName;
                }
                return view;
            }
        }

        public String RecMode
        {
            get
            {
                String view = "";
                if (ManualAutoAddInfo != null)
                {
                    switch (ManualAutoAddInfo.recSetting.RecMode)
                    {
                        case 0:
                            view = "全サービス";
                            break;
                        case 1:
                            view = "指定サービス";
                            break;
                        case 2:
                            view = "全サービス（デコード処理なし）";
                            break;
                        case 3:
                            view = "指定サービス（デコード処理なし）";
                            break;
                        case 4:
                            view = "視聴";
                            break;
                        case 5:
                            view = "無効";
                            break;
                        default:
                            break;
                    }
                }
                return view;
            }
        }

        public String Priority
        {
            get
            {
                String view = "";
                if (ManualAutoAddInfo != null)
                {
                    view = ManualAutoAddInfo.recSetting.Priority.ToString();
                }
                return view;
            }
        }
    }
}

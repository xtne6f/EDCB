using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media;

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

        public string DayOfWeek
        {
            get
            {
                string view = "";
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

        public string Time
        {
            get
            {
                return (new DateTime(2000, 1, 2)).AddSeconds(ManualAutoAddInfo.startTime).ToString("HH\\:mm\\:ss ～ ") +
                       (new DateTime(2000, 1, 2)).AddSeconds(ManualAutoAddInfo.startTime + ManualAutoAddInfo.durationSecond).ToString("HH\\:mm\\:ss");
            }
        }

        public string Title
        {
            get { return ManualAutoAddInfo.title; }
        }

        public string StationName
        {
            get { return ManualAutoAddInfo.stationName; }
        }

        public string RecEnabled
        {
            get { return ManualAutoAddInfo.recSetting.IsNoRec() ? "いいえ" : "はい"; }
        }

        public string RecMode
        {
            get { return CommonManager.Instance.RecModeList[ManualAutoAddInfo.recSetting.GetRecMode()]; }
        }

        public byte Priority
        {
            get { return ManualAutoAddInfo.recSetting.Priority; }
        }

        public string TunerID
        {
            get { return ManualAutoAddInfo.recSetting.TunerID == 0 ? "自動" : "ID:" + ManualAutoAddInfo.recSetting.TunerID.ToString("X8"); }
        }

        public string BatFilePath
        {
            get
            {
                int i = ManualAutoAddInfo.recSetting.BatFilePath.IndexOf('*');
                return i < 0 ? ManualAutoAddInfo.recSetting.BatFilePath : ManualAutoAddInfo.recSetting.BatFilePath.Remove(i);
            }
        }

        public string BatFileTag
        {
            get
            {
                int i = ManualAutoAddInfo.recSetting.BatFilePath.IndexOf('*');
                return i < 0 ? "" : ManualAutoAddInfo.recSetting.BatFilePath.Substring(i + 1);
            }
        }

        public uint ID
        {
            get { return ManualAutoAddInfo.dataID; }
        }

        public SolidColorBrush BackColor
        {
            get { return Settings.BrushCache.ResDefBrush; }
        }
    }
}

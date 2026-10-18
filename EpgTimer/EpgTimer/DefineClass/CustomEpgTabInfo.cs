using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class CustomEpgTabInfo
    {
        public enum SpecialViewServices : ulong
        {
            ViewServiceDttv = 0x1000000000000,
            ViewServiceBS,
            ViewServiceCS,
            ViewServiceCS3,
            ViewServiceOther,
        }

        public CustomEpgTabInfo()
        {
            ViewServiceList = new List<ulong>();
            ViewContentKindList = new List<ushort>();
            StartTimeWeek = 4;
            HighlightContentKind = true;
            SearchKey = new EpgSearchKeyInfo();
        }
        public string TabName
        {
            get;
            set;
        }
        public int EpgSettingIndex
        {
            get;
            set;
        }
        public int ViewMode
        {
            get;
            set;
        }
        public bool NeedTimeOnlyBasic
        {
            get;
            set;
        }
        public bool NeedTimeOnlyWeek
        {
            get;
            set;
        }
        public int StartTimeWeek
        {
            get;
            set;
        }
        public List<ulong> ViewServiceList
        {
            get;
            set;
        }
        public List<ushort> ViewContentKindList
        {
            get;
            set;
        }
        public bool HighlightContentKind
        {
            get;
            set;
        }
        public bool SearchMode
        {
            get;
            set;
        }
        public EpgSearchKeyInfo SearchKey
        {
            get;
            set;
        }
        public bool FilterEnded
        {
            get;
            set;
        }

        [System.Xml.Serialization.XmlIgnore]
        public EpgSetting EpgSetting
        {
            get
            {
                return Settings.Instance.EpgSettingList[Math.Min(Math.Max(EpgSettingIndex, 0), Settings.Instance.EpgSettingList.Count - 1)];
            }
        }

        public CustomEpgTabInfo DeepClone()
        {
            var other = (CustomEpgTabInfo)MemberwiseClone();
            other.ViewServiceList = ViewServiceList.ToList();
            other.ViewContentKindList = ViewContentKindList.ToList();
            other.SearchKey = SearchKey.DeepClone();
            return other;
        }

        public override string ToString()
        {
            return TabName;
        }
    }
}

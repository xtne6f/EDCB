using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class CustomEpgTabInfo
    {
        public CustomEpgTabInfo()
        {
            ViewServiceList = new List<UInt64>();
            ViewContentKindList = new List<UInt16>();
            ViewMode = 0;
            NeedTimeOnlyBasic = false;
            NeedTimeOnlyWeek = false;
            StartTimeWeek = 4;
            SearchMode = false;
            SearchKey = new EpgSearchKeyInfo();
            FilterEnded = false;
        }
        public String TabName
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
        public List<UInt64> ViewServiceList
        {
            get;
            set;
        }
        public List<UInt16> ViewContentKindList
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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class DateItem
    {
        public DateItem()
        {
        }
        public EpgSearchDateInfo DateInfo
        {
            get;
            set;
        }
        public override string ToString()
        {
            if (DateInfo == null)
            {
                return "(empty)";
            }
            return string.Format("{0} {1:00}:{2:00} ～ {3} {4:00}:{5:00}",
                                 CommonManager.Instance.DayOfWeekArray[DateInfo.startDayOfWeek % 7], DateInfo.startHour, DateInfo.startMin,
                                 CommonManager.Instance.DayOfWeekArray[DateInfo.endDayOfWeek % 7], DateInfo.endHour, DateInfo.endMin);
        }
    }
}

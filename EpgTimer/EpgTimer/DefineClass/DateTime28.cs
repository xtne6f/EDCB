using System;
using System.Windows.Data;

namespace EpgTimer
{
    class DateTime28
    {
        /// <summary>[use28] null:深夜判定、true:深夜表示、false:通常表示</summary>
        public DateTime28(DateTime time, bool? use28 = null, DateTime28 ref_start = null)
        {
            Time = time;
            Ref_Start = ref_start;
            IsLate = use28 != null ? (bool)use28 : IsLateHour(Time.Hour) == true;
        }
        public DateTime Time { get; private set; }
        public DateTime28 Ref_Start { get; private set; }
        public bool IsLate { get; private set; }
        public DateTime DateTimeMod 
        { 
            get 
            {
                if (IsLate != true) return Time;

                if (Ref_Start == null)
                {
                    return Time.AddDays(-1);
                }
                else
                {
                    return Ref_Start.DateTimeMod;
                }
            } 
        }
        public int HourMod
        { 
            get 
            {
                if (IsLate != true) return Time.Hour;

                if (Ref_Start == null)
                {
                    return Time.Hour + 24;
                }
                else
                {
                    return (int)(Ref_Start.Time.TimeOfDay + (Time - Ref_Start.Time)).TotalHours + (Ref_Start.IsLate == true ? 24 : 0);
                }
            } 
        }

        public static bool IsLateHour(int hour24)
        {
            return hour24 < Settings.Instance.LaterTimeHour;
        }
        public static bool JudgeLateHour(DateTime end, DateTime ref_start, int end_margin_hour = 2)
        {
            end_margin_hour = Math.Max(end_margin_hour + 1 - (Settings.Instance.LaterTimeHour + 24 - new DateTime28(ref_start).HourMod), 0);
            return (IsLateHour(ref_start.Hour) == true || end.TimeOfDay < ref_start.TimeOfDay || ref_start.AddDays(1) <= end)
                && IsLateHour(end.Hour - end_margin_hour);
        }
    }
}

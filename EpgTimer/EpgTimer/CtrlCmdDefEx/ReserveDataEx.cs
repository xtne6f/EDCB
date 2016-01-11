using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public static class ReserveDataEx
    {
        public static bool IsAutoAddMissing(this ReserveData reserveInfo)
        {
            if (Settings.Instance.DisplayReserveAutoAddMissing == false) return false;
            //
            return CommonManager.Instance.DB.IsReserveAutoAddMissing(reserveInfo);
        }

        public static ReserveData GetNextReserve(this List<ReserveData> resList, bool IsTargetOffRes = false)
        {
            ReserveData ret = null;
            long value = long.MaxValue;

            foreach (ReserveData data in resList)
            {
                if (IsTargetOffRes == true || data.RecSetting.RecMode != 5)
                {
                    if (value > data.StartTime.ToBinary())
                    {
                        ret = data;
                        value = data.StartTime.ToBinary();
                    }
                }
            }

            return ret;
        }

        public static bool IsOnRec(this ReserveData reserveInfo, int MarginMin = 0)
        {
            if (reserveInfo == null) return false;
            //
            int StartMargin = CommonManager.Instance.MUtil.GetMargin(reserveInfo.RecSetting, true) + 60 * MarginMin;
            int EndMargin = CommonManager.Instance.MUtil.GetMargin(reserveInfo.RecSetting, false);

            DateTime startTime = reserveInfo.StartTime.AddSeconds(StartMargin * -1);
            int duration = (int)reserveInfo.DurationSecond + StartMargin + EndMargin;

            return CtrlCmdDefEx.isOnTime(startTime, duration);
        }

        public static bool IsOnAir(this ReserveData reserveInfo)
        {
            if (reserveInfo == null) return false;
            //
            return CtrlCmdDefEx.isOnTime(reserveInfo.StartTime, (int)reserveInfo.DurationSecond);
        }

        public static DateTime StartTimeWithMargin(this ReserveData reserveInfo, int MarginMin = 0)
        {
            if (reserveInfo == null) return new DateTime();
            //
            int StartMargin = CommonManager.Instance.MUtil.GetMargin(reserveInfo.RecSetting, true) + 60 * MarginMin;
            return reserveInfo.StartTime.AddSeconds(StartMargin * -1);
        }
        public static DateTime EndTimeWithMargin(this ReserveData reserveInfo)
        {
            if (reserveInfo == null) return new DateTime();
            //
            int EndMargin = CommonManager.Instance.MUtil.GetMargin(reserveInfo.RecSetting, false);
            return reserveInfo.StartTime.AddSeconds((int)reserveInfo.DurationSecond + EndMargin);
        }

        public static List<RecSettingData> RecSettingList(this List<ReserveData> list)
        {
            return list.Where(item => item != null).Select(item => item.RecSetting).ToList();
        }

        public static UInt64 Create64Key(this ReserveData obj)
        {
            return CommonManager.Create64Key(obj.OriginalNetworkID, obj.TransportStreamID, obj.ServiceID);
        }
        public static UInt64 Create64PgKey(this ReserveData obj)
        {
            return CommonManager.Create64PgKey(obj.OriginalNetworkID, obj.TransportStreamID, obj.ServiceID, obj.EventID);
        }

        public static List<ReserveData> Clone(this List<ReserveData> src) { return CopyObj.Clone(src, CopyData); }
        public static ReserveData Clone(this ReserveData src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this ReserveData src, ReserveData dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(ReserveData src, ReserveData dest)
        {
            dest.Comment = src.Comment;
            dest.DurationSecond = src.DurationSecond;
            dest.EventID = src.EventID;
            dest.OriginalNetworkID = src.OriginalNetworkID;
            dest.OverlapMode = src.OverlapMode;
            dest.RecFileNameList = src.RecFileNameList.ToList();
            dest.RecSetting = src.RecSetting.Clone();               //RecSettingData
            dest.ReserveID = src.ReserveID;
            dest.ReserveStatus = src.ReserveStatus;
            dest.ServiceID = src.ServiceID;
            dest.StartTime = src.StartTime;
            dest.StartTimeEpg = src.StartTimeEpg;
            dest.StationName = src.StationName;
            dest.Title = src.Title;
            dest.TransportStreamID = src.TransportStreamID;
        }

    }
}

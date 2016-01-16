using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

namespace EpgTimer
{
    public static class ReserveDataEx
    {
        static CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;

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

        public static EpgEventInfo SearchEventInfo(this ReserveData info, bool getSrv = false)
        {
            EpgEventInfo eventInfo = null;

            if (info != null)
            {
                try
                {
                    if (info.EventID != 0xFFFF)
                    {
                        UInt64 key = info.Create64Key();
                        if (CommonManager.Instance.DB.ServiceEventList.ContainsKey(key) == true)
                        {
                            foreach (EpgEventInfo eventChkInfo in CommonManager.Instance.DB.ServiceEventList[key].eventList)
                            {
                                if (eventChkInfo.event_id == info.EventID)
                                {
                                    eventInfo = eventChkInfo;
                                    break;
                                }
                            }
                        }
                        if (eventInfo == null && getSrv == true)
                        {
                            UInt64 pgId = info.Create64PgKey();
                            eventInfo = new EpgEventInfo();
                            cmd.SendGetPgInfo(pgId, ref eventInfo);
                        }
                    }
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }

            }

            return eventInfo;
        }

        public static EpgEventInfo SearchEventInfoLikeThat(this ReserveData resInfo)
        {
            if (resInfo == null) return null;
            double dist = double.MaxValue, dist1;
            EpgEventInfo eventPossible = null;

            UInt64 key = resInfo.Create64Key();
            if (CommonManager.Instance.DB.ServiceEventList.ContainsKey(key) == true)
            {
                foreach (EpgEventInfo eventChkInfo in CommonManager.Instance.DB.ServiceEventList[key].eventList)
                {
                    dist1 = Math.Abs((resInfo.StartTime - eventChkInfo.start_time).TotalSeconds);
                    double overlapLength = MenuUtil.CulcOverlapLength(resInfo.StartTime, resInfo.DurationSecond,
                                                            eventChkInfo.start_time, eventChkInfo.durationSec);

                    //開始時間が最も近いものを選ぶ。同じ差なら時間が前のものを選ぶ
                    if (overlapLength >= 0 && (dist > dist1 ||
                        dist == dist1 && (eventPossible == null || resInfo.StartTime > eventChkInfo.start_time)))
                    {
                        dist = dist1;
                        eventPossible = eventChkInfo;
                        if (dist == 0) break;
                    }
                }
            }

            return eventPossible;
        }

        public static List<EpgAutoAddData> GetEpgAutoAddList(this ReserveData resInfo)
        {
            if (resInfo == null || resInfo.IsEpgReserve() != true) return new List<EpgAutoAddData>();
            //
            return CommonManager.Instance.DB.EpgAutoAddList.Values
                .Where(data => data.GetReserveList().Contains(resInfo)).ToList();
        }

        public static List<ManualAutoAddData> GetManualAutoAddList(this ReserveData resInfo)
        {
            if (resInfo == null || resInfo.IsEpgReserve() == true) return new List<ManualAutoAddData>();
            //
            return CommonManager.Instance.DB.ManualAutoAddList.Values
                .Where(data => data.GetReserveList().Contains(resInfo)).ToList();
        }

        public static bool IsEpgReserve(this ReserveData reserveInfo)
        {
            if (reserveInfo == null) return false;
            //
            return reserveInfo.EventID != 0xFFFF;
        }

        public static bool IsAutoAdded(this ReserveData reserveInfo)
        {
            if (reserveInfo == null) return false;
            //
            return reserveInfo.Comment != "";
        }

        public static bool IsOnRec(this ReserveData reserveInfo, int MarginMin = 0)
        {
            if (reserveInfo == null) return false;
            //
            int StartMargin = reserveInfo.RecSetting.GetTrueMargin(true) + 60 * MarginMin;
            int EndMargin = reserveInfo.RecSetting.GetTrueMargin(false);

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
            int StartMargin = reserveInfo.RecSetting.GetTrueMargin(true) + 60 * MarginMin;
            return reserveInfo.StartTime.AddSeconds(StartMargin * -1);
        }
        public static DateTime EndTimeWithMargin(this ReserveData reserveInfo)
        {
            if (reserveInfo == null) return new DateTime();
            //
            int EndMargin = reserveInfo.RecSetting.GetTrueMargin(false);
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

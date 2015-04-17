using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    static class CtrlCmdCLIEx
    {
        //シリアライズとかもろもろは使えないので地道にコピーする。
        //ジェネリックは手続きの統一用で、各メソッドは別途定義する。
        public static List<EpgSearchKeyInfo> Clone(this List<EpgSearchKeyInfo> src) { return CopyObj.Clone(src, CopyData); }
        public static EpgSearchKeyInfo Clone(this EpgSearchKeyInfo src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this EpgSearchKeyInfo src, EpgSearchKeyInfo dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(EpgSearchKeyInfo src, EpgSearchKeyInfo dest)
        {
            dest.aimaiFlag = src.aimaiFlag;
            dest.andKey = src.andKey;
            dest.audioList = src.audioList.ToList();
            dest.contentList = src.contentList.Clone(); //EpgContentData
            dest.dateList = src.dateList.Clone();       //EpgSearchDateInfo
            dest.freeCAFlag = src.freeCAFlag;
            dest.notContetFlag = src.notContetFlag;
            dest.notDateFlag = src.notDateFlag;
            dest.notKey = src.notKey;
            dest.regExpFlag = src.regExpFlag;
            dest.serviceList = src.serviceList.ToList();
            dest.titleOnlyFlag = src.titleOnlyFlag;
            dest.videoList = src.videoList.ToList();
            dest.chkRecEnd = src.chkRecEnd;
            dest.chkRecDay = src.chkRecDay;
            dest.chkRecNoService = src.chkRecNoService;
            dest.chkDurationMin = src.chkDurationMin;
            dest.chkDurationMax = src.chkDurationMax;
        }

        public static List<EpgContentData> Clone(this List<EpgContentData> src) { return CopyObj.Clone(src, CopyData); }
        public static EpgContentData Clone(this EpgContentData src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this EpgContentData src, EpgContentData dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(EpgContentData src, EpgContentData dest)
        {
            dest.content_nibble_level_1 = src.content_nibble_level_1;
            dest.content_nibble_level_2 = src.content_nibble_level_2;
            dest.user_nibble_1 = src.user_nibble_1;
            dest.user_nibble_2 = src.user_nibble_2;
        }

        public static List<EpgSearchDateInfo> Clone(this List<EpgSearchDateInfo> src) { return CopyObj.Clone(src, CopyData); }
        public static EpgSearchDateInfo Clone(this EpgSearchDateInfo src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this EpgSearchDateInfo src, EpgSearchDateInfo dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(EpgSearchDateInfo src, EpgSearchDateInfo dest)
        {
            dest.endDayOfWeek = src.endDayOfWeek;
            dest.endHour = src.endHour;
            dest.endMin = src.endMin;
            dest.startDayOfWeek = src.startDayOfWeek;
            dest.startHour = src.startHour;
            dest.startMin = src.startMin;
        }
        
        public static UInt64 Create64Key(this EpgServiceInfo obj)
        {
            return CommonManager.Create64Key(obj.ONID, obj.TSID, obj.SID);
        }

        public static UInt64 Create64Key(this RecFileInfo obj)
        {
            return CommonManager.Create64Key(obj.OriginalNetworkID, obj.TransportStreamID, obj.ServiceID);
        }
        public static UInt64 Create64PgKey(this RecFileInfo obj)
        {
            return CommonManager.Create64PgKey(obj.OriginalNetworkID, obj.TransportStreamID, obj.ServiceID, obj.EventID);
        }

        public static UInt64 Create64Key(this ManualAutoAddData obj)
        {
            return CommonManager.Create64Key(obj.originalNetworkID, obj.transportStreamID, obj.serviceID);
        }

        public static UInt64 Create64Key(this EpgEventInfo obj)
        {
            return CommonManager.Create64Key(obj.original_network_id, obj.transport_stream_id, obj.service_id);
        }
        public static UInt64 Create64PgKey(this EpgEventInfo obj)
        {
            return CommonManager.Create64PgKey(obj.original_network_id, obj.transport_stream_id, obj.service_id, obj.event_id);
        }
        public static string Title(this EpgEventInfo info)
        {
            return (info.ShortInfo == null ? "" : info.ShortInfo.event_name);
        }
        
        public static UInt64 Create64Key(this EpgEventData obj)
        {
            return CommonManager.Create64Key(obj.original_network_id, obj.transport_stream_id, obj.service_id);
        }
        public static UInt64 Create64PgKey(this EpgEventData obj)
        {
            return CommonManager.Create64PgKey(obj.original_network_id, obj.transport_stream_id, obj.service_id, obj.event_id);
        }

        public static UInt64 Create64Key(this ReserveData obj)
        {
            return CommonManager.Create64Key(obj.OriginalNetworkID, obj.TransportStreamID, obj.ServiceID);
        }
        public static UInt64 Create64PgKey(this ReserveData obj)
        {
            return CommonManager.Create64PgKey(obj.OriginalNetworkID, obj.TransportStreamID, obj.ServiceID, obj.EventID);
        }

        public static bool IsOnRec(this ReserveData reserveInfo)
        {
            if (reserveInfo == null) return false;
            //
            int duration = (int)reserveInfo.DurationSecond;
            DateTime startTime = reserveInfo.StartTime;

            if (reserveInfo.RecSetting.UseMargineFlag == 1)
            {
                startTime = reserveInfo.StartTime.AddSeconds(reserveInfo.RecSetting.StartMargine * -1);
                duration += reserveInfo.RecSetting.StartMargine;
                duration += reserveInfo.RecSetting.EndMargine;
            }
            else
            {
                //TODO: ここでデフォルトマージンを確認するがEpgTimerNWでは無意味。根本的にはSendCtrlCmdの拡張が必要
                int defStartMargin = IniFileHandler.GetPrivateProfileInt("SET", "StartMargin", 0, SettingPath.TimerSrvIniPath);
                int defEndMargin = IniFileHandler.GetPrivateProfileInt("SET", "EndMargin", 0, SettingPath.TimerSrvIniPath);

                startTime = reserveInfo.StartTime.AddSeconds(defStartMargin * -1);
                duration += defStartMargin;
                duration += defEndMargin;
            }

            return isOnTime(startTime, duration);
        }

        public static bool IsOnAir(this ReserveData reserveInfo)
        {
            if (reserveInfo == null) return false;
            //
            return isOnTime(reserveInfo.StartTime, (int)reserveInfo.DurationSecond);
        }
        public static bool IsOnAir(this EpgEventInfo eventInfo)
        {
            if (eventInfo == null) return false;
            //
            return isOnTime(eventInfo.start_time, (int)eventInfo.durationSec);
        }

        private static bool isOnTime(DateTime startTime, int duration)
        {
            if (startTime > System.DateTime.Now) return false;
            //
            return (startTime + TimeSpan.FromSeconds(duration) >= System.DateTime.Now);
        }

        public static List<ReserveData> GetReserveList(this ManualAutoAddData mdata)
        {
            return CommonManager.Instance.DB.ReserveList.Values.Where(info =>
            {
                //イベントID無し、サービス、開始時刻、長さ一致の予約を拾う。EpgTimerSrv側と同じ内容の判定。
                return info != null && info.EventID == 0xFFFF
                    && mdata.Create64Key() == info.Create64Key()
                    && mdata.startTime == info.StartTime.Hour * 3600 + info.StartTime.Minute * 60 + info.StartTime.Second
                    && mdata.durationSecond == info.DurationSecond
                    && (mdata.dayOfWeekFlag & (byte)(0x01 << (int)info.StartTime.DayOfWeek)) != 0
                    ;
            }).ToList();
        }
    }
}

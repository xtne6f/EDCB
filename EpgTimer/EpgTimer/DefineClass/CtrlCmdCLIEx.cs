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

    }
}

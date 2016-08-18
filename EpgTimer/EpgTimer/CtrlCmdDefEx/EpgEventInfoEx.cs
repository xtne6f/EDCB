using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public partial class EpgEventInfo : AutoAddTargetData
    {
        public override string DataTitle { get { return (ShortInfo == null ? "" : ShortInfo.event_name); } }
        public override DateTime PgStartTime { get { return StartTimeFlag == 0 ? new DateTime() : start_time; } }
        public override uint PgDurationSecond { get { return DurationFlag == 0 ? 300 : durationSec; } }
        public override UInt64 Create64Key()
        {
            return CommonManager.Create64Key(original_network_id, transport_stream_id, service_id);
        }
        public override UInt64 Create64PgKey()
        {
            return CommonManager.Create64PgKey(original_network_id, transport_stream_id, service_id, event_id);
        }

        public bool IsOnAir()
        {
            return CtrlCmdDefEx.isOnTime(start_time, (int)durationSec);
        }

        public bool IsAvailable(bool isExceptUnknownStartTime, DateTime? exceptEndedTime = null)
        {
            //開始未定を除外。開始未定のときは時刻判定をしない。
            if (StartTimeFlag == 0)
            {
                return !isExceptUnknownStartTime;
            }
            //指定時刻に終了しているものを除外
            if (exceptEndedTime != null)
            {
                return start_time.AddSeconds(PgDurationSecond) >= exceptEndedTime;
            }
            return true;
        }
    }
    public static class EpgEventInfoEx
    {
        public static IEnumerable<EpgEventInfo> OfAvailable(this IEnumerable<EpgEventInfo> eventList, bool isExceptUnknownStartTime, DateTime? exceptEndedTime = null)
        {
            return eventList.Where(data => data != null && data.IsAvailable(isExceptUnknownStartTime, exceptEndedTime));
        }
    }
}

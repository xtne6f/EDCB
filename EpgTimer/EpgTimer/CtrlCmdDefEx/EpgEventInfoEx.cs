using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public partial class EpgEventInfo : AutoAddTargetData
    {
        public override string DataTitle { get { return (ShortInfo == null ? "" : ShortInfo.event_name); } }
        public override DateTime PgStartTime { get { return StartTimeFlag != 1 ? new DateTime() : start_time; } }
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
    }
}

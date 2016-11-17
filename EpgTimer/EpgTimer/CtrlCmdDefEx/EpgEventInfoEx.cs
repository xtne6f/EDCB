using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public partial class EpgEventInfo : AutoAddTargetData
    {
        public override string DataTitle { get { return (ShortInfo == null ? "" : ShortInfo.event_name); } }
        public override DateTime PgStartTime { get { return StartTimeFlag != 0 ? start_time : this.PastDataFlag == true ? DateTime.MinValue : DateTime.MaxValue; } }
        public override uint PgDurationSecond { get { return DurationFlag != 0 ? durationSec : 300; } }
        public override UInt64 Create64PgKey()
        {
            return CommonManager.Create64PgKey(original_network_id, transport_stream_id, service_id, event_id);
        }

        /// <summary>予約可能。StartTimeFlag != 0 && IsOver() != true</summary>
        public bool IsReservable
        {
            get { return StartTimeFlag != 0 && IsOver() != true; }
        }
        /// <summary>サービス2やサービス3の結合されるべきものはfalse </summary>
        public bool IsGroupMainEvent
        {
            get { return EventGroupInfo == null || EventGroupInfo.eventDataList.Any(data => data.Create64Key() == this.Create64Key()); }
        }
        /// <summary>サービス2やサービス3の結合されているもののメインイベント取得 </summary>
        public EpgEventInfo GetGroupMainEvent()
        {
            if (IsGroupMainEvent == true) return this;
            if (EventGroupInfo.group_type != 1) return null;
            return EventGroupInfo.eventDataList.Select(data => MenuUtil.SearchEventInfo(data.Create64PgKey()))
                                    .FirstOrDefault(data => data != null && data.IsGroupMainEvent == true);
        }
    }
}

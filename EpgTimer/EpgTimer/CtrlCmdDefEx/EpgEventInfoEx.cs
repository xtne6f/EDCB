using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public partial class EpgEventInfo : IAutoAddTargetData
    {
        public string DataTitle { get { return (ShortInfo == null ? "" : ShortInfo.event_name); } }
        public DateTime PgStartTime { get { return StartTimeFlag != 1 ? new DateTime() : start_time; } }
        public uint PgDurationSecond { get { return DurationFlag == 0 ? 300 : durationSec; } }
        public UInt64 Create64Key()
        {
            return CommonManager.Create64Key(original_network_id, transport_stream_id, service_id);
        }
        public UInt64 Create64PgKey()
        {
            return CommonManager.Create64PgKey(original_network_id, transport_stream_id, service_id, event_id);
        }

        public bool IsOnAir()
        {
            return CtrlCmdDefEx.isOnTime(start_time, (int)durationSec);
        }

        public List<EpgAutoAddData> GetEpgAutoAddList(bool? IsEnabled = null, bool ByFazy = false)
        {
            var list = CommonManager.Instance.DB.EpgAutoAddList.Values.GetAutoAddList(IsEnabled)
                .FindAll(data => data.GetSearchList().Any(item => item.EventInfo.Create64PgKey() == this.Create64PgKey()));
            if (ByFazy == true)
            {
                list.AddRange(CommonManager.Instance.MUtil.FazySearchEpgAutoAddData(DataTitle, IsEnabled));
                list = list.Distinct().ToList();
            }
            return list;
        }
        public List<ManualAutoAddData> GetManualAutoAddList(bool? IsEnabled = null)
        {
            return CommonManager.Instance.DB.ManualAutoAddList.Values.GetAutoAddList(IsEnabled)
                .FindAll(data => data.CheckPgHit(this) == true);
        }
    }
}

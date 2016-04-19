using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

namespace EpgTimer
{
    public partial class ReserveData : IAutoAddTargetData, IRecSetttingData
    {
        public string DataTitle { get { return Title; } }
        public DateTime PgStartTime { get { return StartTime; } }
        public uint PgDurationSecond { get { return DurationSecond; } }
        public UInt64 Create64Key()
        {
            return CommonManager.Create64Key(OriginalNetworkID, TransportStreamID, ServiceID);
        }
        public UInt64 Create64PgKey()
        {
            return CommonManager.Create64PgKey(OriginalNetworkID, TransportStreamID, ServiceID, EventID);
        }
        public RecSettingData RecSettingInfo { get { return RecSetting; } }
        public bool IsManual { get { return IsEpgReserve == false; } }

        public ReserveMode ReserveMode
        {
            get
            {
                if (IsAutoAdded == true)
                {
                    return IsEpgReserve == true ? ReserveMode.KeywordAuto : ReserveMode.ManualAuto;
                }
                else
                {
                    return IsEpgReserve == true ? ReserveMode.EPG : ReserveMode.Program;
                }

            }
        }
        public bool IsEpgReserve { get { return EventID != 0xFFFF; } }
        public bool IsAutoAdded { get { return Comment != ""; } }

        public bool IsEnabled { get { return RecSetting.RecMode != 5; } }

        public bool IsOnRec(int MarginMin = 0)
        {
            int StartMargin = RecSetting.GetTrueMargin(true) + 60 * MarginMin;
            int EndMargin = RecSetting.GetTrueMargin(false);

            DateTime startTime = StartTime.AddSeconds(StartMargin * -1);
            int duration = (int)DurationSecond + StartMargin + EndMargin;

            return CtrlCmdDefEx.isOnTime(startTime, duration);
        }

        public bool IsOnAir()
        {
            return CtrlCmdDefEx.isOnTime(StartTime, (int)DurationSecond);
        }

        public DateTime StartTimeWithMargin(int MarginMin = 0)
        {
            int StartMargin = RecSetting.GetTrueMargin(true) + 60 * MarginMin;
            return StartTime.AddSeconds(StartMargin * -1);
        }
        public DateTime EndTimeWithMargin()
        {
            int EndMargin = RecSetting.GetTrueMargin(false);
            return StartTime.AddSeconds((int)DurationSecond + EndMargin);
        }

        public EpgEventInfo SearchEventInfo(bool getSrv = false)
        {
            EpgEventInfo eventInfo = null;

            try
            {
                if (IsEpgReserve == true)
                {
                    UInt64 key = Create64Key();
                    if (CommonManager.Instance.DB.ServiceEventList.ContainsKey(key) == true)
                    {
                        foreach (EpgEventInfo eventChkInfo in CommonManager.Instance.DB.ServiceEventList[key].eventList)
                        {
                            if (eventChkInfo.event_id == EventID)
                            {
                                eventInfo = eventChkInfo;
                                break;
                            }
                        }
                    }
                    if (eventInfo == null && getSrv == true)
                    {
                        UInt64 pgId = Create64PgKey();
                        eventInfo = new EpgEventInfo();
                        CommonManager.Instance.CtrlCmd.SendGetPgInfo(pgId, ref eventInfo);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

            return eventInfo;
        }

        public EpgEventInfo SearchEventInfoLikeThat()
        {
            double dist = double.MaxValue, dist1;
            EpgEventInfo eventPossible = null;

            UInt64 key = Create64Key();
            if (CommonManager.Instance.DB.ServiceEventList.ContainsKey(key) == true)
            {
                foreach (EpgEventInfo eventChkInfo in CommonManager.Instance.DB.ServiceEventList[key].eventList)
                {
                    dist1 = Math.Abs((StartTime - eventChkInfo.start_time).TotalSeconds);
                    double overlapLength = MenuUtil.CulcOverlapLength(StartTime, DurationSecond,
                                                            eventChkInfo.start_time, eventChkInfo.durationSec);

                    //開始時間が最も近いものを選ぶ。同じ差なら時間が前のものを選ぶ
                    if (overlapLength >= 0 && (dist > dist1 ||
                        dist == dist1 && (eventPossible == null || StartTime > eventChkInfo.start_time)))
                    {
                        dist = dist1;
                        eventPossible = eventChkInfo;
                        if (dist == 0) break;
                    }
                }
            }

            return eventPossible;
        }

        //AppendData 関係。ID(元データ)に対して一意の情報なので、データ自体はDB側。
        private ReserveDataAppend Append { get { return CommonManager.Instance.DB.GetReserveDataAppend(this); } }
        public bool IsAutoAddMissing
        {
            get
            {
                if (Settings.Instance.DisplayReserveAutoAddMissing == false) return false;
                return IsAutoAdded && Append.IsAutoAddMissing;
            }
        }
        public bool IsAutoAddInvalid
        {
            get
            {
                if (Settings.Instance.DisplayReserveAutoAddMissing == false) return false;
                return IsAutoAdded && Append.IsAutoAddInvalid;
            }
        }
        public List<EpgAutoAddData> SearchEpgAutoAddList(bool? IsEnabled = null, bool ByFazy = false)
        {
            var list = GetEpgAutoAddList(IsEnabled);
            if (IsEpgReserve == false)
            {
                //プログラム予約の場合はそれっぽい番組を選んで、キーワード予約の検索にヒットしていたら選択する。
                EpgEventInfo trgInfo = this.SearchEventInfoLikeThat();
                if (trgInfo != null)
                {
                    list.AddRange(trgInfo.SearchEpgAutoAddList(IsEnabled, ByFazy));
                }
            }
            else
            {
                if (ByFazy == true)
                {
                    list.AddRange(CommonManager.Instance.MUtil.FazySearchEpgAutoAddData(DataTitle, IsEnabled));
                }
            }
            return list.Distinct().ToList();
        }
        public List<EpgAutoAddData> GetEpgAutoAddList(bool? IsEnabled = null)
        {
            return IsEnabled == null ? Append.EpgAutoList : IsEnabled == true ? Append.EpgAutoListEnabled : Append.EpgAutoListDisabled;
        }
        public List<ManualAutoAddData> GetManualAutoAddList(bool? IsEnabled = null)
        {
            return IsEnabled == null ? Append.ManualAutoList : IsEnabled == true ? Append.ManualAutoListEnabled : Append.ManualAutoListDisabled;
        }
    }

    public static class ReserveDataEx
    {
        public static ReserveData GetNextReserve(this List<ReserveData> resList, bool IsTargetOffRes = false)
        {
            ReserveData ret = null;
            long value = long.MaxValue;

            foreach (ReserveData data in resList)
            {
                if (IsTargetOffRes == true || data.IsEnabled == true)
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

        public static List<ReserveData> Clone(this IEnumerable<ReserveData> src) { return CopyObj.Clone(src, CopyData); }
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

    //AutoAddAppendに依存するので生成時は注意
    public class ReserveDataAppend
    {
        public ReserveDataAppend()
        {
            EpgAutoList = new List<EpgAutoAddData>();
            EpgAutoListEnabled = new List<EpgAutoAddData>();
            ManualAutoList = new List<ManualAutoAddData>();
            ManualAutoListEnabled = new List<ManualAutoAddData>();
        }

        public bool IsAutoAddMissing { get; protected set; }
        public bool IsAutoAddInvalid { get; protected set; }
        public List<EpgAutoAddData> EpgAutoList { get; protected set; }
        public List<EpgAutoAddData> EpgAutoListEnabled { get; protected set; }
        public List<EpgAutoAddData> EpgAutoListDisabled { get { return EpgAutoList.GetAutoAddList(false); } }
        public List<ManualAutoAddData> ManualAutoList { get; protected set; }
        public List<ManualAutoAddData> ManualAutoListEnabled { get; protected set; }
        public List<ManualAutoAddData> ManualAutoListDisabled { get { return ManualAutoList.FindAll(data => data.IsEnabled == false); } }

        //情報の更新をする。
        public void UpdateData()
        {
            EpgAutoListEnabled = EpgAutoList.FindAll(data => data.IsEnabled == true);
            ManualAutoListEnabled = ManualAutoList.GetAutoAddList(true);
            IsAutoAddMissing = (EpgAutoList.Count + ManualAutoList.Count) == 0;
            IsAutoAddInvalid = (EpgAutoListEnabled.Count + ManualAutoListEnabled.Count) == 0;
        }
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;

namespace EpgTimer
{
    public partial class ReserveData : AutoAddTargetData, IRecSetttingData
    {
        public override string DataTitle { get { return Title; } }
        public override DateTime PgStartTime { get { return StartTime; } }
        public override uint PgDurationSecond { get { return DurationSecond; } }
        public override UInt64 Create64PgKey()
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

        public bool IsOnRec(DateTime? time = null) { return OnTime(time) == 0; }
        public override bool IsOnAir(DateTime? time = null) { return base.OnTime(time) == 0; }
        /// <summary>-1:開始前、0:録画中、1:終了</summary>
        public override int OnTime(DateTime? time = null)
        {
            return onTime(StartTimeActual, DurationActual, time);
        }

        public DateTime StartTimeActual
        {
            get { return StartTime.AddSeconds(StartMarginResActual * -1); }
        }
        public UInt32 DurationActual
        {
            get { return (UInt32)Math.Max(0, DurationSecond + StartMarginResActual + EndMarginResActual); }
        }
        public Int32 StartMarginResActual
        {
            get { return (Int32)Math.Max(-DurationSecond, RecSetting.StartMarginActual); }
        }
        public Int32 EndMarginResActual
        {
            get { return (Int32)Math.Max(-DurationSecond, RecSetting.EndMarginActual); }
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
                        eventInfo = new EpgEventInfo();
                        CommonManager.Instance.CtrlCmd.SendGetPgInfo(Create64PgKey(), ref eventInfo);
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return eventInfo;
        }

        public EpgEventInfo SearchEventInfoLikeThat()
        {
            return MenuUtil.SearchEventInfoLikeThat(this);
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
        public bool IsMultiple
        {
            get
            {
                if (Settings.Instance.DisplayReserveMultiple == false) return false;
                return Append.IsMultiple;
            }
        }
        public override List<EpgAutoAddData> SearchEpgAutoAddList(bool? IsEnabled = null, bool ByFazy = false)
        {
            //プログラム予約の場合はそれっぽい番組を選んで、キーワード予約の検索にヒットしていたら選択する。
            var info = IsEpgReserve == true ? this as IAutoAddTargetData : this.SearchEventInfoLikeThat();
            return AutoAddTargetData.SearchEpgAutoAddHitList(info, IsEnabled, ByFazy);
        }
        public override List<EpgAutoAddData> GetEpgAutoAddList(bool? IsEnabled = null)
        {
            return IsEnabled == null ? Append.EpgAutoList : IsEnabled == true ? Append.EpgAutoListEnabled : Append.EpgAutoListDisabled;
        }
        public override List<ManualAutoAddData> GetManualAutoAddList(bool? IsEnabled = null)
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
            MultipleEPGList = new List<ReserveData>();
        }

        public bool IsAutoAddMissing { get; protected set; }
        public bool IsAutoAddInvalid { get; protected set; }
        public List<EpgAutoAddData> EpgAutoList { get; protected set; }
        public List<EpgAutoAddData> EpgAutoListEnabled { get; protected set; }
        public List<EpgAutoAddData> EpgAutoListDisabled { get { return EpgAutoList.GetAutoAddList(false); } }
        public List<ManualAutoAddData> ManualAutoList { get; protected set; }
        public List<ManualAutoAddData> ManualAutoListEnabled { get; protected set; }
        public List<ManualAutoAddData> ManualAutoListDisabled { get { return ManualAutoList.FindAll(data => data.IsEnabled == false); } }
        public bool IsMultiple { get; protected set; }
        public List<ReserveData> MultipleEPGList { get; protected set; }

        //情報の更新をする。
        public void UpdateData()
        {
            EpgAutoListEnabled = EpgAutoList.FindAll(data => data.IsEnabled == true);
            ManualAutoListEnabled = ManualAutoList.GetAutoAddList(true);
            IsAutoAddMissing = (EpgAutoList.Count + ManualAutoList.Count) == 0;
            IsAutoAddInvalid = (EpgAutoListEnabled.Count + ManualAutoListEnabled.Count) == 0;
            IsMultiple = MultipleEPGList.Count != 0;
        }
    }
}

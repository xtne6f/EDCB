using System;
using System.Collections.Generic;
using System.IO;

namespace EpgTimer
{
    // 【StructDef.hから自動生成→型と変数名を調整】
    // sed s/BYTE/byte/g StructDef.h |
    // sed s/DWORD/uint/g |
    // sed s/WORD/ushort/g |
    // sed s/INT/int/g |
    // sed s/BOOL/int/g |
    // sed s/FALSE/0/g |
    // sed s/TRUE/1/g |
    // sed 's/L""/""/g' |
    // sed s/__int64/long/g |
    // sed s/wstring/string/g |
    // sed s/SYSTEMTIME/DateTime/g |
    // sed s/vector/List/g |
    // tr -d '*' |
    // sed 's/}.*$/}/' |
    // sed 's/^\t\([A-Za-z].*;\)/\tpublic \1/' |
    // sed 's#^\(\t[A-Za-z].*;\)\t*//\(.*\)$#\t/// <summary>\2</summary>\n\1#' |
    // sed 's/typedef struct _/public class /' |
    // sed 's/_\(.*\)(void){$/public \1(){/' |
    // sed 's/\t/    /g'

    /// <summary>録画フォルダ情報</summary>
    public class RecFileSetInfo : ICtrlCmdReadWrite
    {
        /// <summary>録画フォルダ</summary>
        public string RecFolder;
        /// <summary>出力PlugIn</summary>
        public string WritePlugIn;
        /// <summary>ファイル名変換PlugInの使用</summary>
        public string RecNamePlugIn;
        /// <summary>ファイル名個別対応 録画開始処理時に内部で使用。予約情報としては必要なし</summary>
        public string RecFileName;
        public RecFileSetInfo()
        {
            RecFolder = "";
            WritePlugIn = "";
            RecNamePlugIn = "";
            RecFileName = "";
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(RecFolder);
            w.Write(WritePlugIn);
            w.Write(RecNamePlugIn);
            w.Write(RecFileName);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref RecFolder);
            r.Read(ref WritePlugIn);
            r.Read(ref RecNamePlugIn);
            r.Read(ref RecFileName);
            r.End();
        }
    }

    /// <summary>録画設定情報</summary>
    public class RecSettingData : ICtrlCmdReadWrite
    {
        /// <summary>録画モード</summary>
        public byte RecMode;
        /// <summary>優先度</summary>
        public byte Priority;
        /// <summary>イベントリレー追従するかどうか</summary>
        public byte TuijyuuFlag;
        /// <summary>処理対象データモード</summary>
        public uint ServiceMode;
        /// <summary>ぴったり？録画</summary>
        public byte PittariFlag;
        /// <summary>録画後BATファイルパス</summary>
        public string BatFilePath;
        /// <summary>録画フォルダパス</summary>
        public List<RecFileSetInfo> RecFolderList;
        /// <summary>休止モード</summary>
        public byte SuspendMode;
        /// <summary>録画後再起動する</summary>
        public byte RebootFlag;
        /// <summary>録画マージンを個別指定</summary>
        public byte UseMargineFlag;
        /// <summary>録画開始時のマージン</summary>
        public int StartMargine;
        /// <summary>録画終了時のマージン</summary>
        public int EndMargine;
        /// <summary>後続同一サービス時、同一ファイルで録画</summary>
        public byte ContinueRecFlag;
        /// <summary>物理CHに部分受信サービスがある場合、同時録画するかどうか</summary>
        public byte PartialRecFlag;
        /// <summary>強制的に使用Tunerを固定</summary>
        public uint TunerID;
        /// <summary>部分受信サービス録画のフォルダ</summary>
        public List<RecFileSetInfo> PartialRecFolder;
        public RecSettingData()
        {
            RecMode = 1;
            Priority = 1;
            TuijyuuFlag = 1;
            ServiceMode = 0;
            PittariFlag = 0;
            BatFilePath = "";
            RecFolderList = new List<RecFileSetInfo>();
            SuspendMode = 0;
            RebootFlag = 0;
            UseMargineFlag = 0;
            StartMargine = 10;
            EndMargine = 5;
            ContinueRecFlag = 0;
            PartialRecFlag = 0;
            TunerID = 0;
            PartialRecFolder = new List<RecFileSetInfo>();
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(RecMode);
            w.Write(Priority);
            w.Write(TuijyuuFlag);
            w.Write(ServiceMode);
            w.Write(PittariFlag);
            w.Write(BatFilePath);
            w.Write(RecFolderList);
            w.Write(SuspendMode);
            w.Write(RebootFlag);
            w.Write(UseMargineFlag);
            w.Write(StartMargine);
            w.Write(EndMargine);
            w.Write(ContinueRecFlag);
            w.Write(PartialRecFlag);
            w.Write(TunerID);
            if (version >= 2)
            {
                w.Write(PartialRecFolder);
            }
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref RecMode);
            r.Read(ref Priority);
            r.Read(ref TuijyuuFlag);
            r.Read(ref ServiceMode);
            r.Read(ref PittariFlag);
            r.Read(ref BatFilePath);
            r.Read(ref RecFolderList);
            r.Read(ref SuspendMode);
            r.Read(ref RebootFlag);
            r.Read(ref UseMargineFlag);
            r.Read(ref StartMargine);
            r.Read(ref EndMargine);
            r.Read(ref ContinueRecFlag);
            r.Read(ref PartialRecFlag);
            r.Read(ref TunerID);
            if (version >= 2)
            {
                r.Read(ref PartialRecFolder);
            }
            r.End();
        }
    }

    /// <summary>登録予約情報</summary>
    public class ReserveData : ICtrlCmdReadWrite
    {
        /// <summary>番組名</summary>
        public string Title;
        /// <summary>録画開始時間</summary>
        public DateTime StartTime;
        /// <summary>録画総時間</summary>
        public uint DurationSecond;
        /// <summary>サービス名</summary>
        public string StationName;
        /// <summary>ONID</summary>
        public ushort OriginalNetworkID;
        /// <summary>TSID</summary>
        public ushort TransportStreamID;
        /// <summary>SID</summary>
        public ushort ServiceID;
        /// <summary>EventID</summary>
        public ushort EventID;
        /// <summary>コメント</summary>
        public string Comment;
        /// <summary>予約識別ID 予約登録時は0</summary>
        public uint ReserveID;
        /// <summary>予約待機入った？ 内部で使用（廃止）</summary>
        private byte UnusedRecWaitFlag;
        /// <summary>かぶり状態 1:かぶってチューナー足りない予約あり 2:チューナー足りなくて予約できない</summary>
        public byte OverlapMode;
        /// <summary>録画ファイルパス 旧バージョン互換用 未使用（廃止）</summary>
        private string UnusedRecFilePath;
        /// <summary>予約時の開始時間</summary>
        public DateTime StartTimeEpg;
        /// <summary>録画設定</summary>
        public RecSettingData RecSetting;
        /// <summary>予約追加状態 内部で使用</summary>
        public uint ReserveStatus;
        /// <summary>録画予定ファイル名</summary>
        public List<string> RecFileNameList;
        /// <summary>将来用</summary>
        private uint UnusedParam1;
        public ReserveData()
        {
            Title = "";
            StartTime = new DateTime();
            DurationSecond = 0;
            StationName = "";
            OriginalNetworkID = 0;
            TransportStreamID = 0;
            ServiceID = 0;
            EventID = 0;
            Comment = "";
            ReserveID = 0;
            UnusedRecWaitFlag = 0;
            OverlapMode = 0;
            UnusedRecFilePath = "";
            StartTimeEpg = new DateTime();
            RecSetting = new RecSettingData();
            ReserveStatus = 0;
            RecFileNameList = new List<string>();
            UnusedParam1 = 0;
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(Title);
            w.Write(StartTime);
            w.Write(DurationSecond);
            w.Write(StationName);
            w.Write(OriginalNetworkID);
            w.Write(TransportStreamID);
            w.Write(ServiceID);
            w.Write(EventID);
            w.Write(Comment);
            w.Write(ReserveID);
            w.Write(UnusedRecWaitFlag);
            w.Write(OverlapMode);
            w.Write(UnusedRecFilePath);
            w.Write(StartTimeEpg);
            w.Write(RecSetting);
            w.Write(ReserveStatus);
            if (version >= 5)
            {
                w.Write(RecFileNameList);
                w.Write(UnusedParam1);
            }
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref Title);
            r.Read(ref StartTime);
            r.Read(ref DurationSecond);
            r.Read(ref StationName);
            r.Read(ref OriginalNetworkID);
            r.Read(ref TransportStreamID);
            r.Read(ref ServiceID);
            r.Read(ref EventID);
            r.Read(ref Comment);
            r.Read(ref ReserveID);
            r.Read(ref UnusedRecWaitFlag);
            r.Read(ref OverlapMode);
            r.Read(ref UnusedRecFilePath);
            r.Read(ref StartTimeEpg);
            r.Read(ref RecSetting);
            r.Read(ref ReserveStatus);
            if (version >= 5)
            {
                r.Read(ref RecFileNameList);
                r.Read(ref UnusedParam1);
            }
            r.End();
        }
    }

    public class RecFileInfo : ICtrlCmdReadWrite
    {
        /// <summary>ID</summary>
        public uint ID;
        /// <summary>録画ファイルパス</summary>
        public string RecFilePath;
        /// <summary>番組名</summary>
        public string Title;
        /// <summary>開始時間</summary>
        public DateTime StartTime;
        /// <summary>録画時間</summary>
        public uint DurationSecond;
        /// <summary>サービス名</summary>
        public string ServiceName;
        /// <summary>ONID</summary>
        public ushort OriginalNetworkID;
        /// <summary>TSID</summary>
        public ushort TransportStreamID;
        /// <summary>SID</summary>
        public ushort ServiceID;
        /// <summary>EventID</summary>
        public ushort EventID;
        /// <summary>ドロップ数</summary>
        public long Drops;
        /// <summary>スクランブル数</summary>
        public long Scrambles;
        /// <summary>録画結果のステータス</summary>
        public uint RecStatus;
        /// <summary>予約時の開始時間</summary>
        public DateTime StartTimeEpg;
        /// <summary>コメント</summary>
        public string Comment;
        /// <summary>.program.txtファイルの内容</summary>
        public string ProgramInfo;
        /// <summary>.errファイルの内容</summary>
        public string ErrInfo;
        public byte ProtectFlag;
        public RecFileInfo()
        {
            ID = 0;
            RecFilePath = "";
            Title = "";
            StartTime = new DateTime();
            DurationSecond = 0;
            ServiceName = "";
            OriginalNetworkID = 0;
            TransportStreamID = 0;
            ServiceID = 0;
            EventID = 0;
            Drops = 0;
            Scrambles = 0;
            RecStatus = 0;
            StartTimeEpg = new DateTime();
            Comment = "";
            ProgramInfo = "";
            ErrInfo = "";
            ProtectFlag = 0;
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(ID);
            w.Write(RecFilePath);
            w.Write(Title);
            w.Write(StartTime);
            w.Write(DurationSecond);
            w.Write(ServiceName);
            w.Write(OriginalNetworkID);
            w.Write(TransportStreamID);
            w.Write(ServiceID);
            w.Write(EventID);
            w.Write(Drops);
            w.Write(Scrambles);
            w.Write(RecStatus);
            w.Write(StartTimeEpg);
            w.Write(Comment);
            w.Write(ProgramInfo);
            w.Write(ErrInfo);
            if (version >= 4)
            {
                w.Write(ProtectFlag);
            }
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref ID);
            r.Read(ref RecFilePath);
            r.Read(ref Title);
            r.Read(ref StartTime);
            r.Read(ref DurationSecond);
            r.Read(ref ServiceName);
            r.Read(ref OriginalNetworkID);
            r.Read(ref TransportStreamID);
            r.Read(ref ServiceID);
            r.Read(ref EventID);
            r.Read(ref Drops);
            r.Read(ref Scrambles);
            r.Read(ref RecStatus);
            r.Read(ref StartTimeEpg);
            r.Read(ref Comment);
            r.Read(ref ProgramInfo);
            r.Read(ref ErrInfo);
            if (version >= 4)
            {
                r.Read(ref ProtectFlag);
            }
            r.End();
        }
    }

    public class TunerReserveInfo : ICtrlCmdReadWrite
    {
        public uint tunerID;
        public string tunerName;
        public List<uint> reserveList;
        public TunerReserveInfo()
        {
            tunerID = 0;
            tunerName = "";
            reserveList = new List<uint>();
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(tunerID);
            w.Write(tunerName);
            w.Write(reserveList);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref tunerID);
            r.Read(ref tunerName);
            r.Read(ref reserveList);
            r.End();
        }
    }

    /// <summary>EPG基本情報</summary>
    public class EpgShortEventInfo : ICtrlCmdReadWrite
    {
        /// <summary>イベント名</summary>
        public string event_name;
        /// <summary>情報</summary>
        public string text_char;
        public EpgShortEventInfo()
        {
            event_name = "";
            text_char = "";
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(event_name);
            w.Write(text_char);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref event_name);
            r.Read(ref text_char);
            r.End();
        }
    }

    /// <summary>EPG拡張情報</summary>
    public class EpgExtendedEventInfo : ICtrlCmdReadWrite
    {
        /// <summary>詳細情報</summary>
        public string text_char;
        public EpgExtendedEventInfo()
        {
            text_char = "";
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(text_char);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref text_char);
            r.End();
        }
    }

    /// <summary>EPGジャンルデータ</summary>
    public class EpgContentData : ICtrlCmdReadWrite
    {
        public byte content_nibble_level_1;
        public byte content_nibble_level_2;
        public byte user_nibble_1;
        public byte user_nibble_2;
        public EpgContentData()
        {
            content_nibble_level_1 = 0;
            content_nibble_level_2 = 0;
            user_nibble_1 = 0;
            user_nibble_2 = 0;
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(content_nibble_level_1);
            w.Write(content_nibble_level_2);
            w.Write(user_nibble_1);
            w.Write(user_nibble_2);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref content_nibble_level_1);
            r.Read(ref content_nibble_level_2);
            r.Read(ref user_nibble_1);
            r.Read(ref user_nibble_2);
            r.End();
        }
    }

    /// <summary>EPGジャンル情報</summary>
    public class EpgContentInfo : ICtrlCmdReadWrite
    {
        public List<EpgContentData> nibbleList;
        public EpgContentInfo()
        {
            nibbleList = new List<EpgContentData>();
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(nibbleList);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref nibbleList);
            r.End();
        }
    }

    /// <summary>EPG映像情報</summary>
    public class EpgComponentInfo : ICtrlCmdReadWrite
    {
        public byte stream_content;
        public byte component_type;
        public byte component_tag;
        /// <summary>情報</summary>
        public string text_char;
        public EpgComponentInfo()
        {
            stream_content = 0;
            component_type = 0;
            component_tag = 0;
            text_char = "";
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(stream_content);
            w.Write(component_type);
            w.Write(component_tag);
            w.Write(text_char);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref stream_content);
            r.Read(ref component_type);
            r.Read(ref component_tag);
            r.Read(ref text_char);
            r.End();
        }
    }

    /// <summary>EPG音声情報データ</summary>
    public class EpgAudioComponentInfoData : ICtrlCmdReadWrite
    {
        public byte stream_content;
        public byte component_type;
        public byte component_tag;
        public byte stream_type;
        public byte simulcast_group_tag;
        public byte ES_multi_lingual_flag;
        public byte main_component_flag;
        public byte quality_indicator;
        public byte sampling_rate;
        /// <summary>詳細情報</summary>
        public string text_char;
        public EpgAudioComponentInfoData()
        {
            stream_content = 0;
            component_type = 0;
            component_tag = 0;
            stream_type = 0;
            simulcast_group_tag = 0;
            ES_multi_lingual_flag = 0;
            main_component_flag = 0;
            quality_indicator = 0;
            sampling_rate = 0;
            text_char = "";
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(stream_content);
            w.Write(component_type);
            w.Write(component_tag);
            w.Write(stream_type);
            w.Write(simulcast_group_tag);
            w.Write(ES_multi_lingual_flag);
            w.Write(main_component_flag);
            w.Write(quality_indicator);
            w.Write(sampling_rate);
            w.Write(text_char);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref stream_content);
            r.Read(ref component_type);
            r.Read(ref component_tag);
            r.Read(ref stream_type);
            r.Read(ref simulcast_group_tag);
            r.Read(ref ES_multi_lingual_flag);
            r.Read(ref main_component_flag);
            r.Read(ref quality_indicator);
            r.Read(ref sampling_rate);
            r.Read(ref text_char);
            r.End();
        }
    }

    /// <summary>EPG音声情報</summary>
    public class EpgAudioComponentInfo : ICtrlCmdReadWrite
    {
        public List<EpgAudioComponentInfoData> componentList;
        public EpgAudioComponentInfo()
        {
            componentList = new List<EpgAudioComponentInfoData>();
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(componentList);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref componentList);
            r.End();
        }
    }

    /// <summary>EPGイベントデータ</summary>
    public class EpgEventData : ICtrlCmdReadWrite
    {
        public ushort original_network_id;
        public ushort transport_stream_id;
        public ushort service_id;
        public ushort event_id;
        public EpgEventData()
        {
            original_network_id = 0;
            transport_stream_id = 0;
            service_id = 0;
            event_id = 0;
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(original_network_id);
            w.Write(transport_stream_id);
            w.Write(service_id);
            w.Write(event_id);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref original_network_id);
            r.Read(ref transport_stream_id);
            r.Read(ref service_id);
            r.Read(ref event_id);
            r.End();
        }
    }

    /// <summary>EPGイベントグループ情報</summary>
    public class EpgEventGroupInfo : ICtrlCmdReadWrite
    {
        public byte group_type;
        public List<EpgEventData> eventDataList;
        public EpgEventGroupInfo()
        {
            group_type = 0;
            eventDataList = new List<EpgEventData>();
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(group_type);
            w.Write(eventDataList);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref group_type);
            r.Read(ref eventDataList);
            r.End();
        }
    }

    public class EpgEventInfo : ICtrlCmdReadWrite
    {
        public ushort original_network_id;
        public ushort transport_stream_id;
        public ushort service_id;
        /// <summary>イベントID</summary>
        public ushort event_id;
        /// <summary>start_timeの値が有効かどうか</summary>
        public byte StartTimeFlag;
        /// <summary>開始時間</summary>
        public DateTime start_time;
        /// <summary>durationの値が有効かどうか</summary>
        public byte DurationFlag;
        /// <summary>総時間（単位：秒）</summary>
        public uint durationSec;
        /// <summary>基本情報</summary>
        public EpgShortEventInfo ShortInfo;
        /// <summary>拡張情報</summary>
        public EpgExtendedEventInfo ExtInfo;
        /// <summary>ジャンル情報</summary>
        public EpgContentInfo ContentInfo;
        /// <summary>映像情報</summary>
        public EpgComponentInfo ComponentInfo;
        /// <summary>音声情報</summary>
        public EpgAudioComponentInfo AudioInfo;
        /// <summary>イベントグループ情報</summary>
        public EpgEventGroupInfo EventGroupInfo;
        /// <summary>イベントリレー情報</summary>
        public EpgEventGroupInfo EventRelayInfo;
        /// <summary>ノンスクランブルフラグ</summary>
        public byte FreeCAFlag;
        public EpgEventInfo()
        {
            original_network_id = 0;
            transport_stream_id = 0;
            service_id = 0;
            event_id = 0;
            StartTimeFlag = 0;
            start_time = new DateTime();
            DurationFlag = 0;
            durationSec = 0;
            ShortInfo = null;
            ExtInfo = null;
            ContentInfo = null;
            ComponentInfo = null;
            AudioInfo = null;
            EventGroupInfo = null;
            EventRelayInfo = null;
            FreeCAFlag = 0;
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(original_network_id);
            w.Write(transport_stream_id);
            w.Write(service_id);
            w.Write(event_id);
            w.Write(StartTimeFlag);
            w.Write(start_time);
            w.Write(DurationFlag);
            w.Write(durationSec);
            if (ShortInfo != null) w.Write(ShortInfo); else w.Write(4);
            if (ExtInfo != null) w.Write(ExtInfo); else w.Write(4);
            if (ContentInfo != null) w.Write(ContentInfo); else w.Write(4);
            if (ComponentInfo != null) w.Write(ComponentInfo); else w.Write(4);
            if (AudioInfo != null) w.Write(AudioInfo); else w.Write(4);
            if (EventGroupInfo != null) w.Write(EventGroupInfo); else w.Write(4);
            if (EventRelayInfo != null) w.Write(EventRelayInfo); else w.Write(4);
            w.Write(FreeCAFlag);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref original_network_id);
            r.Read(ref transport_stream_id);
            r.Read(ref service_id);
            r.Read(ref event_id);
            r.Read(ref StartTimeFlag);
            try
            {
                r.Read(ref start_time);
            }
            catch (ArgumentOutOfRangeException)
            {
            }
            r.Read(ref DurationFlag);
            r.Read(ref durationSec);
            int size = 0;
            ShortInfo = null;
            r.Read(ref size);
            if (size != 4)
            {
                r.Stream.Seek(-4, SeekOrigin.Current);
                ShortInfo = new EpgShortEventInfo();
                r.Read(ref ShortInfo);
            }
            ExtInfo = null;
            r.Read(ref size);
            if (size != 4)
            {
                r.Stream.Seek(-4, SeekOrigin.Current);
                ExtInfo = new EpgExtendedEventInfo();
                r.Read(ref ExtInfo);
            }
            ContentInfo = null;
            r.Read(ref size);
            if (size != 4)
            {
                r.Stream.Seek(-4, SeekOrigin.Current);
                ContentInfo = new EpgContentInfo();
                r.Read(ref ContentInfo);
            }
            ComponentInfo = null;
            r.Read(ref size);
            if (size != 4)
            {
                r.Stream.Seek(-4, SeekOrigin.Current);
                ComponentInfo = new EpgComponentInfo();
                r.Read(ref ComponentInfo);
            }
            AudioInfo = null;
            r.Read(ref size);
            if (size != 4)
            {
                r.Stream.Seek(-4, SeekOrigin.Current);
                AudioInfo = new EpgAudioComponentInfo();
                r.Read(ref AudioInfo);
            }
            EventGroupInfo = null;
            r.Read(ref size);
            if (size != 4)
            {
                r.Stream.Seek(-4, SeekOrigin.Current);
                EventGroupInfo = new EpgEventGroupInfo();
                r.Read(ref EventGroupInfo);
            }
            EventRelayInfo = null;
            r.Read(ref size);
            if (size != 4)
            {
                r.Stream.Seek(-4, SeekOrigin.Current);
                EventRelayInfo = new EpgEventGroupInfo();
                r.Read(ref EventRelayInfo);
            }
            r.Read(ref FreeCAFlag);
            r.End();
        }
    }

    public class EpgServiceInfo : ICtrlCmdReadWrite
    {
        public ushort ONID;
        public ushort TSID;
        public ushort SID;
        public byte service_type;
        public byte partialReceptionFlag;
        public string service_provider_name;
        public string service_name;
        public string network_name;
        public string ts_name;
        public byte remote_control_key_id;
        public EpgServiceInfo()
        {
            ONID = 0;
            TSID = 0;
            SID = 0;
            service_type = 0;
            partialReceptionFlag = 0;
            service_provider_name = "";
            service_name = "";
            network_name = "";
            ts_name = "";
            remote_control_key_id = 0;
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(ONID);
            w.Write(TSID);
            w.Write(SID);
            w.Write(service_type);
            w.Write(partialReceptionFlag);
            w.Write(service_provider_name);
            w.Write(service_name);
            w.Write(network_name);
            w.Write(ts_name);
            w.Write(remote_control_key_id);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref ONID);
            r.Read(ref TSID);
            r.Read(ref SID);
            r.Read(ref service_type);
            r.Read(ref partialReceptionFlag);
            r.Read(ref service_provider_name);
            r.Read(ref service_name);
            r.Read(ref network_name);
            r.Read(ref ts_name);
            r.Read(ref remote_control_key_id);
            r.End();
        }
    }

    public class EpgServiceEventInfo : ICtrlCmdReadWrite
    {
        public EpgServiceInfo serviceInfo;
        public List<EpgEventInfo> eventList;
        public EpgServiceEventInfo()
        {
            serviceInfo = new EpgServiceInfo();
            eventList = new List<EpgEventInfo>();
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(serviceInfo);
            w.Write(eventList);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref serviceInfo);
            r.Read(ref eventList);
            r.End();
        }
    }

    public class EpgSearchDateInfo : ICtrlCmdReadWrite
    {
        public byte startDayOfWeek;
        public ushort startHour;
        public ushort startMin;
        public byte endDayOfWeek;
        public ushort endHour;
        public ushort endMin;
        public EpgSearchDateInfo()
        {
            startDayOfWeek = 0;
            startHour = 0;
            startMin = 0;
            endDayOfWeek = 0;
            endHour = 0;
            endMin = 0;
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(startDayOfWeek);
            w.Write(startHour);
            w.Write(startMin);
            w.Write(endDayOfWeek);
            w.Write(endHour);
            w.Write(endMin);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref startDayOfWeek);
            r.Read(ref startHour);
            r.Read(ref startMin);
            r.Read(ref endDayOfWeek);
            r.Read(ref endHour);
            r.Read(ref endMin);
            r.End();
        }
    }

    /// <summary>検索条件</summary>
    public class EpgSearchKeyInfo : ICtrlCmdReadWrite
    {
        public string andKey;
        public string notKey;
        public int regExpFlag;
        public int titleOnlyFlag;
        public List<EpgContentData> contentList;
        public List<EpgSearchDateInfo> dateList;
        public List<long> serviceList;
        public List<ushort> videoList;
        public List<ushort> audioList;
        public byte aimaiFlag;
        public byte notContetFlag;
        public byte notDateFlag;
        public byte freeCAFlag;
        /// <summary>(自動予約登録の条件専用)録画済かのチェックあり</summary>
        public byte chkRecEnd;
        /// <summary>(自動予約登録の条件専用)録画済かのチェック対象期間</summary>
        public ushort chkRecDay;
        public EpgSearchKeyInfo()
        {
            andKey = "";
            notKey = "";
            regExpFlag = 0;
            titleOnlyFlag = 0;
            contentList = new List<EpgContentData>();
            dateList = new List<EpgSearchDateInfo>();
            serviceList = new List<long>();
            videoList = new List<ushort>();
            audioList = new List<ushort>();
            aimaiFlag = 0;
            notContetFlag = 0;
            notDateFlag = 0;
            freeCAFlag = 0;
            chkRecEnd = 0;
            chkRecDay = 6;
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(andKey);
            w.Write(notKey);
            w.Write(regExpFlag);
            w.Write(titleOnlyFlag);
            w.Write(contentList);
            w.Write(dateList);
            w.Write(serviceList);
            w.Write(videoList);
            w.Write(audioList);
            w.Write(aimaiFlag);
            w.Write(notContetFlag);
            w.Write(notDateFlag);
            w.Write(freeCAFlag);
            if (version >= 3)
            {
                w.Write(chkRecEnd);
                w.Write(chkRecDay);
            }
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref andKey);
            r.Read(ref notKey);
            r.Read(ref regExpFlag);
            r.Read(ref titleOnlyFlag);
            r.Read(ref contentList);
            r.Read(ref dateList);
            r.Read(ref serviceList);
            r.Read(ref videoList);
            r.Read(ref audioList);
            r.Read(ref aimaiFlag);
            r.Read(ref notContetFlag);
            r.Read(ref notDateFlag);
            r.Read(ref freeCAFlag);
            if (version >= 3)
            {
                r.Read(ref chkRecEnd);
                r.Read(ref chkRecDay);
            }
            r.End();
        }
    }

    /// <summary>自動予約登録情報</summary>
    public class EpgAutoAddData : ICtrlCmdReadWrite
    {
        public uint dataID;
        /// <summary>検索キー</summary>
        public EpgSearchKeyInfo searchInfo;
        /// <summary>録画設定</summary>
        public RecSettingData recSetting;
        /// <summary>予約登録数</summary>
        public uint addCount;
        public EpgAutoAddData()
        {
            dataID = 0;
            searchInfo = new EpgSearchKeyInfo();
            recSetting = new RecSettingData();
            addCount = 0;
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(dataID);
            w.Write(searchInfo);
            w.Write(recSetting);
            if (version >= 5)
            {
                w.Write(addCount);
            }
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref dataID);
            r.Read(ref searchInfo);
            r.Read(ref recSetting);
            if (version >= 5)
            {
                r.Read(ref addCount);
            }
            r.End();
        }
    }

    public class ManualAutoAddData : ICtrlCmdReadWrite
    {
        public uint dataID;
        /// <summary>対象曜日</summary>
        public byte dayOfWeekFlag;
        /// <summary>録画開始時間（00:00を0として秒単位）</summary>
        public uint startTime;
        /// <summary>録画総時間</summary>
        public uint durationSecond;
        /// <summary>番組名</summary>
        public string title;
        /// <summary>サービス名</summary>
        public string stationName;
        /// <summary>ONID</summary>
        public ushort originalNetworkID;
        /// <summary>TSID</summary>
        public ushort transportStreamID;
        /// <summary>SID</summary>
        public ushort serviceID;
        /// <summary>録画設定</summary>
        public RecSettingData recSetting;
        public ManualAutoAddData()
        {
            dataID = 0;
            dayOfWeekFlag = 0;
            startTime = 0;
            durationSecond = 0;
            title = "";
            stationName = "";
            originalNetworkID = 0;
            transportStreamID = 0;
            serviceID = 0;
            recSetting = new RecSettingData();
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(dataID);
            w.Write(dayOfWeekFlag);
            w.Write(startTime);
            w.Write(durationSecond);
            w.Write(title);
            w.Write(stationName);
            w.Write(originalNetworkID);
            w.Write(transportStreamID);
            w.Write(serviceID);
            w.Write(recSetting);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref dataID);
            r.Read(ref dayOfWeekFlag);
            r.Read(ref startTime);
            r.Read(ref durationSecond);
            r.Read(ref title);
            r.Read(ref stationName);
            r.Read(ref originalNetworkID);
            r.Read(ref transportStreamID);
            r.Read(ref serviceID);
            r.Read(ref recSetting);
            r.End();
        }
    }

    /// <summary>チャンネル変更情報</summary>
    public class SetChInfo : ICtrlCmdReadWrite
    {
        /// <summary>wONIDとwTSIDとwSIDの値が使用できるかどうか</summary>
        public int useSID;
        public ushort ONID;
        public ushort TSID;
        public ushort SID;
        /// <summary>dwSpaceとdwChの値が使用できるかどうか</summary>
        public int useBonCh;
        public uint space;
        public uint ch;
        public SetChInfo()
        {
            useSID = 0;
            ONID = 0;
            TSID = 0;
            SID = 0;
            useBonCh = 0;
            space = 0;
            ch = 0;
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(useSID);
            w.Write(ONID);
            w.Write(TSID);
            w.Write(SID);
            w.Write(useBonCh);
            w.Write(space);
            w.Write(ch);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref useSID);
            r.Read(ref ONID);
            r.Read(ref TSID);
            r.Read(ref SID);
            r.Read(ref useBonCh);
            r.Read(ref space);
            r.Read(ref ch);
            r.End();
        }
    }

    public class TvTestChChgInfo : ICtrlCmdReadWrite
    {
        public string bonDriver;
        public SetChInfo chInfo;
        public TvTestChChgInfo()
        {
            bonDriver = "";
            chInfo = new SetChInfo();
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(bonDriver);
            w.Write(chInfo);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref bonDriver);
            r.Read(ref chInfo);
            r.End();
        }
    }

    public class TVTestStreamingInfo : ICtrlCmdReadWrite
    {
        public int enableMode;
        public uint ctrlID;
        public uint serverIP;
        public uint serverPort;
        public string filePath;
        public int udpSend;
        public int tcpSend;
        public int timeShiftMode;
        public TVTestStreamingInfo()
        {
            enableMode = 0;
            ctrlID = 0;
            serverIP = 0;
            serverPort = 0;
            filePath = "";
            udpSend = 0;
            tcpSend = 0;
            timeShiftMode = 0;
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(enableMode);
            w.Write(ctrlID);
            w.Write(serverIP);
            w.Write(serverPort);
            w.Write(filePath);
            w.Write(udpSend);
            w.Write(tcpSend);
            w.Write(timeShiftMode);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref enableMode);
            r.Read(ref ctrlID);
            r.Read(ref serverIP);
            r.Read(ref serverPort);
            r.Read(ref filePath);
            r.Read(ref udpSend);
            r.Read(ref tcpSend);
            r.Read(ref timeShiftMode);
            r.End();
        }
    }

    public class NWPlayTimeShiftInfo : ICtrlCmdReadWrite
    {
        public uint ctrlID;
        public string filePath;
        public NWPlayTimeShiftInfo()
        {
            ctrlID = 0;
            filePath = "";
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(ctrlID);
            w.Write(filePath);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref ctrlID);
            r.Read(ref filePath);
            r.End();
        }
    }

    /// <summary>情報通知用パラメーター</summary>
    public class NotifySrvInfo : ICtrlCmdReadWrite
    {
        /// <summary>通知情報の種類</summary>
        public uint notifyID;
        /// <summary>通知状態の発生した時間</summary>
        public DateTime time;
        /// <summary>パラメーター１（種類によって内容変更）</summary>
        public uint param1;
        /// <summary>パラメーター２（種類によって内容変更）</summary>
        public uint param2;
        /// <summary>パラメーター３（通知の巡回カウンタ）</summary>
        public uint param3;
        /// <summary>パラメーター４（種類によって内容変更）</summary>
        public string param4;
        /// <summary>パラメーター５（種類によって内容変更）</summary>
        public string param5;
        /// <summary>パラメーター６（種類によって内容変更）</summary>
        public string param6;
        public NotifySrvInfo()
        {
            notifyID = 0;
            time = new DateTime();
            param1 = 0;
            param2 = 0;
            param3 = 0;
            param4 = "";
            param5 = "";
            param6 = "";
        }
        public void Write(MemoryStream s, ushort version)
        {
            var w = new CtrlCmdWriter(s, version);
            w.Begin();
            w.Write(notifyID);
            w.Write(time);
            w.Write(param1);
            w.Write(param2);
            w.Write(param3);
            w.Write(param4);
            w.Write(param5);
            w.Write(param6);
            w.End();
        }
        public void Read(MemoryStream s, ushort version)
        {
            var r = new CtrlCmdReader(s, version);
            r.Begin();
            r.Read(ref notifyID);
            r.Read(ref time);
            r.Read(ref param1);
            r.Read(ref param2);
            r.Read(ref param3);
            r.Read(ref param4);
            r.Read(ref param5);
            r.Read(ref param6);
            r.End();
        }
    }
}

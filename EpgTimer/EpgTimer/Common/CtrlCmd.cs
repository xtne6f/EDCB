using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace EpgTimer
{
    /// <summary>CtrlCmdバイナリ形式との相互変換インターフェイス</summary>
    public interface ICtrlCmdReadWrite
    {
        /// <summary>ストリームをCtrlCmdバイナリ形式で読み込む</summary>
        void Read(MemoryStream s, ushort version);
        /// <summary>ストリームにCtrlCmdバイナリ形式で書き込む</summary>
        void Write(MemoryStream s, ushort version);
    }

    public class CtrlCmdWriter
    {
        public MemoryStream Stream { get; private set; }
        public ushort Version { get; set; }
        private long lastPos;
        public CtrlCmdWriter(MemoryStream stream, ushort version = 0)
        {
            Stream = stream;
            Version = version;
            lastPos = 0;
        }
        /// <summary>変換可能なオブジェクトをストリームに書き込む</summary>
        public void Write(object v)
        {
            if (v is byte) Stream.WriteByte((byte)v);
            else if (v is ushort) Stream.Write(BitConverter.GetBytes((ushort)v), 0, 2);
            else if (v is int) Stream.Write(BitConverter.GetBytes((int)v), 0, 4);
            else if (v is uint) Stream.Write(BitConverter.GetBytes((uint)v), 0, 4);
            else if (v is long) Stream.Write(BitConverter.GetBytes((long)v), 0, 8);
            else if (v is ulong) Stream.Write(BitConverter.GetBytes((ulong)v), 0, 8);
            else if (v is ICtrlCmdReadWrite) ((ICtrlCmdReadWrite)v).Write(Stream, Version);
            else if (v is DateTime)
            {
                var t = (DateTime)v;
                Write((ushort)t.Year);
                Write((ushort)t.Month);
                Write((ushort)t.DayOfWeek);
                Write((ushort)t.Day);
                Write((ushort)t.Hour);
                Write((ushort)t.Minute);
                Write((ushort)t.Second);
                Write((ushort)t.Millisecond);
            }
            else if (v is string)
            {
                byte[] a = Encoding.Unicode.GetBytes((string)v);
                Write(a.Length + 6);
                Stream.Write(a, 0, a.Length);
                Write((ushort)0);
            }
            else if (v is System.Collections.IList)
            {
                long lpos = Stream.Position;
                Write(0);
                Write(((System.Collections.IList)v).Count);
                foreach (object o in ((System.Collections.IList)v))
                {
                    Write(o);
                }
                long pos = Stream.Position;
                Stream.Seek(lpos, SeekOrigin.Begin);
                Write((int)(pos - lpos));
                Stream.Seek(pos, SeekOrigin.Begin);
            }
            else
            {
                throw new ArgumentException();
            }
        }
        /// <summary>C++構造体型オブジェクトの書き込みを開始する</summary>
        public void Begin()
        {
            lastPos = Stream.Position;
            Write(0);
        }
        /// <summary>C++構造体型オブジェクトの書き込みを終了する</summary>
        public void End()
        {
            long pos = Stream.Position;
            Stream.Seek(lastPos, SeekOrigin.Begin);
            Write((int)(pos - lastPos));
            Stream.Seek(pos, SeekOrigin.Begin);
        }
    }

    public class CtrlCmdReader
    {
        public MemoryStream Stream { get; private set; }
        public ushort Version { get; set; }
        private long tailPos;
        public CtrlCmdReader(MemoryStream stream, ushort version = 0)
        {
            Stream = stream;
            Version = version;
            tailPos = long.MaxValue;
        }
        private byte[] ReadBytes(int size)
        {
            var a = new byte[size];
            if (Stream.Read(a, 0, size) != size)
            {
                // ストリームの内容が不正な場合はこの例外を投げるので、必要であれば利用側でcatchすること
                // (CtrlCmdは通信相手を信頼できることが前提であるはずなので基本的に必要ない(と思う))
                throw new EndOfStreamException();
            }
            return a;
        }
        /// <summary>オブジェクトの型に従ってストリームから読み込む</summary>
        public void Read<T>(ref T v)
        {
            // このメソッドがジェネリックなのは単にこのBoxingのため
            object o = v;
            if (v is byte) o = ReadBytes(1)[0];
            else if (v is ushort) o = BitConverter.ToUInt16(ReadBytes(2), 0);
            else if (v is int) o = BitConverter.ToInt32(ReadBytes(4), 0);
            else if (v is uint) o = BitConverter.ToUInt32(ReadBytes(4), 0);
            else if (v is long) o = BitConverter.ToInt64(ReadBytes(8), 0);
            else if (v is ulong) o = BitConverter.ToUInt64(ReadBytes(8), 0);
            else if (v is ICtrlCmdReadWrite) ((ICtrlCmdReadWrite)o).Read(Stream, Version);
            else if (v is DateTime)
            {
                var a = new ushort[8];
                for (int i = 0; i < 8; i++)
                {
                    Read(ref a[i]);
                }
                o = new DateTime(a[0], a[1], a[3], a[4], a[5], a[6], a[7]);
            }
            else if (v is string)
            {
                int size = 0;
                Read(ref size);
                if (size < 4 || Stream.Length - Stream.Position < size - 4)
                {
                    throw new EndOfStreamException("サイズフィールドの値が異常です");
                }
                o = "";
                if (size > 4)
                {
                    byte[] a = ReadBytes(size - 4);
                    if (a.Length > 2)
                    {
                        o = size <= 6 ? "" : Encoding.Unicode.GetString(a, 0, a.Length - 2);
                    }
                }
            }
            else if (v is System.Collections.IList)
            {
                int size = 0;
                Read(ref size);
                if (size < 4 || Stream.Length - Stream.Position < size - 4)
                {
                    throw new EndOfStreamException("サイズフィールドの値が異常です");
                }
                long tPos = Stream.Position + size - 4;
                int count = 0;
                Read(ref count);
                // ジェネリックList<T>のTを調べる
                Type t = null;
                foreach (Type u in o.GetType().GetInterfaces())
                {
                    if (u.IsGenericType && u.GetGenericTypeDefinition() == typeof(IList<>))
                    {
                        t = u.GetGenericArguments()[0];
                        break;
                    }
                }
                if (t == null)
                {
                    throw new ArgumentException();
                }
                var list = (System.Collections.IList)o;
                for (int i = 0; i < count; i++)
                {
                    // CreateInstanceは遅いとよく言われるが、ここで使う主要な型のほとんどはnew自体のコストのほうがずっと大きい
                    // 実測1000000ループ4回平均[ミリ秒]:
                    // List<uint> : CreateInstance=147, new=14 / List<ReserveData> : CreateInstance=1975, new=1857
                    object e = t == typeof(string) ? "" : Activator.CreateInstance(t);
                    Read(ref e);
                    list.Add(e);
                }
                if (Stream.Position > tPos)
                {
                    throw new EndOfStreamException("サイズフィールドの値を超えて読み込みました");
                }
                Stream.Seek(tPos, SeekOrigin.Begin);
            }
            else
            {
                throw new ArgumentException();
            }
            v = (T)o;
        }
        /// <summary>C++構造体型オブジェクトの読み込みを開始する</summary>
        public void Begin()
        {
            int size = 0;
            Read(ref size);
            if (size < 4 || Stream.Length - Stream.Position < size - 4)
            {
                throw new EndOfStreamException("サイズフィールドの値が異常です");
            }
            tailPos = Stream.Position + size - 4;
        }
        /// <summary>C++構造体型オブジェクトの読み込みを終了する</summary>
        public void End()
        {
            if (Stream.Position > tailPos)
            {
                throw new EndOfStreamException("サイズフィールドの値を超えて読み込みました");
            }
            Stream.Seek(tailPos, SeekOrigin.Begin);
            tailPos = long.MaxValue;
        }
    }

    public enum CtrlCmd : uint
    {
        // 【CtrlCmdDef.hから自動生成】
        // sed 's/[ \t]\+/ /g' CtrlCmdDef.h |
        // grep '^#define CMD2_\(EPG\|TIMER\|VIEW\)_[0-9A-Za-z_]* [0-9]' |
        // sed 's!//\(.*\)!/// <summary>\1</summary>!' |
        // sed 's/#define CMD2_\([0-9A-Za-z_]*\) \([0-9]*\)\(.*\)/\3\nCMD_\1 = \2,/' |
        // sed 's/^ *//'
        /// <summary>Program.txtの追加読み込み（廃止）</summary>
        CMD_EPG_SRV_ADDLOAD_RESERVE = 1,
        /// <summary>EPG再読み込み</summary>
        CMD_EPG_SRV_RELOAD_EPG = 2,
        /// <summary>設定の再読み込み</summary>
        CMD_EPG_SRV_RELOAD_SETTING = 3,
        /// <summary>アプリケーションの終了（CreateProcessで普通に起動した場合に使用）</summary>
        CMD_EPG_SRV_CLOSE = 4,
        /// <summary>GUIアプリケーションのパイプ名と接続待機用イベント名を登録（タイマーGUI用のコマンドが飛ぶようになる）</summary>
        CMD_EPG_SRV_REGIST_GUI = 5,
        /// <summary>GUIアプリケーションのパイプ名と接続待機用イベント名の登録を解除</summary>
        CMD_EPG_SRV_UNREGIST_GUI = 6,
        /// <summary>TCP接続のGUIアプリケーションのIPとポートを登録（タイマーGUI用のコマンドが飛ぶようになる）</summary>
        CMD_EPG_SRV_REGIST_GUI_TCP = 7,
        /// <summary>TCP接続のGUIアプリケーションのIPとポートの登録を解除</summary>
        CMD_EPG_SRV_UNREGIST_GUI_TCP = 8,
        /// <summary>予約一覧取得</summary>
        CMD_EPG_SRV_ENUM_RESERVE = 1011,
        /// <summary>予約情報取得</summary>
        CMD_EPG_SRV_GET_RESERVE = 1012,
        /// <summary>予約追加</summary>
        CMD_EPG_SRV_ADD_RESERVE = 1013,
        /// <summary>予約削除</summary>
        CMD_EPG_SRV_DEL_RESERVE = 1014,
        /// <summary>予約変更</summary>
        CMD_EPG_SRV_CHG_RESERVE = 1015,
        /// <summary>チューナーごとの予約ID一覧取得</summary>
        CMD_EPG_SRV_ENUM_TUNER_RESERVE = 1016,
        /// <summary>録画済み情報一覧取得</summary>
        CMD_EPG_SRV_ENUM_RECINFO = 1017,
        /// <summary>録画済み情報削除</summary>
        CMD_EPG_SRV_DEL_RECINFO = 1018,
        /// <summary>予約一覧取得</summary>
        CMD_EPG_SRV_ENUM_RESERVE2 = 2011,
        /// <summary>予約情報取得</summary>
        CMD_EPG_SRV_GET_RESERVE2 = 2012,
        /// <summary>予約追加</summary>
        CMD_EPG_SRV_ADD_RESERVE2 = 2013,
        /// <summary>予約変更</summary>
        CMD_EPG_SRV_CHG_RESERVE2 = 2015,
        /// <summary>録画済み情報一覧取得</summary>
        CMD_EPG_SRV_ENUM_RECINFO2 = 2017,
        /// <summary>録画済み情報のプロテクト変更</summary>
        CMD_EPG_SRV_CHG_PROTECT_RECINFO2 = 2019,
        /// <summary>録画済み情報一覧取得（programInfoとerrInfoを除く）</summary>
        CMD_EPG_SRV_ENUM_RECINFO_BASIC2 = 2020,
        /// <summary>録画済み情報取得</summary>
        CMD_EPG_SRV_GET_RECINFO2 = 2024,
        /// <summary>サーバー連携用　予約追加できるかのチェック（戻り値 0:追加不可 1:追加可能 2:追加可能だが開始か終了が重なるものあり 3:すでに同じ物がある）</summary>
        CMD_EPG_SRV_ADDCHK_RESERVE2 = 2030,
        /// <summary>サーバー連携用　EPGデータファイルのタイムスタンプ取得</summary>
        CMD_EPG_SRV_GET_EPG_FILETIME2 = 2031,
        /// <summary>サーバー連携用　EPGデータファイル取得</summary>
        CMD_EPG_SRV_GET_EPG_FILE2 = 2032,
        /// <summary>自動予約登録の条件一覧取得</summary>
        CMD_EPG_SRV_ENUM_AUTO_ADD2 = 2131,
        /// <summary>自動予約登録の条件追加</summary>
        CMD_EPG_SRV_ADD_AUTO_ADD2 = 2132,
        /// <summary>自動予約登録の条件変更</summary>
        CMD_EPG_SRV_CHG_AUTO_ADD2 = 2134,
        /// <summary>プログラム予約自動登録の条件一覧取得</summary>
        CMD_EPG_SRV_ENUM_MANU_ADD2 = 2141,
        /// <summary>プログラム予約自動登録の条件追加</summary>
        CMD_EPG_SRV_ADD_MANU_ADD2 = 2142,
        /// <summary>プログラム予約自動登録の条件変更</summary>
        CMD_EPG_SRV_CHG_MANU_ADD2 = 2144,
        /// <summary>サーバーの情報変更通知を取得（ロングポーリング）</summary>
        CMD_EPG_SRV_GET_STATUS_NOTIFY2 = 2200,
        /// <summary>読み込まれたEPGデータのサービスの一覧取得</summary>
        CMD_EPG_SRV_ENUM_SERVICE = 1021,
        /// <summary>サービス指定で番組情報一覧を取得する</summary>
        CMD_EPG_SRV_ENUM_PG_INFO = 1022,
        /// <summary>番組情報取得</summary>
        CMD_EPG_SRV_GET_PG_INFO = 1023,
        /// <summary>番組検索</summary>
        CMD_EPG_SRV_SEARCH_PG = 1025,
        /// <summary>番組情報一覧取得</summary>
        CMD_EPG_SRV_ENUM_PG_ALL = 1026,
        /// <summary>自動予約登録の条件一覧取得</summary>
        CMD_EPG_SRV_ENUM_AUTO_ADD = 1031,
        /// <summary>自動予約登録の条件追加</summary>
        CMD_EPG_SRV_ADD_AUTO_ADD = 1032,
        /// <summary>自動予約登録の条件削除</summary>
        CMD_EPG_SRV_DEL_AUTO_ADD = 1033,
        /// <summary>自動予約登録の条件変更</summary>
        CMD_EPG_SRV_CHG_AUTO_ADD = 1034,
        /// <summary>プログラム予約自動登録の条件一覧取得</summary>
        CMD_EPG_SRV_ENUM_MANU_ADD = 1041,
        /// <summary>プログラム予約自動登録の条件追加</summary>
        CMD_EPG_SRV_ADD_MANU_ADD = 1042,
        /// <summary>プログラム予約自動登録の条件削除</summary>
        CMD_EPG_SRV_DEL_MANU_ADD = 1043,
        /// <summary>プログラム予約自動登録の条件変更</summary>
        CMD_EPG_SRV_CHG_MANU_ADD = 1044,
        /// <summary>スタンバイ、休止、シャットダウンを行っていいかの確認</summary>
        CMD_EPG_SRV_CHK_SUSPEND = 1050,
        /// <summary>スタンバイ、休止、シャットダウンに移行する（1:スタンバイ 2:休止 3:シャットダウン | 0x0100:復帰後再起動）</summary>
        CMD_EPG_SRV_SUSPEND = 1051,
        /// <summary>PC再起動を行う</summary>
        CMD_EPG_SRV_REBOOT = 1052,
        /// <summary>10秒後にEPGデータの取得を行う</summary>
        CMD_EPG_SRV_EPG_CAP_NOW = 1053,
        /// <summary>指定ファイルを転送する</summary>
        CMD_EPG_SRV_FILE_COPY = 1060,
        /// <summary>PlugInファイルの一覧を取得する（1:ReName、2:Write）</summary>
        CMD_EPG_SRV_ENUM_PLUGIN = 1061,
        /// <summary>TVTestのチャンネル切り替え用の情報を取得する</summary>
        CMD_EPG_SRV_GET_CHG_CH_TVTEST = 1062,
        /// <summary>ネットワークモードのEpgDataCap_Bonのチャンネルを切り替え</summary>
        CMD_EPG_SRV_NWTV_SET_CH = 1070,
        /// <summary>ネットワークモードで起動中のEpgDataCap_Bonを終了</summary>
        CMD_EPG_SRV_NWTV_CLOSE = 1071,
        /// <summary>ネットワークモードで起動するときのモード（1:UDP 2:TCP 3:UDP+TCP）</summary>
        CMD_EPG_SRV_NWTV_MODE = 1072,
        /// <summary>ストリーム配信用ファイルを開く</summary>
        CMD_EPG_SRV_NWPLAY_OPEN = 1080,
        /// <summary>ストリーム配信用ファイルを閉じる</summary>
        CMD_EPG_SRV_NWPLAY_CLOSE = 1081,
        /// <summary>ストリーム配信開始</summary>
        CMD_EPG_SRV_NWPLAY_PLAY = 1082,
        /// <summary>ストリーム配信停止</summary>
        CMD_EPG_SRV_NWPLAY_STOP = 1083,
        /// <summary>ストリーム配信で現在の送信位置と総ファイルサイズを取得する</summary>
        CMD_EPG_SRV_NWPLAY_GET_POS = 1084,
        /// <summary>ストリーム配信で送信位置をシークする</summary>
        CMD_EPG_SRV_NWPLAY_SET_POS = 1085,
        /// <summary>ストリーム配信で送信先を設定する</summary>
        CMD_EPG_SRV_NWPLAY_SET_IP = 1086,
        /// <summary>ストリーム配信用ファイルをタイムシフトモードで開く</summary>
        CMD_EPG_SRV_NWPLAY_TF_OPEN = 1087,
        /// <summary>ダイアログを前面に表示</summary>
        CMD_TIMER_GUI_SHOW_DLG = 101,
        /// <summary>予約一覧の情報が更新された</summary>
        CMD_TIMER_GUI_UPDATE_RESERVE = 102,
        /// <summary>EPGデータの再読み込みが完了した</summary>
        CMD_TIMER_GUI_UPDATE_EPGDATA = 103,
        /// <summary>Viewアプリ（EpgDataCap_Bon.exe）を起動</summary>
        CMD_TIMER_GUI_VIEW_EXECUTE = 110,
        /// <summary>スタンバイ、休止、シャットダウンに入っていいかの確認をユーザーに行う（入っていいならCMD_EPG_SRV_SUSPENDを送る）</summary>
        CMD_TIMER_GUI_QUERY_SUSPEND = 120,
        /// <summary>PC再起動に入っていいかの確認をユーザーに行う（入っていいならCMD_EPG_SRV_REBOOTを送る）</summary>
        CMD_TIMER_GUI_QUERY_REBOOT = 121,
        /// <summary>サーバーのステータス変更通知（1:通常、2:EPGデータ取得開始、3:予約録画開始）</summary>
        CMD_TIMER_GUI_SRV_STATUS_CHG = 130,
        /// <summary>サーバーの情報変更通知</summary>
        CMD_TIMER_GUI_SRV_STATUS_NOTIFY2 = 1130,
        /// <summary>BonDriverの切り替え</summary>
        CMD_VIEW_APP_SET_BONDRIVER = 201,
        /// <summary>使用中のBonDriverのファイル名を取得</summary>
        CMD_VIEW_APP_GET_BONDRIVER = 202,
        /// <summary>SpaceとCh or OriginalNetworkID、TSID、ServieIDでチャンネル切り替え</summary>
        CMD_VIEW_APP_SET_CH = 205,
        /// <summary>放送波の時間とPC時間の誤差取得</summary>
        CMD_VIEW_APP_GET_DELAY = 206,
        /// <summary>現在の状態を取得</summary>
        CMD_VIEW_APP_GET_STATUS = 207,
        /// <summary>アプリケーションの終了</summary>
        CMD_VIEW_APP_CLOSE = 208,
        /// <summary>識別用IDの設定</summary>
        CMD_VIEW_APP_SET_ID = 1201,
        /// <summary>識別用IDの取得</summary>
        CMD_VIEW_APP_GET_ID = 1202,
        /// <summary>予約録画用にGUIキープ</summary>
        CMD_VIEW_APP_SET_STANDBY_REC = 1203,
        /// <summary>Viewボタン登録アプリの起動</summary>
        CMD_VIEW_APP_EXEC_VIEW_APP = 1204,
        /// <summary>ストリーム制御用コントロール作成</summary>
        CMD_VIEW_APP_CREATE_CTRL = 1221,
        /// <summary>ストリーム制御用コントロール削除</summary>
        CMD_VIEW_APP_DELETE_CTRL = 1222,
        /// <summary>コントロールの動作を設定（対象サービス、スクランブル、処理対象データ）</summary>
        CMD_VIEW_APP_SET_CTRLMODE = 1223,
        /// <summary>録画処理開始</summary>
        CMD_VIEW_APP_REC_START_CTRL = 1224,
        /// <summary>録画処理停止</summary>
        CMD_VIEW_APP_REC_STOP_CTRL = 1225,
        /// <summary>録画ファイルパスを取得</summary>
        CMD_VIEW_APP_REC_FILE_PATH = 1226,
        /// <summary>即時録画を停止</summary>
        CMD_VIEW_APP_REC_STOP_ALL = 1227,
        /// <summary>ファイル出力したサイズを取得</summary>
        CMD_VIEW_APP_REC_WRITE_SIZE = 1228,
        /// <summary>EPG取得開始</summary>
        CMD_VIEW_APP_EPGCAP_START = 1241,
        /// <summary>EPG取得停止</summary>
        CMD_VIEW_APP_EPGCAP_STOP = 1242,
        /// <summary>EPG情報の検索</summary>
        CMD_VIEW_APP_SEARCH_EVENT = 1251,
        /// <summary>現在or次の番組情報を取得する</summary>
        CMD_VIEW_APP_GET_EVENT_PF = 1252,
        /// <summary>ストリーミング配信制御IDの設定</summary>
        CMD_VIEW_APP_TT_SET_CTRL = 1261,
    }

    /// <summary>CtrlCmdコマンド送信クラス(SendCtrlCmd.h/cppから移植)</summary>
    public class CtrlCmdUtil
    {
        private const ushort CMD_VER = 5;
        private bool tcpFlag = false;
        private int connectTimeOut = 15000;
        private string eventName = "Global\\EpgTimerSrvConnect";
        private string pipeName = "EpgTimerSrvPipe";
        private string ip = "127.0.0.1";
        private uint port = 5678;
        // TODO: 本来この排他用オブジェクトは不要だが、このクラスの利用側がマルチスレッドを考慮していないようなので念のため従来仕様に合わせる
        private object thisLock = new object();

        /// <summary>コマンド送信方法の設定</summary>
        public void SetSendMode(bool tcpFlag)
        {
            this.tcpFlag = tcpFlag;
        }
        /// <summary>名前付きパイプモード時の接続先を設定</summary>
        public void SetPipeSetting(string eventName, string pipeName)
        {
            lock (thisLock)
            {
                this.eventName = eventName;
                this.pipeName = pipeName.Substring(pipeName.StartsWith("\\\\.\\pipe\\", StringComparison.OrdinalIgnoreCase) ? 9 : 0);
            }
        }
        /// <summary>TCP/IPモード時の接続先を設定</summary>
        public void SetNWSetting(string ip, uint port)
        {
            lock (thisLock)
            {
                this.ip = ip;
                this.port = port;
            }
        }
        /// <summary>接続処理時のタイムアウト設定</summary>
        public void SetConnectTimeOut(int timeOut)
        {
            connectTimeOut = timeOut;
        }

        // 【SendCtrlCmd.hから自動生成→型を調整】
        // sed 's/[ \t]\+/ /g' SendCtrlCmd.h |
        // sed 's/^ //' |
        // grep -v '^//\(戻\|引\| \)' |
        // sed 's!//\(.*\)!/// <summary>\1</summary>!' |
        // sed 's/^DWORD Send/public ErrCode Send/' |
        // sed 's/CMD2_/CtrlCmdID.CMD_/' |
        // tr -d '\n' |
        // sed 's!\(</summary>\|}\)!\1\n!g' |
        // sed 's/{/ { /' |
        // sed 's/}/ }/'
        /// <summary>EPGデータを再読み込みする</summary>
        public ErrCode SendReloadEpg() { return SendCmdWithoutData(CtrlCmd.CMD_EPG_SRV_RELOAD_EPG); }
        /// <summary>設定情報を再読み込みする</summary>
        public ErrCode SendReloadSetting() { return SendCmdWithoutData(CtrlCmd.CMD_EPG_SRV_RELOAD_SETTING); }
        /// <summary>EpgTimerSrv.exeを終了する</summary>
        public ErrCode SendClose() { return SendCmdWithoutData(CtrlCmd.CMD_EPG_SRV_CLOSE); }
        /// <summary>EpgTimerSrv.exeのパイプ接続GUIとしてプロセスを登録する</summary>
        public ErrCode SendRegistGUI(uint processID) { return SendCmdData(CtrlCmd.CMD_EPG_SRV_REGIST_GUI, processID); }
        /// <summary>EpgTimerSrv.exeのパイプ接続GUI登録を解除する</summary>
        public ErrCode SendUnRegistGUI(uint processID) { return SendCmdData(CtrlCmd.CMD_EPG_SRV_UNREGIST_GUI, processID); }
        /// <summary>EpgTimerSrv.exeのTCP接続GUIとしてプロセスを登録する</summary>
        public ErrCode SendRegistTCP(uint port) { return SendCmdData(CtrlCmd.CMD_EPG_SRV_REGIST_GUI_TCP, port); }
        /// <summary>EpgTimerSrv.exeのTCP接続GUI登録を解除する</summary>
        public ErrCode SendUnRegistTCP(uint port) { return SendCmdData(CtrlCmd.CMD_EPG_SRV_UNREGIST_GUI_TCP, port); }
        /// <summary>予約を削除する</summary>
        public ErrCode SendDelReserve(List<uint> val) { return SendCmdData(CtrlCmd.CMD_EPG_SRV_DEL_RESERVE, val); }
        /// <summary>チューナーごとの予約一覧を取得する</summary>
        public ErrCode SendEnumTunerReserve(ref List<TunerReserveInfo> val) { object o = val; return ReceiveCmdData(CtrlCmd.CMD_EPG_SRV_ENUM_TUNER_RESERVE, ref o); }
        /// <summary>録画済み情報を削除する</summary>
        public ErrCode SendDelRecInfo(List<uint> val) { return SendCmdData(CtrlCmd.CMD_EPG_SRV_DEL_RECINFO, val); }
        /// <summary>サービス一覧を取得する</summary>
        public ErrCode SendEnumService(ref List<EpgServiceInfo> val) { object o = val; return ReceiveCmdData(CtrlCmd.CMD_EPG_SRV_ENUM_SERVICE, ref o); }
        /// <summary>サービス指定で番組情報を一覧を取得する</summary>
        public ErrCode SendEnumPgInfo(ulong service, ref List<EpgEventInfo> val) { object o = val; return SendAndReceiveCmdData(CtrlCmd.CMD_EPG_SRV_ENUM_PG_INFO, service, ref o); }
        /// <summary>指定イベントの番組情報を取得する</summary>
        public ErrCode SendGetPgInfo(ulong pgID, ref EpgEventInfo val) { object o = val; return SendAndReceiveCmdData(CtrlCmd.CMD_EPG_SRV_GET_PG_INFO, pgID, ref o); }
        /// <summary>指定キーワードで番組情報を検索する</summary>
        public ErrCode SendSearchPg(List<EpgSearchKeyInfo> key, ref List<EpgEventInfo> val) { object o = val; return SendAndReceiveCmdData(CtrlCmd.CMD_EPG_SRV_SEARCH_PG, key, ref o); }
        /// <summary>番組情報一覧を取得する</summary>
        public ErrCode SendEnumPgAll(ref List<EpgServiceEventInfo> val) { object o = val;  return ReceiveCmdData(CtrlCmd.CMD_EPG_SRV_ENUM_PG_ALL, ref o); }
        /// <summary>自動予約登録条件を削除する</summary>
        public ErrCode SendDelEpgAutoAdd(List<uint> val) { return SendCmdData(CtrlCmd.CMD_EPG_SRV_DEL_AUTO_ADD, val); }
        /// <summary>プログラム予約自動登録の条件削除</summary>
        public ErrCode SendDelManualAdd(List<uint> val) { return SendCmdData(CtrlCmd.CMD_EPG_SRV_DEL_MANU_ADD, val); }
        public ErrCode SendChkSuspend() { return SendCmdWithoutData(CtrlCmd.CMD_EPG_SRV_CHK_SUSPEND); }
        public ErrCode SendSuspend(ushort val) { return SendCmdData(CtrlCmd.CMD_EPG_SRV_SUSPEND, val); }
        public ErrCode SendReboot() { return SendCmdWithoutData(CtrlCmd.CMD_EPG_SRV_REBOOT); }
        public ErrCode SendEpgCapNow() { return SendCmdWithoutData(CtrlCmd.CMD_EPG_SRV_EPG_CAP_NOW); }
        /// <summary>PlugInファイルの一覧を取得する</summary>
        public ErrCode SendEnumPlugIn(ushort val, ref List<string> resVal) { object o = resVal; return SendAndReceiveCmdData(CtrlCmd.CMD_EPG_SRV_ENUM_PLUGIN, val, ref o); }
        /// <summary>TVTestのチャンネル切り替え用の情報を取得する</summary>
        public ErrCode SendGetChgChTVTest(ulong val, ref TvTestChChgInfo resVal) { object o = resVal; return SendAndReceiveCmdData(CtrlCmd.CMD_EPG_SRV_GET_CHG_CH_TVTEST, val, ref o); }
        /// <summary>ネットワークモードのEpgDataCap_Bonのチャンネルを切り替え</summary>
        public ErrCode SendNwTVSetCh(SetChInfo val) { return SendCmdData(CtrlCmd.CMD_EPG_SRV_NWTV_SET_CH, val); }
        /// <summary>ネットワークモードで起動中のEpgDataCap_Bonを終了</summary>
        public ErrCode SendNwTVClose() { return SendCmdWithoutData(CtrlCmd.CMD_EPG_SRV_NWTV_CLOSE); }
        /// <summary>ネットワークモードで起動するときのモード</summary>
        public ErrCode SendNwTVMode(uint val) { return SendCmdData(CtrlCmd.CMD_EPG_SRV_NWTV_MODE, val); }
        /// <summary>ストリーム配信用ファイルを開く</summary>
        public ErrCode SendNwPlayOpen(string val, ref uint resVal) { object o = resVal; var ret = SendAndReceiveCmdData(CtrlCmd.CMD_EPG_SRV_NWPLAY_OPEN, val, ref o); resVal = (uint)o; return ret; }
        /// <summary>ストリーム配信用ファイルを閉じる</summary>
        public ErrCode SendNwPlayClose(uint val) { return SendCmdData(CtrlCmd.CMD_EPG_SRV_NWPLAY_CLOSE, val); }
        /// <summary>ストリーム配信用ファイルをタイムシフトモードで開く</summary>
        public ErrCode SendNwTimeShiftOpen(uint val, ref NWPlayTimeShiftInfo resVal) { object o = resVal; return SendAndReceiveCmdData(CtrlCmd.CMD_EPG_SRV_NWPLAY_TF_OPEN, val, ref o); }
        #region // コマンドバージョン対応版
        /// <summary>予約一覧を取得する</summary>
        public ErrCode SendEnumReserve(ref List<ReserveData> val) { object o = val; return ReceiveCmdData2(CtrlCmd.CMD_EPG_SRV_ENUM_RESERVE2, ref o); }
        /// <summary>予約情報を取得する</summary>
        public ErrCode SendGetReserve(uint reserveID, ref ReserveData val) { object o = val; return SendAndReceiveCmdData2(CtrlCmd.CMD_EPG_SRV_GET_RESERVE2, reserveID, ref o); }
        /// <summary>予約を追加する</summary>
        public ErrCode SendAddReserve(List<ReserveData> val) { return SendCmdData2(CtrlCmd.CMD_EPG_SRV_ADD_RESERVE2, val); }
        /// <summary>予約を変更する</summary>
        public ErrCode SendChgReserve(List<ReserveData> val) { return SendCmdData2(CtrlCmd.CMD_EPG_SRV_CHG_RESERVE2, val); }
        /// <summary>自動予約登録条件一覧を取得する</summary>
        public ErrCode SendEnumEpgAutoAdd(ref List<EpgAutoAddData> val) { object o = val; return ReceiveCmdData2(CtrlCmd.CMD_EPG_SRV_ENUM_AUTO_ADD2, ref o); }
        /// <summary>自動予約登録条件を追加する</summary>
        public ErrCode SendAddEpgAutoAdd(List<EpgAutoAddData> val) { return SendCmdData2(CtrlCmd.CMD_EPG_SRV_ADD_AUTO_ADD2, val); }
        /// <summary>自動予約登録条件を変更する</summary>
        public ErrCode SendChgEpgAutoAdd(List<EpgAutoAddData> val) { return SendCmdData2(CtrlCmd.CMD_EPG_SRV_CHG_AUTO_ADD2, val); }
        /// <summary>自動予約登録条件一覧を取得する</summary>
        public ErrCode SendEnumManualAdd(ref List<ManualAutoAddData> val) { object o = val; return ReceiveCmdData2(CtrlCmd.CMD_EPG_SRV_ENUM_MANU_ADD2, ref o); }
        /// <summary>自動予約登録条件を追加する</summary>
        public ErrCode SendAddManualAdd(List<ManualAutoAddData> val) { return SendCmdData2(CtrlCmd.CMD_EPG_SRV_ADD_MANU_ADD2, val); }
        /// <summary>プログラム予約自動登録の条件変更</summary>
        public ErrCode SendChgManualAdd(List<ManualAutoAddData> val) { return SendCmdData2(CtrlCmd.CMD_EPG_SRV_CHG_MANU_ADD2, val); }
        /// <summary>録画済み情報一覧取得</summary>
        public ErrCode SendEnumRecInfo(ref List<RecFileInfo> val) { object o = val; return ReceiveCmdData2(CtrlCmd.CMD_EPG_SRV_ENUM_RECINFO2, ref o); }
        /// <summary>録画済み情報一覧取得（programInfoとerrInfoを除く）</summary>
        public ErrCode SendEnumRecInfoBasic(ref List<RecFileInfo> val) { object o = val; return ReceiveCmdData2(CtrlCmd.CMD_EPG_SRV_ENUM_RECINFO_BASIC2, ref o); }
        /// <summary>録画済み情報取得</summary>
        public ErrCode SendGetRecInfo(uint id, ref RecFileInfo val) { object o = val; return SendAndReceiveCmdData2(CtrlCmd.CMD_EPG_SRV_GET_RECINFO2, id, ref o); }
        /// <summary>録画済み情報のプロテクト変更</summary>
        public ErrCode SendChgProtectRecInfo(List<RecFileInfo> val) { return SendCmdData2(CtrlCmd.CMD_EPG_SRV_CHG_PROTECT_RECINFO2, val); }
        /// <summary>現在のNOTIFY_UPDATE_SRV_STATUSを取得する</summary>
        public ErrCode SendGetNotifySrvStatus(ref NotifySrvInfo val) { object o = val; return SendAndReceiveCmdData2(CtrlCmd.CMD_EPG_SRV_GET_STATUS_NOTIFY2, 0, ref o); }
        #endregion
        /// <summary>BonDriverの切り替え</summary>
        public ErrCode SendViewSetBonDrivere(string val) { return SendCmdData(CtrlCmd.CMD_VIEW_APP_SET_BONDRIVER, val); }
        /// <summary>使用中のBonDriverのファイル名を取得</summary>
        public ErrCode SendViewGetBonDrivere(ref string val) { object o = val; var ret = ReceiveCmdData(CtrlCmd.CMD_VIEW_APP_GET_BONDRIVER, ref o); val = (string)o; return ret; }
        /// <summary>チャンネル切り替え</summary>
        public ErrCode SendViewSetCh(SetChInfo chInfo) { return SendCmdData(CtrlCmd.CMD_VIEW_APP_SET_CH, chInfo); }
        /// <summary>アプリケーションの終了</summary>
        public ErrCode SendViewAppClose() { return SendCmdWithoutData(CtrlCmd.CMD_VIEW_APP_CLOSE); }
        /// <summary>識別用IDの取得</summary>
        public ErrCode SendViewGetID(ref int val) { object o = val; var ret = ReceiveCmdData(CtrlCmd.CMD_VIEW_APP_GET_ID, ref o); val = (int)o; return ret; }
        /// <summary>ストリーミング配信制御IDの設定</summary>
        public ErrCode SendViewSetStreamingInfo(TVTestStreamingInfo val) { return SendCmdData(CtrlCmd.CMD_VIEW_APP_TT_SET_CTRL, val); }

        /// <summary>指定ファイルを転送する</summary>
        public ErrCode SendFileCopy(string val, out byte[] resVal)
        {
            var w = new CtrlCmdWriter(new MemoryStream());
            w.Write(val);
            MemoryStream res = null;
            ErrCode ret = SendCmdStream(CtrlCmd.CMD_EPG_SRV_FILE_COPY, w.Stream, ref res);
            resVal = ret == ErrCode.CMD_SUCCESS ? res.ToArray() : null;
            return ret;
        }

        private ErrCode SendPipe(CtrlCmd param, MemoryStream send, ref MemoryStream res)
        {
            lock (thisLock)
            {
                // 接続待ち
                try
                {
                    using (var waitEvent = System.Threading.EventWaitHandle.OpenExisting(eventName,
                               System.Security.AccessControl.EventWaitHandleRights.Synchronize))
                    {
                        if (waitEvent.WaitOne(connectTimeOut) == false)
                        {
                            return ErrCode.CMD_ERR_TIMEOUT;
                        }
                    }
                }
                catch (System.Threading.WaitHandleCannotBeOpenedException)
                {
                    return ErrCode.CMD_ERR_CONNECT;
                }
                // 接続
                using (var pipe = new System.IO.Pipes.NamedPipeClientStream(pipeName))
                {
                    pipe.Connect(0);
                    // 送信
                    var head = new byte[8];
                    BitConverter.GetBytes((uint)param).CopyTo(head, 0);
                    BitConverter.GetBytes((uint)(send == null ? 0 : send.Length)).CopyTo(head, 4);
                    pipe.Write(head, 0, 8);
                    if (send != null && send.Length != 0)
                    {
                        send.Close();
                        byte[] data = send.ToArray();
                        pipe.Write(data, 0, data.Length);
                    }
                    // 受信
                    if (pipe.Read(head, 0, 8) != 8)
                    {
                        return ErrCode.CMD_ERR;
                    }
                    uint resParam = BitConverter.ToUInt32(head, 0);
                    var resData = new byte[BitConverter.ToUInt32(head, 4)];
                    for (int n = 0; n < resData.Length; )
                    {
                        int m = pipe.Read(resData, n, resData.Length - n);
                        if (m <= 0)
                        {
                            return ErrCode.CMD_ERR;
                        }
                        n += m;
                    }
                    res = new MemoryStream(resData, false);
                    return Enum.IsDefined(typeof(ErrCode), resParam) ? (ErrCode)resParam : ErrCode.CMD_ERR;
                }
            }
        }

        private static int ReadAll(Stream s, byte[] buffer, int offset, int size)
        {
            int n = 0;
            for (int m; n < size && (m = s.Read(buffer, offset + n, size - n)) > 0; n += m) ;
            return n;
        }

        private ErrCode SendTCP(CtrlCmd param, MemoryStream send, ref MemoryStream res)
        {
            lock (thisLock)
            {
                System.Net.IPAddress addr;
                if (System.Net.IPAddress.TryParse(ip, out addr) == false)
                {
                    return ErrCode.CMD_ERR_CONNECT;
                }
                // 接続
                using (var tcp = new System.Net.Sockets.TcpClient())
                {
                    try
                    {
                        tcp.Connect(addr, (int)port);
                    }
                    catch (System.Net.Sockets.SocketException)
                    {
                        return ErrCode.CMD_ERR_CONNECT;
                    }
                    using (System.Net.Sockets.NetworkStream ns = tcp.GetStream())
                    {
                        // 送信
                        var head = new byte[8 + (send == null ? 0 : send.Length)];
                        BitConverter.GetBytes((uint)param).CopyTo(head, 0);
                        BitConverter.GetBytes((uint)(send == null ? 0 : send.Length)).CopyTo(head, 4);
                        if (send != null && send.Length != 0)
                        {
                            send.Close();
                            send.ToArray().CopyTo(head, 8);
                        }
                        ns.Write(head, 0, head.Length);
                        // 受信
                        if (ReadAll(ns, head, 0, 8) != 8)
                        {
                            return ErrCode.CMD_ERR;
                        }
                        uint resParam = BitConverter.ToUInt32(head, 0);
                        var resData = new byte[BitConverter.ToUInt32(head, 4)];
                        if (ReadAll(ns, resData, 0, resData.Length) != resData.Length)
                        {
                            return ErrCode.CMD_ERR;
                        }
                        res = new MemoryStream(resData, false);
                        return Enum.IsDefined(typeof(ErrCode), resParam) ? (ErrCode)resParam : ErrCode.CMD_ERR;
                    }
                }
            }
        }

        private ErrCode SendCmdStream(CtrlCmd param, MemoryStream send, ref MemoryStream res)
        {
            return tcpFlag ? SendTCP(param, send, ref res) : SendPipe(param, send, ref res);
        }
        private ErrCode SendCmdWithoutData(CtrlCmd param)
        {
            MemoryStream res = null;
            return SendCmdStream(param, null, ref res);
        }
        private ErrCode SendCmdData(CtrlCmd param, object val)
        {
            var w = new CtrlCmdWriter(new MemoryStream());
            w.Write(val);
            MemoryStream res = null;
            return SendCmdStream(param, w.Stream, ref res);
        }
        private ErrCode SendCmdData2(CtrlCmd param, object val)
        {
            var w = new CtrlCmdWriter(new MemoryStream(), CMD_VER);
            w.Write(CMD_VER);
            w.Write(val);
            MemoryStream res = null;
            return SendCmdStream(param, w.Stream, ref res);
        }
        private ErrCode ReceiveCmdData(CtrlCmd param, ref object val)
        {
            MemoryStream res = null;
            ErrCode ret = SendCmdStream(param, null, ref res);
            if (ret == ErrCode.CMD_SUCCESS)
            {
                (new CtrlCmdReader(res)).Read(ref val);
            }
            return ret;
        }
        private ErrCode ReceiveCmdData2(CtrlCmd param, ref object val)
        {
            var w = new CtrlCmdWriter(new MemoryStream(), CMD_VER);
            w.Write(CMD_VER);
            MemoryStream res = null;
            ErrCode ret = SendCmdStream(param, w.Stream, ref res);
            if (ret == ErrCode.CMD_SUCCESS)
            {
                var r = new CtrlCmdReader(res);
                ushort version = 0;
                r.Read(ref version);
                r.Version = version;
                r.Read(ref val);
            }
            return ret;
        }
        private ErrCode SendAndReceiveCmdData(CtrlCmd param, object val, ref object resVal)
        {
            var w = new CtrlCmdWriter(new MemoryStream());
            w.Write(val);
            MemoryStream res = null;
            ErrCode ret = SendCmdStream(param, w.Stream, ref res);
            if (ret == ErrCode.CMD_SUCCESS)
            {
                (new CtrlCmdReader(res)).Read(ref resVal);
            }
            return ret;
        }
        private ErrCode SendAndReceiveCmdData2(CtrlCmd param, object val, ref object resVal)
        {
            var w = new CtrlCmdWriter(new MemoryStream(), CMD_VER);
            w.Write(CMD_VER);
            w.Write(val);
            MemoryStream res = null;
            ErrCode ret = SendCmdStream(param, w.Stream, ref res);
            if (ret == ErrCode.CMD_SUCCESS)
            {
                var r = new CtrlCmdReader(res);
                ushort version = 0;
                r.Read(ref version);
                r.Version = version;
                r.Read(ref resVal);
            }
            return ret;
        }
    }
}

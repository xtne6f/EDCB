using System;
using System.Collections.Generic;
using System.Text;
using System.Net;
using System.IO;
using System.Reflection;

namespace EpgTimer
{
    /// <summary>
    /// EDCBの予約をしょぼいカレンダーにアップロードするツール
    /// ・仕様は基本的にtvrockSchUploader.jsなどと同じ
    /// ・以前の送信内容を.lastファイルに書き込み、変化がなければ次回の送信は行わない
    /// ・ビルド: %SystemRoot%\Microsoft.NET\Framework\v3.5\csc.exe /nologo /optimize EdcbSchUploader.cs
    /// </summary>
    class EdcbSchUploader
    {
        private const string DEV_COLORS = "T9999=#ff0000\t#1e90ff";
        private const string USER_AGENT = "edcbSchUploader/1.0.0";
        private const string UPLOAD_URL = "http://cal.syoboi.jp/sch_upload";
        private const int TIMEOUT = 10000;
        // 番組名を省略する文字数
        private const int UPLOAD_TITLE_MAXLEN = 30;
        // 録画優先度がこれ以上の予約だけ送信する
        private const byte UPLOAD_REC_PRIORITY = 1;

        static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("Usage: EdcbSchUploader <user> <pass> [epgurl] [slot] [upload_url]");
                Environment.Exit(2);
            }

            // サービス名としょぼかる放送局名の対応ファイル"SyoboiCh.txt"があれば読み込む
            var chMap = new Dictionary<string, string>();
            try
            {
                string[] chList = File.ReadAllText(
                    Path.Combine(Path.GetDirectoryName(Assembly.GetEntryAssembly().Location), "SyoboiCh.txt"), Encoding.GetEncoding(932)
                    ).Split(new string[] { "\r\n" }, StringSplitOptions.RemoveEmptyEntries);
                foreach (string ch in chList)
                {
                    string[] chSplit = ch.Split(new char[] { '\t' });
                    if (chSplit.Length >= 2 && chSplit[0].StartsWith(";") == false)
                    {
                        chMap[chSplit[0]] = chSplit[1];
                    }
                }
            }
            catch
            {
            }

            // EpgTimerSrvから予約情報を取得
            var cmd = new CtrlCmdUtil();
            var rl = new List<ReserveData>();
            var tl = new List<TunerReserveInfo>();
            if (cmd.SendEnumReserve(ref rl) != ErrCode.CMD_SUCCESS ||
                cmd.SendEnumTunerReserve(ref tl) != ErrCode.CMD_SUCCESS)
            {
                Console.WriteLine("Failed to communicate with EpgTimerSrv.");
                Environment.Exit(1);
            }
            string data = "";
            int dataCount = 0;
            foreach (ReserveData r in rl)
            {
                string tuner = null;
                foreach (TunerReserveInfo tr in tl)
                {
                    if (tr.reserveList.Contains(r.ReserveID))
                    {
                        // T+{BonDriver番号(上2桁)}+{チューナ番号(下2桁)},チューナ不足はT9999
                        tuner = "T" + ("" + (10000 + Math.Min(tr.tunerID / 65536, 99) * 100 + Math.Min(tr.tunerID % 65536, 99))).Substring(1);
                        break;
                    }
                }
                if (tuner != null && r.RecSetting.RecMode <= 3 && r.RecSetting.Priority >= UPLOAD_REC_PRIORITY)
                {
                    // "最大200行"
                    if (dataCount++ >= 200)
                    {
                        break;
                    }
                    else
                    {
                        // "開始時間 終了時間 デバイス名 番組名 放送局名 サブタイトル オフセット XID"
                        string title = r.Title.Replace("\t", "").Replace("\r", "").Replace("\n", "");
                        data += ""
                            + (r.StartTime.ToUniversalTime() - (new DateTime(1970, 1, 1))).TotalSeconds + "\t"
                            + (r.StartTime.ToUniversalTime().AddSeconds(r.DurationSecond) - (new DateTime(1970, 1, 1))).TotalSeconds + "\t"
                            + tuner + "\t"
                            + (title.Length > UPLOAD_TITLE_MAXLEN ? title.Substring(0, UPLOAD_TITLE_MAXLEN - 1) + "…" : title) + "\t"
                            + (chMap.ContainsKey(r.StationName) ? chMap[r.StationName] : r.StationName) + "\t\t0\t"
                            + r.ReserveID + "\n";
                    }
                }
            }

            // 以前の内容と同じなら送信しない
            string lastPath = Path.ChangeExtension(Assembly.GetEntryAssembly().Location, "last");
            try
            {
                if (File.ReadAllText(lastPath) == data.Replace("\n", "\r\n"))
                {
                    Console.WriteLine("Not modified, nothing to do.");
                    Environment.Exit(0);
                }
            }
            catch
            {
            }

            // しょぼかるのPOSTは"Expect: 100-Continue"に対応してないっぽい
            ServicePointManager.Expect100Continue = false;

            // 予約情報をサーバに送信
            using (var wc = new WebClientWithTimeout())
            {
                // WebClientはデフォルトでIEのプロキシ設定に従う(wc.Proxy = WebRequest.DefaultWebProxy)
                // 認証プロキシなど必要であればwc.Proxyをここで設定すること
                wc.Timeout = TIMEOUT;
                wc.Encoding = Encoding.UTF8;
                wc.Credentials = new NetworkCredential(Uri.EscapeDataString(args[0]), Uri.EscapeDataString(args[1]));
                wc.Headers[HttpRequestHeader.ContentType] = "application/x-www-form-urlencoded";
                wc.Headers[HttpRequestHeader.UserAgent] = USER_AGENT;
                var body = new System.Collections.Specialized.NameValueCollection();
                body["slot"] = args.Length < 4 ? "0" : args[3];
                body["devcolors"] = DEV_COLORS;
                body["epgurl"] = args.Length < 3 ? "" : args[2];
                body["data"] = data;
                try
                {
                    wc.UploadValues(args.Length < 5 ? UPLOAD_URL : args[4], body);
                    File.WriteAllText(lastPath, data.Replace("\n", "\r\n"), Encoding.UTF8);
                }
                catch (Exception ex)
                {
                    Console.WriteLine(ex.GetType().ToString() + " : " + ex.Message);
                    Environment.Exit(1);
                }
            }
            Console.WriteLine("Done.");
            Environment.Exit(0);
        }

        public class WebClientWithTimeout : WebClient
        {
            public int Timeout { get; set; }
            protected override WebRequest GetWebRequest(Uri address)
            {
                var wr = base.GetWebRequest(address);
                wr.Timeout = Timeout;
                return wr;
            }
        }
    }

    #region // "DefineEnum.cs"からコピペ

    public enum ErrCode : uint
    {
        CMD_SUCCESS = 1, //成功
        CMD_ERR = 0, //汎用エラー
        CMD_NEXT = 202, //Enumコマンド用、続きあり
        CMD_NON_SUPPORT = 203, //未サポートのコマンド
        CMD_ERR_INVALID_ARG = 204, //引数エラー
        CMD_ERR_CONNECT = 205, //サーバーにコネクトできなかった
        CMD_ERR_DISCONNECT = 206, //サーバーから切断された
        CMD_ERR_TIMEOUT = 207, //タイムアウト発生
        CMD_ERR_BUSY = 208, //ビジー状態で現在処理できない（EPGデータ読み込み中、録画中など）
        CMD_NO_RES = 250 //Post用でレスポンスの必要なし
    };

    #endregion
    #region // "CtrlCmd.cs"からコピペ

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
        public CtrlCmdWriter(MemoryStream stream) : this(stream, 0) { }
        public CtrlCmdWriter(MemoryStream stream, ushort version)
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
        public CtrlCmdReader(MemoryStream stream) : this(stream, 0) { }
        public CtrlCmdReader(MemoryStream stream, ushort version)
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
        /// <summary>チューナーごとの予約ID一覧取得</summary>
        CMD_EPG_SRV_ENUM_TUNER_RESERVE = 1016,
        /// <summary>予約一覧取得</summary>
        CMD_EPG_SRV_ENUM_RESERVE2 = 2011,
    }

    /// <summary>CtrlCmdコマンド送信クラス(SendCtrlCmd.h/cppから移植)</summary>
    public class CtrlCmdUtil
    {
        private const ushort CMD_VER = 5;
        private int connectTimeOut = 15000;
        private string eventName = "Global\\EpgTimerSrvConnect";
        private string pipeName = "EpgTimerSrvPipe";
        // TODO: 本来この排他用オブジェクトは不要だが、このクラスの利用側がマルチスレッドを考慮していないようなので念のため従来仕様に合わせる
        private object thisLock = new object();

        /// <summary>名前付きパイプモード時の接続先を設定</summary>
        public void SetPipeSetting(string eventName, string pipeName)
        {
            lock (thisLock)
            {
                this.eventName = eventName;
                this.pipeName = pipeName.Substring(pipeName.StartsWith("\\\\.\\pipe\\", StringComparison.OrdinalIgnoreCase) ? 9 : 0);
            }
        }
        /// <summary>接続処理時のタイムアウト設定</summary>
        public void SetConnectTimeOut(int timeOut)
        {
            connectTimeOut = timeOut;
        }

        /// <summary>チューナーごとの予約一覧を取得する</summary>
        public ErrCode SendEnumTunerReserve(ref List<TunerReserveInfo> val) { object o = val; return ReceiveCmdData(CtrlCmd.CMD_EPG_SRV_ENUM_TUNER_RESERVE, ref o); }
        #region // コマンドバージョン対応版
        /// <summary>予約一覧を取得する</summary>
        public ErrCode SendEnumReserve(ref List<ReserveData> val) { object o = val; return ReceiveCmdData2(CtrlCmd.CMD_EPG_SRV_ENUM_RESERVE2, ref o); }
        #endregion

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
                    for (int n = 0; n < resData.Length;)
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

        private ErrCode SendCmdStream(CtrlCmd param, MemoryStream send, ref MemoryStream res)
        {
            return SendPipe(param, send, ref res);
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

    #endregion
    #region // "CtrlCmdDef.cs"からコピペ

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

    #endregion
}

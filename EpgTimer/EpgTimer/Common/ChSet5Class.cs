using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Windows.Media.Imaging;

namespace EpgTimer
{
    class ChSet5
    {
        public Dictionary<ulong, ChSet5Item> ChList
        {
            get;
            private set;
        }

        public List<ChSet5Item> ChListOrderByIndex
        {
            get;
            private set;
        }

        public IEnumerable<ChSet5Item> ChListSelected
        {
            get
            {
                IEnumerable<ChSet5Item> ret = ChListOrderByIndex;
                if (Settings.Instance.ShowEpgCapServiceOnly)
                {
                    ret = ret.Where(item => item.EpgCapFlag);
                }
                if (Settings.Instance.SortServiceList)
                {
                    //ネットワーク種別優先かつ限定受信を分離したID順ソート。BSはなるべくSID順
                    var bsmin = new Dictionary<ushort, ushort>();
                    foreach (ChSet5Item item in ret)
                    {
                        if (IsBS(item.ONID) && (bsmin.ContainsKey(item.TSID) == false || bsmin[item.TSID] > item.SID))
                        {
                            bsmin[item.TSID] = item.SID;
                        }
                    }
                    ret = ret.OrderBy(item => (
                        (ulong)(IsDttv(item.ONID) ? 0 : IsBS(item.ONID) ? 1 : IsCS(item.ONID) ? 2 : 3) << 56 |
                        (ulong)(IsDttv(item.ONID) && item.PartialFlag ? 1 : 0) << 48 |
                        CommonManager.Create64Key(item.ONID, (IsBS(item.ONID) ? bsmin[item.TSID] : item.TSID), item.SID)));
                }
                return ret;
            }
        }
        
        private static ChSet5 _instance;
        public static ChSet5 Instance
        {
            get
            {
                if (_instance == null)
                    _instance = new ChSet5();
                return _instance;
            }
        }

        public ChSet5()
        {
            ChList = new Dictionary<ulong, ChSet5Item>();
            ChListOrderByIndex = new List<ChSet5Item>();
        }

        public static bool IsVideo(ushort ServiceType)
        {
            return ServiceType == 0x01 || ServiceType == 0xA5 || ServiceType == 0xAD;
        }
        public static bool IsDttv(ushort ONID)
        {
            return 0x7880 <= ONID && ONID <= 0x7FE8;
        }
        public static bool IsBS(ushort ONID)
        {
            return ONID == 0x0004;
        }
        public static bool IsCS(ushort ONID)
        {
            return IsCS1(ONID) || IsCS2(ONID) || IsCS3(ONID);
        }
        public static bool IsCS1(ushort ONID)
        {
            return ONID == 0x0006;
        }
        public static bool IsCS2(ushort ONID)
        {
            return ONID == 0x0007;
        }
        public static bool IsCS3(ushort ONID)
        {
            return ONID == 0x000A;
        }
        public static bool IsOther(ushort ONID)
        {
            return IsDttv(ONID) == false && IsBS(ONID) == false && IsCS(ONID) == false;
        }

        public static bool LoadWithStreamReader(Stream stream)
        {
            Encoding enc;
            try
            {
                enc = Encoding.GetEncoding(932);
            }
            catch
            {
                enc = Encoding.UTF8;
            }

            try
            {
                Instance.ChList.Clear();
                Instance.ChListOrderByIndex.Clear();
                using (var reader = new StreamReader(stream, enc))
                {
                    for (string buff = reader.ReadLine(); buff != null; buff = reader.ReadLine())
                    {
                        if (buff.StartsWith(";", StringComparison.Ordinal))
                        {
                            //コメント行
                            continue;
                        }
                        string[] list = buff.Split('\t');
                        ChSet5Item item = new ChSet5Item();
                        try
                        {
                            item.ServiceName = list[0];
                            item.NetworkName = list[1];
                            item.ONID = Convert.ToUInt16(list[2]);
                            item.TSID = Convert.ToUInt16(list[3]);
                            item.SID = Convert.ToUInt16(list[4]);
                            item.ServiceType = Convert.ToUInt16(list[5]);
                            item.PartialFlag = Convert.ToInt32(list[6]) != 0;
                            item.EpgCapFlag = Convert.ToInt32(list[7]) != 0;
                            item.SearchFlag = Convert.ToInt32(list[8]) != 0;
                            //リモコンIDのフィールドは必ずしも存在しない
                            item.RemoconID = list.Length < 10 ? (byte)0 : Convert.ToByte(list[9]);
                        }
                        catch
                        {
                            //不正
                            continue;
                        }
                        if (Instance.ChList.ContainsKey(item.Key) == false)
                        {
                            Instance.ChList[item.Key] = item;
                            Instance.ChListOrderByIndex.Add(item);
                        }
                    }
                }
            }
            catch
            {
                return false;
            }
            return true;
        }
    }

    public class ChSet5Item
    {
        public ChSet5Item()
        {
        }
        public ulong Key
        {
            get
            {
                return CommonManager.Create64Key(ONID, TSID, SID);
            }
        }
        public ushort ONID { get; set; }
        public ushort TSID { get; set; }
        public ushort SID { get; set; }
        public ushort ServiceType { get; set; }
        public bool PartialFlag { get; set; }
        public string ServiceName { get; set; }
        public string NetworkName { get; set; }
        public bool EpgCapFlag { get; set; }
        public bool SearchFlag { get; set; }
        public byte RemoconID { get; set; }
        public BitmapSource Logo { get; set; }

        public override string ToString()
        {
            return ServiceName;
        }
    }
}

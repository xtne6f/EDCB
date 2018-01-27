using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    class ChSet5
    {
        public Dictionary<UInt64, ChSet5Item> ChList
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
                    //ネットワーク種別優先かつ限定受信を分離したID順ソート
                    ret = ret.OrderBy(item => (
                        (ulong)(IsDttv(item.ONID) ? 0 : IsBS(item.ONID) ? 1 : IsCS(item.ONID) ? 2 : 3) << 56 |
                        (ulong)(IsDttv(item.ONID) && item.PartialFlag ? 1 : 0) << 48 |
                        item.Key));
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
            ChList = new Dictionary<UInt64, ChSet5Item>();
            ChListOrderByIndex = new List<ChSet5Item>();
        }

        public static bool IsVideo(UInt16 ServiceType)
        {
            return ServiceType == 0x01 || ServiceType == 0xA5 || ServiceType == 0xAD;
        }
        public static bool IsDttv(UInt16 ONID)
        {
            return 0x7880 <= ONID && ONID <= 0x7FE8;
        }
        public static bool IsBS(UInt16 ONID)
        {
            return ONID == 0x0004;
        }
        public static bool IsCS(UInt16 ONID)
        {
            return IsCS1(ONID) || IsCS2(ONID) || IsCS3(ONID);
        }
        public static bool IsCS1(UInt16 ONID)
        {
            return ONID == 0x0006;
        }
        public static bool IsCS2(UInt16 ONID)
        {
            return ONID == 0x0007;
        }
        public static bool IsCS3(UInt16 ONID)
        {
            return ONID == 0x000A;
        }
        public static bool IsOther(UInt16 ONID)
        {
            return IsDttv(ONID) == false && IsBS(ONID) == false && IsCS(ONID) == false;
        }

        public static bool Load(System.IO.StreamReader reader)
        {
            try
            {
                Instance.ChList.Clear();
                Instance.ChListOrderByIndex.Clear();
                for (string buff = reader.ReadLine(); buff != null; buff = reader.ReadLine())
                {
                    if (buff.IndexOf(";") == 0)
                    {
                        //コメント行
                    }
                    else
                    {
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
        public UInt64 Key
        {
            get
            {
                UInt64 key = ((UInt64)ONID)<<32 | ((UInt64)TSID)<<16 | (UInt64)SID;
                return key;
            }
        }
        public UInt16 ONID
        {
            get;
            set;
        }
        public UInt16 TSID
        {
            get;
            set;
        }
        public UInt16 SID
        {
            get;
            set;
        }
        public UInt16 ServiceType
        {
            get;
            set;
        }
        public bool PartialFlag
        {
            get;
            set;
        }
        public String ServiceName
        {
            get;
            set;
        }
        public String NetworkName
        {
            get;
            set;
        }
        public bool EpgCapFlag
        {
            get;
            set;
        }
        public bool SearchFlag
        {
            get;
            set;
        }

        public override string ToString()
        {
            return ServiceName;
        }
    }
}

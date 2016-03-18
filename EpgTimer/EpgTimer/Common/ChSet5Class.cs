using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    class ChSet5
    {
        private Dictionary<UInt64, ChSet5Item> _chList = null;
        public Dictionary<UInt64, ChSet5Item> ChList
        {
            get
            {
                if (_chList == null) LoadFile();
                return _chList != null ? _chList : new Dictionary<UInt64, ChSet5Item>();
            }
            private set { _chList = value; }
        }
        public static void Clear() { Instance._chList = null; }
        
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

        public ChSet5() { }

        public static bool IsVideo(UInt16 ServiceType)
        {
            return ServiceType == 0x01 || ServiceType == 0xA5;
        }
        public static bool IsTere(UInt16 ONID)
        {
            return 0x7880 <= ONID && ONID <= 0x7FE8;
        }
        public static bool IsBS(UInt16 ONID)
        {
            return ONID == 0x0004;
        }
        public static bool IsCS(UInt16 ONID)
        {
            return IsCS1(ONID) || IsCS2(ONID);
        }
        public static bool IsCS1(UInt16 ONID)
        {
            return ONID == 0x0006;
        }
        public static bool IsCS2(UInt16 ONID)
        {
            return ONID == 0x0007;
        }
        public static bool IsOther(UInt16 ONID)
        {
            return IsTere(ONID) == false && IsBS(ONID) == false && IsCS(ONID) == false;
        }

        public static bool LoadFile()
        {
            try
            {
                using (var sr = new System.IO.StreamReader(SettingPath.SettingFolderPath + "\\ChSet5.txt", Encoding.Default))
                {
                    return ChSet5.Load(sr);
                }
            }
            catch { }
            return false;
        }
        public static bool Load(System.IO.StreamReader reader)
        {
            try
            {
                Instance._chList = new Dictionary<UInt64, ChSet5Item>();
                while (reader.Peek() >= 0)
                {
                    string buff = reader.ReadLine();
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
                            item.PartialFlag = Convert.ToByte(list[6]);
                            item.EpgCapFlag = Convert.ToByte(list[7]);
                            item.SearchFlag = Convert.ToByte(list[8]);
                        }
                        finally
                        {
                            UInt64 key = item.Key;
                            Instance._chList.Add(key, item);
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
        public static bool SaveFile()
        {
            try
            {
                String filePath = SettingPath.SettingFolderPath + "\\ChSet5.txt";
                System.IO.StreamWriter writer = (new System.IO.StreamWriter(filePath, false, System.Text.Encoding.Default));
                if (Instance._chList != null)
                {
                    foreach (ChSet5Item info in Instance._chList.Values)
                    {
                        writer.WriteLine("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\t{7}\t{8}",
                            info.ServiceName,
                            info.NetworkName,
                            info.ONID,
                            info.TSID,
                            info.SID,
                            info.ServiceType,
                            info.PartialFlag,
                            info.EpgCapFlag,
                            info.SearchFlag);
                    }
                }
                writer.Close();
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
        public ChSet5Item() { }

        public UInt64 Key { get { return CommonManager.Create64Key(ONID, TSID, SID); } }
        public UInt16 ONID { get; set; }
        public UInt16 TSID { get; set; }
        public UInt16 SID { get; set; }
        public UInt16 ServiceType { get; set; }
        public Byte PartialFlag { get; set; }
        public String ServiceName { get; set; }
        public String NetworkName { get; set; }
        public Byte EpgCapFlag { get; set; }
        public Byte SearchFlag { get; set; }
        public Byte RemoconID { get; set; }

        public bool IsVideo { get { return ChSet5.IsVideo(ServiceType); } }
        public bool IsTere { get { return ChSet5.IsTere(ONID); } }
        public bool IsBS { get { return ChSet5.IsBS(ONID); } }
        public bool IsCS { get { return ChSet5.IsCS(ONID); } }
        public bool IsCS1 { get { return ChSet5.IsCS1(ONID); } }
        public bool IsCS2 { get { return ChSet5.IsCS2(ONID); } }
        public bool IsOther { get { return ChSet5.IsOther(ONID); } }

        public override string ToString()
        {
            return ServiceName;
        }
    }
}

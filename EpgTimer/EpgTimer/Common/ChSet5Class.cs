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
        }

        public static bool IsVideo(UInt16 ServiceType)
        {
            return ServiceType == 0x01 || ServiceType == 0xA5 || ServiceType == 0xAD;
        }

        public static bool Load(System.IO.StreamReader reader)
        {
            try
            {
                Instance.ChList.Clear();
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
                            UInt64 key = ((UInt64)item.ONID) << 32 | ((UInt64)item.TSID) << 16 | ((UInt64)item.SID);
                            Instance.ChList.Add(key, item);
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
                if (Instance.ChList != null)
                {
                    foreach (ChSet5Item info in Instance.ChList.Values)
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
        public Byte PartialFlag
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
        public Byte EpgCapFlag
        {
            get;
            set;
        }
        public Byte SearchFlag
        {
            get;
            set;
        }
        public Byte RemoconID
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

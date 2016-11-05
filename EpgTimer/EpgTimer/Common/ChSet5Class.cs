using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;

namespace EpgTimer
{
    static class ChSet5
    {
        private static Dictionary<UInt64, ChSet5Item> chList = null;
        public static Dictionary<UInt64, ChSet5Item> ChList
        {
            get
            {
                if (chList == null) LoadFile();
                return chList ?? new Dictionary<UInt64, ChSet5Item>();
            }
        }
        public static void Clear() { chList = null; }
        
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
        public static bool IsSkyPerfectv(UInt16 ONID)
        {
            return ONID == 0x000A;
        }
        public static bool IsOther(UInt16 ONID)
        {
            return IsDttv(ONID) == false && IsBS(ONID) == false && IsCS(ONID) == false;
        }

        public static bool LoadFile()
        {
            try
            {
                using (var sr = new StreamReader(SettingPath.SettingFolderPath + "\\ChSet5.txt", Encoding.Default))
                {
                    return ChSet5.Load(sr);
                }
            }
            catch { }
            return false;
        }
        public static bool Load(StreamReader reader)
        {
            try
            {
                chList = new Dictionary<UInt64, ChSet5Item>();
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
                        var item = new ChSet5Item();
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
                            chList.Add(item.Key, item);
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
                if (chList == null) return false;
                //
                using (var writer = new StreamWriter(SettingPath.SettingFolderPath + "\\ChSet5.txt", false, Encoding.Default))
                {
                    foreach (ChSet5Item info in chList.Values)
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
        public bool IsDttv { get { return ChSet5.IsDttv(ONID); } }
        public bool IsBS { get { return ChSet5.IsBS(ONID); } }
        public bool IsCS { get { return ChSet5.IsCS(ONID); } }
        public bool IsCS1 { get { return ChSet5.IsCS1(ONID); } }
        public bool IsCS2 { get { return ChSet5.IsCS2(ONID); } }
        public bool IsSkyPerfectv { get { return ChSet5.IsSkyPerfectv(ONID); } }
        public bool IsOther { get { return ChSet5.IsOther(ONID); } }

        public EpgServiceInfo ToInfo()
        {
            return new EpgServiceInfo
            {
                ONID = this.ONID,
                TSID = this.TSID,
                SID = this.SID,
                network_name = this.NetworkName,
                partialReceptionFlag = this.PartialFlag,
                remote_control_key_id = this.RemoconID,
                service_name = this.ServiceName,
                service_provider_name = this.NetworkName,
                service_type = (byte)this.ServiceType,
                ts_name = this.NetworkName
            };
        }
        public override string ToString()
        {
            return ServiceName;
        }
    }
}

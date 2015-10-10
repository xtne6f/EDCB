using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    class IEPGFileClass
    {
        public ReserveData AddInfo = new ReserveData();
        public bool LoadTVPIDFile(String filePath)
        {
            bool ret = false;
            System.IO.StreamReader reader = null;
            try
            {
                Dictionary<String, String> paramList = new Dictionary<string, string>();
                reader = (new System.IO.StreamReader(filePath, System.Text.Encoding.Default));
                while (reader.Peek() >= 0)
                {
                    string buff = reader.ReadLine();
                    string[] value = buff.Split(':');
                    if (value.Length > 1)
                    {
                        if (value.Length == 2)
                        {
                            paramList.Add(value[0], value[1].Trim());
                        }
                        else
                        {
                            string val = "";
                            for (int i = 1; i < value.Length; i++)
                            {
                                val += value[i];
                                if (i + 1 < value.Length)
                                {
                                    val += ":";
                                }
                            }
                            paramList.Add(value[0], val.Trim());
                        }
                    }
                }

                if (String.Compare(paramList["version"], "2", true) == 0)
                {
                    bool find = false;
                    if (paramList["station"].IndexOf("DFS") == 0)
                    {
                        UInt16 sid = Convert.ToUInt16(paramList["station"].Substring(3), 16);
                        foreach (ChSet5Item info in ChSet5.Instance.ChList.Values)
                        {
                            if (0x7880 <= info.ONID && info.ONID <= 0x7FE8 && info.SID == sid)
                            {
                                AddInfo.OriginalNetworkID = info.ONID;
                                AddInfo.TransportStreamID = info.TSID;
                                AddInfo.ServiceID = info.SID;
                                AddInfo.StationName = info.ServiceName;

                                AddInfo.EventID = Convert.ToUInt16(paramList["program-id"]);

                                find = true;
                                break;
                            }
                        }
                    }
                    else if (paramList["station"].IndexOf("BSDT") == 0)
                    {
                        UInt16 sid = Convert.ToUInt16(paramList["station"].Substring(4));
                        foreach (ChSet5Item info in ChSet5.Instance.ChList.Values)
                        {
                            if (info.ONID == 4 && info.SID == sid)
                            {
                                AddInfo.OriginalNetworkID = info.ONID;
                                AddInfo.TransportStreamID = info.TSID;
                                AddInfo.ServiceID = info.SID;
                                AddInfo.StationName = info.ServiceName;

                                AddInfo.EventID = Convert.ToUInt16(paramList["program-id"]);

                                find = true;
                                break;
                            }
                        }
                    }
                    else if (paramList["station"].IndexOf("CSDT") == 0)
                    {
                        UInt16 sid = Convert.ToUInt16(paramList["station"].Substring(4));
                        foreach (ChSet5Item info in ChSet5.Instance.ChList.Values)
                        {
                            if ((info.ONID == 6 || info.ONID == 7) && info.SID == sid)
                            {
                                AddInfo.OriginalNetworkID = info.ONID;
                                AddInfo.TransportStreamID = info.TSID;
                                AddInfo.ServiceID = info.SID;
                                AddInfo.StationName = info.ServiceName;

                                AddInfo.EventID = Convert.ToUInt16(paramList["program-id"]);

                                find = true;
                                break;
                            }
                        }
                    }
                    else if (paramList["station"].IndexOf("DOS") == 0)
                    {
                        UInt16 sid = Convert.ToUInt16(paramList["station"].Substring(3), 16);
                        foreach (ChSet5Item info in ChSet5.Instance.ChList.Values)
                        {
                            if (0x7880 <= info.ONID && info.ONID <= 0x7FE8 && info.SID == sid)
                            {
                                AddInfo.OriginalNetworkID = info.ONID;
                                AddInfo.TransportStreamID = info.TSID;
                                AddInfo.ServiceID = info.SID;
                                AddInfo.StationName = info.ServiceName;

                                AddInfo.EventID = 0xFFFF;

                                find = true;
                                break;
                            }
                        }
                    }
                    if (find == false)
                    {
                        throw new Exception();
                    }
                    AddInfo.Title = paramList["program-title"];

                    int yy = Convert.ToInt32(paramList["year"]);
                    int mm = Convert.ToInt32(paramList["month"]);
                    int dd = Convert.ToInt32(paramList["date"]);
                    string[] startTime = paramList["start"].Split(':');
                    int startHH = Convert.ToInt32(startTime[0]);
                    int startMM = Convert.ToInt32(startTime[1]);
                    string[] endTime = paramList["end"].Split(':');
                    int endHH = Convert.ToInt32(endTime[0]);
                    int endMM = Convert.ToInt32(endTime[1]);

                    DateTime start = new DateTime(yy, mm, dd, startHH, startMM, 0, DateTimeKind.Utc);
                    DateTime end = new DateTime(yy, mm, dd, endHH, endMM, 0, DateTimeKind.Utc);
                    if (end < start)
                    {
                        end = end.AddDays(1);
                    }

                    AddInfo.StartTime = start;
                    AddInfo.StartTimeEpg = start;
                    AddInfo.DurationSecond = (uint)((end - start).TotalSeconds);

                    RecSettingData recSet = new RecSettingData();
                    Settings.GetDefRecSetting(0, ref recSet);
                    AddInfo.RecSetting = recSet;

                    ret = true;
                }
            }
            catch
            {
            }
            if (reader != null)
            {
                reader.Close();
            }
            return ret;
        }

        public bool LoadTVPIFile(String filePath)
        {
            bool ret = false;
            System.IO.StreamReader reader = null;
            try
            {
                Dictionary<String, String> paramList = new Dictionary<string, string>();
                reader = (new System.IO.StreamReader(filePath, System.Text.Encoding.Default));
                while (reader.Peek() >= 0)
                {
                    string buff = reader.ReadLine();
                    string[] value = buff.Split(':');
                    if (value.Length > 1)
                    {
                        if (value.Length == 2)
                        {
                            paramList.Add(value[0], value[1].Trim());
                        }
                        else
                        {
                            string val = "";
                            for (int i = 1; i < value.Length; i++)
                            {
                                val += value[i];
                                if (i + 1 < value.Length)
                                {
                                    val += ":";
                                }
                            }
                            paramList.Add(value[0], val.Trim());
                        }
                    }
                }

                if (String.Compare(paramList["version"], "1", true) == 0)
                {
                    bool find = false;
                    foreach (IEPGStationInfo info in Settings.Instance.IEpgStationList)
                    {
                        if( String.Compare(info.StationName, paramList["station"]) == 0){
                            if (ChSet5.Instance.ChList.ContainsKey(info.Key) == true)
                            {
                                AddInfo.OriginalNetworkID = ChSet5.Instance.ChList[info.Key].ONID;
                                AddInfo.TransportStreamID = ChSet5.Instance.ChList[info.Key].TSID;
                                AddInfo.ServiceID = ChSet5.Instance.ChList[info.Key].SID;
                                AddInfo.StationName = ChSet5.Instance.ChList[info.Key].ServiceName;

                                AddInfo.EventID = 0xFFFF;
                                find = true;
                            }
                            break;
                        }
                    }

                    if (find == false)
                    {
                        throw new Exception();
                    }
                    AddInfo.Title = paramList["program-title"];

                    int yy = Convert.ToInt32(paramList["year"]);
                    int mm = Convert.ToInt32(paramList["month"]);
                    int dd = Convert.ToInt32(paramList["date"]);
                    string[] startTime = paramList["start"].Split(':');
                    int startHH = Convert.ToInt32(startTime[0]);
                    int startMM = Convert.ToInt32(startTime[1]);
                    string[] endTime = paramList["end"].Split(':');
                    int endHH = Convert.ToInt32(endTime[0]);
                    int endMM = Convert.ToInt32(endTime[1]);

                    DateTime start = new DateTime(yy, mm, dd, startHH, startMM, 0, DateTimeKind.Utc);
                    DateTime end = new DateTime(yy, mm, dd, endHH, endMM, 0, DateTimeKind.Utc);
                    if (end < start)
                    {
                        end = end.AddDays(1);
                    }

                    AddInfo.StartTime = start;
                    AddInfo.StartTimeEpg = start;
                    AddInfo.DurationSecond = (uint)((end - start).TotalSeconds);

                    RecSettingData recSet = new RecSettingData();
                    Settings.GetDefRecSetting(0, ref recSet);
                    AddInfo.RecSetting = recSet;
                    AddInfo.RecSetting.TuijyuuFlag = 0;
                    AddInfo.RecSetting.PartialRecFlag = 0;

                    ret = true;
                }
            }
            catch
            {
            }
            if (reader != null)
            {
                reader.Close();
            }
            return ret;
        }
    }
}

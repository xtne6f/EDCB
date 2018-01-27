using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    static class IEPGFileClass
    {
        public static ReserveData TryLoadTVPID(string filePath, IDictionary<ulong, ChSet5Item> chList)
        {
            Dictionary<string, string> paramList = TryLoadParamList(filePath);
            if (paramList != null &&
                paramList.ContainsKey("station") &&
                paramList.ContainsKey("version") &&
                paramList["version"] == "2")
            {
                // 放送種別とサービスID
                string station = paramList["station"];
                foreach (ChSet5Item info in chList.Values)
                {
                    ushort sid = 0;
                    if (ChSet5.IsDttv(info.ONID) &&
                        (station.StartsWith("DFS", StringComparison.Ordinal) || station.StartsWith("DOS", StringComparison.Ordinal)))
                    {
                        ushort.TryParse(station.Substring(3), NumberStyles.HexNumber, null, out sid);
                    }
                    else if (ChSet5.IsBS(info.ONID) && station.StartsWith("BSDT", StringComparison.Ordinal) ||
                             ChSet5.IsCS(info.ONID) && station.StartsWith("CSDT", StringComparison.Ordinal))
                    {
                        ushort.TryParse(station.Substring(4), out sid);
                    }
                    if (sid != 0 && sid == info.SID)
                    {
                        var addInfo = new ReserveData();
                        addInfo.OriginalNetworkID = info.ONID;
                        addInfo.TransportStreamID = info.TSID;
                        addInfo.ServiceID = info.SID;
                        addInfo.StationName = info.ServiceName;
                        // 開始時間と長さ
                        if (GetTimeValues(paramList, addInfo))
                        {
                            // イベントID(オプション)。なければプログラム予約
                            addInfo.EventID = 0xFFFF;
                            ushort eventID;
                            if (paramList.ContainsKey("program-id") && ushort.TryParse(paramList["program-id"], out eventID))
                            {
                                addInfo.EventID = eventID;
                            }
                            // 番組名(オプション)
                            if (paramList.ContainsKey("program-title"))
                            {
                                addInfo.Title = paramList["program-title"];
                            }
                            return addInfo;
                        }
                        break;
                    }
                }
            }
            return null;
        }

        public static ReserveData TryLoadTVPI(string filePath, IDictionary<ulong, ChSet5Item> chList, IList<IEPGStationInfo> stationList)
        {
            Dictionary<string, string> paramList = TryLoadParamList(filePath);
            if (paramList != null &&
                paramList.ContainsKey("station") &&
                paramList.ContainsKey("version") &&
                paramList["version"] == "1")
            {
                // サービス名からサービスIDを探す
                string station = paramList["station"];
                var ci = new CultureInfo("ja-JP");
                foreach (IEPGStationInfo staInfo in stationList)
                {
                    if (string.Compare(staInfo.StationName, station, ci, CompareOptions.IgnoreWidth | CompareOptions.IgnoreCase) == 0)
                    {
                        ChSet5Item info;
                        if (chList.TryGetValue(staInfo.Key, out info))
                        {
                            var addInfo = new ReserveData();
                            addInfo.OriginalNetworkID = info.ONID;
                            addInfo.TransportStreamID = info.TSID;
                            addInfo.ServiceID = info.SID;
                            addInfo.StationName = info.ServiceName;
                            // 開始時間と長さ
                            if (GetTimeValues(paramList, addInfo))
                            {
                                // 常にプログラム予約
                                addInfo.EventID = 0xFFFF;
                                // 番組名(オプション)
                                if (paramList.ContainsKey("program-title"))
                                {
                                    addInfo.Title = paramList["program-title"];
                                }
                                return addInfo;
                            }
                            break;
                        }
                    }
                }
            }
            return null;
        }

        private static Dictionary<string, string> TryLoadParamList(string filePath)
        {
            try
            {
                using (var reader = new System.IO.StreamReader(filePath, Encoding.GetEncoding(932)))
                {
                    var paramList = new Dictionary<string, string>();
                    // 空行まで読む
                    for (string buff = reader.ReadLine(); buff != null && buff.Length > 0; buff = reader.ReadLine())
                    {
                        int sepIndex = buff.IndexOf(':');
                        if (sepIndex >= 0)
                        {
                            paramList[buff.Remove(sepIndex).Trim()] = buff.Substring(sepIndex + 1).Trim();
                        }
                    }
                    return paramList;
                }
            }
            catch
            {
                return null;
            }
        }

        private static bool GetTimeValues(Dictionary<string, string> paramList, ReserveData addInfo)
        {
            if (paramList.ContainsKey("year") &&
                paramList.ContainsKey("month") &&
                paramList.ContainsKey("date") &&
                paramList.ContainsKey("start") &&
                paramList.ContainsKey("end") &&
                paramList["start"].Split(':').Length >= 2 &&
                paramList["end"].Split(':').Length >= 2)
            {
                ushort year, month, date, hour, min, endHour, endMin;
                ushort.TryParse(paramList["year"], out year);
                ushort.TryParse(paramList["month"], out month);
                ushort.TryParse(paramList["date"], out date);
                ushort.TryParse(paramList["start"].Split(':')[0], out hour);
                ushort.TryParse(paramList["start"].Split(':')[1], out min);
                ushort.TryParse(paramList["end"].Split(':')[0], out endHour);
                ushort.TryParse(paramList["end"].Split(':')[1], out endMin);
                if (1900 < year && year < 3000 && 1 <= month && month <= 12 && 1 <= date && date <= 31 &&
                    hour < 30 && min < 60 && endHour < 30 && endMin < 60 && hour * 60 + min < (endHour + 24) * 60 + endMin)
                {
                    // 存在しない日付は翌月
                    if (date > DateTime.DaysInMonth(year, month))
                    {
                        date -= (ushort)DateTime.DaysInMonth(year, month++);
                    }
                    // 24時以上は翌日
                    var startTime = new DateTime(year, month, date, hour % 24, min, 0).AddDays(hour / 24);
                    var endTime = new DateTime(year, month, date, endHour % 24, endMin, 0).AddDays(endHour / 24);
                    // 開始時間より小さいときは翌日
                    if (endTime < startTime)
                    {
                        endTime = endTime.AddDays(1);
                    }
                    addInfo.StartTime = startTime;
                    addInfo.StartTimeEpg = startTime;
                    addInfo.DurationSecond = (uint)(endTime - startTime).TotalSeconds;
                    return true;
                }
            }
            return false;
        }
    }
}

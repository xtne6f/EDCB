using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public interface IRecSetttingData
    {
        RecSettingData RecSettingInfo { get; }
    }

    public static class RecSettingDataEx
    {
        public static List<RecSettingData> RecSettingList(this IEnumerable<IRecSetttingData> list)
        {
            return list.Where(item => item != null).Select(item => item.RecSettingInfo).ToList();
        }

        public static RecPresetItem LookUpPreset(this RecSettingData data, bool IsManual = false, bool CopyData = false)
        {
            RecPresetItem preset = Settings.Instance.RecPresetList.FirstOrDefault(p1 =>
            {
                return p1.RecPresetData.EqualsSettingTo(data, IsManual);
            });
            return preset == null ? new RecPresetItem("登録時", 0xFFFFFFFF, CopyData == true ? data.Clone() : null) : preset;
        }

        public static List<string> GetRecFolderViewList(this RecSettingData recSetting)
        {
            var list = new List<string>();
            List<string> defs = Settings.Instance.DefRecFolders;
            string def1 = defs.Count == 0 ? "!Default" : defs[0];
            Func<string, string> AdjustName = (f => f == "!Default" ? def1 : f);
            if (recSetting != null)
            {
                recSetting.RecFolderList.ForEach(info => list.Add(AdjustName(info.RecFolder)));
                recSetting.PartialRecFolder.ForEach(info => list.Add("(ワンセグ) " + AdjustName(info.RecFolder)));
            }
            return list;
        }

        public static string GetTrueMarginText(this RecSettingData recSetting, bool start)
        {
            return CustomTimeFormat(recSetting.GetTrueMargin(start) * (start ? -1 : 1), recSetting.UseMargineFlag);
        }

        public static int GetTrueMargin(this RecSettingData recSetting, bool start)
        {
            if (recSetting == null) return 0;

            int marginTime;
            if (recSetting.UseMargineFlag == 1)
            {
                marginTime = start ? recSetting.StartMargine : recSetting.EndMargine;
            }
            else
            {
                marginTime = start ? Settings.Instance.DefStartMargin : Settings.Instance.DefEndMargin;
            }
            return marginTime;
        }

        public static double GetTrueMarginForSort(this RecSettingData recSetting, bool start)
        {
            if (recSetting == null) return 0;
            //
            return recSetting.GetTrueMargin(start) * (start ? -1 : 1) + (recSetting.UseMargineFlag == 1 ? 0.1 : 0);
        }

        private static string CustomTimeFormat(int span, byte useMarginFlag)
        {
            string hours;
            string minutes;
            string seconds = (span % 60).ToString("00;00");
            if (Math.Abs(span) < 3600)
            {
                hours = "";
                minutes = (span / 60).ToString("0;0") + ":";
            }
            else
            {
                hours = (span / 3600).ToString("0;0") + ":";
                minutes = ((span % 3600) / 60).ToString("00;00") + ":";
            }
            return span.ToString("+;-") + hours + minutes + seconds + (useMarginFlag == 1 ? " " : "*");
        }

        public static List<RecSettingData> Clone(this IEnumerable<RecSettingData> src) { return CopyObj.Clone(src, CopyData); }
        public static RecSettingData Clone(this RecSettingData src) { return CopyObj.Clone(src, CopyData); }
        public static void CopyTo(this RecSettingData src, RecSettingData dest) { CopyObj.CopyTo(src, dest, CopyData); }
        private static void CopyData(RecSettingData src, RecSettingData dest)
        {
            dest.BatFilePath = src.BatFilePath;
            dest.ContinueRecFlag = src.ContinueRecFlag;
            dest.EndMargine = src.EndMargine;
            dest.PartialRecFlag = src.PartialRecFlag;
            dest.PartialRecFolder = src.PartialRecFolder.Clone();   //RecFileSetInfo
            dest.PittariFlag = src.PittariFlag;
            dest.Priority = src.Priority;
            dest.RebootFlag = src.RebootFlag;
            dest.RecFolderList = src.RecFolderList.Clone();         //RecFileSetInfo
            dest.RecMode = src.RecMode;
            dest.ServiceMode = src.ServiceMode;
            dest.StartMargine = src.StartMargine;
            dest.SuspendMode = src.SuspendMode;
            dest.TuijyuuFlag = src.TuijyuuFlag;
            dest.TunerID = src.TunerID;
            dest.UseMargineFlag = src.UseMargineFlag;
        }
        /*
        public static bool EqualsTo(this IList<RecSettingData> src, IList<RecSettingData> dest) { return CopyObj.EqualsTo(src, dest, EqualsValue); }
        public static bool EqualsTo(this RecSettingData src, RecSettingData dest) { return CopyObj.EqualsTo(src, dest, EqualsValue); }
        public static bool EqualsValue(RecSettingData src, RecSettingData dest)
        {
            return src.BatFilePath == dest.BatFilePath
                && src.ContinueRecFlag == dest.ContinueRecFlag
                && src.EndMargine == dest.EndMargine
                && src.PartialRecFlag == dest.PartialRecFlag
                && src.PartialRecFolder.EqualsTo(dest.PartialRecFolder) //RecFileSetInfo
                && src.PittariFlag == dest.PittariFlag
                && src.Priority == dest.Priority
                && src.RebootFlag == dest.RebootFlag
                && src.RecFolderList.EqualsTo(dest.RecFolderList)       //RecFileSetInfo
                && src.RecMode == dest.RecMode
                && src.ServiceMode == dest.ServiceMode
                && src.StartMargine == dest.StartMargine
                && src.SuspendMode == dest.SuspendMode
                && src.TuijyuuFlag == dest.TuijyuuFlag
                && src.TunerID == dest.TunerID
                && src.UseMargineFlag == dest.UseMargineFlag;
        }
        */
        public static bool EqualsSettingTo(this RecSettingData src, RecSettingData dest, bool IsManual = false)
        {
            if (src == null || dest == null) return false;
            return src.BatFilePath == dest.BatFilePath
                && src.ContinueRecFlag == dest.ContinueRecFlag
                && (src.EndMargine == dest.EndMargine || src.UseMargineFlag == 0)//マージンデフォルト時
                && src.PartialRecFlag == dest.PartialRecFlag
                && src.PartialRecFolder.EqualsTo(dest.PartialRecFolder)
                && (src.PittariFlag == dest.PittariFlag || IsManual == true)//プログラム予約時
                && src.Priority == dest.Priority
                && (src.RebootFlag == dest.RebootFlag || src.SuspendMode == 0)//動作後設定デフォルト時
                && src.RecFolderList.EqualsTo(dest.RecFolderList)
                && (src.RecMode == dest.RecMode || src.RecMode == 5 || dest.RecMode == 5)
                && (src.ServiceMode == dest.ServiceMode || ((src.ServiceMode | dest.ServiceMode) & 0x0F) == 0)//字幕等データ設定デフォルト時
                && (src.StartMargine == dest.StartMargine || src.UseMargineFlag == 0)//マージンデフォルト時
                && src.SuspendMode == dest.SuspendMode//動作後設定
                && (src.TuijyuuFlag == dest.TuijyuuFlag || IsManual == true)//プログラム予約時
                && src.TunerID == dest.TunerID
                && src.UseMargineFlag == dest.UseMargineFlag;
        }

    }
}

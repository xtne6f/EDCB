using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public static class RecSettingDataEx
    {
        public static RecPresetItem LookUpPreset(this RecSettingData data, bool IsManual = false)
        {
            RecPresetItem preset = Settings.Instance.RecPresetList.FirstOrDefault(p1 =>
            {
                var pdata = new RecSettingData();
                Settings.GetDefRecSetting(p1.ID, ref pdata);
                return pdata.EqualsSettingTo(data, IsManual);
            });
            return preset == null ? new RecPresetItem("登録時", 0xFFFFFFFF) : preset;
        }

        public static List<RecSettingData> Clone(this List<RecSettingData> src) { return CopyObj.Clone(src, CopyData); }
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

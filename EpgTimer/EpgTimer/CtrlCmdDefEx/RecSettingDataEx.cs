using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public interface IRecSetttingData
    {
        RecSettingData RecSettingInfo { get; }
        bool IsManual { get; }
    }

    public partial class RecSettingData
    {
        public RecPresetItem LookUpPreset(bool IsManual = false, bool CopyData = false)
        {
            return LookUpPreset(Settings.Instance.RecPresetList, IsManual, CopyData);
        }
        public RecPresetItem LookUpPreset(IEnumerable<RecPresetItem> refdata, bool IsManual = false, bool CopyData = false)
        {
            RecPresetItem preset = refdata.FirstOrDefault(p1 =>
            {
                return p1.RecPresetData.EqualsSettingTo(this, IsManual);
            });
            return preset == null ? new RecPresetItem("登録時", RecPresetItem.CustomID, CopyData == true ? this.Clone() : null) : preset;
        }

        public List<string> RecFolderViewList
        {
            get
            {
                var list = new List<string>();
                List<string> defs = Settings.Instance.DefRecFolders;
                string def1 = defs.Count == 0 ? "!Default" : defs[0];
                Func<string, string> AdjustName = (f => f == "!Default" ? def1 : f);

                this.RecFolderList.ForEach(info => list.Add(AdjustName(info.RecFolder)));
                this.PartialRecFolder.ForEach(info => list.Add("(ワンセグ) " + AdjustName(info.RecFolder)));

                return list;
            }
        }

        //真のマージン値
        public int StartMarginActual
        {
            get { return UseMargineFlag != 0 ? StartMargine : Settings.Instance.DefStartMargin; }
        }
        public int EndMarginActual
        {
            get { return UseMargineFlag != 0 ? EndMargine : Settings.Instance.DefEndMargin; }
        }

        //指定サービス対象モードの補助
        public bool ServiceModeIsDefault
        {
            get { return (ServiceMode & 0x0Fu) == 0; }
            set { ServiceMode = (ServiceMode & ~0x0Fu) | (value == true ? 0x00u : 0x01u); }
        }
        public bool ServiceCaption
        {
            get { return (ServiceMode & 0x10u) != 0; }
            set { ServiceMode = (ServiceMode & ~0x10u) | (value == true ? 0x10u : 0x00u); }
        }
        public bool ServiceData
        {
            get { return (ServiceMode & 0x20u) != 0; }
            set { ServiceMode = (ServiceMode & ~0x20u) | (value == true ? 0x20u : 0x00u); }
        }
        public bool ServiceCaptionActual
        {
            get { return ServiceModeIsDefault == false ? ServiceCaption : IniFileHandler.GetPrivateProfileInt("SET", "Caption", 1, SettingPath.EdcbIniPath) != 0; }
        }
        public bool ServiceDataActual
        {
            get { return ServiceModeIsDefault == false ? ServiceData : IniFileHandler.GetPrivateProfileInt("SET", "Data", 0, SettingPath.EdcbIniPath) != 0; }
        }

        //録画後動作モードの補助。ToRecEndMode()はRecEndMode自体の範囲修正にも使用している。
        private static int ToRecEndMode(int val) { return (1 <= val && val <= 3) ? val : 0; }
        private static byte ToSuspendMode(int val) { return (byte)((1 <= val && val <= 3) ? val : 4); }
        public void SetSuspendMode(bool isDefault, int recEndMode = 0)
        {
            SuspendMode = (byte)(isDefault == true ? 0 : ToSuspendMode(recEndMode));
        }
        public int RecEndModeActual
        {
            get { return ToRecEndMode(SuspendMode != 0 ? SuspendMode : IniFileHandler.GetPrivateProfileInt("SET", "RecEndMode", 2, SettingPath.TimerSrvIniPath)); }
        }
        public byte RebootFlagActual
        {
            get { return SuspendMode != 0 ? RebootFlag : (byte)IniFileHandler.GetPrivateProfileInt("SET", "Reboot", 0, SettingPath.TimerSrvIniPath); }
        }
    }

    public static class RecSettingDataEx
    {
        public static List<RecSettingData> RecSettingList(this IEnumerable<IRecSetttingData> list)
        {
            return list.Where(item => item != null).Select(item => item.RecSettingInfo).ToList();
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

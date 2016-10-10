using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class RecPresetItem
    {
        public RecPresetItem() { }
        public RecPresetItem(string name, UInt32 id, RecSettingData data = null)
        { DisplayName = name; ID = id; recPresetData = data; }
        public String DisplayName { get; set; }
        public UInt32 ID { get; set; }
        public override string ToString() { return DisplayName; }

        public static List<RecPresetItem> Clone(IEnumerable<RecPresetItem> src) { return CopyObj.Clone(src, CopyData); }
        public RecPresetItem Clone() { return CopyObj.Clone(this, CopyData); }
        protected static void CopyData(RecPresetItem src, RecPresetItem dest)
        {
            dest.DisplayName = src.DisplayName;
            dest.ID = src.ID;
            dest.recPresetData = src.recPresetData.Clone();//nullのときはnullが返る。
        }

        private RecSettingData recPresetData = null;
        public RecSettingData RecPresetData
        {
            get
            {
                if (recPresetData == null) LoadRecPresetData();
                return recPresetData;
            }
            set
            {
                recPresetData = value;
            }
        }

        public const UInt32 CustomID = 0xFFFFFFFF;
        public bool IsCustom { get { return ID == RecPresetItem.CustomID; } }

        public void LoadRecPresetData(bool NotLoadedOnly = false)
        {
            if (NotLoadedOnly == true && recPresetData != null) return;

            string IDS = ID == 0 ? "" : ID.ToString();
            string defName = "REC_DEF" + IDS;
            string defFolderName = "REC_DEF_FOLDER" + IDS;
            string defFolder1SegName = "REC_DEF_FOLDER_1SEG" + IDS;

            recPresetData = new RecSettingData();
            recPresetData.RecMode = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "RecMode", 1, SettingPath.TimerSrvIniPath);
            recPresetData.Priority = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "Priority", 2, SettingPath.TimerSrvIniPath);
            recPresetData.TuijyuuFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "TuijyuuFlag", 1, SettingPath.TimerSrvIniPath);
            recPresetData.ServiceMode = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "ServiceMode", 0, SettingPath.TimerSrvIniPath);
            recPresetData.PittariFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "PittariFlag", 0, SettingPath.TimerSrvIniPath);
            recPresetData.BatFilePath = IniFileHandler.GetPrivateProfileString(defName, "BatFilePath", "", SettingPath.TimerSrvIniPath);

            int count = IniFileHandler.GetPrivateProfileInt(defFolderName, "Count", 0, SettingPath.TimerSrvIniPath);
            for (int i = 0; i < count; i++)
            {
                RecFileSetInfo folderInfo = new RecFileSetInfo();
                folderInfo.RecFolder = IniFileHandler.GetPrivateProfileString(defFolderName, i.ToString(), "", SettingPath.TimerSrvIniPath);
                folderInfo.WritePlugIn = IniFileHandler.GetPrivateProfileString(defFolderName, "WritePlugIn" + i.ToString(), "Write_Default.dll", SettingPath.TimerSrvIniPath);
                folderInfo.RecNamePlugIn = IniFileHandler.GetPrivateProfileString(defFolderName, "RecNamePlugIn" + i.ToString(), "", SettingPath.TimerSrvIniPath);

                recPresetData.RecFolderList.Add(folderInfo);
            }

            count = IniFileHandler.GetPrivateProfileInt(defFolder1SegName, "Count", 0, SettingPath.TimerSrvIniPath);
            for (int i = 0; i < count; i++)
            {
                RecFileSetInfo folderInfo = new RecFileSetInfo();
                folderInfo.RecFolder = IniFileHandler.GetPrivateProfileString(defFolder1SegName, i.ToString(), "", SettingPath.TimerSrvIniPath);
                folderInfo.WritePlugIn = IniFileHandler.GetPrivateProfileString(defFolder1SegName, "WritePlugIn" + i.ToString(), "Write_Default.dll", SettingPath.TimerSrvIniPath);
                folderInfo.RecNamePlugIn = IniFileHandler.GetPrivateProfileString(defFolder1SegName, "RecNamePlugIn" + i.ToString(), "", SettingPath.TimerSrvIniPath);

                recPresetData.PartialRecFolder.Add(folderInfo);
            }

            recPresetData.SuspendMode = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "SuspendMode", 0, SettingPath.TimerSrvIniPath);
            recPresetData.RebootFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "RebootFlag", 0, SettingPath.TimerSrvIniPath);
            recPresetData.UseMargineFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "UseMargineFlag", 0, SettingPath.TimerSrvIniPath);
            recPresetData.StartMargine = IniFileHandler.GetPrivateProfileInt(defName, "StartMargine", 0, SettingPath.TimerSrvIniPath);
            recPresetData.EndMargine = IniFileHandler.GetPrivateProfileInt(defName, "EndMargine", 0, SettingPath.TimerSrvIniPath);
            recPresetData.ContinueRecFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "ContinueRec", 0, SettingPath.TimerSrvIniPath);
            recPresetData.PartialRecFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "PartialRec", 0, SettingPath.TimerSrvIniPath);
            recPresetData.TunerID = (UInt32)IniFileHandler.GetPrivateProfileInt(defName, "TunerID", 0, SettingPath.TimerSrvIniPath);
        }
        public void SaveRecPresetData()
        {
            if (recPresetData == null) return;

            string IDS = ID == 0 ? "" : ID.ToString();
            string defName = "REC_DEF" + IDS;
            string defFolderName = "REC_DEF_FOLDER" + IDS;
            string defFolder1SegName = "REC_DEF_FOLDER_1SEG" + IDS;

            IniFileHandler.WritePrivateProfileString(defName, "SetName", DisplayName, SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "RecMode", recPresetData.RecMode.ToString(), SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "Priority", recPresetData.Priority.ToString(), SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "TuijyuuFlag", recPresetData.TuijyuuFlag.ToString(), SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "ServiceMode", recPresetData.ServiceMode.ToString(), SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "PittariFlag", recPresetData.PittariFlag.ToString(), SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "BatFilePath", recPresetData.BatFilePath, SettingPath.TimerSrvIniPath);

            IniFileHandler.WritePrivateProfileString(defFolderName, "Count", recPresetData.RecFolderList.Count.ToString(), SettingPath.TimerSrvIniPath);
            for (int j = 0; j < recPresetData.RecFolderList.Count; j++)
            {
                IniFileHandler.WritePrivateProfileString(defFolderName, j.ToString(), recPresetData.RecFolderList[j].RecFolder, SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defFolderName, "WritePlugIn" + j.ToString(), recPresetData.RecFolderList[j].WritePlugIn, SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defFolderName, "RecNamePlugIn" + j.ToString(), recPresetData.RecFolderList[j].RecNamePlugIn, SettingPath.TimerSrvIniPath);
            }
            IniFileHandler.WritePrivateProfileString(defFolder1SegName, "Count", recPresetData.PartialRecFolder.Count.ToString(), SettingPath.TimerSrvIniPath);
            for (int j = 0; j < recPresetData.PartialRecFolder.Count; j++)
            {
                IniFileHandler.WritePrivateProfileString(defFolder1SegName, j.ToString(), recPresetData.PartialRecFolder[j].RecFolder, SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defFolder1SegName, "WritePlugIn" + j.ToString(), recPresetData.PartialRecFolder[j].WritePlugIn, SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defFolder1SegName, "RecNamePlugIn" + j.ToString(), recPresetData.PartialRecFolder[j].RecNamePlugIn, SettingPath.TimerSrvIniPath);
            }

            IniFileHandler.WritePrivateProfileString(defName, "SuspendMode", recPresetData.SuspendMode.ToString(), SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "RebootFlag", recPresetData.RebootFlag.ToString(), SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "UseMargineFlag", recPresetData.UseMargineFlag.ToString(), SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "StartMargine", recPresetData.StartMargine.ToString(), SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "EndMargine", recPresetData.EndMargine.ToString(), SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "ContinueRec", recPresetData.ContinueRecFlag.ToString(), SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "PartialRec", recPresetData.PartialRecFlag.ToString(), SettingPath.TimerSrvIniPath);
            IniFileHandler.WritePrivateProfileString(defName, "TunerID", recPresetData.TunerID.ToString(), SettingPath.TimerSrvIniPath);
        }

        static public void ChecknFixRecPresetList(ref List<RecPresetItem> list)
        {
            list = list ?? new List<RecPresetItem>();
            if (list.Count == 0) list.Add(new RecPresetItem("", 0, new RecSettingData()));
            list[0].DisplayName = "デフォルト";

            uint i = 0;
            list.ForEach(item => item.ID = i++);
        }
        static public void SaveRecPresetList(ref List<RecPresetItem> list, bool ChecknFixSourceList = false)
        {
            if (ChecknFixSourceList == true)
            {
                ChecknFixRecPresetList(ref list);
            }

            if (list == null) return;

            string saveID = "";
            list.ForEach(item =>
            {
                item.SaveRecPresetData();
                if (item.ID != 0) saveID += item.ID + ",";
            });
            IniFileHandler.WritePrivateProfileString("SET", "PresetID", saveID, SettingPath.TimerSrvIniPath);
        }
        static public List<RecPresetItem> LoadRecPresetList()
        {
            var list = new List<RecPresetItem> { new RecPresetItem("デフォルト", 0) };

            foreach (string s in IniFileHandler.GetPrivateProfileString("SET", "PresetID", "", SettingPath.TimerSrvIniPath).Split(','))
            {
                uint id;
                uint.TryParse(s, out id);
                if (list.Exists(p => p.ID == id) == false)
                {
                    string name = IniFileHandler.GetPrivateProfileString("REC_DEF" + id, "SetName", "", SettingPath.TimerSrvIniPath);
                    list.Add(new RecPresetItem(name, id));
                }
            }

            return list;
        }
    }
    public static class RecPresetItemEx
    {
        public static List<RecPresetItem> Clone(this IEnumerable<RecPresetItem> src) { return RecPresetItem.Clone(src); }
        public static void LoadRecPresetData(this List<RecPresetItem> list, bool NotLoadedItemOnly = true)
        {
            if (list == null) return;
            list.ForEach(item => item.LoadRecPresetData(NotLoadedItemOnly));
        }
    }

}

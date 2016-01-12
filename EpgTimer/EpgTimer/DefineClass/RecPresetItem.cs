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

        public RecPresetItem Clone() { return CopyObj.Clone(this, CopyData); }
        protected static void CopyData(RecPresetItem src, RecPresetItem dest)
        {
            dest.DisplayName = src.DisplayName;
            dest.ID = src.ID;
            dest.recPresetData = src.recPresetData.Clone();//nullのときはnullが返る。
        }

        private RecSettingData recPresetData = null;
        [System.Xml.Serialization.XmlIgnore]
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

        public void LoadRecPresetData()
        {
            recPresetData = new RecSettingData();

            String defName = "REC_DEF";
            String defFolderName = "REC_DEF_FOLDER";
            String defFolder1SegName = "REC_DEF_FOLDER_1SEG";

            if (ID > 0)
            {
                defName += ID.ToString();
                defFolderName += ID.ToString();
                defFolder1SegName += ID.ToString();
            }

            recPresetData.RecMode = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "RecMode", 1, SettingPath.TimerSrvIniPath);
            recPresetData.Priority = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "Priority", 2, SettingPath.TimerSrvIniPath);
            recPresetData.TuijyuuFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "TuijyuuFlag", 1, SettingPath.TimerSrvIniPath);
            recPresetData.ServiceMode = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "ServiceMode", 0, SettingPath.TimerSrvIniPath);
            recPresetData.PittariFlag = (Byte)IniFileHandler.GetPrivateProfileInt(defName, "PittariFlag", 0, SettingPath.TimerSrvIniPath);

            recPresetData.BatFilePath = IniFileHandler.GetPrivateProfileString(defName, "BatFilePath", "", SettingPath.TimerSrvIniPath);

            recPresetData.RecFolderList.Clear();
            int count = IniFileHandler.GetPrivateProfileInt(defFolderName, "Count", 0, SettingPath.TimerSrvIniPath);
            for (int i = 0; i < count; i++)
            {
                RecFileSetInfo folderInfo = new RecFileSetInfo();
                folderInfo.RecFolder = IniFileHandler.GetPrivateProfileString(defFolderName, i.ToString(), "", SettingPath.TimerSrvIniPath);
                folderInfo.WritePlugIn = IniFileHandler.GetPrivateProfileString(defFolderName, "WritePlugIn" + i.ToString(), "Write_Default.dll", SettingPath.TimerSrvIniPath);
                folderInfo.RecNamePlugIn = IniFileHandler.GetPrivateProfileString(defFolderName, "RecNamePlugIn" + i.ToString(), "", SettingPath.TimerSrvIniPath);

                recPresetData.RecFolderList.Add(folderInfo);
            }

            recPresetData.PartialRecFolder.Clear();
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

            String defName = "REC_DEF";
            String defFolderName = "REC_DEF_FOLDER";
            String defFolder1SegName = "REC_DEF_FOLDER_1SEG";

            if (ID > 0)
            {
                defName += ID.ToString();
                defFolderName += ID.ToString();
                defFolder1SegName += ID.ToString();
            }

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
    }
}

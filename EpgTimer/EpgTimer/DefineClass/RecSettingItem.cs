using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class RecSettingItem : IRecSetttingData
    {
        public static string GetValuePropertyName(string key)
        {
            var obj = new RecSettingItem();
            if (key == CommonUtil.GetMemberName(() => obj.MarginStart))
            {
                return CommonUtil.GetMemberName(() => obj.MarginStartValue);
            }
            else if (key == CommonUtil.GetMemberName(() => obj.MarginEnd))
            {
                return CommonUtil.GetMemberName(() => obj.MarginEndValue);
            }
            else
            {
                return key;
            }
        }
        public virtual RecSettingData RecSettingInfo { get { return null; } }
        public virtual bool IsManual { get { return false; } }

        public virtual String MarginStart
        {
            get
            {
                if (RecSettingInfo == null) return "";
                //
                return RecSettingInfo.GetTrueMarginText(true);
            }
        }
        public virtual Double MarginStartValue
        {
            get
            {
                if (RecSettingInfo == null) return Double.MinValue;
                //
                return RecSettingInfo.GetTrueMarginForSort(true);
            }
        }
        public virtual String MarginEnd
        {
            get
            {
                if (RecSettingInfo == null) return "";
                //
                return RecSettingInfo.GetTrueMarginText(false);
            }
        }
        public virtual Double MarginEndValue
        {
            get
            {
                if (RecSettingInfo == null) return Double.MinValue;
                //
                return RecSettingInfo.GetTrueMarginForSort(false);
            }
        }
        protected String preset = null;
        public virtual String Preset
        {
            get
            {
                if (RecSettingInfo == null) return "";
                //
                if (preset == null) preset = RecSettingInfo.LookUpPreset(IsManual).DisplayName;
                return preset;
            }
        }
        public virtual String RecMode
        {
            get
            {
                if (RecSettingInfo == null) return "";
                //
                return CommonManager.Instance.ConvertRecModeText(RecSettingInfo.RecMode);
            }
        }
        public virtual String Priority
        {
            get
            {
                if (RecSettingInfo == null) return "";
                //
                return RecSettingInfo.Priority.ToString();
            }
        }
        public virtual String Tuijyu
        {
            get
            {
                if (RecSettingInfo == null) return "";
                //
                return CommonManager.Instance.YesNoDictionary[RecSettingInfo.TuijyuuFlag].DisplayName;
            }
        }
        public virtual String Pittari
        {
            get
            {
                if (RecSettingInfo == null) return "";
                //
                return CommonManager.Instance.YesNoDictionary[RecSettingInfo.PittariFlag].DisplayName;
            }
        }
        public virtual String Tuner
        {
            get
            {
                if (RecSettingInfo == null) return "";
                //
                return CommonManager.Instance.ConvertTunerText(RecSettingInfo.TunerID);
            }
        }
        public virtual List<String> RecFolder
        {
            get
            {
                if (RecSettingInfo == null) new List<string>();
                //
                return RecSettingInfo.GetRecFolderViewList();
            }
        }

        public String ConvertRecSettingText()
        {
            if (RecSettingInfo == null) return "";
            //
            String view = "";

            view += "録画モード : " + RecMode + "\r\n";
            view += "優先度 : " + Priority + "\r\n";
            view += "追従 : " + Tuijyu + "\r\n";
            view += "ぴったり(?): " + Pittari + "\r\n";
            {
                bool isDefault = (RecSettingInfo.ServiceMode & 0x01) == 0;
                bool isCaption = (RecSettingInfo.ServiceMode & 0x10) > 0;
                bool isData = (RecSettingInfo.ServiceMode & 0x20) > 0;
                if (isDefault == true)
                {
                    isCaption = IniFileHandler.GetPrivateProfileInt("SET", "Caption", 1, SettingPath.EdcbIniPath) != 0;
                    isData = IniFileHandler.GetPrivateProfileInt("SET", "Data", 0, SettingPath.EdcbIniPath) != 0;
                }
                view += "指定サービス対象データ : 字幕含" + (isCaption ? "む" : "まない")
                                              + " データカルーセル含" + (isData ? "む" : "まない")
                                              + (isDefault == true ? " (デフォルト)" : "") + "\r\n";
            }
            view += "録画実行bat : " + (RecSettingInfo.BatFilePath == "" ? "なし" : RecSettingInfo.BatFilePath) + "\r\n";
            {
                List<RecFileSetInfo> recFolderList = RecSettingInfo.RecFolderList;
                view += "録画フォルダ : " + (recFolderList.Count == 0 ? "(デフォルト)" : "") + "\r\n";
                if (recFolderList.Count == 0)
                {
                    String plugInFile = IniFileHandler.GetPrivateProfileString("SET", "RecNamePlugInFile", "RecName_Macro.dll", SettingPath.TimerSrvIniPath);
                    foreach (string info in Settings.Instance.DefRecFolders)
                    {
                        view += info + " (WritePlugIn:Write_Default.dll ファイル名PlugIn:" + plugInFile + ")\r\n";
                    }
                }
                else
                {
                    foreach (RecFileSetInfo info in RecSettingInfo.RecFolderList)
                    {
                        view += info.RecFolder + " (WritePlugIn:" + info.WritePlugIn + " ファイル名PlugIn:" + info.RecNamePlugIn + ")\r\n";
                    }
                }
            }
            view += "録画マージン : 開始 " + RecSettingInfo.GetTrueMargin(true).ToString() +
                                  " 終了 " + RecSettingInfo.GetTrueMargin(false).ToString()
                     + (RecSettingInfo.UseMargineFlag == 0 ? " (デフォルト)" : "") + "\r\n";
            {
                bool isDefault = RecSettingInfo.SuspendMode == 0;
                int recEndMode = RecSettingInfo.SuspendMode;
                bool reboot = RecSettingInfo.RebootFlag == 1;
                if (isDefault == true)
                {
                    recEndMode = IniFileHandler.GetPrivateProfileInt("SET", "RecEndMode", 2, SettingPath.TimerSrvIniPath);
                    reboot = IniFileHandler.GetPrivateProfileInt("SET", "Reboot", 0, SettingPath.TimerSrvIniPath) == 1;
                }
                view += "録画後動作 : ";
                switch (recEndMode)
                {
                    case 1:
                        view += "スタンバイ";
                        break;
                    case 2:
                        view += "休止";
                        break;
                    case 3:
                        view += "シャットダウン";
                        break;
                    case 4:
                        view += "何もしない";
                        break;
                }
                view += (reboot == true ? " 復帰後再起動する" : "") + (isDefault == true ? " (デフォルト)" : "") + "\r\n";
            }
            if (RecSettingInfo.PartialRecFlag == 0)
            {
                view += "部分受信 : 同時出力なし\r\n";
            }
            else
            {
                view += "部分受信 : 同時出力あり\r\n";
                view += "部分受信　録画フォルダ : \r\n";
                foreach (RecFileSetInfo info in RecSettingInfo.PartialRecFolder)
                {
                    view += info.RecFolder + " (WritePlugIn:" + info.WritePlugIn + " ファイル名PlugIn:" + info.RecNamePlugIn + ")\r\n";
                }
            }
            view += "連続録画動作 : " + (RecSettingInfo.ContinueRecFlag == 0 ? "分割" : "同一ファイル出力") + "\r\n";
            view += "使用チューナー強制指定 : " + Tuner;

            return view;
        }
    }
}

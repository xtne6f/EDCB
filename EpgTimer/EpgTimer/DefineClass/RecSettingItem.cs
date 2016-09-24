using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace EpgTimer
{
    public class RecSettingItem : DataListItemBase, IRecSetttingData
    {
        public virtual RecSettingData RecSettingInfo { get { return null; } }
        public virtual void Reset() { preset = null; }
        public virtual bool IsManual { get { return false; } }

        public virtual String MarginStart
        {
            get
            {
                if (RecSettingInfo == null) return "";
                //
                return CustomTimeFormat(RecSettingInfo.StartMarginActual * -1);
            }
        }
        public virtual Double MarginStartValue
        {
            get
            {
                if (RecSettingInfo == null) return Double.MinValue;
                //
                return CustomMarginValue(RecSettingInfo.StartMarginActual * -1);
            }
        }
        public virtual String MarginEnd
        {
            get
            {
                if (RecSettingInfo == null) return "";
                //
                return CustomTimeFormat(RecSettingInfo.EndMarginActual);
            }
        }
        public virtual Double MarginEndValue
        {
            get
            {
                if (RecSettingInfo == null) return Double.MinValue;
                //
                return CustomMarginValue(RecSettingInfo.EndMarginActual);
            }
        }
        private string CustomTimeFormat(int span)
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
            return span.ToString("+;-") + hours + minutes + seconds + (RecSettingInfo.UseMargineFlag != 0 ? " " : "*");
        }
        private Double CustomMarginValue(int span)
        {
            return span + (RecSettingInfo.UseMargineFlag != 0 ? 0.1 : 0);
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
                return CommonManager.ConvertRecModeText(RecSettingInfo.RecMode);
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
                return CommonManager.ConvertYesNoText(RecSettingInfo.TuijyuuFlag);
            }
        }
        public virtual String Pittari
        {
            get
            {
                if (RecSettingInfo == null) return "";
                //
                return CommonManager.ConvertYesNoText(RecSettingInfo.PittariFlag);
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
                if (RecSettingInfo == null) return new List<string>();
                //
                return RecSettingInfo.RecFolderViewList;
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
            view += "指定サービス対象データ : 字幕含" + (RecSettingInfo.ServiceCaptionActual ? "める" : "めない")
                                            + " データカルーセル含" + (RecSettingInfo.ServiceDataActual ? "める" : "めない")
                                            + (RecSettingInfo.ServiceModeIsDefault ? " (デフォルト)" : "") + "\r\n";
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
            view += "録画マージン : 開始 " + RecSettingInfo.StartMarginActual.ToString() +
                                  " 終了 " + RecSettingInfo.EndMarginActual.ToString()
                     + (RecSettingInfo.UseMargineFlag == 0 ? " (デフォルト)" : "") + "\r\n";

            view += "録画後動作 : "
                + new string[] { "何もしない", "スタンバイ", "休止", "シャットダウン" }[RecSettingInfo.RecEndModeActual]
                + (RecSettingInfo.RebootFlagActual == 1 ? " 復帰後再起動する" : "")
                + (RecSettingInfo.SuspendMode == 0 ? " (デフォルト)" : "") + "\r\n";

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

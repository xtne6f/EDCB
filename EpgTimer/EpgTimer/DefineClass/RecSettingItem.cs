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
        public virtual String Preset
        {
            get
            {
                if (RecSettingInfo == null) return "";
                //
                return RecSettingInfo.LookUpPreset().DisplayName;
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
    }
}

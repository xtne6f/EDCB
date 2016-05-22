using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Input;
using System.Reflection;

namespace EpgTimer
{
    //設定画面用
    public static class MenuSettingDataEx
    {
        public static List<MenuSettingData> Clone(this IEnumerable<MenuSettingData> src) { return MenuSettingData.Clone(src); }
        public static List<MenuSettingData.CmdSaveData> Clone(this IEnumerable<MenuSettingData.CmdSaveData> src) { return MenuSettingData.CmdSaveData.Clone(src); }
    }
    public class MenuSettingData
    {
        public class CmdSaveData
        {
            public string Name { get; set; }
            public string TypeName { get; set; }
            public bool IsMenuEnabled { get; set; }
            public bool IsGestureEnabled { get; set; }
            public bool IsGesNeedMenu { get; set; }
            public List<ShortCutData> ShortCuts { get; set; }

            public CmdSaveData() { ShortCuts = new List<ShortCutData>(); }
            public static List<CmdSaveData> Clone(IEnumerable<CmdSaveData> src) { return CopyObj.Clone(src, CopyData); }
            public CmdSaveData Clone() { return CopyObj.Clone(this, CopyData); }
            protected static void CopyData(CmdSaveData src, CmdSaveData dest)
            {
                dest.Name = src.Name;
                dest.TypeName = src.TypeName;
                dest.IsMenuEnabled = src.IsMenuEnabled;
                dest.IsGestureEnabled = src.IsGestureEnabled;
                dest.IsGesNeedMenu = src.IsGesNeedMenu;
                dest.ShortCuts = src.ShortCuts.ToList();
            }

            public CmdSaveData(ICommand cmd, bool isMenuE, bool isGestureE, bool isGesNeedMenu, InputGestureCollection igc)
            {
                SetCommand(cmd);
                SetGestuers(igc);
                IsMenuEnabled = isMenuE;
                IsGestureEnabled = isGestureE;
                IsGesNeedMenu = isGesNeedMenu;
            }
            public void SetGestuers(InputGestureCollection igc)
            {
                ShortCuts = new List<ShortCutData>();
                foreach (var kg in igc.OfType<KeyGesture>()) { ShortCuts.Add(new ShortCutData(kg)); }
            }
            public InputGestureCollection GetGestuers()
            {
                var igc = new InputGestureCollection();
                ShortCuts.ForEach(item =>
                {
                    KeyGesture kg = item.GetKeyGesture();
                    if (kg != null) igc.Add(kg);
                });
                return igc;
            }
            public void SetCommand(ICommand cmd)
            {
                if (cmd is RoutedUICommand)
                {
                    Name = (cmd as RoutedUICommand).Name;
                    TypeName = (cmd as RoutedUICommand).OwnerType.FullName;
                }
                else
                {
                    Name = null;
                    TypeName = null;
                }
            }
            private RoutedUICommand command = null;
            public RoutedUICommand GetCommand()
            {
                try
                {
                    if (command == null || command.Name != Name || command.OwnerType.FullName != TypeName)
                    {
                        command = null;
                        Type t = Type.GetType(TypeName);
                        foreach (PropertyInfo info in t.GetProperties())
                        {
                            var cmd = info.GetValue(null, null) as RoutedUICommand;
                            if (cmd.Name == Name)
                            {
                                command = cmd;
                                break;
                            }
                        }
                    }
                    return command;
                }
                catch { }
                return null;
            }
        }
        public struct ShortCutData
        {
            public Key skey;
            public ModifierKeys mKey;
            public ShortCutData(KeyGesture kg) { skey = kg.Key; mKey = kg.Modifiers; }
            public KeyGesture GetKeyGesture() 
            {
                try
                {
                    //エラーの場合があり得る
                    return new KeyGesture(skey, mKey); 
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }
                return null;
            }
        }

        public const int CautionDisplayItemNum = 10;
    
        public List<CtxmCode> IsManualAssign { get; set; }
        public bool NoMessageKeyGesture { get; set; }
        public bool NoMessageDeleteAll { get; set; }
        public bool NoMessageDelete2 { get; set; }
        public bool NoMessageAdjustRes { get; set; }
        public bool CancelAutoAddOff { get; set; }
        public bool AutoAddFazySerach { get; set; }
        public bool AutoAddSerachToolTip { get; set; }
        public bool OpenParentFolder { get; set; }
        public bool Keyword_Trim { get; set; }
        public bool CopyTitle_Trim { get; set; }
        public bool CopyContentBasic { get; set; }
        public bool SearchTitle_Trim { get; set; }
        public String SearchURI { get; set; }
        public bool NoMessageNotKEY { get; set; }
        public List<CmdSaveData> EasyMenuItems { get; set; }
        public List<CtxmSetting> ManualMenuItems { get; set; }

        public MenuSettingData() 
        {
            IsManualAssign = new List<CtxmCode>();
            NoMessageKeyGesture = false;
            NoMessageDeleteAll = false;
            NoMessageDelete2 = false;
            NoMessageAdjustRes = false;
            CancelAutoAddOff = false;
            AutoAddFazySerach = false;
            AutoAddSerachToolTip = false;
            OpenParentFolder = false;
            Keyword_Trim = true;
            CopyTitle_Trim = false;
            CopyContentBasic = false;
            SearchTitle_Trim = true;
            SearchURI = "https://www.google.co.jp/search?hl=ja&q=";
            NoMessageNotKEY = false;
            EasyMenuItems = new List<CmdSaveData>();
            ManualMenuItems = new List<CtxmSetting>();
        }
        public static List<MenuSettingData> Clone(IEnumerable<MenuSettingData> src) { return CopyObj.Clone(src, CopyData); }
        public MenuSettingData Clone() { return CopyObj.Clone(this, CopyData); }
        protected static void CopyData(MenuSettingData src, MenuSettingData dest)
        {
            dest.IsManualAssign = src.IsManualAssign.ToList();
            dest.NoMessageKeyGesture = src.NoMessageKeyGesture;
            dest.NoMessageDeleteAll = src.NoMessageDeleteAll;
            dest.NoMessageDelete2 = src.NoMessageDelete2;
            dest.NoMessageAdjustRes = src.NoMessageAdjustRes;
            dest.CancelAutoAddOff = src.CancelAutoAddOff;
            dest.AutoAddFazySerach = src.AutoAddFazySerach;
            dest.AutoAddSerachToolTip = src.AutoAddSerachToolTip;
            dest.OpenParentFolder = src.OpenParentFolder;
            dest.Keyword_Trim = src.Keyword_Trim;
            dest.CopyTitle_Trim = src.CopyTitle_Trim;
            dest.CopyContentBasic = src.CopyContentBasic;
            dest.SearchTitle_Trim = src.SearchTitle_Trim;
            dest.SearchURI = src.SearchURI;
            dest.NoMessageNotKEY = src.NoMessageNotKEY;
            dest.EasyMenuItems = src.EasyMenuItems.Clone();
            dest.ManualMenuItems = src.ManualMenuItems.Clone();
        }
    }

    public class CtxmSetting
    {
        public CtxmCode ctxmCode { set; get; }
        public List<string> Items { set; get; }

        public CtxmSetting() { Items = new List<string>(); }
        public CtxmSetting(CtxmSetting data) { CopyData(data, this); }

        //デフォルト内部データ → デフォルトセーブデータ用
        public CtxmSetting(CtxmData data)
        {
            if (data == null) return;
            //
            ctxmCode = data.ctxmCode;
            Items = new List<string>();
            data.Items.ForEach(item => Items.Add(item.Header));
        }

        public static List<CtxmSetting> Clone(IEnumerable<CtxmSetting> src) { return CopyObj.Clone(src, CopyData); }
        public CtxmSetting Clone() { return CopyObj.Clone(this, CopyData); }
        protected static void CopyData(CtxmSetting src, CtxmSetting dest)
        {
            dest.ctxmCode = src.ctxmCode;
            dest.Items = src.Items.ToList();
        }
    }

    public static class CtxmSettingEx
    {
        public static List<CtxmSetting> Clone(this IEnumerable<CtxmSetting> src) { return CtxmSetting.Clone(src); }
        public static CtxmSetting FindData(this List<CtxmSetting> list, CtxmCode code)
        { return list.Find(data => data.ctxmCode == code); }
    }
}

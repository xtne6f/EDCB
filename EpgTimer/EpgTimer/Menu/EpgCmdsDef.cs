using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using System.Windows.Input;
using System.Reflection;

namespace EpgTimer
{
    public static class EpgCmds
    {
        public static RoutedUICommand Add { get; private set; }
        public static RoutedUICommand ShowAddDialog { get; private set; }
        public static RoutedUICommand AddOnPreset { get; private set; }
        public static RoutedUICommand ChgOnOff { get; private set; }
        public static RoutedUICommand ChgOnPreset { get; private set; }
        public static RoutedUICommand ChgBulkRecSet { get; private set; }
        public static RoutedUICommand ChgGenre { get; private set; }
        public static RoutedUICommand ChgRecmode { get; private set; }
        public static RoutedUICommand ChgPriority { get; private set; }
        public static RoutedUICommand ChgRelay { get; private set; }
        public static RoutedUICommand ChgPittari { get; private set; }
        public static RoutedUICommand ChgTuner { get; private set; }
        public static RoutedUICommand ChgMarginStart { get; private set; }
        public static RoutedUICommand ChgMarginStartValue { get; private set; }
        public static RoutedUICommand ChgMarginEnd { get; private set; }
        public static RoutedUICommand ChgMarginEndValue { get; private set; }
        public static RoutedUICommand Delete { get; private set; }
        public static RoutedUICommand Delete2 { get; private set; }
        public static RoutedUICommand DeleteAll { get; private set; }
        public static RoutedUICommand ShowDialog { get; private set; }
        public static RoutedUICommand JumpTable { get; private set; }
        public static RoutedUICommand ToAutoadd { get; private set; }
        public static RoutedUICommand ReSearch { get; private set; }
        public static RoutedUICommand ReSearch2 { get; private set; }
        public static RoutedUICommand Play { get; private set; }
        public static RoutedUICommand OpenFolder { get; private set; }
        public static RoutedUICommand CopyTitle { get; private set; }
        public static RoutedUICommand CopyContent { get; private set; }
        public static RoutedUICommand SearchTitle { get; private set; }
        public static RoutedUICommand CopyNotKey { get; private set; }
        public static RoutedUICommand SetNotKey { get; private set; }
        public static RoutedUICommand ProtectChange { get; private set; }
        public static RoutedUICommand ViewChgSet { get; private set; }
        public static RoutedUICommand ViewChgMode { get; private set; }
        public static RoutedUICommand MenuSetting { get; private set; }

        public static RoutedUICommand AddInDialog { get; private set; }
        public static RoutedUICommand ChangeInDialog { get; private set; }
        public static RoutedUICommand DeleteInDialog { get; private set; }
        public static RoutedUICommand Search { get; private set; }
        public static RoutedUICommand UpItem { get; private set; }
        public static RoutedUICommand DownItem { get; private set; }
        public static RoutedUICommand SaveOrder { get; private set; }
        public static RoutedUICommand RestoreOrder { get; private set; }
        public static RoutedUICommand DragCancel { get; private set; }
        public static RoutedUICommand Cancel { get; private set; }

        static EpgCmds() { EpgCmdsEx.InitCommands(typeof(EpgCmds)); }
    }
    public static class EpgCmdsEx
    {
        //メニュー用のダミーコマンドを定義。メニューコードの列挙体作った方が良さげだけど、手抜き。
        //でも定数じゃないのでswitch caseは使えない。
        public static RoutedUICommand AddMenu { get; private set; }
        public static RoutedUICommand ChgMenu { get; private set; }
        public static RoutedUICommand ChgRecmodeMenu { get; private set; }
        public static RoutedUICommand ChgPriorityMenu { get; private set; }
        public static RoutedUICommand ChgRelayMenu { get; private set; }
        public static RoutedUICommand ChgPittariMenu { get; private set; }
        public static RoutedUICommand ChgTunerMenu { get; private set; }
        public static RoutedUICommand ChgMarginStartMenu { get; private set; }
        public static RoutedUICommand ChgMarginEndMenu { get; private set; }
        public static RoutedUICommand ChgOnPresetMenu { get; private set; }
        public static RoutedUICommand OpenFolderMenu { get; private set; }
        public static RoutedUICommand ViewMenu { get; private set; }
        public static RoutedUICommand Separator { get; private set; }
        public const string SeparatorString = "－－－－－－";

        static EpgCmdsEx() { EpgCmdsEx.InitCommands(typeof(EpgCmdsEx)); }
        static public bool IsDummyCmd(ICommand icmd)
        {
            return icmd is RoutedUICommand && (icmd as RoutedUICommand).OwnerType == typeof(EpgCmdsEx);
        }

        //初期化して、自身のプロパティ名をName、所属クラスをOwnerTypeに与える。
        public static void InitCommands(Type t)
        {
            foreach (PropertyInfo info in t.GetProperties())
            {
                info.SetValue(null, new RoutedUICommand("", info.Name, t), null);
            }
        }
        /* 必要無かった //クラス内で見つかればTRUE
        public static bool Find(ICommand icmd, Type t)
        {
            if (t == null) return false;
            foreach (PropertyInfo info in t.GetProperties())
            {
                if (info.GetValue(null, null) == icmd) return true;//info.Nameならプロパティ名を返せる。
            }
            return false;
        }*/
    }

    public class EpgCmdParam
    {
        public Type SourceType { get; set; }
        public CtxmCode Code { get; set; }
        public int ID { get; set; }
        public object Data { get; set; }

        public EpgCmdParam(Type type, CtxmCode code = CtxmCode.EtcWindow, int id = 0, object data = null)
        { 
            SourceType = type; 
            Code = code; 
            ID = id;
            Data = data;
        }
        public EpgCmdParam(EpgCmdParam data)
        {
            if (data == null) return;
            SourceType = data.SourceType;
            Code = data.Code;
            ID = data.ID;
            Data = data.Data;
        }
    }
}

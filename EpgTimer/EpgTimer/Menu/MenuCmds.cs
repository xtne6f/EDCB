using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Input;
using System.Reflection;

namespace EpgTimer
{
    public class MenuCmds
    {
        private static Dictionary<ICommand, CmdData> _DefCmdOptions = null;//デフォルトのコマンドオプション
        public Dictionary<ICommand, CmdData> DefCmdOptions { get { return _DefCmdOptions; } }
        public Dictionary<ICommand, CmdData> WorkCmdOptions { get; private set; }//現在のコマンドオプション

        public struct CmdData
        {
            public ICommand Command;
            public bool IsSaveSetting;              //主要項目、XMLに書き出す。
            public bool IsMenuEnabled;              //メニューの有効無効。初期設定でのみ使用。
            public InputGestureCollection Gestures; //ショートカット
            public GestureTrg GesTrg;               //ショートカットの範囲
            public bool IsGestureEnabled;           //ショートカットの有効無効
            public bool IsGesNeedMenu;              //ショートカットの有効にメニュー表示必要かどうか。こちらはView単位で働く。

            public CmdData(ICommand icmd, InputGestureCollection gs, GestureTrg trg, bool isEnable = true, bool gesEnabled = false, bool gesNeedMenu = true, bool isSave = false)
            {
                Command = icmd;
                IsSaveSetting = isSave;
                IsMenuEnabled = isEnable;
                Gestures = new InputGestureCollection(gs);
                GesTrg = trg;
                IsGestureEnabled = gesEnabled;
                IsGesNeedMenu = gesNeedMenu;
            }
            public CmdData Copy()
            {
                return new CmdData(Command, Gestures, GesTrg, IsMenuEnabled, IsGestureEnabled, IsGesNeedMenu, IsSaveSetting);
            }
        }
        public enum GestureTrg : byte
        {
            None = 0x00,//未使用
            ToView = 0x01,
            ToList = 0x02
        }

        public MenuCmds()
        {
            if (_DefCmdOptions == null) SetDefCmdOption();
        }

        private static void SetDefCmdOption()
        {
            _DefCmdOptions = new Dictionary<ICommand, CmdData>();

            //ショートカットは、個別に無効にしたり範囲を限定したりするのでこちらで管理する。
            //AddCommand:コマンド、ショートカット、isEnable: 有効無効、
            //gesNeedMenu:メニューと連動してショートカットを無効にするか、spc:ショートカット範囲

            //コンテキストメニュー用
            AddCommand(EpgCmds.Add);
            AddCommand(EpgCmds.ShowAddDialog, spc: GestureTrg.ToView, isEnable: false);
            AddCommand(EpgCmds.ChgOnOff, Key.S, ModifierKeys.Control);
            AddCommand(EpgCmds.ChgGenre, isEnable: false, isSave: false);
            AddCommand(EpgCmds.Delete, Key.D, ModifierKeys.Control, Key.Delete);
            AddCommand(EpgCmds.Delete2, Key.D, ModifierKeys.Control | ModifierKeys.Shift);
            AddCommand(EpgCmds.DeleteAll, Key.D, ModifierKeys.Control | ModifierKeys.Alt, spc: GestureTrg.ToView);
            AddCommand(EpgCmds.AdjustReserve, isEnable: false);
            AddCommand(EpgCmds.ShowDialog, Key.Enter, gesNeedMenu: false);//doubleclickは上手く入らないので省略
            AddCommand(EpgCmds.JumpReserve, Key.F3, ModifierKeys.Shift, isEnable: false);
            AddCommand(EpgCmds.JumpTuner, Key.F3, ModifierKeys.Control, isEnable: false);
            AddCommand(EpgCmds.JumpTable, Key.F3);
            AddCommand(EpgCmds.JumpList, Key.F3);//簡易検索画面用のジャンプ。とりあえず分けておく。
            AddCommand(EpgCmds.ToAutoadd);
            AddCommand(EpgCmds.ReSearch, Key.F, ModifierKeys.Control | ModifierKeys.Shift);
            AddCommand(EpgCmds.ReSearch2, Key.F, ModifierKeys.Control | ModifierKeys.Alt, isEnable: false);
            AddCommand(EpgCmds.InfoSearchTitle, isEnable: false);
            AddCommand(EpgCmds.Play, Key.P, ModifierKeys.Control);
            AddCommand(EpgCmds.OpenFolder, isEnable: false);
            AddCommand(EpgCmds.CopyTitle, Key.C, ModifierKeys.Control, isEnable: false);
            AddCommand(EpgCmds.CopyContent, isEnable: false);
            AddCommand(EpgCmds.SearchTitle, isEnable: false);
            AddCommand(EpgCmds.CopyNotKey, Key.X, ModifierKeys.Control, isEnable: false);
            AddCommand(EpgCmds.SetNotKey, Key.V, ModifierKeys.Control, isEnable: false);
            AddCommand(EpgCmds.ProtectChange, Key.S, ModifierKeys.Control, isEnable: false);
            AddCommand(EpgCmds.ViewChgSet, spc: GestureTrg.ToView);
            AddCommand(EpgCmds.ViewChgMode, spc: GestureTrg.ToView);
            AddCommand(EpgCmds.MenuSetting, spc: GestureTrg.ToView);

            //主にボタン用、Up,Downはリストビューのキー操作と干渉するのでウィンドウにもリストビューにもバインディングさせる。
            AddCommand(EpgCmds.AddInDialog, Key.A, ModifierKeys.Control | ModifierKeys.Shift, spc: GestureTrg.ToView, gesNeedMenu: false);
            AddCommand(EpgCmds.ChangeInDialog, Key.C, ModifierKeys.Control | ModifierKeys.Shift, spc: GestureTrg.ToView, gesNeedMenu: false);
            AddCommand(EpgCmds.DeleteInDialog, Key.X, ModifierKeys.Control | ModifierKeys.Shift, spc: GestureTrg.ToView, gesNeedMenu: false);
            AddCommand(EpgCmds.Delete2InDialog, Key.D, ModifierKeys.Control | ModifierKeys.Shift, spc: GestureTrg.ToView, gesNeedMenu: false);
            AddCommand(EpgCmds.Search, Key.F, ModifierKeys.Control, spc: GestureTrg.ToView, gesNeedMenu: false);
            AddCommand(EpgCmds.InfoSearch, Key.F, ModifierKeys.Control | ModifierKeys.Shift, spc: GestureTrg.ToView, gesNeedMenu: false);
            AddCommand(EpgCmds.TopItem, Key.Up, ModifierKeys.Control | ModifierKeys.Shift, spc: GestureTrg.ToList | GestureTrg.ToView, gesNeedMenu: false);
            AddCommand(EpgCmds.UpItem, Key.Up, ModifierKeys.Control, spc: GestureTrg.ToList | GestureTrg.ToView, gesNeedMenu: false);
            AddCommand(EpgCmds.DownItem, Key.Down, ModifierKeys.Control, spc: GestureTrg.ToList | GestureTrg.ToView, gesNeedMenu: false);
            AddCommand(EpgCmds.BottomItem, Key.Down, ModifierKeys.Control | ModifierKeys.Shift, spc: GestureTrg.ToList | GestureTrg.ToView, gesNeedMenu: false);
            AddCommand(EpgCmds.SaveOrder, Key.S, ModifierKeys.Control, spc: GestureTrg.ToView, gesNeedMenu: false);
            AddCommand(EpgCmds.RestoreOrder, Key.Z, ModifierKeys.Control, spc: GestureTrg.ToView, gesNeedMenu: false);
            AddCommand(EpgCmds.DragCancel, Key.Escape, spc: GestureTrg.ToView, gesNeedMenu: false, isSave: false);
            AddCommand(EpgCmds.Cancel, Key.Escape, spc: GestureTrg.ToView, gesNeedMenu: false, isSave: false);

            //ダミーコマンドは、キーとして使用しているが、メニュー自体には割り付けされない。
            AddCommand(EpgCmdsEx.AddMenu);
            AddCommand(EpgCmdsEx.ChgMenu);
            AddCommand(EpgCmdsEx.ShowAutoAddDialogMenu, isEnable: false);
            AddCommand(EpgCmdsEx.ChgMarginStartMenu, isEnable: false, isSave: false);
            AddCommand(EpgCmdsEx.ChgMarginEndMenu, isEnable: false, isSave: false);
            AddCommand(EpgCmdsEx.OpenFolderMenu, isEnable: false);
            AddCommand(EpgCmdsEx.ViewMenu);

            //特段の設定を持っていないサブメニュー用コマンドなどはまとめて登録
            foreach (PropertyInfo info in typeof(EpgCmds).GetProperties())
            {
                AddCommand(info.GetValue(null, null) as ICommand, isSave: false);//設定ファイルに書き出さない
            }
            foreach (PropertyInfo info in typeof(EpgCmdsEx).GetProperties())
            {
                AddCommand(info.GetValue(null, null) as ICommand, isSave: false);
            }
        }

        private static void AddCommand(ICommand icmd,
            Key key1 = Key.None, ModifierKeys modifiers1 = ModifierKeys.None,
            Key key2 = Key.None, ModifierKeys modifiers2 = ModifierKeys.None,
            GestureTrg spc = GestureTrg.ToList, bool isEnable = true, bool gesNeedMenu = true, bool isSave = true)
        {
            var iGestures = new InputGestureCollection();
            if (key1 != Key.None) iGestures.Add(new KeyGesture(key1, modifiers1));
            if (key2 != Key.None) iGestures.Add(new KeyGesture(key2, modifiers2));
            //if (doubleClick == true) gestures.Add(new MouseGesture(MouseAction.LeftDoubleClick));

            if (_DefCmdOptions.ContainsKey(icmd) == false)
            {
                _DefCmdOptions.Add(icmd, new CmdData(icmd, iGestures, spc, isEnable, isEnable, gesNeedMenu, isSave));
            }
        }

        public void SetWorkCmdOptions()
        {
            //ショートカット有効とメニュー連動有効を設定から読み込む
            WorkCmdOptions = new Dictionary<ICommand, CmdData>();
            foreach (var item in DefCmdOptions)
            {
                CmdData cmdData = item.Value.Copy();
                MenuSettingData.CmdSaveData cmdSave = Settings.Instance.MenuSet.EasyMenuItems.Find(data => data.GetCommand() == item.Key);
                if (cmdSave != null)
                {
                    cmdData.IsMenuEnabled = cmdSave.IsMenuEnabled;
                    cmdData.IsGestureEnabled = cmdSave.IsGestureEnabled;
                    cmdData.IsGesNeedMenu = cmdSave.IsGesNeedMenu;

                    //入出力はキージェスチャだけなので、それだけ入れ替える。(キージェスチャ以外はそのまま)
                    var delList = cmdData.Gestures.OfType<KeyGesture>().ToList();
                    delList.ForEach(gs => cmdData.Gestures.Remove(gs));
                    cmdData.Gestures.AddRange(cmdSave.GetGestuers());
                }
                WorkCmdOptions.Add(item.Key, cmdData);
            }
        }

        //設定へオプションを送る
        public List<MenuSettingData.CmdSaveData> GetDefEasyMenuSetting() { return GetEasyMenuSetting(DefCmdOptions); }
        public List<MenuSettingData.CmdSaveData> GetWorkEasyMenuSetting() { return GetEasyMenuSetting(WorkCmdOptions); }
        private List<MenuSettingData.CmdSaveData> GetEasyMenuSetting(Dictionary<ICommand, CmdData> dic)
        {
            var set = new List<MenuSettingData.CmdSaveData>();
            foreach (CmdData item in dic.Values.Where(item => item.IsSaveSetting == true))
            {
                set.Add(new MenuSettingData.CmdSaveData(
                    item.Command, item.IsMenuEnabled, item.IsGestureEnabled, item.IsGesNeedMenu, item.Gestures));
            }
            return set;
        }
    }
}

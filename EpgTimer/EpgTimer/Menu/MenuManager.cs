using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Reflection;

using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class MenuManager
    {
        public DateTime? LastMenuSettingTime { get; private set; }

        private Dictionary<CtxmCode, CtxmData> DefCtxmData;//デフォルトのコンテキストメニュー
        private Dictionary<CtxmCode, List<ICommand>> DefCtxmCmdList;//デフォルトのコンテキストメニューのコマンドリスト

        private Dictionary<CtxmCode, CtxmData> WorkCtxmData;//現在のコンテキストメニュー
//        private Dictionary<CtxmCode, List<ICommand>> WorCtxmCmdList;//各ビューのコンテキストメニューのコマンドリスト
        private Dictionary<CtxmCode, List<ICommand>> WorkGestureCmdList;//各ビューのショートカット管理用のコマンドリスト
        private Dictionary<CtxmCode, List<ICommand>> WorkDelGesCmdList;//メニューから削除され、shortcut無効なコマンド

        public MenuCmds MC { get; private set; } //コマンドオプション関係

        public MenuManager()
        {
            MC = new MenuCmds();
            SetDefCtxmData();
            SetDefGestureCmdList();
        }
        //コンテキストメニューの定義
        private void SetDefCtxmData()
        {
            //各画面のコンテキストメニュー。空にならないようとりあえず全部作っておく。
            DefCtxmData = new Dictionary<CtxmCode, CtxmData>();
            foreach (CtxmCode code in Enum.GetValues(typeof(CtxmCode)))
            {
                DefCtxmData.Add(code, new CtxmData(code));
            }

            //標準操作のメニューアイテム。2回以上出てくるものは、情報固定のためいったん定義する。
            var cm_ChangeOnOff = new CtxmItemData("予約←→無効", EpgCmds.ChgOnOff);
            var cm_ShowAddDialog = new CtxmItemData("予約追加", EpgCmds.ShowAddDialog);//マニュアル予約など
            var cm_AddMenu = new CtxmItemData("予約追加", EpgCmdsEx.AddMenu);
            var cm_ChangeMenu = new CtxmItemData("変更", EpgCmdsEx.ChgMenu);
            var cm_Delete = new CtxmItemData("削除", EpgCmds.Delete);
            var cm_Delete2 = new CtxmItemData("予約ごと削除", EpgCmds.Delete2);
            var cm_Delete3 = new CtxmItemData("予約のみ削除", EpgCmds.Delete3);
            var cm_AdjustReserve = new CtxmItemData("予約を自動登録に合わせる", EpgCmds.AdjustReserve);
            var cm_JumpTable = new CtxmItemData("番組表へジャンプ", EpgCmds.JumpTable);
            var cm_ToAutoadd = new CtxmItemData("自動予約登録", EpgCmds.ToAutoadd);
            var cm_Play = new CtxmItemData("追っかけ再生", EpgCmds.Play);
            var cm_OpenFolderMenu = new CtxmItemData("録画フォルダを開く", EpgCmdsEx.OpenFolderMenu);
            var cm_CopyTitle = new CtxmItemData("番組名をコピー", EpgCmds.CopyTitle);
            var cm_CopyContent = new CtxmItemData("番組情報をコピー", EpgCmds.CopyContent);
            var cm_SearchTitle = new CtxmItemData("番組名をネットで検索", EpgCmds.SearchTitle);

            var cm_ShowDialog = new CtxmItemData("ダイアログ表示", EpgCmds.ShowDialog);
            var cm_Separator = new CtxmItemData(EpgCmdsEx.SeparatorString, EpgCmdsEx.Separator);


            //予約追加サブメニュー 実行時、セパレータ以降にプリセットを展開する。
            cm_AddMenu.Items.Add(new CtxmItemData("ダイアログ表示", cm_ShowDialog));
            cm_AddMenu.Items.Add(new CtxmItemData(cm_Separator));
            cm_AddMenu.Items.Add(new CtxmItemData("デフォルト", EpgCmds.AddOnPreset, 0));//仮

            //予約変更サブメニューの各サブメニュー
            ////プリセット変更 実行時、サブメニューにプリセットを展開する。
            var cm_ChgOnPresetMenu = new CtxmItemData("プリセットへ変更", EpgCmdsEx.ChgOnPresetMenu);
            cm_ChgOnPresetMenu.Items.Add(new CtxmItemData("デフォルト", EpgCmds.ChgOnPreset, 0));//仮

            ////録画モード
            var cm_ChgRecmodeMenu = new CtxmItemData("録画モード", EpgCmdsEx.ChgRecmodeMenu);
            for (int i = 0; i <= 5; i++)
            {
                cm_ChgRecmodeMenu.Items.Add(new CtxmItemData(string.Format("{0} (_{1})"
                    , CommonManager.Instance.ConvertRecModeText((byte)i), i), EpgCmds.ChgRecmode, i));
            }

            ////優先度
            var cm_ChgPriorityMenu = new CtxmItemData("優先度", EpgCmdsEx.ChgPriorityMenu);
            for (int i = 1; i <= 5; i++)
            {
                cm_ChgPriorityMenu.Items.Add(new CtxmItemData(string.Format("{0} (_{0})", i)
                    , EpgCmds.ChgPriority, i));
            }

            ////イベントリレー変更
            var cm_ChgRelayMenu = new CtxmItemData("イベントリレー追従", EpgCmdsEx.ChgRelayMenu);
            for (int i = 0; i <= 1; i++)
            {
                cm_ChgRelayMenu.Items.Add(new CtxmItemData(string.Format("{0} (_{1})"
                    , CommonManager.Instance.YesNoDictionary[(byte)i].DisplayName, i), EpgCmds.ChgRelay, i));
            }

            ////ぴったり変更
            var cm_ChgPittariMenu = new CtxmItemData("ぴったり（？）録画", EpgCmdsEx.ChgPittariMenu);
            for (int i = 0; i <= 1; i++)
            {
                cm_ChgPittariMenu.Items.Add(new CtxmItemData(string.Format("{0} (_{1})"
                    , CommonManager.Instance.YesNoDictionary[(byte)i].DisplayName, i), EpgCmds.ChgPittari, i));
            }

            ////チューナー変更、実行時、セパレータ以降に一覧を展開する。
            var cm_ChgTunerMenu = new CtxmItemData("チューナー", EpgCmdsEx.ChgTunerMenu);
            cm_ChgTunerMenu.Items.Add(new CtxmItemData("自動", EpgCmds.ChgTuner, 0));
            cm_ChgTunerMenu.Items.Add(new CtxmItemData(cm_Separator));
            
            ////開始マージン
            var cm_ChgMarginStartMenu = new CtxmItemData("開始マージン", EpgCmdsEx.ChgMarginStartMenu);
            cm_ChgMarginStartMenu.Items.Add(new CtxmItemData("デフォルトへ変更", EpgCmds.ChgMarginStart, 0));
            cm_ChgMarginStartMenu.Items.Add(new CtxmItemData("指定値へ変更", EpgCmds.ChgMarginStartValue));
            cm_ChgMarginStartMenu.Items.Add(new CtxmItemData(cm_Separator));
            new List<int> { -60, -30, -5, -1, 1, 5, 30, 60 }.ForEach(val => cm_ChgMarginStartMenu.Items.Add(
                new CtxmItemData(string.Format("{0:増やす : ＋0;減らす : －0} 秒", val), EpgCmds.ChgMarginStart, val)));
            
            ////終了マージン、複製してコマンドだけ差し替える。
            var cm_ChgMarginEndMenu = new CtxmItemData("終了マージン", cm_ChgMarginStartMenu);
            cm_ChgMarginEndMenu.Command = EpgCmdsEx.ChgMarginEndMenu;
            cm_ChgMarginEndMenu.Items = cm_ChgMarginStartMenu.Items.Clone();
            cm_ChgMarginEndMenu.Items.ForEach(menu => menu.Command = menu.Command == EpgCmds.ChgMarginStart ? EpgCmds.ChgMarginEnd : menu.Command);
            cm_ChgMarginEndMenu.Items.ForEach(menu => menu.Command = menu.Command == EpgCmds.ChgMarginStartValue ? EpgCmds.ChgMarginEndValue : menu.Command);

            //予約変更サブメニュー登録
            cm_ChangeMenu.Items.Add(new CtxmItemData("ダイアログ表示", cm_ShowDialog));
            cm_ChangeMenu.Items.Add(new CtxmItemData(cm_Separator));
            cm_ChangeMenu.Items.Add(new CtxmItemData("プリセットへ変更", cm_ChgOnPresetMenu));
            cm_ChangeMenu.Items.Add(new CtxmItemData("まとめて録画設定を変更", EpgCmds.ChgBulkRecSet));
            cm_ChangeMenu.Items.Add(new CtxmItemData("まとめてジャンル絞り込みを変更", EpgCmds.ChgGenre));
            cm_ChangeMenu.Items.Add(new CtxmItemData(cm_Separator));
            cm_ChangeMenu.Items.Add(new CtxmItemData("録画モード", cm_ChgRecmodeMenu));
            cm_ChangeMenu.Items.Add(new CtxmItemData("優先度", cm_ChgPriorityMenu));
            cm_ChangeMenu.Items.Add(new CtxmItemData("イベントリレー追従", cm_ChgRelayMenu));
            cm_ChangeMenu.Items.Add(new CtxmItemData("ぴったり（？）録画", cm_ChgPittariMenu));
            cm_ChangeMenu.Items.Add(new CtxmItemData("チューナー", cm_ChgTunerMenu));
            cm_ChangeMenu.Items.Add(new CtxmItemData("開始マージン", cm_ChgMarginStartMenu));
            cm_ChangeMenu.Items.Add(new CtxmItemData("終了マージン", cm_ChgMarginEndMenu));

            CtxmData ctmd = DefCtxmData[CtxmCode.EditChgMenu];
            ctmd.Items = cm_ChangeMenu.Items;
            
            //ビューモードサブメニュー
            var cm_ViewMenu = new CtxmItemData("表示モード", EpgCmdsEx.ViewMenu);
            cm_ViewMenu.Items.Add(new CtxmItemData("表示設定", EpgCmds.ViewChgSet));
            cm_ViewMenu.Items.Add(new CtxmItemData(cm_Separator));
            for (int i = 0; i <= 2; i++)
            {
                cm_ViewMenu.Items.Add(new CtxmItemData(CommonManager.Instance.ConvertViewModeText(i)
                    + string.Format(" (_{0})", i + 1), EpgCmds.ViewChgMode, i));
            }

            //共通メニューの追加用リスト
            var AddAppendMenus = new List<CtxmItemData>();
            AddAppendMenus.Add(new CtxmItemData(cm_Separator));
            AddAppendMenus.Add(new CtxmItemData("番組名をコピー", cm_CopyTitle));
            AddAppendMenus.Add(new CtxmItemData("番組情報をコピー", cm_CopyContent));
            AddAppendMenus.Add(new CtxmItemData("番組名をネットで検索", cm_SearchTitle));

            var AddMenuSetting = new List<CtxmItemData>();
            AddMenuSetting.Add(new CtxmItemData(cm_Separator));
            AddMenuSetting.Add(new CtxmItemData("右クリックメニューの設定...", EpgCmds.MenuSetting));


            //メニューアイテム:予約一覧
            ctmd = DefCtxmData[CtxmCode.ReserveView];
            ctmd.Items.Add(new CtxmItemData("予約←→無効", cm_ChangeOnOff));
            ctmd.Items.Add(new CtxmItemData("変更", cm_ChangeMenu));
            ctmd.Items.Add(new CtxmItemData("削除", cm_Delete));
            ctmd.Items.Add(new CtxmItemData("プログラム予約追加", cm_ShowAddDialog));
            ctmd.Items.Add(new CtxmItemData("番組表へジャンプ", cm_JumpTable));
            ctmd.Items.Add(new CtxmItemData("自動予約登録", cm_ToAutoadd));
            ctmd.Items.Add(new CtxmItemData("追っかけ再生", cm_Play));
            ctmd.Items.Add(new CtxmItemData("録画フォルダを開く", cm_OpenFolderMenu));
            ctmd.Items.AddRange(AddAppendMenus.Clone());
            ctmd.Items.AddRange(AddMenuSetting.Clone());

            //メニューアイテム:使用予定チューナー
            ctmd = DefCtxmData[CtxmCode.TunerReserveView];
            ctmd.Items.Add(new CtxmItemData("変更", cm_ChangeMenu));
            ctmd.Items.Add(new CtxmItemData("削除", cm_Delete));
            ctmd.Items.Add(new CtxmItemData("プログラム予約追加", cm_ShowAddDialog));
            ctmd.Items.Add(new CtxmItemData("番組表へジャンプ", cm_JumpTable));
            ctmd.Items.Add(new CtxmItemData("自動予約登録", cm_ToAutoadd));
            ctmd.Items.Add(new CtxmItemData("追っかけ再生", cm_Play));
            ctmd.Items.Add(new CtxmItemData("録画フォルダを開く", cm_OpenFolderMenu));
            ctmd.Items.AddRange(AddAppendMenus.Clone());
            ctmd.Items.AddRange(AddMenuSetting.Clone());

            //メニューアイテム:録画済み一覧
            ctmd = DefCtxmData[CtxmCode.RecInfoView];
            ctmd.Items.Add(new CtxmItemData("録画情報", cm_ShowDialog));
            ctmd.Items.Add(new CtxmItemData("削除", cm_Delete));
            ctmd.Items.Add(new CtxmItemData("プロテクト←→解除", EpgCmds.ProtectChange));
            ctmd.Items.Add(new CtxmItemData("自動予約登録", cm_ToAutoadd));
            ctmd.Items.Add(new CtxmItemData("再生", cm_Play));
            ctmd.Items.Add(new CtxmItemData("録画フォルダを開く", EpgCmds.OpenFolder));//他の画面と違う
            ctmd.Items.AddRange(AddAppendMenus.Clone());
            ctmd.Items.AddRange(AddMenuSetting.Clone());

            //メニューアイテム:EPG自動予約登録
            ctmd = DefCtxmData[CtxmCode.EpgAutoAddView];
            ctmd.Items.Add(new CtxmItemData("変更", cm_ChangeMenu));
            ctmd.Items.Add(new CtxmItemData("削除", cm_Delete));
            ctmd.Items.Add(new CtxmItemData("予約ごと削除", cm_Delete2));
            ctmd.Items.Add(new CtxmItemData("予約のみ削除", cm_Delete3));
            ctmd.Items.Add(new CtxmItemData("予約を自動登録に合わせる", cm_AdjustReserve));
            ctmd.Items.Add(new CtxmItemData("自動予約登録を追加", cm_ShowAddDialog));
            ctmd.Items.Add(new CtxmItemData("録画フォルダを開く", cm_OpenFolderMenu));
            ctmd.Items.Add(new CtxmItemData(cm_Separator));
            ctmd.Items.Add(new CtxmItemData("Andキーワードをコピー", cm_CopyTitle));
            ctmd.Items.Add(new CtxmItemData("Andキーワードをネットで検索", cm_SearchTitle));
            ctmd.Items.Add(new CtxmItemData("Notキーワードをコピー", EpgCmds.CopyNotKey));
            ctmd.Items.Add(new CtxmItemData("Notキーワードに貼り付け", EpgCmds.SetNotKey));
            ctmd.Items.AddRange(AddMenuSetting.Clone());

            //メニューアイテム:プログラム自動予約登録
            ctmd = DefCtxmData[CtxmCode.ManualAutoAddView];
            ctmd.Items.Add(new CtxmItemData("変更", cm_ChangeMenu));
            ctmd.Items.Add(new CtxmItemData("削除", cm_Delete));
            ctmd.Items.Add(new CtxmItemData("予約ごと削除", cm_Delete2));
            ctmd.Items.Add(new CtxmItemData("予約のみ削除", cm_Delete3));
            ctmd.Items.Add(new CtxmItemData("予約を自動登録に合わせる", cm_AdjustReserve));
            ctmd.Items.Add(new CtxmItemData("自動予約登録を追加", cm_ShowAddDialog));
            ctmd.Items.Add(new CtxmItemData("録画フォルダを開く", cm_OpenFolderMenu));
            ctmd.Items.Add(new CtxmItemData(cm_Separator));
            ctmd.Items.Add(new CtxmItemData("番組名をコピー", cm_CopyTitle));
            ctmd.Items.Add(new CtxmItemData("番組名をネットで検索", cm_SearchTitle));
            ctmd.Items.AddRange(AddMenuSetting.Clone());

            //メニューアイテム:番組表
            ctmd = DefCtxmData[CtxmCode.EpgView];
            ctmd.Items.Add(new CtxmItemData("簡易予約/予約←→無効", cm_ChangeOnOff));
            ctmd.Items.Add(new CtxmItemData("予約追加", cm_AddMenu));
            ctmd.Items.Add(new CtxmItemData("変更", cm_ChangeMenu));
            ctmd.Items.Add(new CtxmItemData("削除", cm_Delete));
            ctmd.Items.Add(new CtxmItemData("番組表(標準モード)へジャンプ", cm_JumpTable));
            ctmd.Items.Add(new CtxmItemData("自動予約登録", cm_ToAutoadd));
            ctmd.Items.Add(new CtxmItemData("追っかけ再生", cm_Play));
            ctmd.Items.Add(new CtxmItemData("録画フォルダを開く", cm_OpenFolderMenu));
            ctmd.Items.AddRange(AddAppendMenus.Clone());
            ctmd.Items.Add(new CtxmItemData(cm_Separator));
            ctmd.Items.Add(new CtxmItemData("表示モード", cm_ViewMenu));
            ctmd.Items.AddRange(AddMenuSetting.Clone());

            //メニューアイテム:検索ダイアログ、EPG予約ダイアログ
            ctmd = DefCtxmData[CtxmCode.SearchWindow];
            ctmd.Items.Add(new CtxmItemData("簡易予約/予約←→無効", cm_ChangeOnOff));
            ctmd.Items.Add(new CtxmItemData("予約追加", cm_AddMenu));
            ctmd.Items.Add(new CtxmItemData("変更", cm_ChangeMenu));
            ctmd.Items.Add(new CtxmItemData("削除", cm_Delete));
            ctmd.Items.Add(new CtxmItemData("番組表へジャンプ", cm_JumpTable));
            ctmd.Items.Add(new CtxmItemData("番組名で再検索", EpgCmds.ReSearch));
            ctmd.Items.Add(new CtxmItemData("番組名で再検索(サブウィンドウ)", EpgCmds.ReSearch2));
            ctmd.Items.Add(new CtxmItemData("追っかけ再生", cm_Play));
            ctmd.Items.Add(new CtxmItemData("録画フォルダを開く", cm_OpenFolderMenu));
            ctmd.Items.AddRange(AddAppendMenus.Clone());
            ctmd.Items.AddRange(AddMenuSetting.Clone());
        }
        private void SetDefGestureCmdList()
        {
            DefCtxmCmdList = new Dictionary<CtxmCode, List<ICommand>>();
            foreach (var ctxm in DefCtxmData)
            {
                DefCtxmCmdList.Add(ctxm.Key, GetCmdFromMenuItem(ctxm.Value.Items).Distinct().ToList());
            }
        }
        private List<ICommand> GetCmdFromMenuItem(List<CtxmItemData> items)
        {
            var clist = new List<ICommand>();
            items.ForEach(item =>
            {
                if (item != null && EpgCmdsEx.IsDummyCmd(item.Command) == false)
                {
                    clist.Add(item.Command);
                }
                clist.AddRange(GetCmdFromMenuItem(item.Items));
            });
            return clist;
        }

        public void ReloadWorkData()
        {
            LastMenuSettingTime = DateTime.Now;

            MC.SetWorkCmdOptions();
            SetWorkCxtmData();
            SetWorkGestureCmdList();

            //簡易設定側の、修正済みデータの書き戻し
            Settings.Instance.MenuSet = GetWorkMenuSettingData();
        }
        private void SetWorkCxtmData()
        {
            WorkCtxmData = new Dictionary<CtxmCode, CtxmData>();
            foreach (var data in DefCtxmData.Values) { WorkCtxmData.Add(data.ctxmCode, GetWorkCtxmDataView(data.ctxmCode)); }

            //編集メニューの差し替え
            foreach (CtxmData ctxm in WorkCtxmData.Values)
            {
                foreach (var chgMenu in ctxm.Items.Where(item => item.Command == EpgCmdsEx.ChgMenu))
                {
                    chgMenu.Items = WorkCtxmData[CtxmCode.EditChgMenu].Items.Clone();
                }
            }
        }
        private CtxmData GetWorkCtxmDataView(CtxmCode code)
        {
            CtxmData ctxm = new CtxmData(code);
            CtxmData ctxmDef = DefCtxmData[code];

            //存在するものをコピーしていく。編集メニューは常に個別設定が有効になる。
            if (Settings.Instance.MenuSet.IsManualMenu == true || code == CtxmCode.EditChgMenu)
            {
                CtxmSetting ctxmEdited = Settings.Instance.MenuSet.ManualMenuItems.FindData(code);
                if (ctxmEdited == null)
                {
                    //編集サブメニューの場合は、初期無効アイテムを削除したデフォルトセッティングを返す。
                    return code != CtxmCode.EditChgMenu ? ctxmDef.Clone() : GetDefaultChgSubMenuCtxmData();
                }

                ctxmEdited.Items.ForEach(setMenuString =>
                {
                    CtxmItemData item1 = ctxmDef.Items.Find(item => item.Header == setMenuString);
                    if (item1 != null)
                    {
                        ctxm.Items.Add(item1.Clone());
                    }
                });
            }
            else
            {
                ctxmDef.Items.ForEach(item1 =>
                {
                    if (MC.WorkCmdOptions[item1.Command].IsMenuEnabled == true)
                    {
                        ctxm.Items.Add(item1.Clone());
                    }
                });
            }

            //・連続したセパレータの除去
            for (int i = ctxm.Items.Count - 1; i >= 1; i--)
            {
                if (ctxm.Items[i].Command == EpgCmdsEx.Separator && ctxm.Items[i - 1].Command == EpgCmdsEx.Separator)
                {
                    ctxm.Items.RemoveAt(i);
                }
            }
            //・先頭と最後のセパレータ除去
            if (ctxm.Items.Count != 0 && ctxm.Items[ctxm.Items.Count - 1].Command == EpgCmdsEx.Separator)
            {
                ctxm.Items.RemoveAt(ctxm.Items.Count - 1);
            }
            if (ctxm.Items.Count != 0 && ctxm.Items[0].Command == EpgCmdsEx.Separator)
            {
                ctxm.Items.RemoveAt(0);
            }

            return ctxm;
        }
        private void SetWorkGestureCmdList()
        {
            //WorCtxmCmdList = new Dictionary<CtxmCode, List<ICommand>>();
            WorkGestureCmdList = new Dictionary<CtxmCode, List<ICommand>>();
            WorkDelGesCmdList = new Dictionary<CtxmCode, List<ICommand>>();

            foreach (var ctxm in WorkCtxmData)
            {
                var cmdlist = GetCmdFromMenuItem(ctxm.Value.Items);
                //var cmdlist = GetCmdFromMenuItem(ctxm.Value.Items).Distinct().ToList();

                //コンテキストメニューのコマンドリスト
                //WorCtxmCmdList.Add(ctxm.Key, cmdlist.ToList());

                //常時有効なショートカットのあるコマンドを追加
                cmdlist.AddRange(DefCtxmCmdList[ctxm.Key].Where(command => MC.WorkCmdOptions[command].IsGesNeedMenu == false));
                cmdlist = cmdlist.Distinct().ToList();

                //無効なコマンドを除外
                cmdlist = cmdlist.Where(command => MC.WorkCmdOptions[command].IsGestureEnabled == true).ToList();

                WorkGestureCmdList.Add(ctxm.Key, cmdlist);

                //ショートカット無効なコマンドリスト
                WorkDelGesCmdList.Add(ctxm.Key, DefCtxmCmdList[ctxm.Key].Except(cmdlist).ToList());
            }
        }

        //設定画面へデフォルト値を送る
        public MenuSettingData GetDefaultMenuSettingData()
        {
            var set = new MenuSettingData();
            set.EasyMenuItems = MC.GetDefEasyMenuSetting();
            set.ManualMenuItems = GetManualMenuSetting(DefCtxmData);
            //編集メニュー初期値の設定、差し替え
            set.ManualMenuItems.RemoveAll(item => item.ctxmCode == CtxmCode.EditChgMenu);
            set.ManualMenuItems.Add(new CtxmSetting(GetDefaultChgSubMenuCtxmData()));
            return set;
        }
        //設定画面へメニュー編集選択画面用の全てを含む初期値を送る
        public List<CtxmSetting> GetDefaultCtxmSettingForEditor()
        {
            return GetManualMenuSetting(DefCtxmData);
        }
        private CtxmData GetDefaultChgSubMenuCtxmData()
        {
            var set = new CtxmData(CtxmCode.EditChgMenu);
            DefCtxmData[CtxmCode.EditChgMenu].Items.ForEach(item =>
            {
                //初期無効データは入れない
                if (MC.DefCmdOptions[item.Command].IsMenuEnabled == true)
                {
                    set.Items.Add(item.Clone());
                }
            });
            return set;
        }
        private MenuSettingData GetWorkMenuSettingData()
        {
            var set = Settings.Instance.MenuSet.Clone();
            set.EasyMenuItems = MC.GetWorkEasyMenuSetting();
            if (set.IsManualMenu == true)
            {
                set.ManualMenuItems = GetManualMenuSetting(WorkCtxmData);
            }
            else if (set.ManualMenuItems.Count != Enum.GetValues(typeof(CtxmCode)).Length)
            {
                set.ManualMenuItems = GetManualMenuSetting(DefCtxmData);
            }

            //編集メニューは常にマニュアルモードと同等のため差し替える。
            set.ManualMenuItems.RemoveAll(item => item.ctxmCode == CtxmCode.EditChgMenu);
            set.ManualMenuItems.Add(new CtxmSetting(WorkCtxmData[CtxmCode.EditChgMenu]));

            return set;
        }
        private List<CtxmSetting> GetManualMenuSetting(Dictionary<CtxmCode, CtxmData> dic)
        {
            var cmManualMenuSetting = new List<CtxmSetting>();
            foreach (CtxmCode code in Enum.GetValues(typeof(CtxmCode)))
            {
                cmManualMenuSetting.Add(new CtxmSetting(dic[code]));
            }
            return cmManualMenuSetting;
        }

        public bool IsGestureEnableOnView(ICommand icmd, CtxmCode code)
        {
            if (icmd == null) return false;
            return WorkDelGesCmdList[code].Contains(icmd);
        }

        public List<ICommand> GetWorkGestureCmdList(CtxmCode code)
        {
            return WorkGestureCmdList[code].ToList();
        }

        public void CtxmGenerateContextMenu(ContextMenu ctxm, CtxmCode code, bool shortcutTextforListType)
        {
            try
            {
                ctxm.Name = code.ToString();
                CtxmConvertToMenuItems(WorkCtxmData[code].Items, ctxm.Items, code, shortcutTextforListType);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
        private void CtxmConvertToMenuItems(List<CtxmItemData> src, ItemCollection dest, CtxmCode code, bool shortcutTextforListType)
        {
            dest.Clear();

            src.ForEach(data =>
            {
                Control item;
                if (data.Command == EpgCmdsEx.Separator)
                {
                    item = new Separator();
                }
                else
                {
                    var menu = new MenuItem();
                    menu.Header = data.Header;
                    menu.Command = (EpgCmdsEx.IsDummyCmd(data.Command) ? null : data.Command);
                    if (menu.Command != null)
                    {
                        if ((shortcutTextforListType == true || (MC.WorkCmdOptions[data.Command].GesTrg & MenuCmds.GestureTrg.ToView) == MenuCmds.GestureTrg.ToView) 
                            && (MC.WorkCmdOptions.ContainsKey(data.Command) == false || MC.WorkCmdOptions[data.Command].IsGestureEnabled == true)
                            && data.ID == 0)
                        {
                            menu.InputGestureText = MenuBinds.GetInputGestureText(data.Command);
                        }
                    }
                    menu.CommandParameter = new EpgCmdParam(typeof(MenuItem), code, data.ID);
                    CtxmConvertToMenuItems(data.Items, menu.Items, code, shortcutTextforListType);
                    item = menu;
                }
                item.Name = data.Name;
                item.Tag = data.Command;
                item.ToolTip = GetCtxmTooltip(data.Command);
                ToolTipService.SetShowOnDisabled(item, true);

                dest.Add(item);
            });
        }
        private string GetCtxmTooltip(ICommand icmd)
        {
            MenuUtil mutil = CommonManager.Instance.MUtil;
            if (icmd == EpgCmds.ToAutoadd || icmd == EpgCmds.ReSearch || icmd == EpgCmds.ReSearch2)
            {
                return mutil.EpgKeyword_TrimMode();
            }
            else if (icmd == EpgCmds.CopyTitle)
            {
                return mutil.CopyTitle_TrimMode();
            }
            else if (icmd == EpgCmds.CopyContent)
            {
                return mutil.CopyContent_Mode();
            }
            else if (icmd == EpgCmds.SearchTitle)
            {
                return mutil.SearchText_TrimMode();
            }
            return null;
        }

        public void CtxmGenerateAddOnPresetItems(MenuItem menu) { CtxmGenerateOnPresetItems(menu, EpgCmds.AddOnPreset); }
        public void CtxmGenerateChgOnPresetItems(MenuItem menu) { CtxmGenerateOnPresetItems(menu, EpgCmds.ChgOnPreset); }
        public void CtxmGenerateOnPresetItems(MenuItem menu, ICommand icmd)
        {
            if (menu.IsEnabled == false) return;

            var delList = menu.Items.OfType<MenuItem>().Where(item => item.Command == icmd).ToList();
            delList.ForEach(item => menu.Items.Remove(item));

            foreach (var item in Settings.Instance.RecPresetList.Select((info, id) => new { info, id }))
            {
                var menuItem = new MenuItem();
                menuItem.Header = string.Format("プリセット - {0} (_{1})", item.info.DisplayName, item.id);
                menuItem.Command = icmd;
                menuItem.CommandParameter = new EpgCmdParam(menu.CommandParameter as EpgCmdParam);
                (menuItem.CommandParameter as EpgCmdParam).ID = (int)item.info.ID;
                menuItem.Tag = menuItem.Command;
                menu.Items.Add(menuItem);
            }
        }

        public void CtxmGenerateTunerMenuItems(MenuItem menu)
        {
            if (menu.IsEnabled == false) return;

            var delList = menu.Items.OfType<MenuItem>().Where(item => (item.CommandParameter as EpgCmdParam).ID != 0).ToList();
            delList.ForEach(item => menu.Items.Remove(item));

            foreach (var info in CommonManager.Instance.DB.TunerReserveList.Values.Where(info => info.tunerID != 0xFFFFFFFF)
                .Select(info => new TunerSelectInfo(info.tunerName, info.tunerID)))
            {
                    var menuItem = new MenuItem();
                    menuItem.Header = string.Format("{0}", info);
                    menuItem.Command = EpgCmds.ChgTuner;
                    menuItem.CommandParameter = new EpgCmdParam(menu.CommandParameter as EpgCmdParam);
                    (menuItem.CommandParameter as EpgCmdParam).ID = (int)info.ID;
                    menuItem.Tag = menuItem.Command;
                    menu.Items.Add(menuItem);
                }
        }

        public void CtxmGenerateOpenFolderItems(MenuItem menu, RecSettingData recSetting = null)
        {
            if (menu.IsEnabled == false) return;

            menu.Items.Clear();

            if (recSetting != null && recSetting.RecFolderList.Count != 0)
            {
                recSetting.RecFolderList.ForEach(info => CtxmGenerateOpenFolderItem(menu, info.RecFolder));
            }
            else
            {
                Settings.GetDefRecFolders().ForEach(folder => CtxmGenerateOpenFolderItem(menu, folder, "(デフォルト) "));
            }

            if (recSetting != null)
            {
                recSetting.PartialRecFolder.ForEach(info => CtxmGenerateOpenFolderItem(menu, info.RecFolder, "(ワンセグ) "));
            }
        }
        private void CtxmGenerateOpenFolderItem(MenuItem menu, string path, string header_exp = "")
        {
            var menuItem = new MenuItem();
            menuItem.Header = header_exp + path;
            menuItem.Command = EpgCmds.OpenFolder;
            menuItem.CommandParameter = new EpgCmdParam(menu.CommandParameter as EpgCmdParam);
            (menuItem.CommandParameter as EpgCmdParam).Data = path;
            menuItem.Tag = menuItem.Command;
            menu.Items.Add(menuItem);
        }

    }
}

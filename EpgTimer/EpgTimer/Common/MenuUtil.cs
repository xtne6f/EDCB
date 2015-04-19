using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Diagnostics;
using System.Windows;
using System.Windows.Input;
using System.Windows.Controls;
using System.Collections;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    public class MenuUtil
    {
        private CtrlCmdUtil cmd = null;

        public MenuUtil(CtrlCmdUtil ctrlCmd)
        {
            cmd = ctrlCmd;
        }

        public string TrimEpgKeyword(string txtKey, bool NotToggle = false)//NotToggleはショートカット用
        {
            string txtKey1 = txtKey;
            bool setting = Settings.Instance.CmEpgKeyword_Trim;
            if (Keyboard.Modifiers == ModifierKeys.Shift && NotToggle == false)
            {
                setting=!setting;
            }

            if (setting == true)
            {
                txtKey1 = TrimKeyword(txtKey1);
            }

            return txtKey1;
        }

        public string EpgKeyword_TrimMode()
        {
            return TrimModeTooltip(Settings.Instance.CmEpgKeyword_Trim);
        }

        private string TrimModeTooltip(bool mode)
        {
            string str_mode = (mode == true ? "オン" : "オフ");
            string str_mode_toggle = (mode == false ? "'オン'" : "'オフ'");
            return "記号除去モード : " + str_mode + " (Shift+クリックで一時的に" + str_mode_toggle + ")";
        }

        public void CopyTitle2Clipboard(string txtTitle, bool NotToggle = false)
        {
            string txtTitle1 = txtTitle;
            bool setting = Settings.Instance.CmCopyTitle_Trim;
            if (Keyboard.Modifiers == ModifierKeys.Shift && NotToggle == false)
            {
                setting = !setting;
            }

            if (setting == true)
            {
                txtTitle1 = TrimKeyword(txtTitle1);
            }

            Clipboard.SetDataObject(txtTitle1, true);
        }

        public string CopyTitle_TrimMode()
        {
            return TrimModeTooltip(Settings.Instance.CmCopyTitle_Trim);
        }

        public void CopyContent2Clipboard(EpgEventInfo eventInfo, bool NotToggle = false)
        {
            string text = "";

            if (eventInfo != null)
            {
                bool setting = Settings.Instance.CmCopyContentBasic;
                if (Keyboard.Modifiers == ModifierKeys.Shift && NotToggle == false)
                {
                    setting = !setting;
                }

                if (setting == true)
                {
                    //text = eventInfo.ShortInfo.text_char;
                    text = CommonManager.Instance.ConvertProgramText(eventInfo, EventInfoTextMode.BasicOnly);
                }
                else
                {
                    text = CommonManager.Instance.ConvertProgramText(eventInfo, EventInfoTextMode.All);
                }

                text = text.TrimEnd() + "\r\n";
            }

            Clipboard.SetDataObject(text, true);
        }

        public void CopyContent2Clipboard(ReserveData resInfo, bool NotToggle = false)
        {
            CopyContent2Clipboard(CommonManager.Instance.GetEpgEventInfoFromReserveData(resInfo, true), NotToggle);
        }

        public void CopyContent2Clipboard(RecInfoItem recInfo, bool NotToggle = false)
        {
            string text = "";

            if (recInfo != null)
            {
                bool setting = Settings.Instance.CmCopyContentBasic;
                if (Keyboard.Modifiers == ModifierKeys.Shift && NotToggle == false)
                {
                    setting = !setting;
                }

                if (setting == true)
                {
                    string[] stArrayData = recInfo.RecInfo.ProgramInfo.Replace("\r\n", "\n").Split('\n');
                    int endI = Math.Min(stArrayData.Length, 3);

                    for (int i = 0; i < endI; i++)
                    {
                        text += stArrayData[i] + "\r\n";
                    }
                }
                else
                {
                    text = recInfo.RecInfo.ProgramInfo;
                }

                text = text.TrimEnd() + "\r\n";
            }

            Clipboard.SetDataObject(text, true);
        }

        public string CopyContent_Mode()
        {
            string mode = (Settings.Instance.CmCopyContentBasic == true ? "基本情報のみ" : "詳細情報");
            string mode_toggle = (Settings.Instance.CmCopyContentBasic == false ? "'基本情報のみ'" : "'詳細情報'");
            return "取得モード : " + mode + " (Shift+クリックで一時的に" + mode_toggle + ")";
        }

        public void SearchText(string txtKey, bool NotToggle = false)
        {
            string txtKey1 = txtKey;
            bool setting = Settings.Instance.CmSearchTitle_Trim;
            if (Keyboard.Modifiers == ModifierKeys.Shift && NotToggle == false)
            {
                setting = !setting;
            }

            if (setting == true)
            {
                txtKey1 = TrimKeyword(txtKey1);
            }

            string txtURI = Settings.Instance.CmSearchURI;
            txtURI += UrlEncode(txtKey1, System.Text.Encoding.UTF8);

            try
            {
                System.Diagnostics.Process.Start(txtURI);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                MessageBox.Show("'検索のURI'の設定を確認してください。");
            }
        }

        public string SearchText_TrimMode()
        {
            return TrimModeTooltip(Settings.Instance.CmSearchTitle_Trim);
        }

        public string TrimKeyword(string txtKey)
        {
            //
            // 前後の記号を取り除く
            //
            // [二][字]NHKニュース7
            // ５．１[SS][字]NHK歌謡コンサート「人生の旅路に　この歌を」
            // 午後のロードショー「私がウォシャウスキー」魅惑のヒロイン特集[字][S][二]
            // 【Ｍｏｔｈｅｒ’ｓ　Ｄａｙ　Ｓｐｅｃｉａｌ】【映】バンガー・シスターズ
            string markExp1 =
                "(" +
                    "(\\[[^\\]]+\\])+" +
                    "|" +
                    "(【[^】]+】)+" +
                    "|" +
                    "(［[^］]+］)+" +
                ")";
            string[] exp = {
                                "^((５．１)|(5.1)|" + markExp1 + ")+", // 先頭の記号
                                "^(([#＃][\\d０-９]+)|(第[\\d０-９]+話))", // 先頭にある話数の除去
                                "^[\\(（][\\d０-９]+[\\)）]\\s*「[^」]+」", // 先頭にある話数の除去
                                "(([#＃][\\d０-９]+)|(第[\\d０-９]+話)).*", // ドラマ等の話数から後ろ全て
                                "[\\(（][\\d０-９]+[\\)）]\\s*「[^」]+」.*", // ドラマ等の話数から後ろ全て その2
                                //"「[^」]+」.*", // ドラマ等のサブタイトルから後ろ全て、ちょっと強すぎるかも
                                "<[^>]+>", // NHK・フジテレビが使う補足
                                "＜[^＞]+＞", // NHK・フジテレビが使う補足
                                "[◆▽].*", // タイトルに埋め込まれた番組説明
                                "[\\[［【\\(（][^\\]］】\\)）]*リマスター[^\\]］】\\)）]*[\\]］】\\)）]",// ときどき見かけるので1
                                "(((HD)|(ＨＤ)|(ハイビジョン)|(デジタル))リマスター((HD)|(ＨＤ))?版?)|(リマスター((HD)|(ＨＤ)|版)版?)",// 同上、括弧無しは特定パタンのみ
                                markExp1 + "$" // 末尾の記号
                                };
            foreach (string str1 in exp)
            {
                txtKey = Regex.Replace(txtKey, str1, string.Empty).Trim();
            }
            // 映画のタイトル抽出
            // TODO:正規表現を設定ファイルで変更可能にする
            string[] titleExp = {
                                "^「(?<Title>[^」]+)」",
                                "((サタ☆シネ)|(シネマズ?)|(シアター)|(プレミア)|(ロードショー)|(ロードSHOW!)|(午後ロード)|(木曜デラックス)|(映画天国))\\s*「(?<Title>[^」]+)」",
                                "((シネマ)|(キネマ)).*『(?<Title>[^』]+)』"
                                };
            foreach (string str1 in titleExp)
            {
                Match m = Regex.Match(txtKey, str1);
                if (m.Success == true)
                {
                    txtKey = m.Groups["Title"].Value;
                    break;
                }
            }

            return txtKey;
        }

        //
        // HttpUtility を使わないUrlEncodeの実装
        // From http://d.hatena.ne.jp/kazuv3/20080605/1212656674
        //
        public string UrlEncode(string s, System.Text.Encoding enc)
        {
            System.Text.StringBuilder rt = new System.Text.StringBuilder();
            foreach (byte i in enc.GetBytes(s))
                if (i == 0x20)
                    rt.Append('+');
                else if (i >= 0x30 && i <= 0x39 || i >= 0x41 && i <= 0x5a || i >= 0x61 && i <= 0x7a)
                    rt.Append((char)i);
                else
                    rt.Append("%" + i.ToString("X2"));
            return rt.ToString();
        }
        
        public string MarginStartText(RecSettingData recSetting)
        {
            string view = "";
            if (recSetting != null)
            {
                int marginTime;
                if (recSetting.UseMargineFlag == 1)
                {
                    marginTime = recSetting.StartMargine;
                }
                else
                {
                    //TODO: ここでデフォルトマージンを確認するがEpgTimerNWでは無意味。根本的にはSendCtrlCmdの拡張が必要
                    marginTime = IniFileHandler.GetPrivateProfileInt("SET", "StartMargin", 0, SettingPath.TimerSrvIniPath);
                }
                view = CustomTimeFormat(marginTime * -1, recSetting.UseMargineFlag);
            }
            return view;
        }

        public string MarginEndText(RecSettingData recSetting)
        {
            string view = "";
            if (recSetting != null)
            {
                int marginTime;
                if (recSetting.UseMargineFlag == 1)
                {
                    marginTime = recSetting.EndMargine;
                }
                else
                {
                    //TODO: ここでデフォルトマージンを確認するがEpgTimerNWでは無意味。根本的にはSendCtrlCmdの拡張が必要
                    marginTime = IniFileHandler.GetPrivateProfileInt("SET", "EndMargin", 0, SettingPath.TimerSrvIniPath);
                }
                view = CustomTimeFormat(marginTime, recSetting.UseMargineFlag);
            }
            return view;
        }

        private string CustomTimeFormat(int span, byte useMarginFlag)
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
            return span.ToString("+;-") + hours + minutes + seconds + CustomTimeMark(useMarginFlag);
        }

        private string CustomTimeMark(byte useMarginFlag)
        {
            //EpgtimerNWの場合、デフォルト値不明のため。不明でなくなったら要らない
            string mark = "";
            if (CommonManager.Instance.NWMode == true)
            {
                mark = (useMarginFlag == 1 ? " " : "?");
            }
            return mark;
        }
        
        public TextBlock GetTooltipBlockStandard(string text)
        {
            TextBlock block = new TextBlock();
            block.Text = text;
            block.MaxWidth = 400;
            block.TextWrapping = TextWrapping.Wrap;
            return block;
        }

        public List<T> GetList<T>(T item)
        {
            return new T[]{item}.ToList();
        }

        /// <summary>
        /// 変換エラーの場合、デフォルト値を返し、テキストボックスの内容をデフォルト値に置き換える。
        /// </summary>
        public T MyToNumerical<T>(TextBox box, Func<string, T> converter, T defValue = default(T))
        {
            try
            {
                return converter(box.Text.ToString());
            }
            catch
            {
                box.Text = defValue.ToString();
                return defValue;
            }
        }
        
        //取りあえずある程度集約。仮。各画面のコンテキストメニュー共通化したい。

        /// <summary>
        /// プリセットメニューの展開、リスト系でもパネル系でも使用
        /// </summary>
        public void ExpandPresetItems(MenuItem menu, Action<object, RoutedEventArgs> eventHandler)
        {
            int bar_pos = SearchMenuPosByName(menu, "bar_preset");
            while (menu.Items.Count - 1 >= bar_pos + 1)
            {
                menu.Items.RemoveAt(menu.Items.Count - 1);
            }

            int i = 0;
            foreach (RecPresetItem info in Settings.Instance.RecPresetList)
            {
                var menuItem = new MenuItem();
                menuItem.Header = string.Format("プリセット - {0} (_{1})", info.DisplayName, i++);
                menuItem.DataContext = info.ID;
                menuItem.Click += new RoutedEventHandler(eventHandler);

                menu.Items.Add(menuItem);
            }
        }

        /// <summary>
        /// パネル系の右クリックメニュー　予約追加
        /// </summary>
        public MenuItem GenerateAddMenu(Action<object, RoutedEventArgs>[] eventHandler)
        {
            var menuItemAdd = new MenuItem();
            menuItemAdd.Header = "予約追加";

            var menuItemAddDlg = new MenuItem();
            menuItemAddDlg.Header = "ダイアログ表示";
            menuItemAddDlg.Click += new RoutedEventHandler(eventHandler[0]);

            menuItemAdd.Items.Add(menuItemAddDlg);

            //プリセットメニュー作成
            var menuItemPresetBar = new Separator();
            menuItemPresetBar.Name = "bar_preset";
            menuItemAdd.Items.Add(menuItemPresetBar);
            ExpandPresetItems(menuItemAdd, eventHandler[1]);

            return menuItemAdd;
        }

        /// <summary>
        /// パネル系の右クリックメニュー　予約変更
        /// </summary>
        public MenuItem GenerateChgMenu(Action<object, RoutedEventArgs>[] eventHandler)
        {
            var menuItemChg = new MenuItem();
            menuItemChg.Header = "変更";

            var menuItemChgDlg = new MenuItem();
            menuItemChgDlg.Header = "ダイアログ表示";
            menuItemChgDlg.Click += new RoutedEventHandler(eventHandler[0]);
            menuItemChg.Items.Add(menuItemChgDlg);

            //録画モードメニュー作成
            var menuItemRecmodeBar = new Separator();
            menuItemRecmodeBar.Name = "bar_recmode";
            menuItemChg.Items.Add(menuItemRecmodeBar);
            GenerateRecModeItems(menuItemChg, eventHandler[1]);

            //優先度モードメニュー作成
            menuItemChg.Items.Add(new Separator());
            menuItemChg.Items.Add(GeneratePriorityItems(eventHandler[2]));

            return menuItemChg;
        }

        public void GenerateRecModeItems(MenuItem menu, Action<object, RoutedEventArgs> eventHandler)
        {
            string[] HeaderStr = {   "全サービス",
                                     "指定サービス",
                                     "全サービス（デコード処理なし）",
                                     "指定サービス（デコード処理なし）",
                                     "視聴",
                                     "無効"};

            for (int i = 0; i <= 5; i++)
            {
                var menuItem = new MenuItem();
                menuItem.Header = string.Format("{0} (_{1})", HeaderStr[i], i);
                menuItem.DataContext = (uint)i;//今は無くても大丈夫
                menuItem.Click += new RoutedEventHandler(eventHandler);

                menu.Items.Add(menuItem);
            }
        }

        public MenuItem GeneratePriorityItems(Action<object, RoutedEventArgs> eventHandler)
        {
            var menuPri = new MenuItem();
            menuPri.Name = "cm_pri";

            for (int i = 1; i <= 5; i++)
            {
                var menuItem = new MenuItem();
                menuItem.Header = string.Format("{0} (_{0})", i);
                menuItem.DataContext = (uint)i;//今は無くても大丈夫
                menuItem.Click += new RoutedEventHandler(eventHandler);

                menuPri.Items.Add(menuItem);
            }

            return menuPri;
        }

        private int SearchMenuPosByName(MenuItem menu, string name)
        {
            for (int i = 0; i < menu.Items.Count; i++)
            {
                if ((menu.Items[i] as Control).Name == name)
                {
                    return i;
                }
            }
            return -1;
        }
        
        public void CheckChgItems(MenuItem menu, List<SearchItem> list)
        {
            CheckChgItems(menu, list.ReserveInfoList());
        }
        public void CheckChgItems(MenuItem menu, List<ReserveData> list)
        {
            if (list.Count == 0) return;

            //選択されているすべての予約が同じ設定の場合だけチェックを表示する
            byte recMode = list.All(info => info.RecSetting.RecMode == list[0].RecSetting.RecMode) ? list[0].RecSetting.RecMode : (byte)0xFF; ;
            byte priority = list.All(info => info.RecSetting.Priority == list[0].RecSetting.Priority) ? list[0].RecSetting.Priority : (byte)0xFF; ;

            int bar_pos = SearchMenuPosByName(menu, "bar_recmode");
            if (bar_pos < 0) return;

            for (int i = 0; i <= 5; i++)
            {
                ((MenuItem)menu.Items[i + bar_pos + 1]).DataContext = (uint)i;
                ((MenuItem)menu.Items[i + bar_pos + 1]).IsChecked = (i == recMode);
            }

            int pri_pos = SearchMenuPosByName(menu, "cm_pri");
            if (pri_pos < 0) return;

            MenuItem priMenu = menu.Items[pri_pos] as MenuItem;
            priMenu.Header = string.Format("優先度 {0}", priority < 0xFF ? "" + priority : "*");
            for (int i = 1; i <= priMenu.Items.Count; i++)
            {
                ((MenuItem)priMenu.Items[i - 1]).DataContext = (uint)i;
                ((MenuItem)priMenu.Items[i - 1]).IsChecked = (i == priority);
            }
        }

        /// <summary>
        /// パネル系番組表の右クリックメニュー　表示関係
        /// </summary>
        public MenuItem GenerateViewMenu(int mode, Action<object, RoutedEventArgs>[] eventHandler)
        {
            var menuItemView = new MenuItem();
            menuItemView.Header = "表示モード";

            var menuItemViewSetDlg = new MenuItem();
            menuItemViewSetDlg.Header = "表示設定";
            menuItemViewSetDlg.Click += new RoutedEventHandler(eventHandler[0]);
            menuItemView.Items.Add(menuItemViewSetDlg);

            menuItemView.Items.Add(new Separator());

            string[] HeaderStr = { "標準モード", "1週間モード", "リスト表示モード" };
            for (int i = 0; i < 3; i++)
            {
                var menuItemChgViewMode = new MenuItem();
                menuItemChgViewMode.Header = string.Format("{0} (_{1})", HeaderStr[i], i + 1);
                if (i != mode)
                {
                    menuItemChgViewMode.DataContext = i;
                    menuItemChgViewMode.Click += new RoutedEventHandler(eventHandler[1]);
                }
                else
                {
                    menuItemChgViewMode.IsChecked = true;
                }
                menuItemView.Items.Add(menuItemChgViewMode);
            }

            return menuItemView;
        }

        /// <summary>
        /// パネル系の右クリックメニュー　追加メニュー挿入
        /// </summary>
        public void InsertAppendMenu(ContextMenu menu, Action<object, RoutedEventArgs>[] eventHandler, bool enabled = true)
        {
            if (Settings.Instance.CmAppendMenu == false) return;

            var menuItemCopy = new MenuItem();
            menuItemCopy.Header = "番組名をコピー";
            menuItemCopy.ToolTip = CopyTitle_TrimMode();
            ToolTipService.SetShowOnDisabled(menuItemCopy, true);
            menuItemCopy.Click += new RoutedEventHandler(eventHandler[0]);
            menuItemCopy.IsEnabled = enabled;

            var menuItemContent = new MenuItem();
            menuItemContent.Header = "番組情報をコピー";
            menuItemContent.ToolTip = CopyContent_Mode();
            ToolTipService.SetShowOnDisabled(menuItemContent, true);
            menuItemContent.Click += new RoutedEventHandler(eventHandler[1]);
            menuItemContent.IsEnabled = enabled;

            var menuItemSearch = new MenuItem();
            menuItemSearch.Header = "番組名をネットで検索";
            ToolTipService.SetShowOnDisabled(menuItemSearch, true);
            menuItemSearch.ToolTip = SearchText_TrimMode();
            menuItemSearch.Click += new RoutedEventHandler(eventHandler[2]);
            menuItemSearch.IsEnabled = enabled;

            menu.Items.Add(new Separator());
            if (Settings.Instance.CmCopyTitle == true) menu.Items.Add(menuItemCopy);
            if (Settings.Instance.CmCopyContent == true) menu.Items.Add(menuItemContent);
            if (Settings.Instance.CmSearchTitle == true) menu.Items.Add(menuItemSearch);
        }

        /// <summary>
        /// リスト系の右クリックメニュー　追加メニュー表示制御
        /// </summary>
        public bool AppendMenuVisibleControl(object item)
        {
            if (((string)(((Control)item).Tag)) == "EpgKeyword")
            {
                ((Control)item).ToolTip = EpgKeyword_TrimMode();
                return true;
            }

            return AppendMenuVisibleSet(item, "cm_CmAppend", Settings.Instance.CmAppendMenu, null) ||
                    AppendMenuVisibleSet(item, "cm_CopyTitle", Settings.Instance.CmCopyTitle, CopyTitle_TrimMode()) ||
                    AppendMenuVisibleSet(item, "cm_CopyContent", Settings.Instance.CmCopyContent, CopyContent_Mode()) ||
                    AppendMenuVisibleSet(item, "cm_SearchTitle", Settings.Instance.CmSearchTitle, SearchText_TrimMode());
        }

        private bool AppendMenuVisibleSet(object item1, string name, bool flg, string tooltip = null)
        {
            var item = (Control)item1;
            bool retv = (item.Name == name);
            if (retv)
            {
                if (Settings.Instance.CmAppendMenu == true && flg == true)
                {
                    item.Visibility = System.Windows.Visibility.Visible;
                    item.ToolTip = tooltip;
                }
                else
                {
                    item.Visibility = System.Windows.Visibility.Collapsed;
                }
            }
            return retv;
        }

        public void SendAutoAdd(SearchItem item, Control Owner)
        {
            SendAutoAdd(item.EventInfo, Owner);
        }
        public void SendAutoAdd(EpgEventInfo item, Control Owner)
        {
            SendAutoAdd(item.Title(), item.Create64Key(), Owner);
        }
        public void SendAutoAdd(RecFileInfo item, Control Owner)
        {
            SendAutoAdd(item.Title, item.Create64Key(), Owner);
        }
        public void SendAutoAdd(ReserveData item, Control Owner)
        {
            SendAutoAdd(item.Title, item.Create64Key(), Owner);
        }
        public void SendAutoAdd(string Title, UInt64 sidKey, Control Owner)
        {
            try
            {
                var dlg = new SearchWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(Owner).RootVisual;
                dlg.SetViewMode(1);

                var key = new EpgSearchKeyInfo();
                key.andKey = TrimEpgKeyword(Title);
                key.serviceList.Add((Int64)sidKey);

                dlg.SetSearchDefKey(key);
                dlg.ShowDialog();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public bool ReserveAdd(List<SearchItem> itemlist, RecSettingView recSettingView = null, object sender = null)
        {
            return ReserveAdd(itemlist.NoReserveInfoList(), recSettingView, sender);
        }
        public bool ReserveAdd(EpgEventInfo item, RecSettingView recSettingView = null, object sender = null)
        {
            return ReserveAdd(GetList(item), recSettingView, sender);
        }
        public bool ReserveAdd(List<EpgEventInfo> itemlist, RecSettingView recSettingView = null, object sender = null, bool cautionMany = true)
        {
            try
            {
                if (itemlist.Count == 1)
                {
                    if (IsEnableReserveAdd(itemlist[0]) == false) return false;
                }

                //Memo:未使用時は即破棄、使用時もEpgTimerSrv側に送信後破棄されるので、forループの外にいられる。
                var setInfo = new RecSettingData();
                if (recSettingView != null)
                {
                    //ダイアログからの予約、SearchWindowの簡易予約
                    recSettingView.GetRecSetting(ref setInfo);
                }
                else
                {
                    uint presetID = 0;  //EPG画面の簡易予約
                    if (sender != null)
                    {
                        //コンテキストメニューからのプリセット予約
                        presetID = ReadDataContext(sender, uint.MinValue, uint.MaxValue - 1);
                        if (presetID == 0xFF) return false;

                    }
                    Settings.GetDefRecSetting(presetID, ref setInfo);
                }

                var list = new List<ReserveData>();

                foreach (EpgEventInfo item in itemlist)
                {
                    if (item.StartTimeFlag != 0)
                    {
                        var resInfo = new ReserveData();
                        CommonManager.ConvertEpgToReserveData(item, ref resInfo);
                        resInfo.RecSetting = setInfo;
                        list.Add(resInfo);
                    }
                }

                return ReserveAdd(list, cautionMany);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return false;
            }
        }
        public bool IsEnableReserveAdd(EpgEventInfo item)
        {
            if (item == null) return false;

            bool retv = (item.StartTimeFlag != 0);
            if (retv == false)
            {
                MessageBox.Show("開始時間未定のため予約できません");
            }
            return retv;
        }
        public bool ReserveAdd(ReserveData item)
        {
            return ReserveAdd(GetList(item));
        }
        public bool ReserveAdd(List<ReserveData> list, bool cautionMany = true)
        {
            return ReserveCmdSend(list, cmd.SendAddReserve, "予約追加", cautionMany);
        }

        public bool ReserveChangeOnOff(List<SearchItem> itemlist, RecSettingView recSettingView = null)
        {
            //取りあえずここにも入れておく。これのせいで苦しいが、また後で考える。
            if (CautionManyMessage(itemlist.Count, "簡易予約/有効←→無効") == false) return false;

            bool retv = ReserveAdd(itemlist.NoReserveInfoList(), recSettingView, null, false);
            retv = ReserveChangeOnOff(itemlist.ReserveInfoList(), recSettingView, false) || retv;//順番重要
            return retv;
        }
        public bool ReserveChangeOnOff(ReserveData item, RecSettingView recSettingView = null)
        {
            return ReserveChangeOnOff(GetList(item), recSettingView);
        }
        public bool ReserveChangeOnOff(List<ReserveData> itemlist, RecSettingView recSettingView = null, bool cautionMany = true)
        {
            try
            {
                //無効から戻す録画モードの選択
                var setInfo = new RecSettingData();

                //現在の設定を読み込む。SearchWindowの場合だけ。
                if (recSettingView != null)
                {
                    recSettingView.GetRecSetting(ref setInfo);
                    
                    //現在の設定が無効で登録の場合は、デフォルトの設定を読み込みに行く
                    if (setInfo.RecMode == 5)
                    {
                        recSettingView = null;
                    }
                }
                //デフォルト設定を読み込む
                if (recSettingView == null)
                {
                    Settings.GetDefRecSetting(0, ref setInfo);
                }
                //デフォルトも無効で登録なら、指定サービスにする
                byte recMode = setInfo.RecMode != 5 ? setInfo.RecMode : (byte)1;

                itemlist.ForEach(item =>
                    item.RecSetting.RecMode = (item.RecSetting.RecMode == 5 ? recMode : (byte)5));

                return ReserveChange(itemlist, cautionMany);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return false;
            }
        }

        public bool ReserveChangeRecmode(List<SearchItem> itemlist, object sender)
        {
            return ReserveChangeRecmode(itemlist.ReserveInfoList(), sender);
        }
        public bool ReserveChangeRecmode(ReserveData item, object sender)
        {
            return ReserveChangeRecmode(GetList(item), sender);
        }
        public bool ReserveChangeRecmode(List<ReserveData> itemlist, object sender)
        {
            try
            {
                byte recMode = (byte)ReadDataContext(sender, 0, 5);
                if (recMode == 0xFF) return false;

                itemlist.ForEach(item => item.RecSetting.RecMode = recMode);

                return ReserveChange(itemlist);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return false;
            }
        }

        public bool ReserveChangePriority(List<SearchItem> itemlist, object sender)
        {
            return ReserveChangePriority(itemlist.ReserveInfoList(), sender);
        }
        public bool ReserveChangePriority(ReserveData item, object sender)
        {
            return ReserveChangePriority(GetList(item), sender);
        }
        public bool ReserveChangePriority(List<ReserveData> itemlist, object sender)
        {
            try
            {
                byte priority = (byte)ReadDataContext(sender, 1, 5);
                if (priority == 0xFF) return false;

                itemlist.ForEach(item => item.RecSetting.Priority = priority);

                return ReserveChange(itemlist);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return false;
            }
        }
        private uint ReadDataContext(object sender, uint min, uint max)
        {
            try
            {
                if (sender == null || sender.GetType() != typeof(MenuItem))
                {
                    return 0xFF;
                }
                return Math.Max(Math.Min((uint)((sender as MenuItem).DataContext), max), min);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return 0xFF;
            }
        }
        public bool ReserveChange(ReserveData item)
        {
            return ReserveChange(GetList(item));
        }
        public bool ReserveChange(List<ReserveData> list, bool cautionMany = true)
        {
            return ReserveCmdSend(list, cmd.SendChgReserve, "予約変更", cautionMany);
        }

        public bool ReserveDelete(List<SearchItem> itemlist)
        {
            return ReserveDelete(itemlist.ReserveInfoList());
        }
        public bool ReserveDelete(ReserveData item)
        {
            return ReserveDelete(GetList(item));
        }
        public bool ReserveDelete(List<ReserveData> itemlist)
        {
            try
            {
                List<uint> list = itemlist.Select(item => item.ReserveID).ToList();
                return ReserveCmdSend(list, cmd.SendDelReserve, "予約削除");
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return false;
            }
        }

        public bool EpgAutoAddAdd(EpgAutoAddData item)
        {
            return EpgAutoAddAdd(GetList(item));
        }
        public bool EpgAutoAddAdd(List<EpgAutoAddData> itemlist)
        {
            return ReserveCmdSend(itemlist, cmd.SendAddEpgAutoAdd, "EPG自動予約の追加");
        }

        public bool EpgAutoAddChange(EpgAutoAddData item)
        {
            return EpgAutoAddChange(GetList(item));
        }
        public bool EpgAutoAddChange(List<EpgAutoAddData> itemlist, bool cautionMany = true)
        {
            return ReserveCmdSend(itemlist, cmd.SendChgEpgAutoAdd, "EPG自動予約の変更", cautionMany);
        }

        public bool EpgAutoAddDelete(List<EpgAutoDataItem> itemlist)
        {
            try
            {
                List<uint> list = itemlist.Select(item => item.EpgAutoAddInfo.dataID).ToList();
                return EpgAutoAddDelete(list);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return false;
            }
        }
        public bool EpgAutoAddDelete(uint item)
        {
            return EpgAutoAddDelete(GetList(item));
        }
        public bool EpgAutoAddDelete(List<uint> itemlist)
        {
            return ReserveCmdSend(itemlist, cmd.SendDelEpgAutoAdd, "EPG自動予約の削除");
        }

        private bool ReserveCmdSend<T>(List<T> list, Func<List<T>, uint> cmdSend, string description = "", bool cautionMany = true)
        {
            try
            {
                if (list.Count == 0) return false;

                if (cautionMany == true && CautionManyMessage(list.Count, description) == false) return false;

                ErrCode err = (ErrCode)cmdSend(list);
                return CommonManager.CmdErrMsgTypical(err, description);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return false;
            }
        }

        public bool CautionManyMessage(int Count, string description = "")
        {
            if (Settings.Instance.CautionManyChange == true && Count >= Settings.Instance.CautionManyNum)
            {
                if (MessageBox.Show("多数の項目を処理しようとしています。\r\nよろしいですか？\r\n"
                    + "　項目数: " + Count + "\r\n"
                    , description, MessageBoxButton.OKCancel) == MessageBoxResult.Cancel)
                {
                    return false;
                }
            }
            return true;
        }

        public bool? OpenSearchItemWithWindow(SearchItem item, Control Owner, byte openMode = 0)
        {
            if (item == null) return null;

            if (item.IsReserved == true)
            {
                return OpenChangeReserveWindow(item.ReserveInfo, Owner, openMode);
            }
            else
            {
                return OpenEpgReserveWindow(item.EventInfo, Owner, openMode);
            }
        }

        public bool? OpenManualReserveWindow(Control Owner)
        {
            try
            {
                ChgReserveWindow dlg = new ChgReserveWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(Owner).RootVisual;
                dlg.SetAddReserveMode();
                return dlg.ShowDialog();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return null;
            }
        }

        public bool? OpenEpgReserveWindow(EpgEventInfo epgInfo, Control Owner, byte openMode = 0)
        {
            try
            {
                AddReserveEpgWindow dlg = new AddReserveEpgWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(Owner).RootVisual;
                dlg.SetEventInfo(epgInfo);
                dlg.SetOpenMode(openMode);
                return dlg.ShowDialog();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return null;
            }
        }

        public bool? OpenChangeReserveWindow(ReserveData resInfo, Control Owner, byte openMode = 0)
        {
            try
            {
                ChgReserveWindow dlg = new ChgReserveWindow();
                dlg.Owner = (Window)PresentationSource.FromVisual(Owner).RootVisual;
                dlg.SetReserveInfo(resInfo);
                dlg.SetOpenMode(openMode);
                return dlg.ShowDialog();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return null;
            }
        }

        public void SetSearchItemReserved(List<SearchItem> list)
        {
            var listKeys = new Dictionary<ulong, SearchItem>();

            foreach (SearchItem listItem1 in list)
            {
                //重複するキーは基本的に無いという前提
                try
                {
                    listKeys.Add(listItem1.EventInfo.Create64PgKey(), listItem1);
                    listItem1.ReserveInfo = null;
                }
                catch { }
            }

            SearchItem setItem;
            foreach (ReserveData data in CommonManager.Instance.DB.ReserveList.Values)
            {
                if (listKeys.TryGetValue(data.Create64PgKey(), out setItem))
                {
                    setItem.ReserveInfo = data;
                }
            }
        }

        public void FilePlay(String filePath)
        {
            try
            {
                if (filePath.Length == 0) return;

                CommonManager cmg = CommonManager.Instance;
                if (cmg.NWMode == false)
                {
                    System.Diagnostics.Process process;
                    if (Settings.Instance.FilePlayExe.Length == 0)
                    {
                        process = System.Diagnostics.Process.Start(filePath);
                    }
                    else
                    {
                        String cmdLine = Settings.Instance.FilePlayCmd;
                        cmdLine = cmdLine.Replace("$FilePath$", filePath);
                        process = System.Diagnostics.Process.Start(Settings.Instance.FilePlayExe, cmdLine);
                    }
                }
                else
                {
                    cmg.TVTestCtrl.StartStreamingPlay(filePath, cmg.NW.ConnectedIP, cmg.NW.ConnectedPort);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        //複数選択から単一要素を処理する際にどれを選ぶかの手続き。
        //今は最後に選択したものとしている。
        public T SelectSingleItem<T>(ListBox list)
        {
            if (list.SelectedItems.Count <= 1) return (T)list.SelectedItem;//SingleMode用
            object item = list.SelectedItems[list.SelectedItems.Count - 1];
            list.UnselectAll();
            list.SelectedItem = item;
            return (T)item;
        }

    }

}

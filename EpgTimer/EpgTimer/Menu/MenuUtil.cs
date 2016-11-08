using System;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Input;
using System.Windows.Controls;
using System.Windows.Threading;

namespace EpgTimer
{
    public static class MenuUtil
    {
        private static CtrlCmdUtil cmd { get { return CommonManager.Instance.CtrlCmd; } }

        public static string TrimEpgKeyword(string KeyWord, bool NotToggle = false)//NotToggleはショートカット用
        {
            return TrimKeywordCheckToggled(KeyWord, Settings.Instance.MenuSet.Keyword_Trim, NotToggle);
        }

        public static void CopyTitle2Clipboard(string Title, bool NotToggle = false)
        {
            Title = TrimKeywordCheckToggled(Title, Settings.Instance.MenuSet.CopyTitle_Trim, NotToggle);
            Clipboard.SetDataObject(Title, true);
        }

        public static void CopyContent2Clipboard(EpgEventInfo eventInfo, bool NotToggle = false)
        {
            string text = "";

            if (eventInfo != null)
            {
                bool setting = CheckShiftToggled(Settings.Instance.MenuSet.CopyContentBasic, NotToggle);
                if (setting == true)
                {
                    //text = eventInfo.ShortInfo.text_char;
                    text = CommonManager.ConvertProgramText(eventInfo, EventInfoTextMode.BasicOnly);
                }
                else
                {
                    text = CommonManager.ConvertProgramText(eventInfo, EventInfoTextMode.All);
                }

                text = text.TrimEnd() + "\r\n";
            }

            Clipboard.SetDataObject(text, true);
        }

        public static void CopyContent2Clipboard(ReserveData resInfo, bool NotToggle = false)
        {
            EpgEventInfo info = resInfo == null ? null : resInfo.SearchEventInfo(true);
            CopyContent2Clipboard(info, NotToggle);
        }

        public static void CopyContent2Clipboard(RecFileInfo recInfo, bool NotToggle = false)
        {
            string text = "";

            if (recInfo != null)
            {
                bool setting = CheckShiftToggled(Settings.Instance.MenuSet.CopyContentBasic, NotToggle);
                if (setting == true)
                {
                    string[] stArrayData = recInfo.ProgramInfo.Replace("\r\n", "\n").Split('\n');
                    int endI = Math.Min(stArrayData.Length, 3);

                    for (int i = 0; i < endI; i++)
                    {
                        text += stArrayData[i] + "\r\n";
                    }
                }
                else
                {
                    text = recInfo.ProgramInfo;
                }

                text = text.TrimEnd() + "\r\n";
            }

            Clipboard.SetDataObject(text, true);
        }

        public static void SearchTextWeb(string KeyWord, bool NotToggle = false)
        {
            KeyWord = TrimKeywordCheckToggled(KeyWord, Settings.Instance.MenuSet.SearchTitle_Trim, NotToggle);
            string txtURI = Settings.Instance.MenuSet.SearchURI + UrlEncode(KeyWord, System.Text.Encoding.UTF8);

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

        private static string TrimKeywordCheckToggled(string s, bool setting, bool NotToggle = false)
        {
            return CheckShiftToggled(setting, NotToggle) == true ? TrimKeyword(s) : s;
        }
        private static bool CheckShiftToggled(bool setting, bool NotToggle = false)
        {
            return (Keyboard.Modifiers == ModifierKeys.Shift && NotToggle == false) ? !setting : setting;
        }

        public static string TrimKeyword(string txtKey)
        {
            if (string.IsNullOrEmpty(txtKey)) return txtKey;

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
                                "^(([#＃♯][\\d０-９]+)|(第[\\d０-９]+話))", // 先頭にある話数の除去
                                "^[\\(（][\\d０-９]+[\\)）]\\s*「[^」]+」", // 先頭にある話数の除去
                                "(([#＃♯][\\d０-９]+)|(第[\\d０-９]+話)).*", // ドラマ等の話数から後ろ全て
                                "[\\(（][\\d０-９]+[\\)）]\\s*「[^」]+」.*", // ドラマ等の話数から後ろ全て その2
                                //"「[^」]+」.*", // ドラマ等のサブタイトルから後ろ全て、ちょっと強すぎるかも
                                "<[^>]+>", // NHK・フジテレビが使う補足
                                "＜[^＞]+＞", // NHK・フジテレビが使う補足
                                "[◆▽].*", // タイトルに埋め込まれた番組説明
                                "[\\[［【\\(（][^\\]］】\\)）]*リマスター[^\\]］】\\)）]*[\\]］】\\)）]",// ときどき見かけるので1
                                "(((HD)|(ＨＤ)|(ハイビジョン)|(デジタル))リマスター((HD)|(ＨＤ))?版?)|(リマスター((HD)|(ＨＤ)|版)版?)",// 同上、括弧無しは特定パタンのみ
                                "(（二）|（字幕版）|（吹替版）|" + markExp1 + ")+$" // 末尾の記号
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
        public static string UrlEncode(string s, System.Text.Encoding enc)
        {
            var rt = new System.Text.StringBuilder();
            foreach (byte i in enc.GetBytes(s))
                if (i == 0x20)
                    rt.Append('+');
                else if (i >= 0x30 && i <= 0x39 || i >= 0x41 && i <= 0x5a || i >= 0x61 && i <= 0x7a)
                    rt.Append((char)i);
                else
                    rt.Append("%" + i.ToString("X2"));
            return rt.ToString();
        }

        /// <summary>
        /// 変換エラーの場合、デフォルト値を返し、テキストボックスの内容をデフォルト値に置き換える。
        /// </summary>
        public static T MyToNumerical<T>(TextBox box, Func<string, T> converter, T defValue = default(T))
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
        public static T MyToNumerical<T>(TextBox box, Func<string, T> converter, T max, T min, T defValue = default(T)) where T : IComparable
        {
            try
            {
                T val = MyToNumerical(box, converter, defValue);
                if (val.CompareTo(min) < 0)
                {
                    box.Text = min.ToString();
                    return min;
                }
                if (val.CompareTo(max) > 0)
                {
                    box.Text = max.ToString();
                    return max;
                }
                return val;
            }
            catch
            {
                box.Text = defValue.ToString();
                return defValue;
            }
        }
        
        public static bool ReserveAdd(List<EpgEventInfo> itemlist, RecSettingView recSettingView, uint presetID = 0, bool cautionMany = true)
        {
            try
            {
                itemlist = CheckReservable(itemlist);
                if (itemlist == null) return false;

                var setInfo = new RecSettingData();
                if (recSettingView != null)
                {
                    //ダイアログからの予約、SearchWindowの簡易予約
                    setInfo = recSettingView.GetRecSetting();
                }
                else
                {
                    //簡易予約やプリセット予約
                    Settings.GetDefRecSetting(presetID, ref setInfo);
                }

                var list = new List<ReserveData>();

                foreach (EpgEventInfo item in itemlist)
                {
                    var resInfo = new ReserveData();
                    item.ConvertToReserveData(ref resInfo);
                    resInfo.RecSetting = setInfo;
                    list.Add(resInfo);
                }

                return ReserveAdd(list, cautionMany);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }
        public static List<EpgEventInfo> CheckReservable(List<EpgEventInfo> list, bool fixlist = true)
        {
            if (list.Count == 0) return list;

            //開始未定と終了番組を除外
            list = list.FindAll(item => item.StartTimeFlag != 0);
            if (list.Count == 0)
            {
                MessageBox.Show("開始時間未定のため予約できません");
                return null;
            }
            list = list.FindAll(item => item.IsOver() == false);
            if (list.Count == 0)
            {
                MessageBox.Show("放映終了しているため予約できません");
                return null;
            }
            return list;
        }
        public static bool ReserveAdd(List<ReserveData> list, bool cautionMany = true)
        {
            if (list.Count == 0) return true;

            //録画時間過ぎているものを除外
            list = list.FindAll(item => item.IsOver() == false);
            if (list.Count == 0)
            {
                MessageBox.Show("録画時間が既に終了しています。\r\n(番組が放映中の場合は録画マージンも確認してください。)");
                return false;
            }
            return ReserveCmdSend(list, cmd.SendAddReserve, "予約追加", cautionMany);
        }

        public static bool ReserveChangeOnOff(List<ReserveData> itemlist, RecSettingView recSettingView = null, bool cautionMany = true)
        {
            try
            {
                //無効から戻す録画モードの選択
                var setInfo = new RecSettingData();

                //現在の設定を読み込む。SearchWindowの場合だけ。
                if (recSettingView != null)
                {
                    setInfo = recSettingView.GetRecSetting();
                    
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

                //個別設定なので、ChangeRecmode()は不可。
                itemlist.ForEach(item => item.RecSetting.RecMode = (item.RecSetting.RecMode == 5 ? recMode : (byte)5));

                return ReserveChange(itemlist, cautionMany);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool ChangeOnPreset(List<RecSettingData> infoList, uint presetID)
        {
            try
            {
                infoList.ForEach(info => Settings.GetDefRecSetting(presetID, ref info));
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool ChangeRecmode(List<RecSettingData> infoList, byte recMode)
        {
            try
            {
                infoList.ForEach(info => info.RecMode = recMode);
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool ChangePriority(List<RecSettingData> infoList, byte priority)
        {
            try
            {
                infoList.ForEach(info => info.Priority = priority);
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool ChangeRelay(List<RecSettingData> infoList, byte relay)
        {
            try
            {
                infoList.ForEach(info => info.TuijyuuFlag = relay);
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool ChangePittari(List<RecSettingData> infoList, byte pittari)
        {
            try
            {
                infoList.ForEach(info => info.PittariFlag = pittari);
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool ChangeMargin(List<RecSettingData> infoList, int margin_offset, bool start)
        {
            try
            {
                if (margin_offset == 0)
                {
                    infoList.ForEach(info => info.UseMargineFlag = 0);
                }
                else
                {
                    infoList.ForEach(info =>
                    {
                        if (info.UseMargineFlag == 0)
                        {
                            info.StartMargine = info.StartMarginActual;
                            info.EndMargine = info.EndMarginActual;
                        }

                        info.UseMargineFlag = 1;
                        if (start == true)
                        {
                            info.StartMargine += margin_offset;
                        }
                        else
                        {
                            info.EndMargine += margin_offset;
                        }
                    });
                }

                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool ChangeMarginValue(List<RecSettingData> infoList, bool start, UIElement owner = null)
        {
            try
            {
                infoList[0].UseMargineFlag = 1;

                var dlg = new Setting.SetDefRecSettingWindow();
                dlg.Owner = CommonUtil.GetTopWindow(owner);
                dlg.SetSettingMode(start == true ? "開始マージン設定" : "終了マージン設定", start == true ? 0 : 1);
                dlg.recSettingView.SetDefSetting(infoList[0]);

                if (dlg.ShowDialog() == false) return false;

                RecSettingData setData = dlg.recSettingView.GetRecSetting();

                infoList.ForEach(info =>
                {
                    info.UseMargineFlag = 1;
                    if (start == true)
                    {
                        info.StartMargine = setData.StartMargine;
                    }
                    else
                    {
                        info.EndMargine = setData.EndMargine;
                    }
                });
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool ChangeTuner(List<RecSettingData> infoList, uint tuner)
        {
            try
            {
                infoList.ForEach(info => info.TunerID = tuner);
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool ChangeBulkSet(List<RecSettingData> infoList, UIElement owner = null, bool pgAll = false)
        {
            try
            {
                var dlg = new Setting.SetDefRecSettingWindow();
                dlg.Owner = CommonUtil.GetTopWindow(owner);
                dlg.SetSettingMode("まとめて録画設定を変更");
                dlg.recSettingView.SetDefSetting(infoList[0], pgAll == true);
                dlg.recSettingView.SetViewMode(pgAll != true);

                if (dlg.ShowDialog() == false) return false;

                RecSettingData setData = dlg.recSettingView.GetRecSetting();
                
                infoList.ForEach(info => setData.CopyTo(info));
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool ChgGenre(List<EpgSearchKeyInfo> infoList, UIElement owner = null)
        {
            try
            {
                var dlg = new SetDefSearchSettingWindow();
                dlg.Owner = CommonUtil.GetTopWindow(owner);
                dlg.SetDefSetting(infoList[0]);
                dlg.searchKey.searchKeyDescView.SetChangeMode(0);

                if (dlg.ShowDialog() == false) return false;

                EpgSearchKeyInfo setData = dlg.GetSetting();
                infoList.ForEach(info => info.contentList = setData.contentList.Clone());
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool ReserveChangeResMode(List<ReserveData> itemlist, uint resMode)
        {
            try
            {
                List<ReserveData> list;
                if (resMode == 0)//EPG予約へ変更
                {
                    list = itemlist.Where(item => item.ReserveMode == ReserveMode.KeywordAuto ||
                        item.IsEpgReserve == false && item.SearchEventInfoLikeThat().ConvertToReserveData(ref item) == true).ToList();
                }
                else if (resMode == 1)//プログラム予約へ変更
                {
                    list = itemlist.Where(item => item.ReserveMode != ReserveMode.Program).ToList();
                    list.ForEach(item => { item.EventID = 0xFFFF; });
                }
                else
                {
                    return true;
                }

                list.ForEach(item => item.Comment = "");
                return ReserveChange(list);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool ReserveChangeResModeAutoAdded(List<ReserveData> itemList, AutoAddData autoAdd)
        {
            if (ReserveDelete(itemList, false) == false) return false;
            return AutoAddChange(CommonUtil.ToList(autoAdd), false, false, false, false);
        }

        public static bool ReserveChange(List<ReserveData> itemlist, bool cautionMany = true)
        {
            if (CheckReserveOnRec(itemlist,"変更") == false) return false;
            return ReserveCmdSend(itemlist, cmd.SendChgReserve, "予約変更", cautionMany);
        }

        public static bool ReserveDelete(List<ReserveData> itemlist, bool cautionMany = true)
        {
            try
            {
                if (CheckReserveOnRec(itemlist, "削除") == false) return false;
                List<uint> list = itemlist.Select(item => item.ReserveID).ToList();
                return ReserveCmdSend(list, cmd.SendDelReserve, "予約削除", cautionMany);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        public static bool CheckReserveOnRec(List<ReserveData> itemlist, string description)
        {
            if (Settings.Instance.CautionOnRecChange == false) return true;
            int cMin = Settings.Instance.CautionOnRecMarginMin;

            List<string> list = itemlist.Where(item => item.IsEnabled == true && item.OnTime(DateTime.Now.AddMinutes(cMin)) >= 0)
                .Select(item => new ReserveItem(item).StartTime + "　" + item.Title).ToList();

            if (list.Count == 0) return true;

            string text = string.Format("録画中または{0}分以内に録画開始される予約が含まれています。\r\n"
                + "処理を続けますか?\r\n\r\n"
                + "[該当予約数: {1}]\r\n\r\n", cMin, list.Count)
                + CmdExeUtil.FormatTitleListForDialog(list);

            return MessageBox.Show(text, "[予約" + description + "]の確認", MessageBoxButton.OKCancel,
                                MessageBoxImage.Exclamation, MessageBoxResult.Cancel) == MessageBoxResult.OK;
        }

        public static bool AutoAddChangeKeyEnabled(IEnumerable<AutoAddData> itemlist, bool value)
        {
            try
            {
                if (AutoAddChangeKeyEnabledCautionMany(itemlist) == false) return false;

                foreach (var item in itemlist) item.IsEnabled = value;
                return AutoAddChange(itemlist, false);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }
        public static bool AutoAddChangeOnOffKeyEnabled(IEnumerable<AutoAddData> itemlist, bool cautionMany = true)
        {
            try
            {
                if (AutoAddChangeKeyEnabledCautionMany(itemlist) == false) return false;

                foreach (var item in itemlist) item.IsEnabled = !item.IsEnabled;
                return AutoAddChange(itemlist, false, cautionMany);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }
        public static bool AutoAddChangeKeyEnabledCautionMany(IEnumerable<AutoAddData> itemlist)
        {
            if (Settings.Instance.CautionManyChange == true)
            {
                long addReserveNum = itemlist.Where(item => item.IsEnabled == false)
                    .Sum(item => item.SearchCount - item.ReserveCount);
                if (itemlist.Count() >= Settings.Instance.CautionManyNum
                    || addReserveNum >= Settings.Instance.CautionManyNum)
                {
                    if (MessageBox.Show("多数の項目を処理しようとしています。\r\n"
                        + "または多数の予約が追加されます。\r\n"
                        + "よろしいですか？\r\n\r\n"
                        + "[項目数 : " + itemlist.Count() + "]\r\n"
                        + "[追加される予約数 : " + addReserveNum + "]\r\n"
                        , "自動予約登録の変更", MessageBoxButton.OKCancel,
                        MessageBoxImage.Exclamation, MessageBoxResult.Cancel) == MessageBoxResult.Cancel)
                    {
                        return false;
                    }
                }
            }
            return true;
        }
        public static bool EpgAutoAddChangeNotKey(List<EpgAutoAddData> itemlist)
        {
            try
            {
                itemlist.ForEach(item => item.searchInfo.notKey = Clipboard.GetText());
                return AutoAddChange(itemlist);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }
        public static bool AutoAddAdd(IEnumerable<AutoAddData> itemlist)
        {
            return AutoAddCmdSend(itemlist, 0);
        }
        public static bool AutoAddChange(IEnumerable<AutoAddData> itemlist, bool cautionMany = true)
        {
            return AutoAddChange(itemlist, Settings.Instance.SyncResAutoAddChange, Settings.Instance.SyncResAutoAddChgNewRes, cautionMany);
        }
        public static bool AutoAddChange(IEnumerable<AutoAddData> itemlist, bool SyncChange, bool NewRes, bool cautionMany = true, bool isViewOrder = true)
        {
            if (SyncChange == true)
            {
                //操作前にリストを作成する
                List<ReserveData> deleteList = NewRes == false ? null : new List<ReserveData>();
                List<ReserveData> syncList = AutoAddSyncChangeList(itemlist, false, deleteList);
                return AutoAddCmdSend(itemlist, 1, deleteList, syncList, cautionMany, isViewOrder);
            }
            else
            {
                return AutoAddCmdSend(itemlist, 1, null, null, cautionMany, isViewOrder);
            }
        }
        public static bool AutoAddChangeSyncReserve(IEnumerable<AutoAddData> itemlist)
        {
            return ReserveChange(AutoAddSyncChangeList(itemlist, true), false);
        }
        private static List<ReserveData> AutoAddSyncChangeList(IEnumerable<AutoAddData> itemlist, bool SyncAll, List<ReserveData> deleteList = null)
        {
            var syncDict = new Dictionary<uint, ReserveData>();

            foreach (AutoAddData data in itemlist)
            {
                IEnumerable<ReserveData> list = SyncAll == true ?
                    data.GetReserveList() : data.GetReserveList().Where(info => info.IsAutoAdded == true);
                foreach (ReserveData resinfo in list)
                {
                    if (syncDict.ContainsKey(resinfo.ReserveID) == false)
                    {
                        ReserveData rdata = resinfo.Clone();//変更かけるのでコピーする
                        rdata.RecSetting = data.RecSettingInfo.Clone();
                        //無効は保持する
                        if (resinfo.RecSetting.RecMode == 5)
                        {
                            rdata.RecSetting.RecMode = 5;
                        }
                        if (data.IsManual == true && resinfo.IsManual == true)
                        {
                            //プログラム予約の場合は名前も追従させる。
                            rdata.Title = data.DataTitle;
                        }
                        syncDict.Add(resinfo.ReserveID, rdata);
                    }
                }
            }

            List<ReserveData> syncList = syncDict.Values.ToList();

            if (deleteList != null)
            {
                List<ReserveData> modList = (SyncAll == true ? syncList : AutoAddSyncModifyReserveList(syncList, itemlist));

                int cMin = Settings.Instance.CautionOnRecChange == true ? Settings.Instance.CautionOnRecMarginMin : 1;
                deleteList.AddRange(modList.FindAll(data => data.IsEnabled == true && data.OnTime(DateTime.Now.AddMinutes(cMin)) >= 0));
                syncList = syncList.Except(deleteList).ToList();
            }

            //無効になっている自動登録からの連動変更で、他の有効な自動登録の予約が変更されないようにする
            if (SyncAll == false)
            {
                //syncListのReserveDataはコピーなのでIDで処理する
                var extList1 = new List<uint>();
                var extList2 = new List<uint>();
                foreach (AutoAddData data in itemlist)
                {
                    (data.IsEnabled == false ? extList1 : extList2).AddRange(data.GetReserveList().Where(info => info.IsAutoAdded == true).Select(info => info.ReserveID));
                }
                extList1 = extList1.Distinct().ToList();//処理対象のうち無効の自動登録の予約一覧
                extList2 = extList2.Distinct().ToList();//処理対象のうち有効の自動登録の予約一覧
                var extDict = extList1.Except(extList2).ToDictionary(data => data, data => data);
                syncList = syncList.Where(resinfo => extDict.ContainsKey(resinfo.ReserveID) == false).ToList();
            }

            return syncList;
        }
        public static bool AutoAddDelete(IEnumerable<AutoAddData> itemlist, bool cautionMany = true)
        {
            return AutoAddDelete(itemlist, Settings.Instance.SyncResAutoAddDelete, false, cautionMany);
        }
        public static bool AutoAddDelete(IEnumerable<AutoAddData> itemlist, bool SyncDelete, bool SyncAll, bool cautionMany = true)
        {
            //操作前にリストを作成する
            return AutoAddCmdSend(itemlist, 2, SyncDelete == false ? null : AutoAddSyncDeleteList(itemlist, SyncAll), null, cautionMany);
        }
        private static List<ReserveData> AutoAddSyncDeleteList(IEnumerable<AutoAddData> itemlist, bool SyncAll)
        {
            var list = itemlist.GetReserveList();
            return SyncAll == true ? list : AutoAddSyncModifyReserveList(list, itemlist);
        }

        private static List<ReserveData> AutoAddSyncModifyReserveList(List<ReserveData> reslist, IEnumerable<AutoAddData> itemlist)
        {
            var epgAutoList = reslist.ToDictionary(info => info.ReserveID, info => info.GetEpgAutoAddList(true).Select(item => item.DataID).ToList());
            var manualAutoList = reslist.ToDictionary(info => info.ReserveID, info => info.GetManualAutoAddList(true).Select(item => item.DataID).ToList());

            foreach (AutoAddData data in itemlist)
            {
                var autoList = data is EpgAutoAddData ? epgAutoList : manualAutoList;
                reslist.ForEach(resinfo => autoList[resinfo.ReserveID].Remove(data.DataID));
            }

            // 1)個別予約を除外
            // 2)処理する自動登録リスト以外の有効な自動登録に含まれている予約を除外
            return reslist.FindAll(info => info.IsAutoAdded == true && (epgAutoList[info.ReserveID].Count + manualAutoList[info.ReserveID].Count) == 0);
        }

        //mode 0:追加、1:変更、2:削除
        private static bool AutoAddCmdSend(IEnumerable<AutoAddData> itemlist, int mode,
            List<ReserveData> delReserveList = null, List<ReserveData> chgReserveList = null, bool cautionMany = true, bool isViewOrder = true)
        {
            try
            {
                var message = "自動予約登録の" + (new List<string> { "追加", "変更", "削除" }[(int)mode]);
                if (cautionMany == true && CautionManyMessage(itemlist.Count(), message) == false) return false;

                var epgList = itemlist.OfType<EpgAutoAddData>().ToList();
                var manualList = itemlist.OfType<ManualAutoAddData>().ToList();

                if (isViewOrder == true)
                {
                    //自動予約登録データ変更の前に、並び順を自動保存する。
                    if ((AutoAddOrderAutoSave(ref epgList, mode != 0) && AutoAddOrderAutoSave(ref manualList, mode != 0)) == false)
                    {
                        MessageBox.Show("自動登録の並べ替え保存中に問題が発生しました。\r\n処理を中止します。", message, MessageBoxButton.OK, MessageBoxImage.Exclamation);
                        return false;
                    }
                }

                switch (mode)
                {
                    case 0:
                        return ReserveCmdSend(epgList, cmd.SendAddEpgAutoAdd, "キーワード予約の追加", false)
                            && ReserveCmdSend(manualList, cmd.SendAddManualAdd, "プログラム自動予約の追加", false);
                    case 1:
                        return (delReserveList == null ? true : ReserveDelete(delReserveList, false))
                            && ReserveCmdSend(epgList, cmd.SendChgEpgAutoAdd, "キーワード予約の変更", false)
                            && ReserveCmdSend(manualList, cmd.SendChgManualAdd, "プログラム自動予約の変更", false)
                            && (chgReserveList == null ? true : ReserveChange(chgReserveList, false));
                    case 2:
                        return ReserveCmdSend(epgList.Select(item => item.DataID).ToList(), cmd.SendDelEpgAutoAdd, "キーワード予約の削除", false)
                            && ReserveCmdSend(manualList.Select(item => item.DataID).ToList(), cmd.SendDelManualAdd, "プログラム自動予約の削除", false)
                            && (delReserveList == null ? true : ReserveDelete(delReserveList, false));
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        private static bool AutoAddOrderAutoSave<T>(ref List<T> list, bool changeID) where T : AutoAddData
        {
            //並べ替え不要
            if (list.Count == 0) return true;

            var autoView = ViewUtil.MainWindow.autoAddView;
            var view = (list[0] is EpgAutoAddData) ? (AutoAddListView)autoView.epgAutoAddView : autoView.manualAutoAddView;

            if (changeID == true)
            {
                //並べ替えの影響回避のため。
                list = list.Select(item => (T)item.CloneObj()).ToList();
            }

            //並べ替えしなかった
            Dictionary<uint, uint> changeIDTable = null;
            if (AutoAddViewOrderCheckAndSave(view, out changeIDTable) == false) return true;

            //並べ替え保存時に何か問題があった
            if (changeIDTable == null) return false;

            if (changeID == true)
            {
                foreach (var item in list)
                {
                    //通常無いはずだが、並べ替えが上手くできない時に継続するのはとても危険なので中止する。
                    if (changeIDTable.ContainsKey(item.DataID) == false) return false;

                    //新しいIDに張り替え
                    item.DataID = changeIDTable[item.DataID];
                }
            }
            return true;
        }

        public static bool? AutoAddViewOrderCheckAndSave(AutoAddListView view, out Dictionary<uint, uint> changeIDTable)
        {
            changeIDTable = null;
            try
            {
                if (view == null || view.IsVisible == false || view.dragMover.NotSaved == false) return false;
                //
                var cmdPrm = new EpgCmdParam(null);
                EpgCmds.SaveOrder.Execute(cmdPrm, view);
                changeIDTable = cmdPrm.Data as Dictionary<uint, uint>;
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return null;
        }

        public static bool RecinfoChgProtect(List<RecFileInfo> itemlist, bool cautionMany = true)
        {
            try
            {
                itemlist.ForEach(item => item.ProtectFlag = (byte)(item.ProtectFlag == 0 ? 1 : 0));
                return ReserveCmdSend(itemlist, cmd.SendChgProtectRecInfo, "録画情報の変更", cautionMany);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }
        public static bool RecinfoDelete(List<RecFileInfo> itemlist, bool cautionMany = true)
        {
            try
            {
                List<uint> list = itemlist.Select(item => item.ID).ToList();
                return ReserveCmdSend(list, cmd.SendDelRecInfo, "録画情報の削除", cautionMany);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        private static bool ReserveCmdSend<T>(List<T> list, Func<List<T>, ErrCode> cmdSend, string description = "", bool cautionMany = true)
        {
            try
            {
                if (list.Count == 0) return true;

                if (cautionMany == true && CautionManyMessage(list.Count, description) == false) return false;

                ErrCode err = cmdSend(list);
                return CommonManager.CmdErrMsgTypical(err, description);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }
        public static bool CautionManyMessage(int Count, string description = "")
        {
            if (Settings.Instance.CautionManyChange == true && Count >= Settings.Instance.CautionManyNum)
            {
                if (MessageBox.Show("多数の項目を処理しようとしています。\r\nよろしいですか？\r\n"
                    + "　項目数: " + Count + "\r\n"
                    , description, MessageBoxButton.OKCancel, MessageBoxImage.Exclamation, MessageBoxResult.Cancel) == MessageBoxResult.Cancel)
                {
                    return false;
                }
            }
            return true;
        }

        public static bool? OpenEpgReserveDialog(EpgEventInfo Data, UIElement Owner, byte epgInfoOpenMode = 0)
        {
            try
            {
                var win = AddReserveEpgWindow.GetDataReplaceWindow() as AddReserveEpgWindow;
                if (win != null)
                {
                    win.SetEventInfo(Data);
                    return true;
                }

                var dlg = new AddReserveEpgWindow();
                dlg.SetEventInfo(Data, epgInfoOpenMode);
                dlg.Dispatcher.BeginInvoke(new Action(() => dlg.Show()));//番組表でのフォーカス対策
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return null;
        }

        public static bool? OpenChangeReserveDialog(ReserveData Data, UIElement Owner, byte epgInfoOpenMode = 0)
        {
            var win = ChgReserveWindow.GetDataReplaceWindow() as ChgReserveWindow;
            if (win != null)
            {
                win.ChangeReserveInfo(Data);
                return true;
            }
            return OpenChgReserveDialog(Data, Owner, epgInfoOpenMode);
        }
        public static bool? OpenManualReserveDialog(UIElement Owner)
        {
            return OpenChgReserveDialog(null, Owner);
        }
        public static bool? OpenChgReserveDialog(ReserveData Data, UIElement Owner, byte epgInfoOpenMode = 0)
        {
            try
            {
                var dlg = new ChgReserveWindow();
                if (Data != null)
                {
                    dlg.SetReserveInfo(Data, epgInfoOpenMode);
                }
                dlg.Dispatcher.BeginInvoke(new Action(() => dlg.Show()));//番組表でのフォーカス対策
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return null;
        }

        public static bool? OpenSearchEpgDialog()
        {
            return OpenEpgAutoAddDialog(null, AutoAddMode.Find);
        }
        public static bool? OpenAddEpgAutoAddDialog()
        {
            return OpenEpgAutoAddDialog(null, AutoAddMode.NewAdd);
        }
        public static bool? OpenChangeEpgAutoAddDialog(EpgAutoAddData Data)
        {
            var win = SearchWindow.GetDataReplaceWindow() as SearchWindow;
            if (win != null)
            {
                win.ChangeAutoAddData(Data);
                return true;
            }
            return OpenEpgAutoAddDialog(Data, AutoAddMode.Change);
        }
        private static bool? OpenEpgAutoAddDialog(EpgAutoAddData Data, AutoAddMode mode)
        {
            try
            {
                var dlg = new SearchWindow();
                dlg.SetViewMode(mode);
                if (Data != null)
                {
                    dlg.SetAutoAddData(Data);
                }
                dlg.Show();
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return null;
        }
        public static void SendAutoAdd(IBasicPgInfo item, bool NotToggle = false)
        {
            try
            {
                if (item == null) return;

                var dlg = new SearchWindow();
                dlg.SetViewMode(AutoAddMode.NewAdd);

                EpgSearchKeyInfo key = Settings.Instance.DefSearchKey.Clone();
                key.andKey = TrimEpgKeyword(item.DataTitle, NotToggle);
                key.regExpFlag = 0;
                key.serviceList.Clear();
                key.serviceList.Add((Int64)item.Create64Key());
                dlg.SetSearchKey(key);

                if (item is IRecSetttingData)
                {
                    var item_r = (item as IRecSetttingData);
                    RecPresetItem recPreSet = item_r.RecSettingInfo.LookUpPreset(item_r.IsManual, true);
                    RecSettingData recSet = recPreSet.RecPresetData;
                    if (recPreSet.IsCustom == true && recSet.RecMode == 5)
                    {
                        recSet.RecMode = 1;
                    }
                    dlg.SetRecSetting(recSet);
                }

                dlg.Show();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public static bool? OpenAddManualAutoAddDialog()
        {
            return OpenManualAutoAddDialog(null);
        }
        public static bool? OpenChangeManualAutoAddDialog(ManualAutoAddData Data)
        {
            var win = AddManualAutoAddWindow.GetDataReplaceWindow() as AddManualAutoAddWindow;
            if (win != null)
            {
                win.ChangeAutoAddData(Data);
                return true;
            }
            return OpenManualAutoAddDialog(Data);
        }
        public static bool? OpenManualAutoAddDialog(ManualAutoAddData Data)
        {
            try
            {
                var dlg = new AddManualAutoAddWindow();
                if (Data != null)
                {
                    dlg.SetViewMode(AutoAddMode.Change);
                    dlg.SetAutoAddData(Data);
                }
                dlg.Show();
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return null;
        }

        public static bool? OpenChangeAutoAddDialog(Type t, uint id)
        {
            AutoAddData autoAdd = AutoAddData.AutoAddList(t, id);
            if (t == typeof(EpgAutoAddData))
            {
                return OpenChangeEpgAutoAddDialog(autoAdd as EpgAutoAddData);
            }
            else if (t == typeof(ManualAutoAddData))
            {
                return OpenChangeManualAutoAddDialog(autoAdd as ManualAutoAddData);
            }
            return null;
        }

        public static bool? OpenRecInfoDialog(RecFileInfo info, UIElement Owner)
        {
            try
            {
                if (info == null) return null;

                var win = RecInfoDescWindow.GetDataReplaceWindow() as RecInfoDescWindow;
                if (win != null)
                {
                    win.SetRecInfo(info);
                    return true;
                }

                var dlg = new RecInfoDescWindow();
                dlg.SetRecInfo(info);
                dlg.Show();
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return null;
        }

        public static bool? OpenInfoSearchDialog(string word = null, bool NotToggle = false)
        {
            try
            {
                word = TrimKeywordCheckToggled(word, Settings.Instance.MenuSet.InfoSearchTitle_Trim, NotToggle);

                var dlg = new InfoSearchWindow();
                dlg.SetSearchWord(word);
                dlg.Show();
                return true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return null;
        }

        public static EpgEventInfo SearchEventInfoLikeThat(IAutoAddTargetData item, bool includeArc = false)
        {
            double dist = double.MaxValue;
            EpgEventInfo eventPossible = null;

            UInt64 key = item.Create64Key();
            var eventDic = CommonManager.Instance.DB.ServiceEventList;
            if (eventDic.ContainsKey(key) == true)
            {
                var eList = includeArc == true ? eventDic[key].eventMergeList : eventDic[key].eventList;
                foreach (EpgEventInfo eventChkInfo in eList)
                {
                    //itemが調べている番組に完全に含まれているならそれを選択する
                    double overlapLength = CulcOverlapLength(item.PgStartTime, item.PgDurationSecond,
                                                            eventChkInfo.start_time, eventChkInfo.durationSec);
                    if (overlapLength > 0 && overlapLength == item.PgDurationSecond)
                    {
                        eventPossible = eventChkInfo;
                        break;
                    }

                    //開始時間が最も近いものを選ぶ。同じ差なら時間が前のものを選ぶ
                    double dist1 = Math.Abs((item.PgStartTime - eventChkInfo.start_time).TotalSeconds);
                    if (overlapLength >= 0 && (dist > dist1 ||
                        dist == dist1 && (eventPossible == null || item.PgStartTime > eventChkInfo.start_time)))
                    {
                        dist = dist1;
                        eventPossible = eventChkInfo;
                        if (dist == 0) break;
                    }
                }
            }

            return eventPossible;
        }

        /// <summary>重複してない場合は負数が返る。</summary>
        public static double CulcOverlapLength(DateTime s1, uint d1, DateTime s2, uint d2)
        {
            TimeSpan ts1 = s1 + TimeSpan.FromSeconds(d1) - s2;
            TimeSpan ts2 = s2 + TimeSpan.FromSeconds(d2) - s1;
            return Math.Min(Math.Min(ts1.TotalSeconds, ts2.TotalSeconds), Math.Min(d1, d2));
        }

        public static List<EpgAutoAddData> FazySearchEpgAutoAddData(string title, bool? IsEnabled = null)
        {
            Func<string, string> _regulate_str = s => CommonManager.AdjustSearchText(TrimKeyword(s));

            string title_key = _regulate_str(title);

            List<EpgAutoAddData> list = CommonManager.Instance.DB.EpgAutoAddList.Values
                .Where(data => data.DataTitle != "" && title_key.Contains(_regulate_str(data.DataTitle)) == true).ToList();

            foreach (ReserveData info in CommonManager.Instance.DB.ReserveList.Values
                .Where(data => data.DataTitle != "" && title_key == _regulate_str(data.DataTitle)))
            {
                list.AddRange(info.GetEpgAutoAddList());
            }

            list = list.Distinct().ToList();
            return IsEnabled == null ? list : list.FindAll(data => data.IsEnabled == IsEnabled);
        }

        public static string ConvertAutoddTextMenu(AutoAddData data)
        {
            if(data is EpgAutoAddData)
            {
                return "キーワード予約:" + (data.DataTitle == "" ? "(空白)" : data.DataTitle);
            }
            else
            {
                var view = new ManualAutoAddDataItem(data as ManualAutoAddData);
                return "プログラム自動:" + string.Format("({0}){1} {2}", view.DayOfWeek, view.StartTimeShort, view.EventName == "" ? "(空白)" : view.EventName);
            }
        }
    }

}

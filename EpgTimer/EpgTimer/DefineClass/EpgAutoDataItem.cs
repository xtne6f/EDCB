using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace EpgTimer
{
    public class EpgAutoDataItem
    {
        public EpgAutoDataItem(EpgAutoAddData item)
        {
            this.EpgAutoAddInfo = item;
        }

        public EpgAutoAddData EpgAutoAddInfo
        {
            get;
            private set;
        }

        public string AndKey
        {
            get { return Regex.Replace(EpgAutoAddInfo.searchInfo.andKey, @"^(?:\^!\{999\})?(?:C!\{999\})?(?:D!\{1[0-9]{8}\})?", ""); }
        }
        public string NotKey
        {
            get { return Regex.Replace(EpgAutoAddInfo.searchInfo.notKey, "^:note:[^ 　]*[ 　]?", ""); }
        }
        public string Note
        {
            get
            {
                Match m = Regex.Match(EpgAutoAddInfo.searchInfo.notKey, "^:note:([^ 　]*)");
                if (m.Success)
                {
                    return m.Groups[1].Value.Replace("\\s", " ").Replace("\\m", "　").Replace("\\\\", "\\");
                }
                return "";
            }
        }
        public string RegExp
        {
            get { return EpgAutoAddInfo.searchInfo.regExpFlag == 1 ? "○" : "×"; }
        }
        public string Aimai
        {
            get { return EpgAutoAddInfo.searchInfo.aimaiFlag == 1 ? "○" : "×"; }
        }
        public string TitleOnly
        {
            get { return EpgAutoAddInfo.searchInfo.titleOnlyFlag == 1 ? "○" : "×"; }
        }
        public string DateKey
        {
            get
            {
                if (EpgAutoAddInfo.searchInfo.dateList.Count < 1)
                {
                    return "なし";
                }
                if (EpgAutoAddInfo.searchInfo.dateList.Count > 1)
                {
                    return "複数指定";
                }
                EpgSearchDateInfo info = EpgAutoAddInfo.searchInfo.dateList[0];
                return (new DateTime(2000, 1, 2 + info.startDayOfWeek % 7, info.startHour % 24, info.startMin % 60, 0)).ToString("ddd HH\\:mm") +
                       (new DateTime(2000, 1, 2 + info.endDayOfWeek % 7, info.endHour % 24, info.endMin % 60, 0)).ToString(" ～ ddd HH\\:mm");
            }
        }
        public string RecEnabled
        {
            get { return EpgAutoAddInfo.recSetting.IsNoRec() ? "いいえ" : "はい"; }
        }
        public string RecMode
        {
            get { return CommonManager.Instance.RecModeList[EpgAutoAddInfo.recSetting.GetRecMode()]; }
        }
        public byte Priority
        {
            get { return EpgAutoAddInfo.recSetting.Priority; }
        }
        public string Tuijyu
        {
            get { return EpgAutoAddInfo.recSetting.TuijyuuFlag == 1 ? "する" : "しない"; }
        }
        public uint AddCount
        {
            get { return EpgAutoAddInfo.addCount; }
        }

        public string JyanruKey
        {
            get
            {
                string view = "";
                {
                    // 小ジャンルを大ジャンルでまとめる
                    foreach (EpgContentData ecd1 in this.EpgAutoAddInfo.searchInfo.contentList)
                    {
                        int nibble1 = ecd1.content_nibble_level_1;
                        int nibble2 = ecd1.content_nibble_level_2;
                        string name;
                        if (CommonManager.Instance.ContentKindDictionary.TryGetValue((ushort)(nibble1 << 8 | 0xFF), out name) == false)
                        {
                            name = "(0x" + nibble1.ToString("X2") + ")";
                        }
                        string key = "[" + name + " - ";
                        int i = view.IndexOf(key, StringComparison.Ordinal);
                        if (i < 0)
                        {
                            key = "[" + name + "]";
                            i = view.IndexOf(key, StringComparison.Ordinal);
                            if (i < 0)
                            {
                                view += (view.Length > 0 ? ", " : "") + key;
                                i = view.Length - 1;
                            }
                        }
                        if (nibble2 != 0xFF)
                        {
                            if (CommonManager.Instance.ContentKindDictionary.TryGetValue((ushort)(nibble1 << 8 | nibble2), out name) == false)
                            {
                                name = "(0x" + nibble2.ToString("X2") + ")";
                            }
                            view = view.Insert(view.IndexOf(']', i), (key[key.Length - 1] == ']' ? " - " : ", ") + name);
                        }
                    }
                }
                return view;
            }
        }

        /// <summary>
        /// NHK総合１・東京、NHKBS1
        /// </summary>
        public string ServiceKey
        {
            get
            {
                string view = "";
                {
                    foreach (ulong service1 in EpgAutoAddInfo.searchInfo.serviceList)
                    {
                        try
                        {
                            if (view != "") { view += ", "; }
                            view += ChSet5.Instance.ChList[service1].ServiceName;
                        }
                        catch
                        {
                            view += "(x_x)";
                        }
                    }
                    //if (EpgAutoAddInfo.searchInfo.serviceList.Count == 1)
                    //{
                    //    try
                    //    {
                    //        view = ChSet5.Instance.ChList[(ulong)EpgAutoAddInfo.searchInfo.serviceList[0]].ServiceName;
                    //    }
                    //    catch
                    //    {
                    //        view = "検索エラー";
                    //    }
                    //}
                    //else if (EpgAutoAddInfo.searchInfo.serviceList.Count > 1)
                    //{
                    //    view = "複数指定";
                    //}
                }
                if (view == "")
                {
                    view = "なし";
                }
                return view;
            }
        }

        /// <summary>
        /// 地デジ、BS、CS
        /// </summary>
        public string NetworkKey
        {
            get
            {
                string view1 = "";
                {
                    List<string> networkKeyList1 = new List<string>();
                    foreach (ulong service1 in this.EpgAutoAddInfo.searchInfo.serviceList)
                    {
                        ushort onid = (ushort)(service1 >> 32);
                        string network1 = ChSet5.IsDttv(onid) ? "地デジ" : ChSet5.IsBS(onid) ? "BS" : ChSet5.IsCS(onid) ? "CS" : "その他";
                        if (!networkKeyList1.Contains(network1))
                        {
                            networkKeyList1.Add(network1);
                        }
                    }
                    foreach (string network1 in networkKeyList1)
                    {
                        if (view1 != "") { view1 += ", "; }
                        view1 += network1;
                    }
                }
                if (view1 == "")
                {
                    view1 = "なし";
                }
                return view1;
            }
        }

        public string Pittari
        {
            get { return EpgAutoAddInfo.recSetting.PittariFlag == 1 ? "する" : "しない"; }
        }

        public string KeyEnabled
        {
            get { return EpgAutoAddInfo.searchInfo.andKey.StartsWith("^!{999}", StringComparison.Ordinal) ? "いいえ" : "はい"; }
        }

        public string CaseSensitive
        {
            get { return Regex.IsMatch(EpgAutoAddInfo.searchInfo.andKey, @"^(?:\^!\{999\})?C!\{999\}") ? "はい" : "いいえ"; }
        }

        public string TunerID
        {
            get { return EpgAutoAddInfo.recSetting.TunerID == 0 ? "自動" : "ID:" + EpgAutoAddInfo.recSetting.TunerID.ToString("X8"); }
        }

        public string BatFilePath
        {
            get
            {
                int i = EpgAutoAddInfo.recSetting.BatFilePath.IndexOf('*');
                return i < 0 ? EpgAutoAddInfo.recSetting.BatFilePath : EpgAutoAddInfo.recSetting.BatFilePath.Remove(i);
            }
        }

        public string BatFileTag
        {
            get
            {
                int i = EpgAutoAddInfo.recSetting.BatFilePath.IndexOf('*');
                return i < 0 ? "" : EpgAutoAddInfo.recSetting.BatFilePath.Substring(i + 1);
            }
        }

        public uint ID
        {
            get { return EpgAutoAddInfo.dataID; }
        }

        public SolidColorBrush BackColor
        {
            get { return Settings.Instance.ResColorPosition == 0 ? KeyEnabledBackColor : null; }
        }

        public SolidColorBrush AlternationBackColor
        {
            get { return (Settings.Instance.ResColorPosition == 0 ? KeyEnabledBackColor : null) ?? Settings.BrushCache.ResDefBrush; }
        }

        public SolidColorBrush AndKeyBackColor
        {
            get { return Settings.Instance.ResColorPosition != 0 ? KeyEnabledBackColor : null; }
        }

        private SolidColorBrush KeyEnabledBackColor
        {
            get
            {
                return EpgAutoAddInfo.searchInfo.andKey.StartsWith("^!{999}", StringComparison.Ordinal) ?
                       Settings.BrushCache.ResNoBrush : null;
            }
        }

        public TextBlock ToolTipView
        {
            get
            {
                if (Settings.Instance.NoToolTip == true)
                {
                    return null;
                }
                string view = "";
                {
                    if (EpgAutoAddInfo.searchInfo != null)
                    {
                        view += "検索条件\r\n";
                        view += "Andキーワード：" + AndKey + "\r\n";
                        view += "Notキーワード：" + NotKey + "\r\n";
                        view += "正規表現モード：" + RegExp + "\r\n";
                        view += "番組名のみ検索対象：" + TitleOnly + "\r\n";
                        view += "ジャンル絞り込み：" + JyanruKey + "\r\n";
                        view += "時間絞り込み：" + DateKey + "\r\n";
                        view += "検索対象サービス：" + ServiceKey;
                    }
                    if (EpgAutoAddInfo.recSetting != null)
                    {
                        view += (view.Length > 0 ? "\r\n\r\n" : "") +
                                "録画設定\r\n" +
                                CommonManager.Instance.ConvertRecSettingText(EpgAutoAddInfo.recSetting);
                    }
                }

                TextBlock block = new TextBlock();
                block.Text = view;
                block.MaxWidth = 400;
                block.TextWrapping = TextWrapping.Wrap;
                return block;
            }
        }

        public Brush BorderBrush
        {
            get
            {
                if (EpgAutoAddInfo.searchInfo.contentList.Count > 0 &&
                    (EpgAutoAddInfo.searchInfo.contentList[0].content_nibble_level_1 <= 0x0B ||
                     EpgAutoAddInfo.searchInfo.contentList[0].content_nibble_level_1 == 0x0F))
                {
                    return Settings.BrushCache.ContentBrushList[EpgAutoAddInfo.searchInfo.contentList[0].content_nibble_level_1];
                }
                return null;
            }
        }

    }
}

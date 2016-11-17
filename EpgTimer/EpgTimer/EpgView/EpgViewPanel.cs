using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace EpgTimer.EpgView
{
    using EpgTimer.UserCtrlView;

    class EpgViewPanel : PanelBase
    {
        public List<ProgramViewItem> Items { get; set; }

        protected List<List<TextDrawItem>> CreateDrawTextList(double offset)
        {
            if (Items == null) return null;

            var textDrawLists = new List<List<TextDrawItem>>(Items.Count);

            try
            {
                var ItemFontNormal = ItemFontCache.ItemFont(Settings.Instance.FontName, false);
                var ItemFontTitle = ItemFontCache.ItemFont(Settings.Instance.FontNameTitle, Settings.Instance.FontBoldTitle);
                if (ItemFontNormal.GlyphType == null || ItemFontTitle.GlyphType == null)
                {
                    return null;
                }

                double sizeMin = Settings.Instance.FontSizeTitle - 1;
                double sizeTitle = Settings.Instance.FontSizeTitle;
                double sizeNormal = Settings.Instance.FontSize;
                double indentTitle = sizeMin * 1.7;
                double indentNormal = Settings.Instance.EpgTitleIndent ? indentTitle : 3;
                Brush colorTitle = CommonManager.Instance.CustTitle1Color;
                Brush colorNormal = CommonManager.Instance.CustTitle2Color;

                foreach (ProgramViewItem info in Items)
                {
                    var textDrawList = new List<TextDrawItem>();
                    textDrawLists.Add(textDrawList);

                    if (info.Height > 2)
                    {
                        if (info.Height < sizeTitle + 3)
                        {
                            //高さ足りない
                            info.TitleDrawErr = true;
                        }

                        double totalHeight = 0;
                        double useHeight = 0;
                        double leftPos = info.LeftPos - offset;

                        //分
                        string min = (info.EventInfo.StartTimeFlag == 0 ? "未定 " : info.EventInfo.start_time.Minute.ToString("d02"));
                        if (RenderText(textDrawList, min, ItemFontTitle, sizeMin, info.Width - 4, info.Height + 10, leftPos, info.TopPos, ref useHeight, colorTitle) == false)
                        {
                            info.TitleDrawErr = true;
                            continue;
                        }

                        //番組情報
                        if (info.EventInfo.ShortInfo != null)
                        {
                            //タイトル
                            if (info.EventInfo.ShortInfo.event_name.Length > 0)
                            {
                                if (RenderText(textDrawList, info.EventInfo.ShortInfo.event_name, ItemFontTitle, sizeTitle, info.Width - 6 - indentTitle, info.Height - 3 - totalHeight, leftPos + indentTitle, info.TopPos + totalHeight, ref useHeight, colorTitle) == false)
                                {
                                    info.TitleDrawErr = true;
                                    continue;
                                }
                                totalHeight += useHeight + sizeTitle / 3;
                            }
                            //説明
                            if (info.EventInfo.ShortInfo.text_char.Length > 0)
                            {
                                if (RenderText(textDrawList, info.EventInfo.ShortInfo.text_char, ItemFontNormal, sizeNormal, info.Width - 6 - indentNormal, info.Height - 7 - totalHeight, leftPos + indentNormal, info.TopPos + totalHeight, ref useHeight, colorNormal) == false)
                                {
                                    continue;
                                }
                                totalHeight += useHeight + sizeNormal;
                            }

                            //詳細
                            if (Settings.Instance.EpgExtInfoTable == true && info.EventInfo.ExtInfo != null && info.EventInfo.ExtInfo.text_char.Length > 0)
                            {
                                if (RenderText(textDrawList, info.EventInfo.ExtInfo.text_char, ItemFontNormal, sizeNormal, info.Width - 6 - indentNormal, info.Height - 7 - totalHeight, leftPos + indentNormal, info.TopPos + totalHeight, ref useHeight, colorNormal) == false)
                                {
                                    continue;
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }

            return textDrawLists;
        }

        protected override void OnRender(DrawingContext dc)
        {
            try
            {
                dc.DrawRectangle(Background, null, new Rect(RenderSize));

                double offset = Canvas.GetLeft(this);
                List<List<TextDrawItem>> textDrawLists = CreateDrawTextList(offset);
                if (Items == null || textDrawLists == null || Items.Count < textDrawLists.Count)
                {
                    return;
                }

                for (int i = 0; i < textDrawLists.Count; i++)
                {
                    ProgramViewItem info = Items[i];
                    dc.DrawRectangle(info.BorderBrush, null, new Rect(info.LeftPos - offset, info.TopPos, info.Width + 1, Math.Max(info.Height + 0.5, 1)));
                    if (info.Height > 1)
                    {
                        var textArea = new Rect(info.LeftPos - offset + 1, info.TopPos + 0.5, info.Width - 1, info.Height - 0.5);
                        dc.DrawRectangle(info.ContentColor, null, textArea);
                        DrawTextDrawList(dc, textDrawLists[i], textArea);
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
    }
}

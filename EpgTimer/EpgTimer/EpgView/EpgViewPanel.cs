using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace EpgTimer.EpgView
{
    class EpgViewPanel : EpgTimer.UserCtrlView.PanelBase
    {
        public List<ProgramViewItem> Items { get; set; }

        public EpgViewPanel()
        {
            // これらの設定を OnRender 中に行うと、再度 OnRender イベントが発生してしまうようだ。
            // 2度同じ Render を行うことになりパフォーマンスを落とすので、OnRender の外に出しておく。
            this.VisualTextRenderingMode = TextRenderingMode.ClearType;
            this.VisualTextHintingMode = TextHintingMode.Fixed;
            this.UseLayoutRounding = true;
        }

        protected List<List<TextDrawItem>> CreateDrawTextList()
        {
            if (Items == null) return null;

            Matrix m = PresentationSource.FromVisual(Application.Current.MainWindow).CompositionTarget.TransformToDevice;
            var textDrawLists = new List<List<TextDrawItem>>(Items.Count);

            var ItemFontNormal = ItemFontCache.ItemFont(Settings.Instance.FontName, false);
            var ItemFontTitle = ItemFontCache.ItemFont(Settings.Instance.FontNameTitle, Settings.Instance.FontBoldTitle);
            if (ItemFontNormal.GlyphType == null || ItemFontTitle.GlyphType == null)
            {
                return null;
            }

            try
            {
                double selfLeft = Canvas.GetLeft(this); 
                double sizeMin = Settings.Instance.FontSizeTitle - 1;
                double sizeTitle = Settings.Instance.FontSizeTitle;
                double sizeNormal = Settings.Instance.FontSize;
                double indentTitle = Math.Floor(sizeMin * 1.7);
                double indentNormal = Math.Floor(Settings.Instance.EpgTitleIndent ? indentTitle : 2);
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

                        //分
                        string min = (info.EventInfo.StartTimeFlag == 0 ? "未定 " : info.EventInfo.start_time.Minute.ToString("d02"));
                        double useHeight = 0;
                        if (RenderText(min, ref textDrawList, ItemFontTitle, sizeMin, info.Width - 4, info.Height + 10, info.LeftPos - selfLeft - 1, info.TopPos - 2, ref useHeight, colorTitle, m) == false)
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
                                if (RenderText(info.EventInfo.ShortInfo.event_name, ref textDrawList, ItemFontTitle, sizeTitle, info.Width - 6 - indentTitle, info.Height - 3 - totalHeight, info.LeftPos - selfLeft + indentTitle, info.TopPos - 2 + totalHeight, ref useHeight, colorTitle, m) == false)
                                {
                                    info.TitleDrawErr = true;
                                    continue;
                                }
                                totalHeight += Math.Floor(useHeight + (sizeTitle / 3));
                            }
                            //説明
                            if (info.EventInfo.ShortInfo.text_char.Length > 0)
                            {
                                if (RenderText(info.EventInfo.ShortInfo.text_char, ref textDrawList, ItemFontNormal, sizeNormal, info.Width - 6 - indentNormal, info.Height - 7 - totalHeight, info.LeftPos - selfLeft + indentNormal, info.TopPos - 2 + totalHeight, ref useHeight, colorNormal, m) == false)
                                {
                                    continue;
                                }
                                totalHeight += useHeight + sizeNormal;
                            }

                            //詳細
                            if (Settings.Instance.EpgExtInfoTable == true && info.EventInfo.ExtInfo != null && info.EventInfo.ExtInfo.text_char.Length > 0)
                            {
                                if (RenderText(info.EventInfo.ExtInfo.text_char, ref textDrawList, ItemFontNormal, sizeNormal, info.Width - 6 - indentNormal, info.Height - 7 - totalHeight, info.LeftPos - selfLeft + indentNormal, info.TopPos - 2 + totalHeight, ref useHeight, colorNormal, m) == false)
                                {
                                    continue;
                                }
                                totalHeight += useHeight + sizeNormal;
                            }
                        }
                    }
                }
                return textDrawLists;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return null;
        }

        protected bool RenderText(String text, ref List<TextDrawItem> textDrawList, ItemFont itemFont, double fontSize, double maxWidth, double maxHeight, double x, double y, ref double useHeight, Brush fontColor, Matrix m)
        {
            double totalHeight = 0;

            string[] lineText = text.Replace("\r", "").Split('\n');
            foreach (string line in lineText)
            {
                totalHeight += Math.Floor(2 + fontSize);
                var glyphIndexes = new List<ushort>();
                var advanceWidths = new List<double>();
                double totalWidth = 0;
                for (int n = 0; n < line.Length; n++)
                {
                    //この辞書検索が負荷の大部分を占めているのでテーブルルックアップする
                    //ushort glyphIndex = itemFont.GlyphType.CharacterToGlyphMap[line[n]];
                    //double width = itemFont.GlyphType.AdvanceWidths[glyphIndex] * fontSize;
                    ushort glyphIndex = itemFont.GlyphIndex(line[n]);
                    double width = itemFont.GlyphWidth(glyphIndex) * fontSize;
                    if (totalWidth + width > maxWidth)
                    {
                        if (glyphIndexes.Count > 0)
                        {
                            double dpix = Math.Ceiling((x + 2) * m.M11);
                            double dpiy = Math.Ceiling((y + totalHeight) * m.M22);
                            var origin = new Point(dpix / m.M11, dpiy / m.M22);
                            var item = new TextDrawItem();
                            item.FontColor = fontColor;
                            item.Text = new GlyphRun(itemFont.GlyphType, 0, false, fontSize,
                                glyphIndexes, origin, advanceWidths, null, null, null, null,
                                null, null);
                            textDrawList.Add(item);

                        }
                        if (totalHeight > maxHeight)
                        {
                            //次の行無理
                            useHeight = totalHeight;
                            return false;
                        }
                        else
                        {
                            //次の行いける
                            totalHeight += fontSize + 2;

                            glyphIndexes = new List<ushort>();
                            advanceWidths = new List<double>();
                            totalWidth = 0;
                        }
                    }
                    glyphIndexes.Add(glyphIndex);
                    advanceWidths.Add(width);
                    totalWidth += width;
                }
                if (glyphIndexes.Count > 0)
                {
                    double dpix = Math.Ceiling((x + 2) * m.M11);
                    double dpiy = Math.Ceiling((y + totalHeight) * m.M22);
                    var origin = new Point(dpix / m.M11, dpiy / m.M22);
                    var item = new TextDrawItem();
                    item.FontColor = fontColor;
                    item.Text = new GlyphRun(itemFont.GlyphType, 0, false, fontSize,
                        glyphIndexes, origin, advanceWidths, null, null, null, null,
                        null, null);
                    textDrawList.Add(item);

                }
            }
            useHeight = Math.Floor(totalHeight);
            return true;
        }

        protected override void OnRender(DrawingContext dc)
        {
            Brush bgBrush = Background;
            dc.DrawRectangle(bgBrush, null, new Rect(RenderSize));
            List<List<TextDrawItem>> textDrawLists = CreateDrawTextList();

            if (Items == null || textDrawLists == null || Items.Count < textDrawLists.Count)
            {
                return;
            }

            try
            {
                double selfLeft = Canvas.GetLeft(this); 
                double sizeNormal = Settings.Instance.FontSize;
                double sizeTitle = Settings.Instance.FontSizeTitle;

                for (int i = 0; i < textDrawLists.Count; i++)
                {
                    ProgramViewItem info = Items[i];
                    dc.DrawRectangle(bgBrush, null, new Rect(info.LeftPos - selfLeft, info.TopPos, info.Width, 1));
                    dc.DrawRectangle(bgBrush, null, new Rect(info.LeftPos - selfLeft, info.TopPos + info.Height, info.Width, 1));
                    if (info.Height > 1)
                    {
                        dc.DrawRectangle(info.ContentColor, null, new Rect(info.LeftPos - selfLeft, info.TopPos + 0.5, info.Width - 1, info.Height - 0.5));
                        dc.PushClip(new RectangleGeometry(new Rect(info.LeftPos - selfLeft, info.TopPos + 0.5, info.Width - 1, info.Height - 0.5)));
                        foreach (TextDrawItem txtinfo in textDrawLists[i])
                        {
                            dc.DrawGlyphRun(txtinfo.FontColor, txtinfo.Text);
                        }
                        dc.Pop();
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
    }

    class TextDrawItem
    {
        public Brush FontColor;
        public GlyphRun Text;
    }
}

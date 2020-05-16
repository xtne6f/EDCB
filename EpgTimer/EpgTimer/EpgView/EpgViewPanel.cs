using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.ComponentModel;
using System.Windows.Input;

namespace EpgTimer.EpgView
{
    class EpgViewPanel : FrameworkElement
    {
        public class ItemFont
        {
            public bool NoCache { get; private set; }
            public GlyphTypeface GlyphType { get; private set; }
            public GlyphTypeface FallbackGlyphType { get; private set; }
            public ushort[] GlyphIndexCache { get; private set; }
            public float[] GlyphWidthCache { get; private set; }

            public ItemFont(string familyName, bool isBold, bool noCache)
            {
                NoCache = noCache;
                GlyphTypeface glyphType;
                int fallback = familyName.IndexOf(',');
                if ((new Typeface(new FontFamily(fallback < 0 ? familyName : familyName.Remove(fallback).Trim()),
                                  FontStyles.Normal,
                                  isBold ? FontWeights.Bold : FontWeights.Normal,
                                  FontStretches.Normal)).TryGetGlyphTypeface(out glyphType))
                {
                    GlyphType = glyphType;
                }
                if (fallback >= 0 && (new Typeface(new FontFamily(familyName.Substring(fallback + 1).Trim()),
                                                   FontStyles.Normal,
                                                   isBold ? FontWeights.Bold : FontWeights.Normal,
                                                   FontStretches.Normal)).TryGetGlyphTypeface(out glyphType))
                {
                    if (GlyphType == null)
                    {
                        GlyphType = glyphType;
                    }
                    else
                    {
                        FallbackGlyphType = glyphType;
                    }
                }
                if (GlyphType == null && (new Typeface(SystemFonts.MessageFontFamily,
                                                       FontStyles.Normal,
                                                       isBold ? FontWeights.Bold : FontWeights.Normal,
                                                       FontStretches.Normal)).TryGetGlyphTypeface(out glyphType))
                {
                    GlyphType = glyphType;
                }
            }
            public void PrepareCache()
            {
                if (NoCache == false && GlyphIndexCache == null)
                {
                    GlyphIndexCache = new ushort[ushort.MaxValue + 1];
                    GlyphWidthCache = new float[ushort.MaxValue + 1];
                }
            }
            public void ClearCache()
            {
                GlyphIndexCache = null;
                GlyphWidthCache = null;
            }
        }

        public static readonly DependencyProperty BackgroundProperty =
            Panel.BackgroundProperty.AddOwner(typeof(EpgViewPanel));
        private double _borderLeftSize;
        private double _borderTopSize;
        private List<List<Tuple<Brush, GlyphRun>>> textDrawLists;
        public Brush Background
        {
            set { SetValue(BackgroundProperty, value); }
            get { return (Brush)GetValue(BackgroundProperty); }
        }

        public List<ProgramViewItem> Items
        {
            get;
            private set;
        }

        public byte FilteredAlpha
        {
            get;
            private set;
        }

        public List<Brush> ContentBrushList
        {
            get;
            private set;
        }

        public double LastItemRenderTextHeight
        {
            get;
            private set;
        }

        public void Initialize(List<ProgramViewItem> items, double borderLeftSize, double borderTopSize,
                               bool drawHour, bool isTitleIndent, bool extInfoMode,
                               Dictionary<char, List<KeyValuePair<string, string>>> replaceDictionaryTitle,
                               Dictionary<char, List<KeyValuePair<string, string>>> replaceDictionaryNormal,
                               ItemFont itemFontTitle, ItemFont itemFontNormal, double sizeTitle, double sizeNormal, Brush brushTitle, Brush brushNormal,
                               byte filteredAlpha, List<Brush> contentBrushList)
        {
            LastItemRenderTextHeight = 0;
            textDrawLists = null;
            Items = items;
            FilteredAlpha = filteredAlpha;
            ContentBrushList = contentBrushList;
            var ps = PresentationSource.FromVisual(Application.Current.MainWindow);
            if (ps != null && itemFontTitle.GlyphType != null && itemFontNormal.GlyphType != null)
            {
                Matrix m = ps.CompositionTarget.TransformToDevice;
                textDrawLists = new List<List<Tuple<Brush, GlyphRun>>>(Items.Count);
                _borderLeftSize = borderLeftSize;
                _borderTopSize = borderTopSize;
                itemFontTitle.PrepareCache();
                itemFontNormal.PrepareCache();

                //ProgramViewItemの座標系は番組表基準なので、この時点でCanvas.SetLeft()によりEpgViewPanel自身の座標を添付済みでなければならない
                double selfLeft = Canvas.GetLeft(this);
                sizeTitle = Math.Max(sizeTitle, 1);
                sizeNormal = Math.Max(sizeNormal, 1);
                double indentTitle = sizeTitle * 1.7;
                double indentNormal = isTitleIndent ? indentTitle : 2;

                foreach (ProgramViewItem info in Items)
                {
                    var textDrawList = new List<Tuple<Brush, GlyphRun>>();
                    textDrawLists.Add(textDrawList);

                    double innerLeft = info.LeftPos + borderLeftSize / 2;
                    //0.26は細枠線での微調整
                    double innerTop = info.TopPos + borderTopSize / 2 - 0.26;
                    double innerWidth = info.Width - borderLeftSize;
                    double innerHeight = info.Height - borderTopSize;
                    double useHeight = 0;

                    if (drawHour && isTitleIndent)
                    {
                        //時
                        string hour = (info.EventInfo.StartTimeFlag == 0 ? "?" : info.EventInfo.start_time.Hour.ToString()) + ":";
                        RenderText(hour, textDrawList, itemFontTitle, sizeTitle * 0.8, innerWidth - 1, innerHeight,
                                   innerLeft + 1, innerTop, out useHeight, brushTitle, m, selfLeft);
                    }
                    //分
                    string min = (info.EventInfo.StartTimeFlag == 0 ? "?" : info.EventInfo.start_time.Minute.ToString("d02"));
                    RenderText(min, textDrawList, itemFontTitle, sizeTitle * 0.95, innerWidth - 1, innerHeight,
                               innerLeft + 1, innerTop + useHeight, out useHeight, brushTitle, m, selfLeft);

                    //タイトル
                    string title = info.EventInfo.ShortInfo == null ? "" : info.EventInfo.ShortInfo.event_name;
                    if (replaceDictionaryTitle != null)
                    {
                        title = CommonManager.ReplaceText(title, replaceDictionaryTitle);
                    }
                    if (RenderText(title.Length > 0 ? title : " ", textDrawList, itemFontTitle, sizeTitle,
                                   innerWidth - sizeTitle * 0.5 - indentTitle, innerHeight,
                                   innerLeft + indentTitle, innerTop, out useHeight, brushTitle, m, selfLeft) == false)
                    {
                        info.TitleDrawErr = true;
                        LastItemRenderTextHeight = info.Height;
                        continue;
                    }
                    if (info.Height < sizeTitle)
                    {
                        //高さ足りない
                        info.TitleDrawErr = true;
                    }
                    LastItemRenderTextHeight = useHeight + sizeNormal * 0.5;

                    if (info.EventInfo.ShortInfo != null)
                    {
                        //説明
                        string detail = info.EventInfo.ShortInfo.text_char;
                        //詳細
                        detail += extInfoMode == false || info.EventInfo.ExtInfo == null ? "" : "\r\n\r\n" + info.EventInfo.ExtInfo.text_char;
                        if (replaceDictionaryNormal != null)
                        {
                            detail = CommonManager.ReplaceText(detail, replaceDictionaryNormal);
                        }
                        RenderText(detail, textDrawList, itemFontNormal, sizeNormal,
                                   innerWidth - sizeTitle * 0.5 - indentNormal, innerHeight - LastItemRenderTextHeight,
                                   innerLeft + indentNormal, innerTop + LastItemRenderTextHeight, out useHeight, brushNormal, m, selfLeft);
                        LastItemRenderTextHeight += useHeight + sizeNormal * 0.25;
                    }
                    LastItemRenderTextHeight = Math.Floor(LastItemRenderTextHeight + borderTopSize + sizeNormal * 0.5);
                    LastItemRenderTextHeight = Math.Min(LastItemRenderTextHeight, info.Height);
                }
            }
        }

        private static GlyphRun CreateGlyphRun(double x, double y, Matrix m, double selfLeft,
                                               GlyphTypeface glyphType, double fontSize, List<ushort> indexes, List<double> widths)
        {
            //originを物理ピクセルに合わせる。selfLeftはレンダリング時に加算されるので混ぜてはいけない
            Point origin = new Point(Math.Ceiling(x * m.M11) / m.M11 - selfLeft, Math.Ceiling(y * m.M22) / m.M22);
            return new GlyphRun(glyphType, 0, false, fontSize, indexes, origin, widths, null, null, null, null, null, null);
        }

        public static bool RenderText(string text, List<Tuple<Brush, GlyphRun>> textDrawList, ItemFont itemFont, double fontSize,
                                      double maxWidth, double maxHeight, double x, double y,
                                      out double totalHeight, Brush fontColor, Matrix m, double selfLeft)
        {
            totalHeight = 0;

            for (int i = 0; i < text.Length; )
            {
                if (totalHeight > maxHeight)
                {
                    //次の行無理
                    return false;
                }
                //行間は2行目から適用する
                totalHeight += totalHeight == 0 ? fontSize : fontSize * 1.2;
                List<ushort> glyphIndexes = new List<ushort>();
                List<double> advanceWidths = new List<double>();
                double totalWidth = 0;
                double originWidth = 0;
                GlyphTypeface currentGlyphType = null;
                for (; i < text.Length && text[i] != '\r' && text[i] != '\n'; i++)
                {
                    //この辞書検索が負荷の大部分を占めているのでテーブルルックアップする
                    //ushort glyphIndex = itemFont.GlyphType.CharacterToGlyphMap[text[i]];
                    //double width = itemFont.GlyphType.AdvanceWidths[glyphIndex] * fontSize;
                    ushort glyphIndex = itemFont.NoCache ? (ushort)0 : itemFont.GlyphIndexCache[text[i]];
                    double glyphWidth;
                    GlyphTypeface glyphType = itemFont.GlyphType;
                    if (glyphIndex == 0)
                    {
                        //NoCacheまたはキャッシュミス
                        int key = 0;
                        if (char.IsSurrogatePair(text, i))
                        {
                            key = char.ConvertToUtf32(text, i++);
                        }
                        else if (char.IsSurrogate(text[i]) == false)
                        {
                            key = text[i];
                        }
                        if (glyphType.CharacterToGlyphMap.TryGetValue(key, out glyphIndex) || itemFont.FallbackGlyphType == null)
                        {
                            glyphType.AdvanceWidths.TryGetValue(glyphIndex, out glyphWidth);
                            if (itemFont.NoCache == false && key < itemFont.GlyphIndexCache.Length)
                            {
                                itemFont.GlyphIndexCache[key] = glyphIndex;
                                itemFont.GlyphWidthCache[glyphIndex] = (float)glyphWidth;
                            }
                        }
                        else
                        {
                            glyphType = itemFont.FallbackGlyphType;
                            glyphType.CharacterToGlyphMap.TryGetValue(key, out glyphIndex);
                            glyphType.AdvanceWidths.TryGetValue(glyphIndex, out glyphWidth);
                        }
                        glyphWidth = (float)glyphWidth;
                    }
                    else
                    {
                        glyphWidth = itemFont.GlyphWidthCache[glyphIndex];
                    }
                    double width = glyphWidth * fontSize;

                    if (totalWidth + width > maxWidth)
                    {
                        if (glyphIndexes.Count > 0)
                        {
                            textDrawList.Add(new Tuple<Brush, GlyphRun>(fontColor, CreateGlyphRun(
                                x + originWidth, y + totalHeight, m, selfLeft, currentGlyphType, fontSize, glyphIndexes, advanceWidths)));
                            glyphIndexes = new List<ushort>();
                            advanceWidths = new List<double>();
                        }
                        if (totalHeight > maxHeight)
                        {
                            //次の行無理
                            return false;
                        }
                        totalHeight += fontSize * 1.2;
                        originWidth = totalWidth = 0;
                    }
                    if (glyphType != currentGlyphType)
                    {
                        //フォントが変わった
                        if (glyphIndexes.Count > 0)
                        {
                            textDrawList.Add(new Tuple<Brush, GlyphRun>(fontColor, CreateGlyphRun(
                                x + originWidth, y + totalHeight, m, selfLeft, currentGlyphType, fontSize, glyphIndexes, advanceWidths)));
                            glyphIndexes = new List<ushort>();
                            advanceWidths = new List<double>();
                        }
                        currentGlyphType = glyphType;
                        originWidth = totalWidth;
                    }
                    glyphIndexes.Add(glyphIndex);
                    advanceWidths.Add(width);
                    totalWidth += width;
                }
                if (glyphIndexes.Count > 0)
                {
                    textDrawList.Add(new Tuple<Brush, GlyphRun>(fontColor, CreateGlyphRun(
                        x + originWidth, y + totalHeight, m, selfLeft, currentGlyphType, fontSize, glyphIndexes, advanceWidths)));
                }
                i = text.IndexOf('\n', i);
                i = i < 0 ? text.Length : i + 1;
            }
            return true;
        }

        protected override void OnRender(DrawingContext dc)
        {
            dc.DrawRectangle(Background, null, new Rect(RenderSize));

            if (Items != null && textDrawLists != null && Items.Count >= textDrawLists.Count &&
                ContentBrushList != null && ContentBrushList.Count >= 17)
            {
                double selfLeft = Canvas.GetLeft(this);
                Brush bgBrush = Background;
                Brush filteredBrush = null;
                if (bgBrush is SolidColorBrush)
                {
                    Color c = ((SolidColorBrush)bgBrush).Color;
                    filteredBrush = new SolidColorBrush(Color.FromArgb(FilteredAlpha, c.R, c.G, c.B));
                    filteredBrush.Freeze();
                }
                //位置がずれないように枠線の幅が1より大きいときは両側で分け合う
                double borderLeft = _borderLeftSize > 1 ? _borderLeftSize / 2 : _borderLeftSize;
                double borderTop = _borderTopSize > 1 ? _borderTopSize / 2 : _borderTopSize;
                for (int i = 0; i < textDrawLists.Count; i++)
                {
                    ProgramViewItem info = Items[i];
                    double bgHeight = Math.Min(borderTop, info.Height);
                    if (info.Width > _borderLeftSize && bgHeight > 0)
                    {
                        dc.DrawRectangle(bgBrush, null, new Rect(info.LeftPos + borderLeft - selfLeft, info.TopPos, info.Width - _borderLeftSize, bgHeight));
                        dc.DrawRectangle(bgBrush, null, new Rect(info.LeftPos + borderLeft - selfLeft, info.TopPos + info.Height - bgHeight, info.Width - _borderLeftSize, bgHeight));
                    }
                    if (info.Width > _borderLeftSize && info.Height > _borderTopSize)
                    {
                        Brush contentBrush = ContentBrushList[0x10];
                        if (info.EventInfo.ContentInfo != null)
                        {
                            foreach (EpgContentData nibble in info.EventInfo.ContentInfo.nibbleList)
                            {
                                if (nibble.content_nibble_level_1 <= 0x0B || nibble.content_nibble_level_1 == 0x0F)
                                {
                                    contentBrush = ContentBrushList[nibble.content_nibble_level_1];
                                    break;
                                }
                            }
                        }
                        var textArea = new Rect(info.LeftPos + borderLeft - selfLeft, info.TopPos + borderTop, info.Width - _borderLeftSize, info.Height - _borderTopSize);
                        dc.DrawRectangle(contentBrush, null, textArea);
                        dc.PushClip(new RectangleGeometry(textArea));
                        foreach (Tuple<Brush, GlyphRun> txtinfo in textDrawLists[i])
                        {
                            dc.DrawGlyphRun(txtinfo.Item1, txtinfo.Item2);
                        }
                        dc.Pop();
                        if (info.Filtered && filteredBrush != null)
                        {
                            dc.DrawRectangle(filteredBrush, null, textArea);
                        }
                    }
                }
            }
        }
    }
}

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
        private List<ProgramViewItem> items;
        private List<List<Tuple<Brush, GlyphRun>>> textDrawLists;
        public Brush Background
        {
            set { SetValue(BackgroundProperty, value); }
            get { return (Brush)GetValue(BackgroundProperty); }
        }

        public List<ProgramViewItem> Items
        {
            get
            {
                return items;
            }
            set
            {
                //ProgramViewItemの座標系は番組表基準なので、この時点でCanvas.SetLeft()によりEpgViewPanel自身の座標を添付済みでなければならない
                items = null;
                items = value;
                CreateDrawTextList();
            }
        }

        public double BorderLeftSize
        {
            get;
            set;
        }

        public double BorderTopSize
        {
            get;
            set;
        }

        public bool IsTitleIndent
        {
            get;
            set;
        }

        public Dictionary<char, List<KeyValuePair<string, string>>> ReplaceDictionaryNormal
        {
            get;
            set;
        }

        public Dictionary<char, List<KeyValuePair<string, string>>> ReplaceDictionaryTitle
        {
            get;
            set;
        }

        public ItemFont ItemFontNormal
        {
            get;
            set;
        }

        public ItemFont ItemFontTitle
        {
            get;
            set;
        }

        public double LastItemRenderTextHeight
        {
            get;
            private set;
        }

        protected void CreateDrawTextList()
        {
            LastItemRenderTextHeight = 0;
            textDrawLists = null;
            Matrix m = PresentationSource.FromVisual(Application.Current.MainWindow).CompositionTarget.TransformToDevice;

            if (Items == null)
            {
                return;
            }
            textDrawLists = new List<List<Tuple<Brush, GlyphRun>>>(Items.Count);

            if (ItemFontNormal == null || ItemFontNormal.GlyphType == null ||
                ItemFontTitle == null || ItemFontTitle.GlyphType == null)
            {
                return;
            }
            ItemFontNormal.PrepareCache();
            ItemFontTitle.PrepareCache();

            {
                double selfLeft = Canvas.GetLeft(this);
                double sizeTitle = Math.Max(Settings.Instance.FontSizeTitle, 1);
                double sizeNormal = Math.Max(Settings.Instance.FontSize, 1);
                double indentTitle = sizeTitle * 1.7;
                double indentNormal = IsTitleIndent ? indentTitle : 2;
                SolidColorBrush colorTitle = CommonManager.Instance.CustTitle1Color;
                SolidColorBrush colorNormal = CommonManager.Instance.CustTitle2Color;

                foreach (ProgramViewItem info in Items)
                {
                    var textDrawList = new List<Tuple<Brush, GlyphRun>>();
                    textDrawLists.Add(textDrawList);

                    double innerLeft = info.LeftPos + BorderLeftSize / 2;
                    //0.26は細枠線での微調整
                    double innerTop = info.TopPos + BorderTopSize / 2 - 0.26;
                    double innerWidth = info.Width - BorderLeftSize;
                    double innerHeight = info.Height - BorderTopSize;
                    double useHeight;

                    //分
                    string min = (info.EventInfo.StartTimeFlag == 0 ? "?" : info.EventInfo.start_time.Minute.ToString("d02"));
                    if (RenderText(min, textDrawList, ItemFontTitle, sizeTitle * 0.95,
                                   innerWidth - 1, innerHeight,
                                   innerLeft + 1, innerTop, out useHeight, colorTitle, m, selfLeft) == false)
                    {
                        info.TitleDrawErr = true;
                        LastItemRenderTextHeight = info.Height;
                        continue;
                    }

                    //タイトル
                    string title = info.EventInfo.ShortInfo == null ? "" : info.EventInfo.ShortInfo.event_name;
                    if (ReplaceDictionaryTitle != null)
                    {
                        title = CommonManager.ReplaceText(title, ReplaceDictionaryTitle);
                    }
                    if (RenderText(title.Length > 0 ? title : " ", textDrawList, ItemFontTitle, sizeTitle,
                                   innerWidth - sizeTitle * 0.5 - indentTitle, innerHeight,
                                   innerLeft + indentTitle, innerTop, out useHeight, colorTitle, m, selfLeft) == false)
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
                        //detail += info.EventInfo.ExtInfo == null ? "" : "\r\n\r\n" + info.EventInfo.ExtInfo.text_char;
                        if (ReplaceDictionaryNormal != null)
                        {
                            detail = CommonManager.ReplaceText(detail, ReplaceDictionaryNormal);
                        }
                        RenderText(detail, textDrawList, ItemFontNormal, sizeNormal,
                                   innerWidth - sizeTitle * 0.5 - indentNormal, innerHeight - LastItemRenderTextHeight,
                                   innerLeft + indentNormal, innerTop + LastItemRenderTextHeight, out useHeight, colorNormal, m, selfLeft);
                        LastItemRenderTextHeight += useHeight;
                    }
                    LastItemRenderTextHeight = Math.Floor(LastItemRenderTextHeight + BorderTopSize + sizeNormal * 0.5);
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

            string[] lineText = text.Replace("\r", "").Split('\n');
            foreach (string line in lineText)
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
                for (int n = 0; n < line.Length; n++)
                {
                    //この辞書検索が負荷の大部分を占めているのでテーブルルックアップする
                    //ushort glyphIndex = itemFont.GlyphType.CharacterToGlyphMap[line[n]];
                    //double width = itemFont.GlyphType.AdvanceWidths[glyphIndex] * fontSize;
                    ushort glyphIndex = itemFont.NoCache ? (ushort)0 : itemFont.GlyphIndexCache[line[n]];
                    double glyphWidth;
                    GlyphTypeface glyphType = itemFont.GlyphType;
                    if (glyphIndex == 0)
                    {
                        //NoCacheまたはキャッシュミス
                        int key = 0;
                        if (char.IsSurrogatePair(line, n))
                        {
                            key = char.ConvertToUtf32(line, n++);
                        }
                        else if (char.IsSurrogate(line[n]) == false)
                        {
                            key = line[n];
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
            }
            return true;
        }

        protected override void OnRender(DrawingContext dc)
        {
            dc.DrawRectangle(Background, null, new Rect(RenderSize));

            if (Items == null || textDrawLists == null || Items.Count < textDrawLists.Count)
            {
                return;
            }
            
            {
                double selfLeft = Canvas.GetLeft(this);
                Brush bgBrush = Background;
                //位置がずれないように枠線の幅が1より大きいときは両側で分け合う
                double borderLeft = BorderLeftSize > 1 ? BorderLeftSize / 2 : BorderLeftSize;
                double borderTop = BorderTopSize > 1 ? BorderTopSize / 2 : BorderTopSize;
                for (int i = 0; i < textDrawLists.Count; i++)
                {
                    ProgramViewItem info = Items[i];
                    double bgHeight = Math.Min(borderTop, info.Height);
                    if (info.Width > BorderLeftSize && bgHeight > 0)
                    {
                        dc.DrawRectangle(bgBrush, null, new Rect(info.LeftPos + borderLeft - selfLeft, info.TopPos, info.Width - BorderLeftSize, bgHeight));
                        dc.DrawRectangle(bgBrush, null, new Rect(info.LeftPos + borderLeft - selfLeft, info.TopPos + info.Height - bgHeight, info.Width - BorderLeftSize, bgHeight));
                    }
                    if (info.Width > BorderLeftSize && info.Height > BorderTopSize)
                    {
                        var textArea = new Rect(info.LeftPos + borderLeft - selfLeft, info.TopPos + borderTop, info.Width - BorderLeftSize, info.Height - BorderTopSize);
                        dc.DrawRectangle(info.ContentColor, null, textArea);
                        dc.PushClip(new RectangleGeometry(textArea));
                        foreach (Tuple<Brush, GlyphRun> txtinfo in textDrawLists[i])
                        {
                            dc.DrawGlyphRun(txtinfo.Item1, txtinfo.Item2);
                        }
                        dc.Pop();
                    }
                }
            }
        }
    }
}

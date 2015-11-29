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
            public string FamilyName { get; private set; }
            public bool IsBold { get; private set; }
            public GlyphTypeface GlyphType { get; private set; }
            public ushort[] GlyphIndexCache { get; private set; }
            public float[] GlyphWidthCache { get; private set; }

            public ItemFont(string familyName, bool isBold)
            {
                FamilyName = familyName;
                IsBold = isBold;
                GlyphTypeface glyphType;
                if ((new Typeface(new FontFamily(FamilyName),
                                  FontStyles.Normal,
                                  IsBold ? FontWeights.Bold : FontWeights.Normal,
                                  FontStretches.Normal)).TryGetGlyphTypeface(out glyphType))
                {
                    GlyphType = glyphType;
                }
            }
            public void PrepareCache()
            {
                if (GlyphIndexCache == null)
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
        private Dictionary<ProgramViewItem, List<TextDrawItem>> textDrawDict = new Dictionary<ProgramViewItem, List<TextDrawItem>>();
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

        public bool IsTitleIndent
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

        protected void CreateDrawTextList()
        {
            textDrawDict.Clear();
            textDrawDict = null;
            textDrawDict = new Dictionary<ProgramViewItem, List<TextDrawItem>>();
            Matrix m = PresentationSource.FromVisual(Application.Current.MainWindow).CompositionTarget.TransformToDevice;

            this.VisualTextRenderingMode = TextRenderingMode.ClearType;
            this.VisualTextHintingMode = TextHintingMode.Fixed;
            this.UseLayoutRounding = true;
            if (Items == null)
            {
                return;
            }

            if (ItemFontNormal == null || ItemFontNormal.GlyphType == null ||
                ItemFontTitle == null || ItemFontTitle.GlyphType == null)
            {
                return;
            }
            ItemFontNormal.PrepareCache();
            ItemFontTitle.PrepareCache();

            try
            {
                double selfLeft = Canvas.GetLeft(this);
                double sizeNormal = Settings.Instance.FontSize;
                double sizeTitle = Settings.Instance.FontSizeTitle;
                foreach (ProgramViewItem info in Items)
                {
                    List<TextDrawItem> textDrawList = new List<TextDrawItem>();
                    textDrawDict[info] = textDrawList;
                    if (info.Height > 2)
                    {
                        if (info.Height < sizeTitle + 3)
                        {
                            //高さ足りない
                            info.TitleDrawErr = true;
                        }

                        double totalHeight = -2;

                        //分
                        string min;
                        if (info.EventInfo.StartTimeFlag == 1)
                        {
                            min = info.EventInfo.start_time.Minute.ToString("d02") + "  ";
                        }
                        else
                        {
                            min = "未定 ";
                        }
                        double useHeight = 0;
                        if (RenderText(min, ref textDrawList, ItemFontTitle, sizeTitle - 0.5, info.Width - 4, info.Height + 10, info.LeftPos - selfLeft - 1, info.TopPos - 1, ref useHeight, CommonManager.Instance.CustTitle1Color, m) == false)
                        {
                            info.TitleDrawErr = true;
                            continue;
                        }

                        double widthOffset = sizeNormal * 1.7;
                        //番組情報
                        if (info.EventInfo.ShortInfo != null)
                        {
                            //タイトル
                            if (info.EventInfo.ShortInfo.event_name.Length > 0)
                            {
                                if (RenderText(info.EventInfo.ShortInfo.event_name, ref textDrawList, ItemFontTitle, sizeTitle, info.Width - 6 - widthOffset, info.Height - 1 - totalHeight, info.LeftPos - selfLeft + widthOffset, info.TopPos + totalHeight, ref useHeight, CommonManager.Instance.CustTitle1Color, m) == false)
                                {
                                    info.TitleDrawErr = true;
                                    continue;
                                }
                                totalHeight += Math.Floor(useHeight + (sizeNormal / 2));
                            }
                            if (IsTitleIndent == false)
                            {
                                widthOffset = 0;
                            }
                            //説明
                            if (info.EventInfo.ShortInfo.text_char.Length > 0)
                            {
                                if (RenderText(info.EventInfo.ShortInfo.text_char, ref textDrawList, ItemFontNormal, sizeNormal, info.Width - 10 - widthOffset, info.Height - 5 - totalHeight, info.LeftPos - selfLeft + widthOffset, info.TopPos + totalHeight, ref useHeight, CommonManager.Instance.CustTitle2Color, m) == false)
                                {
                                    continue;
                                }
                                totalHeight += useHeight + sizeNormal;
                            }

                            //詳細
//                            if (info.EventInfo.ExtInfo != null)
//                            {
//                                if (info.EventInfo.ExtInfo.text_char.Length > 0)
//                                {
//                                    if (RenderText(info.EventInfo.ExtInfo.text_char, ref textDrawList, glyphTypefaceNormal, sizeNormal, info.Width - 6 - widthOffset, info.Height - 6 - totalHeight, info.LeftPos + widthOffset, info.TopPos + totalHeight, ref useHeight, CommonManager.Instance.CustTitle2Color, m) == false)
//                                    {
//                                        continue;
//                                    }
//                                    totalHeight += useHeight;
//                                }
//                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        protected bool RenderText(String text, ref List<TextDrawItem> textDrawList, ItemFont itemFont, double fontSize, double maxWidth, double maxHeight, double x, double y, ref double useHeight, SolidColorBrush fontColor, Matrix m)
        {
            double totalHeight = 0;

            string[] lineText = text.Replace("\r", "").Split('\n');
            foreach (string line in lineText)
            {
                totalHeight += Math.Floor(2 + fontSize);
                List<ushort> glyphIndexes = new List<ushort>();
                List<double> advanceWidths = new List<double>();
                double totalWidth = 0;
                for (int n = 0; n < line.Length; n++)
                {
                    //この辞書検索が負荷の大部分を占めているのでテーブルルックアップする
                    //ushort glyphIndex = itemFont.GlyphType.CharacterToGlyphMap[line[n]];
                    //double width = itemFont.GlyphType.AdvanceWidths[glyphIndex] * fontSize;
                    ushort glyphIndex = itemFont.GlyphIndexCache[line[n]];
                    if (glyphIndex == 0)
                    {
                        itemFont.GlyphIndexCache[line[n]] = glyphIndex = itemFont.GlyphType.CharacterToGlyphMap[line[n]];
                        itemFont.GlyphWidthCache[glyphIndex] = (float)itemFont.GlyphType.AdvanceWidths[glyphIndex];
                    }
                    double width = itemFont.GlyphWidthCache[glyphIndex] * fontSize;
                    if (totalWidth + width > maxWidth)
                    {
                        if (glyphIndexes.Count > 0)
                        {
                            double dpix = Math.Ceiling((x + 2) * m.M11);
                            double dpiy = Math.Ceiling((y + totalHeight) * m.M22);
                            Point origin = new Point(dpix / m.M11, dpiy / m.M22);
                            TextDrawItem item = new TextDrawItem();
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
                    Point origin = new Point(dpix / m.M11, dpiy / m.M22);
                    TextDrawItem item = new TextDrawItem();
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
            dc.DrawRectangle(Background, null, new Rect(RenderSize));
            this.VisualTextRenderingMode = TextRenderingMode.ClearType;
            this.VisualTextHintingMode = TextHintingMode.Fixed;
            this.UseLayoutRounding = true;

            if (Items == null)
            {
                return;
            }
            
            try
            {
                double selfLeft = Canvas.GetLeft(this);
                double sizeNormal = Settings.Instance.FontSize;
                double sizeTitle = Settings.Instance.FontSizeTitle;
                Brush bgBrush = Background;
                foreach (ProgramViewItem info in Items)
                {
                    dc.DrawRectangle(bgBrush, null, new Rect(info.LeftPos - selfLeft, info.TopPos, info.Width, 1));
                    dc.DrawRectangle(bgBrush, null, new Rect(info.LeftPos - selfLeft, info.TopPos + info.Height, info.Width, 1));
                    if (info.Height > 1)
                    {
                        dc.DrawRectangle(info.ContentColor, null, new Rect(info.LeftPos - selfLeft, info.TopPos + 0.5, info.Width - 1, info.Height - 0.5));
                        if (textDrawDict.ContainsKey(info))
                        {
                            dc.PushClip(new RectangleGeometry(new Rect(info.LeftPos - selfLeft, info.TopPos + 0.5, info.Width - 1, info.Height - 0.5)));
                            foreach (TextDrawItem txtinfo in textDrawDict[info])
                            {
                                dc.DrawGlyphRun(txtinfo.FontColor, txtinfo.Text);
                            }
                            dc.Pop();
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }
    }

    class TextDrawItem
    {
        public SolidColorBrush FontColor;
        public GlyphRun Text;
    }
}

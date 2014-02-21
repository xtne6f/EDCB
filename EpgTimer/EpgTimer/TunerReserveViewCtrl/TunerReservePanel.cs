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

namespace EpgTimer.TunerReserveViewCtrl
{
    class TunerReservePanel : FrameworkElement
    {
        public static readonly DependencyProperty BackgroundProperty =
            Panel.BackgroundProperty.AddOwner(typeof(TunerReservePanel));

        public Brush Background
        {
            set { SetValue(BackgroundProperty, value); }
            get { return (Brush)GetValue(BackgroundProperty); }
        }

        public List<ReserveViewItem> Items
        {
            get;
            set;
        }

        protected bool RenderText(String text, DrawingContext dc, GlyphTypeface glyphType, double fontSize, double maxWidth, double maxHeight, double x, double y, ref double useHeight)
        {
            if (maxHeight < fontSize + 2)
            {
                useHeight = 0;
                return false;
            }
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
                    ushort glyphIndex = glyphType.CharacterToGlyphMap[line[n]];
                    double width = glyphType.AdvanceWidths[glyphIndex] * fontSize;
                    if (totalWidth + width > maxWidth)
                    {
                        if (totalHeight + fontSize > maxHeight)
                        {
                            //次の行無理
                            glyphIndex = glyphType.CharacterToGlyphMap['…'];
                            glyphIndexes[glyphIndexes.Count - 1] = glyphIndex;
                            advanceWidths[advanceWidths.Count - 1] = width;

                            Point origin = new Point(x + 2, y + totalHeight);
                            GlyphRun glyphRun = new GlyphRun(glyphType, 0, false, fontSize,
                                glyphIndexes, origin, advanceWidths, null, null, null, null,
                                null, null);

                            dc.DrawGlyphRun(Brushes.Black, glyphRun);

                            useHeight = totalHeight;
                            return false;
                        }
                        else
                        {
                            //次の行いけるので今までの分出力
                            //次の行いける
                            Point origin = new Point(x + 2, y + totalHeight);
                            GlyphRun glyphRun = new GlyphRun(glyphType, 0, false, fontSize,
                                glyphIndexes, origin, advanceWidths, null, null, null, null,
                                null, null);

                            dc.DrawGlyphRun(Brushes.Black, glyphRun);
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
                    Point origin = new Point(x + 2, y + totalHeight);
                    GlyphRun glyphRun = new GlyphRun(glyphType, 0, false, fontSize,
                        glyphIndexes, origin, advanceWidths, null, null, null, null,
                        null, null);

                    dc.DrawGlyphRun(Brushes.Black, glyphRun);
                }
                //高さ確認
                if (totalHeight + fontSize > maxHeight)
                {
                    //これ以上は無理
                    useHeight = totalHeight;
                    return false;
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

            if (Items == null)
            {
                return;
            }

            Typeface typefaceNormal = null;
            Typeface typefaceTitle = null;
            GlyphTypeface glyphTypefaceNormal = null;
            GlyphTypeface glyphTypefaceTitle = null;

            try
            {
                if (Settings.Instance.FontName.Length > 0)
                {
                    typefaceNormal = new Typeface(new FontFamily(Settings.Instance.FontName),
                                                 FontStyles.Normal,
                                                 FontWeights.Normal,
                                                 FontStretches.Normal);
                }
                if (Settings.Instance.FontNameTitle.Length > 0)
                {
                    if (Settings.Instance.FontBoldTitle == true)
                    {
                        typefaceTitle = new Typeface(new FontFamily(Settings.Instance.FontNameTitle),
                                                     FontStyles.Normal,
                                                     FontWeights.Bold,
                                                     FontStretches.Normal);
                    }
                    else
                    {
                        typefaceTitle = new Typeface(new FontFamily(Settings.Instance.FontNameTitle),
                                                     FontStyles.Normal,
                                                     FontWeights.Normal,
                                                     FontStretches.Normal);
                    }
                }
                if (!typefaceNormal.TryGetGlyphTypeface(out glyphTypefaceNormal))
                {
                    typefaceNormal = null;
                }
                if (!typefaceTitle.TryGetGlyphTypeface(out glyphTypefaceTitle))
                {
                    typefaceTitle = null;
                }

                if (typefaceNormal == null)
                {
                    typefaceNormal = new Typeface(new FontFamily("MS UI Gothic"),
                                                 FontStyles.Normal,
                                                 FontWeights.Normal,
                                                 FontStretches.Normal);
                    if (!typefaceNormal.TryGetGlyphTypeface(out glyphTypefaceNormal))
                    {
                        MessageBox.Show("フォント指定が不正です");
                        return;
                    }
                }
                if (typefaceTitle == null)
                {
                    typefaceTitle = new Typeface(new FontFamily("MS UI Gothic"),
                                                 FontStyles.Normal,
                                                 FontWeights.Bold,
                                                 FontStretches.Normal);
                    if (!typefaceTitle.TryGetGlyphTypeface(out glyphTypefaceTitle))
                    {
                        MessageBox.Show("フォント指定が不正です");
                        return;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

            try
            {
                double sizeNormal = Settings.Instance.FontSize;
                double sizeTitle = Settings.Instance.FontSizeTitle;
                foreach (ReserveViewItem info in Items)
                {
                    // ビットマップフォントがかすれる問題
                    double dInfoTopPos = Math.Floor(info.TopPos);
                    double dInfoHeight = Math.Floor(info.Height);

                    dc.DrawRectangle(Brushes.LightGray, null, new Rect(info.LeftPos, dInfoTopPos, info.Width, dInfoHeight));
                    if (dInfoHeight > 2)
                    {
                        SolidColorBrush color = Brushes.White;
                        if (info.ReserveInfo.OverlapMode == 1)
                        {
                            color = Brushes.Yellow;
                            //color = CommonManager.Instance.CustContentColorList[0x14];
                        }
                        dc.DrawRectangle(color, null, new Rect(info.LeftPos + 1, dInfoTopPos + 1, info.Width - 2, dInfoHeight - 2));
                        if (dInfoHeight < 4 + sizeTitle + 2)
                        {
                            //高さ足りない
                            info.TitleDrawErr = true;
                            continue;
                        }

                        double totalHeight = 0;

                        //分
                        string min;
                        min = info.ReserveInfo.StartTime.Minute.ToString("d02") + "  ";

                        double useHeight = 0;
                        if (RenderText(min, dc, glyphTypefaceNormal, sizeNormal, info.Width - 4, dInfoHeight - 4, info.LeftPos, dInfoTopPos, ref useHeight) == false)
                        {
                            info.TitleDrawErr = true;
                            continue;
                        }

                        double widthOffset = sizeNormal * 2;

                        //サービス名
                        if (info.ReserveInfo.StationName.Length > 0)
                        {
                            String serviceName = info.ReserveInfo.StationName;
                            if (0x7880 <= info.ReserveInfo.OriginalNetworkID && info.ReserveInfo.OriginalNetworkID <= 0x7FE8)
                            {
                                serviceName += " (地デジ)";
                            }
                            else if (info.ReserveInfo.OriginalNetworkID == 0x0004)
                            {
                                serviceName += " (BS)";
                            }
                            else if (info.ReserveInfo.OriginalNetworkID == 0x0006)
                            {
                                serviceName += " (CS1)";
                            }
                            else if (info.ReserveInfo.OriginalNetworkID == 0x0007)
                            {
                                serviceName += " (CS2)";
                            }
                            else
                            {
                                serviceName += " (その他)";
                            }
                            if (RenderText(serviceName, dc, glyphTypefaceTitle, sizeTitle, info.Width - 6 - widthOffset, dInfoHeight - 6 - totalHeight, info.LeftPos + widthOffset, dInfoTopPos + totalHeight, ref useHeight) == false)
                            {
                                info.TitleDrawErr = true;
                                continue;
                            }
                            // ビットマップフォントがかすれる問題
                            totalHeight += useHeight + Math.Floor(sizeNormal / 2);
                        }
                        widthOffset = 2;
                        //番組名
                        if (info.ReserveInfo.Title.Length > 0)
                        {
                            if (RenderText(info.ReserveInfo.Title, dc, glyphTypefaceNormal, sizeNormal, info.Width - 6 - widthOffset, dInfoHeight - 6 - totalHeight, info.LeftPos + widthOffset, dInfoTopPos + totalHeight, ref useHeight) == false)
                            {
                                info.TitleDrawErr = true;
                                continue;
                            }
                            totalHeight += useHeight + sizeNormal;
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
}

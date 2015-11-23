using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Media;

namespace EpgTimer.TunerReserveViewCtrl
{
    class TunerReservePanel : EpgTimer.UserCtrlView.PanelBase
    {
        public List<ReserveViewItem> Items { get; set; }

        public override void ClearInfo() 
        {
            Items = new List<ReserveViewItem>();
        }

        protected bool RenderText(String text, DrawingContext dc, GlyphTypeface glyphType, SolidColorBrush brush, double fontSize, double maxWidth, double maxHeight, double x, double y, ref double useHeight, bool nowrap = false)
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
                //高さ確認
                if (totalHeight + fontSize > maxHeight)
                {
                    //これ以上は無理
                    useHeight = totalHeight;
                    return false;
                }
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
                        if (nowrap == true) break;//改行しない場合ここで終り
                        if (totalWidth == 0) return false;//一文字も置けなかった(glyphIndexesなどのCount=0のまま)

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

                            dc.DrawGlyphRun(brush, glyphRun);

                            useHeight = totalHeight;
                            return false;
                        }
                        else
                        {
                            //次の行いけるので今までの分出力
                            Point origin = new Point(x + 2, y + totalHeight);
                            GlyphRun glyphRun = new GlyphRun(glyphType, 0, false, fontSize,
                                glyphIndexes, origin, advanceWidths, null, null, null, null,
                                null, null);

                            dc.DrawGlyphRun(brush, glyphRun);
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

                    dc.DrawGlyphRun(brush, glyphRun);
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

            if (Items == null) return;

            try
            {
                // ビットマップフォントがかすれる問題 とりあえず整数にしておく
                double sizeMin = Settings.Instance.TunerFontSize;
                double sizeTitle = Settings.Instance.TunerFontSizeService;
                double sizeNormal = Settings.Instance.TunerFontSize;
                double indentTitle = Math.Floor(sizeMin * 1.7);
                double indentNormal = Math.Floor(Settings.Instance.TunerTitleIndent ? indentTitle : 2);
                GlyphTypeface glyphTypefaceSeervice = vutil.GetGlyphTypeface(Settings.Instance.TunerFontNameService, Settings.Instance.TunerFontBoldService);
                GlyphTypeface glyphTypefaceNormal = vutil.GetGlyphTypeface(Settings.Instance.TunerFontName, false);
                SolidColorBrush colorTitle = CommonManager.Instance.CustTunerServiceColor;
                SolidColorBrush colorNormal = CommonManager.Instance.CustTunerTextColor;

                foreach (ReserveViewItem info in Items)
                {
                    colorTitle = Settings.Instance.TunerColorModeUse == true ? info.ForeColorPri : colorTitle;

                    double dInfoTopPos = Math.Floor(info.TopPos);
                    double dInfoHeight = Math.Floor(info.Height);

                    dc.DrawRectangle(Brushes.LightGray, null, new Rect(info.LeftPos, dInfoTopPos, info.Width, Math.Max(dInfoHeight, 0)));
                    if (dInfoHeight > 2)
                    {
                        dc.DrawRectangle(info.BackColor, null, new Rect(info.LeftPos + 1, dInfoTopPos + 1, info.Width - 2, dInfoHeight - 2));
                        if (dInfoHeight < 4 + sizeTitle + 2)
                        {
                            //高さ足りない
                            info.TitleDrawErr = true;
                            continue;
                        }

                        double totalHeight = 0;
                        double useHeight = 0;

                        //分
                        string min = info.ReserveInfo.StartTime.Minute.ToString("d02");
                        if (RenderText(min, dc, glyphTypefaceNormal, colorTitle, sizeMin, info.Width - 4, dInfoHeight - 4, info.LeftPos, dInfoTopPos - 2, ref useHeight) == false)
                        {
                            info.TitleDrawErr = true;
                            continue;
                        }

                        //サービス名
                        if (info.ReserveInfo.StationName.Length > 0)
                        {
                            string serviceName = info.ReserveInfo.StationName
                                + "(" + CommonManager.ConvertNetworkNameText(info.ReserveInfo.OriginalNetworkID) + ")";
                            if (RenderText(serviceName, dc, glyphTypefaceSeervice, colorTitle, sizeTitle, info.Width - 6 - indentTitle, dInfoHeight - 6 - totalHeight, info.LeftPos + indentTitle, dInfoTopPos - 2 + totalHeight, ref useHeight, Settings.Instance.TunerServiceNoWrap) == false)
                            {
                                info.TitleDrawErr = true;
                                continue;
                            }
                            totalHeight += Math.Floor(useHeight + sizeTitle / 3);
                        }

                        //番組名
                        if (info.ReserveInfo.Title.Length > 0)
                        {
                            if (RenderText(info.ReserveInfo.Title, dc, glyphTypefaceNormal, colorNormal, sizeNormal, info.Width - 6 - indentNormal, dInfoHeight - 6 - totalHeight, info.LeftPos + indentNormal, dInfoTopPos - 2 + totalHeight, ref useHeight) == false)
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

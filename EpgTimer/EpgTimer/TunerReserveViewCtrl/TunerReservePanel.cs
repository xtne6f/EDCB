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

        public TunerReservePanel()
        {
            this.VisualTextRenderingMode = TextRenderingMode.ClearType;
            this.VisualTextHintingMode = TextHintingMode.Fixed;
            this.UseLayoutRounding = true;
        }

        protected bool RenderText(String text, DrawingContext dc, ItemFont itemFont, Brush brush, double fontSize, double maxWidth, double maxHeight, double x, double y, ref double useHeight, bool nowrap = false)
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
                var glyphIndexes = new List<ushort>();
                var advanceWidths = new List<double>();
                double totalWidth = 0;
                for (int n = 0; n < line.Length; n++)
                {
                    ushort glyphIndex = itemFont.GlyphIndexCache[line[n]];
                    if (glyphIndex == 0)
                    {
                        itemFont.GlyphType.CharacterToGlyphMap.TryGetValue(line[n], out glyphIndex);
                        itemFont.GlyphIndexCache[line[n]] = glyphIndex;
                        itemFont.GlyphWidthCache[glyphIndex] = (float)itemFont.GlyphType.AdvanceWidths[glyphIndex];
                    }
                    double width = itemFont.GlyphWidthCache[glyphIndex] * fontSize;
                    if (totalWidth + width > maxWidth)
                    {
                        if (nowrap == true) break;//改行しない場合ここで終り
                        if (totalWidth == 0) return false;//一文字も置けなかった(glyphIndexesなどのCount=0のまま)

                        if (totalHeight + fontSize > maxHeight)
                        {
                            //次の行無理
                            glyphIndex = itemFont.GlyphType.CharacterToGlyphMap['…'];
                            glyphIndexes[glyphIndexes.Count - 1] = glyphIndex;
                            advanceWidths[advanceWidths.Count - 1] = width;

                            var origin = new Point(x + 2, y + totalHeight);
                            var glyphRun = new GlyphRun(itemFont.GlyphType, 0, false, fontSize,
                                glyphIndexes, origin, advanceWidths, null, null, null, null,
                                null, null);

                            dc.DrawGlyphRun(brush, glyphRun);

                            useHeight = totalHeight;
                            return false;
                        }
                        else
                        {
                            //次の行いけるので今までの分出力
                            var origin = new Point(x + 2, y + totalHeight);
                            var glyphRun = new GlyphRun(itemFont.GlyphType, 0, false, fontSize,
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
                    var origin = new Point(x + 2, y + totalHeight);
                    var glyphRun = new GlyphRun(itemFont.GlyphType, 0, false, fontSize,
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
            //背景だけは先に描画
            dc.DrawRectangle(Background, null, new Rect(RenderSize));

            if (Items == null) return;

            var ItemFontNormal = ItemFontCache.ItemFont(Settings.Instance.TunerFontName, false);
            var ItemFontTitle = ItemFontCache.ItemFont(Settings.Instance.TunerFontNameService, Settings.Instance.TunerFontBoldService);
            if (ItemFontNormal.GlyphType == null || ItemFontTitle.GlyphType == null)
            {
                return;
            }

            try
            {
                // ビットマップフォントがかすれる問題 とりあえず整数にしておく
                double sizeMin = Settings.Instance.TunerFontSize;
                double sizeTitle = Settings.Instance.TunerFontSizeService;
                double sizeNormal = Settings.Instance.TunerFontSize;
                double indentTitle = Math.Floor(sizeMin * 1.7);
                double indentNormal = Math.Floor(Settings.Instance.TunerTitleIndent ? indentTitle : 2);
                Brush colorTitle = CommonManager.Instance.CustTunerServiceColor;
                Brush colorNormal = CommonManager.Instance.CustTunerTextColor;

                //録画中のものを後で描画する
                List<ReserveViewItem> postdrawList = Items.Where(info => info.ReserveInfo.IsOnRec()).ToList();
                postdrawList.ForEach(info => Items.Remove(info));
                Items.AddRange(postdrawList);

                foreach (ReserveViewItem info in Items)
                {
                    colorTitle = Settings.Instance.TunerColorModeUse == true ? info.ForeColorPriTuner : colorTitle;

                    double dInfoTopPos = Math.Floor(info.TopPos);
                    double dInfoHeight = Math.Floor(info.Height);

                    dc.DrawRectangle(info.BorderBrushTuner, null, new Rect(info.LeftPos, dInfoTopPos, info.Width + 1, Math.Max(dInfoHeight + 1, 0)));
                    if (dInfoHeight > 2)
                    {
                        dc.DrawRectangle(info.BackColorTuner, null, new Rect(info.LeftPos + 1, dInfoTopPos + 1, info.Width - 1, dInfoHeight - 1));
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
                        if (RenderText(min, dc, ItemFontNormal, colorTitle, sizeMin, info.Width - 4, dInfoHeight - 4, info.LeftPos, dInfoTopPos - 2, ref useHeight) == false)
                        {
                            info.TitleDrawErr = true;
                            continue;
                        }

                        //サービス名
                        if (info.ReserveInfo.StationName.Length > 0)
                        {
                            string serviceName = info.ReserveInfo.StationName
                                + "(" + CommonManager.ConvertNetworkNameText(info.ReserveInfo.OriginalNetworkID) + ")";
                            if (RenderText(serviceName, dc, ItemFontTitle, colorTitle, sizeTitle, info.Width - 6 - indentTitle, dInfoHeight - 6 - totalHeight, info.LeftPos + indentTitle, dInfoTopPos - 2 + totalHeight, ref useHeight, Settings.Instance.TunerServiceNoWrap) == false)
                            {
                                info.TitleDrawErr = true;
                                continue;
                            }
                            totalHeight += Math.Floor(useHeight + sizeTitle / 3);
                        }

                        //番組名
                        if (info.ReserveInfo.Title.Length > 0)
                        {
                            if (RenderText(info.ReserveInfo.Title, dc, ItemFontNormal, colorNormal, sizeNormal, info.Width - 6 - indentNormal, dInfoHeight - 6 - totalHeight, info.LeftPos + indentNormal, dInfoTopPos - 2 + totalHeight, ref useHeight) == false)
                            {
                                info.TitleDrawErr = true;
                                continue;
                            }
                            totalHeight += useHeight + sizeNormal;
                        }
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
    }
}

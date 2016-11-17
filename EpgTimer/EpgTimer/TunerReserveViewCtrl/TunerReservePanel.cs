using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Media;

namespace EpgTimer.TunerReserveViewCtrl
{
    using EpgTimer.UserCtrlView;

    class TunerReservePanel : PanelBase
    {
        public List<ReserveViewItem> Items { get; set; }

        protected List<List<TextDrawItem>> CreateDrawTextList()
        {
            if (Items == null) return null;

            var textDrawLists = new List<List<TextDrawItem>>(Items.Count);

            try
            {
                var ItemFontNormal = ItemFontCache.ItemFont(Settings.Instance.TunerFontName, false);
                var ItemFontTitle = ItemFontCache.ItemFont(Settings.Instance.TunerFontNameService, Settings.Instance.TunerFontBoldService);
                if (ItemFontNormal.GlyphType == null || ItemFontTitle.GlyphType == null)
                {
                    return null;
                }

                double sizeMin = Settings.Instance.TunerFontSize;
                double sizeTitle = Settings.Instance.TunerFontSizeService;
                double sizeNormal = Settings.Instance.TunerFontSize;
                double indentTitle = sizeMin * 1.7;
                double indentNormal = Settings.Instance.TunerTitleIndent ? indentTitle : 2;
                Brush colorTitleBase = CommonManager.Instance.CustTunerServiceColor;
                Brush colorNormal = CommonManager.Instance.CustTunerTextColor;

                //録画中のものを後で描画する
                Items = Items.Where(info => info.ReserveInfo.IsOnRec() == false)
                    .Concat(Items.Where(info => info.ReserveInfo.IsOnRec() == true)).ToList();

                foreach (ReserveViewItem info in Items)
                {
                    var textDrawList = new List<TextDrawItem>();
                    textDrawLists.Add(textDrawList);

                    if (info.Height > 2)
                    {
                        double totalHeight = 0;
                        double useHeight = 0;
                        var colorTitle = Settings.Instance.TunerColorModeUse == true ? info.ForeColorPriTuner : colorTitleBase;

                        //分
                        string min = info.ReserveInfo.StartTime.Minute.ToString("d02");
                        if (RenderText(textDrawList, min, ItemFontNormal, sizeMin, info.Width - 4, info.Height - 4, info.LeftPos, info.TopPos, ref useHeight, colorTitle) == false)
                        {
                            continue;
                        }

                        //サービス名
                        if (info.ReserveInfo.StationName.Length > 0)
                        {
                            string serviceName = info.ReserveInfo.StationName
                                + "(" + CommonManager.ConvertNetworkNameText(info.ReserveInfo.OriginalNetworkID) + ")";
                            if (RenderText(textDrawList, serviceName, ItemFontTitle, sizeTitle, info.Width - 6 - indentTitle, info.Height - 6 - totalHeight, info.LeftPos + indentTitle, info.TopPos + totalHeight, ref useHeight, colorTitle, Settings.Instance.TunerServiceNoWrap) == false)
                            {
                                continue;
                            }
                            totalHeight += useHeight + sizeTitle / 3;
                        }

                        //番組名
                        if (info.ReserveInfo.Title.Length > 0)
                        {
                            if (RenderText(textDrawList, info.ReserveInfo.Title, ItemFontNormal, sizeNormal, info.Width - 6 - indentNormal, info.Height - 6 - totalHeight, info.LeftPos + indentNormal, info.TopPos + totalHeight, ref useHeight, colorNormal) == false)
                            {
                                continue;
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

                List<List<TextDrawItem>> textDrawLists = CreateDrawTextList();
                if (Items == null || textDrawLists == null || Items.Count < textDrawLists.Count)
                {
                    return;
                }

                for (int i = 0; i < textDrawLists.Count; i++)
                {
                    ReserveViewItem info = Items[i];
                    dc.DrawRectangle(info.BorderBrushTuner, null, new Rect(info.LeftPos, info.TopPos, info.Width + 1, Math.Max(info.Height + 1, 1)));
                    if (info.Height > 1)
                    {
                        var textArea = new Rect(info.LeftPos + 1, info.TopPos + 1, info.Width - 1, info.Height - 1);
                        dc.DrawRectangle(info.BackColorTuner, null, textArea);
                        DrawTextDrawList(dc, textDrawLists[i], textArea);
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
    }
}

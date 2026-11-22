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

        protected override void OnRender(DrawingContext dc)
        {
            dc.DrawRectangle(Background, null, new Rect(RenderSize));

            var ps = PresentationSource.FromVisual(this);
            if (ps == null || Items == null)
            {
                return;
            }
            Matrix m = ps.CompositionTarget.TransformToDevice;

            EpgSetting epgSetting = Settings.Instance.EpgSettingList[0];
            var itemFontTitle = new EpgView.EpgViewPanel.ItemFont(epgSetting.FontNameTitle, epgSetting.FontBoldTitle, true);
            var itemFontNormal = new EpgView.EpgViewPanel.ItemFont(epgSetting.FontName, false, true);
            if (itemFontTitle.GlyphType == null || itemFontNormal.GlyphType == null)
            {
                return;
            }

            {
                double sizeTitle = Math.Max(epgSetting.FontSizeTitle, 1);
                double sizeNormal = Math.Max(epgSetting.FontSize, 1);
                double indentTitle = sizeTitle * 1.7;
                double indentNormal = epgSetting.EpgTitleIndent ? indentTitle : 2;
                Brush colorTitle = ColorDef.CustColorBrush(epgSetting.TitleColor1, epgSetting.TitleCustColor1);
                Brush colorNormal = ColorDef.CustColorBrush(epgSetting.TitleColor2, epgSetting.TitleCustColor2);
                //ジャンル「なし」の色
                SolidColorBrush textAreaSolidBrush = ColorDef.CustColorBrush(epgSetting.ContentColorList[0x10], epgSetting.ContentCustColorList[0x10]);
                Brush textAreaBrush = epgSetting.EpgGradation ? (Brush)ColorDef.GradientBrush(textAreaSolidBrush.Color) : textAreaSolidBrush;
                //位置がずれないように枠線の幅が1より大きいときは両側で分け合う
                double borderLeftSize = epgSetting.EpgBorderLeftSize;
                double borderTopSize = epgSetting.EpgBorderTopSize;
                double borderHalfLeft = borderLeftSize > 1 ? borderLeftSize / 2 : borderLeftSize;
                double borderHalfTop = borderTopSize > 1 ? borderTopSize / 2 : borderTopSize;

                foreach (ReserveViewItem info in Items)
                {
                    var textDrawList = new List<Tuple<Brush, GlyphRun>>();

                    double innerLeft = info.LeftPos + borderLeftSize / 2;
                    //0.26は細枠線での微調整
                    double innerTop = info.TopPos + borderTopSize / 2 - 0.26;
                    double innerWidth = info.Width - borderLeftSize;
                    double innerHeight = info.Height - borderTopSize;
                    double useHeight;

                    info.TitleDrawErr = true;

                    //分
                    string min = info.ReserveInfo.StartTime.Minute.ToString("d02");
                    //設計的にやや微妙だがやる事が同じなのでEpgViewPanelのメソッドを流用する
                    if (EpgView.EpgViewPanel.RenderText(min, textDrawList, itemFontTitle, sizeTitle * 0.95,
                                                        innerWidth - 1, innerHeight,
                                                        innerLeft + 1, innerTop, out useHeight, colorTitle, m, 0))
                    {
                        //サービス名
                        string serviceName = info.ReserveInfo.StationName;
                        serviceName += " (" + CommonManager.ConvertNetworkNameText(info.ReserveInfo.OriginalNetworkID) + ")";
                        if (EpgView.EpgViewPanel.RenderText(serviceName, textDrawList, itemFontTitle, sizeTitle,
                                                            innerWidth - sizeTitle * 0.5 - indentTitle, innerHeight,
                                                            innerLeft + indentTitle, innerTop, out useHeight, colorTitle, m, 0))
                        {
                            double renderTextHeight = useHeight + sizeNormal * 0.5;
                            //番組名
                            if (EpgView.EpgViewPanel.RenderText(info.ReserveInfo.Title, textDrawList, itemFontNormal, sizeNormal,
                                                                innerWidth - sizeTitle * 0.5 - indentNormal, innerHeight - renderTextHeight,
                                                                innerLeft + indentNormal, innerTop + renderTextHeight, out useHeight, colorNormal, m, 0))
                            {
                                info.TitleDrawErr = innerHeight < renderTextHeight + useHeight;
                            }
                        }
                    }

                    double bgHeight = Math.Min(borderHalfTop, info.Height);
                    if (innerWidth > 0 && bgHeight > 0)
                    {
                        dc.DrawRectangle(Background, null, new Rect(info.LeftPos + borderHalfLeft, info.TopPos, innerWidth, bgHeight));
                        dc.DrawRectangle(Background, null, new Rect(info.LeftPos + borderHalfLeft, info.TopPos + info.Height - bgHeight, innerWidth, bgHeight));
                    }
                    if (innerWidth > 0 && innerHeight > 0)
                    {
                        var textArea = new Rect(info.LeftPos + borderHalfLeft, info.TopPos + borderHalfTop, innerWidth, innerHeight);
                        dc.DrawRectangle(textAreaBrush, null, textArea);
                        dc.PushClip(new RectangleGeometry(textArea));
                        foreach (Tuple<Brush, GlyphRun> txtinfo in textDrawList)
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

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

            var itemFontTitle = new EpgView.EpgViewPanel.ItemFont(Settings.Instance.FontNameTitle, Settings.Instance.FontBoldTitle, true);
            var itemFontNormal = new EpgView.EpgViewPanel.ItemFont(Settings.Instance.FontName, false, true);
            if (itemFontTitle.GlyphType == null || itemFontNormal.GlyphType == null)
            {
                return;
            }

            {
                double sizeTitle = Math.Max(Settings.Instance.FontSizeTitle, 1);
                double sizeNormal = Math.Max(Settings.Instance.FontSize, 1);
                double indentTitle = sizeTitle * 1.7;
                double indentNormal = 2;
                SolidColorBrush colorTitle = CommonManager.Instance.CustTitle1Color;
                SolidColorBrush colorNormal = CommonManager.Instance.CustTitle2Color;

                foreach (ReserveViewItem info in Items)
                {
                    var textDrawList = new List<Tuple<Brush, GlyphRun>>();

                    double innerLeft = info.LeftPos + 1;
                    double innerTop = info.TopPos + 1;
                    double innerWidth = info.Width - 2;
                    double innerHeight = info.Height - 2;
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

                    if (info.Width > 0 && info.Height > 0)
                    {
                        dc.DrawRectangle(Brushes.LightGray, null, new Rect(info.LeftPos, info.TopPos, info.Width, info.Height));
                    }
                    if (innerWidth > 0 && innerHeight > 0)
                    {
                        var textArea = new Rect(innerLeft, innerTop, innerWidth, innerHeight);
                        dc.DrawRectangle(info.ReserveInfo.OverlapMode == 1 ? Brushes.Yellow : Brushes.White, null, textArea);
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

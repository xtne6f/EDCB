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

            if (Settings.Instance.MinimumHeight > 0)
            {
                // Items を表示順にソートする
                Items.Sort(Compare);
                // 前番組へのリンク
                ProgramViewItem wkPrev = null;
                String wkPrevDate = "";
                foreach (ProgramViewItem info in Items)
                {
                    if (wkPrev != null)
                    {
                        if (wkPrev.EventInfo.service_id != wkPrev.EventInfo.service_id)
                        {   //  サービスIDが変わる => 局が変わる => その局の一番先頭の番組
                            wkPrev = null;
                        }
                        if (!wkPrevDate.Equals(info.EventInfo.start_time.AddHours(-4).ToShortDateString()))
                        {   //  1週間モード時、先頭は4時に切り替わる＝4時間前に日付が変わるタイミング
                            wkPrev = null;
                        }
                    }
                    info.prevItem = wkPrev;
                    wkPrev = info;
                    wkPrevDate = info.EventInfo.start_time.AddHours(-4).ToShortDateString();
                }

                double minimum = (Settings.Instance.FontSizeTitle + 2) * Settings.Instance.MinimumHeight;

                foreach (ProgramViewItem info in Items)
                {
                    // 最低表示dot数よりも小さければ
                    if (info.Height < minimum)
                    {
                        double wk1 = minimum - info.Height;                            // 調整幅(Total)
                        double wk2 = 0;                                                // 調整可能幅
                        double wk3 = wk1;                                              // 調整後の要再調整幅
                        ProgramViewItem pr = info.prevItem;
                        if (pr != null) {                                              // 先頭ならやりようがない
                            while (pr.Height < minimum + wk3) {                        // 調整できるだけの十分な高さが無い
                                wk2 = pr.Height - minimum;                             // 無理にでも調整可能な幅を求める
                                if (wk2 > 0) {} else { wk2 = 0; }                      // 念の為マイナスを排除
                                wk3 = wk3 - wk2;                                       // 調整後の要再調整幅

                                if (  pr.prevTop == 0) {   pr.prevTop =   pr.TopPos; }
                                pr.Height -= wk2;                                      // 高さと
                                if (pr.Height >= minimum) { pr.TopPos -= wk3; }        // 開始位置 の調整

                                pr = pr.prevItem;                                      // 次(前)
                                if (pr == null) { break; }                             // が無い？なら終わり
                            }

                            if (pr != null) {
                                if (  pr.prevTop == 0) {   pr.prevTop =   pr.TopPos; }
                                pr.Height -= wk3;                                      //  高さと
                                if (info.prevTop == 0) { info.prevTop = info.TopPos; }
                            }

                            info.TopPos -= wk1;                                        //  開始位置 の調整
                            info.Height  = minimum;                                    //  最低表示dot数
                        }
                        else
                        {
                            info.TopPos -= wk;      //  先頭位置をずらす
                            info.Height = minimum;    //  最低表示dot数
                        }
                    }
                }
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
                        if (RenderText(min, ref textDrawList, glyphTypefaceTitle, sizeTitle - 0.5, info.Width - 4, info.Height + 10, info.LeftPos - 1, info.TopPos - 1, ref useHeight, CommonManager.Instance.CustTitle1Color, m) == false)
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
                                if (RenderText(info.EventInfo.ShortInfo.event_name, ref textDrawList, glyphTypefaceTitle, sizeTitle, info.Width - 6 - widthOffset, info.Height - 1 - totalHeight, info.LeftPos + widthOffset, info.TopPos + totalHeight, ref useHeight, CommonManager.Instance.CustTitle1Color, m) == false)
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
                                if (RenderText(info.EventInfo.ShortInfo.text_char, ref textDrawList, glyphTypefaceNormal, sizeNormal, info.Width - 10 - widthOffset, info.Height - 5 - totalHeight, info.LeftPos + widthOffset, info.TopPos + totalHeight, ref useHeight, CommonManager.Instance.CustTitle2Color, m) == false)
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

        protected bool RenderText(String text, ref List<TextDrawItem> textDrawList, GlyphTypeface glyphType, double fontSize, double maxWidth, double maxHeight, double x, double y, ref double useHeight, SolidColorBrush fontColor, Matrix m)
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
                    ushort glyphIndex = glyphType.CharacterToGlyphMap[line[n]];
                    double width = glyphType.AdvanceWidths[glyphIndex] * fontSize;
                    if (totalWidth + width > maxWidth)
                    {
                        if (totalHeight > maxHeight)
                        {
                            //次の行無理
//                            glyphIndex = glyphType.CharacterToGlyphMap['…'];
//                            glyphIndexes[glyphIndexes.Count - 1] = glyphIndex;
//                            advanceWidths[advanceWidths.Count - 1] = width;

                            double dpix = Math.Ceiling((x + 2) * m.M11);
                            double dpiy = Math.Ceiling((y + totalHeight) * m.M22);
                            Point origin = new Point(dpix / m.M11, dpiy / m.M22);
                            TextDrawItem item = new TextDrawItem();
                            item.FontColor = fontColor;
                            item.Text = new GlyphRun(glyphType, 0, false, fontSize,
                                glyphIndexes, origin, advanceWidths, null, null, null, null,
                                null, null);
                            textDrawList.Add(item);

                            useHeight = totalHeight;
                            return false;
                        }
                        else
                        {
                            //次の行いけるので今までの分出力
                            //次の行いける
                            double dpix = Math.Ceiling((x + 2) * m.M11);
                            double dpiy = Math.Ceiling((y + totalHeight) * m.M22);
                            Point origin = new Point(dpix / m.M11, dpiy / m.M22); 
                            TextDrawItem item = new TextDrawItem();
                            item.FontColor = fontColor;
                            item.Text = new GlyphRun(glyphType, 0, false, fontSize,
                                glyphIndexes, origin, advanceWidths, null, null, null, null,
                                null, null);
                            textDrawList.Add(item);

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
                    item.Text = new GlyphRun(glyphType, 0, false, fontSize,
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
                double sizeNormal = Settings.Instance.FontSize;
                double sizeTitle = Settings.Instance.FontSizeTitle;
                foreach (ProgramViewItem info in Items)
                {
                    dc.DrawRectangle(Brushes.DarkGray, null, new Rect(info.LeftPos, info.TopPos, info.Width, info.Height));
                    if (info.Height > 2)
                    {
                        dc.DrawRectangle(info.ContentColor, null, new Rect(info.LeftPos + 0, info.TopPos + 0.5, info.Width - 1, info.Height - 0.5));
                        if (textDrawDict.ContainsKey(info))
                        {
                            dc.PushClip(new RectangleGeometry(new Rect(info.LeftPos + 0, info.TopPos + 0.5, info.Width - 1, info.Height - 0.5)));
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

        // サービスID/開始日付/TopPos 順にソート
        public static int Compare(ProgramViewItem x, ProgramViewItem y)
        {
            //nullが最も小さいとする
            if (x == null && y == null)
            {
                return 0;
            }
            if (x == null)
            {
                return -1;
            }
            if (y == null)
            {
                return 1;
            }

            if (x.EventInfo.service_id < y.EventInfo.service_id)
            {
                return -1;
            }
            else if (x.EventInfo.service_id > y.EventInfo.service_id)
            {
                return 1;
            }
            else
            {
                if (x.EventInfo.start_time < y.EventInfo.start_time)
                {
                    return -1;
                }
                else if (x.EventInfo.start_time > y.EventInfo.start_time)
                {
                    return 1;
                }
                else
                {
                    return 0;
                    //// 不必要？
                    //if (x.TopPos < y.TopPos)
                    //{
                    //    return -1;
                    //}
                    //else if (x.TopPos > y.TopPos)
                    //{
                    //    return 1;
                    //}
                    //else
                    //{
                    //    return 0;
                    //}
                }
            }
        }
    }

    class TextDrawItem
    {
        public SolidColorBrush FontColor;
        public GlyphRun Text;
    }
}

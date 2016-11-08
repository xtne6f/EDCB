using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Shapes;

namespace EpgTimer.EpgView
{
    /// <summary>
    /// ProgramView.xaml の相互作用ロジック
    /// </summary>
    public partial class ProgramView : EpgTimer.UserCtrlView.PanelViewBase
    {
        protected override bool IsSingleClickOpen { get { return Settings.Instance.EpgInfoSingleClick; } }
        protected override double DragScroll { get { return Settings.Instance.DragScroll; } }
        protected override bool IsMouseScrollAuto { get { return Settings.Instance.MouseScrollAuto; } }
        protected override double ScrollSize { get { return Settings.Instance.ScrollSize; } }
        protected override bool IsPopEnabled { get { return Settings.Instance.EpgPopup == true; } }
        protected override bool PopOnOver { get { return Settings.Instance.EpgPopupMode != 1; } }
        protected override bool PopOnClick { get { return Settings.Instance.EpgPopupMode != 0; } }
        protected override FrameworkElement Popup { get { return popupItem; } }
        protected override double PopWidth { get { return Settings.Instance.ServiceWidth * Settings.Instance.EpgPopupWidth; } }

        private List<ReserveViewItem> reserveList = null;
        private List<Rectangle> rectBorder = new List<Rectangle>();
        private ReserveViewItem popInfoRes = null;

        protected override bool IsTooltipEnabled { get { return Settings.Instance.EpgToolTip == true; } }
        protected override int TooltipViweWait { get { return Settings.Instance.EpgToolTipViewWait; } }

        public ProgramView()
        {
            InitializeComponent();

            base.scroll = scrollViewer;
            base.cnvs = canvas;
        }

        public override void ClearInfo()
        {
            base.ClearInfo();
            reserveList = null;
            rectBorder.ForEach(item => canvas.Children.Remove(item));
            rectBorder.Clear();

            for (int i = 0; i < canvas.Children.Count; i++)
            {
                if (canvas.Children[i] is EpgViewPanel)
                {
                    canvas.Children.RemoveAt(i--);
                }
            }
        }

        protected override void PopupClear()
        {
            base.PopupClear();
            popInfoRes = null;
        }
        protected override PanelItem GetPopupItem(Point cursorPos, bool onClick)
        {
            ProgramViewItem popInfo = GetProgramViewData(cursorPos);
            ReserveViewItem lastPopInfoRes = popInfoRes;
            popInfoRes = reserveList == null ? null : reserveList.Find(pg => pg.IsPicked(cursorPos));

            if (Settings.Instance.EpgPopupMode == 2 && popInfoRes == null && (
                onClick == false && !(lastPopInfoRes == null && popInfo == lastPopInfo) ||
                onClick == true && lastPopInfo != null)) return null;

            //予約枠を通過したので同じ番組でもポップアップを書き直させる。
            if (lastPopInfoRes != popInfoRes)
            {
                base.PopupClear();
            }

            return popInfo;
        }
        protected override void SetPopup(PanelItem item)
        {
            var viewInfo = (ProgramViewItem)item;
            EpgEventInfo epgInfo = viewInfo.EventInfo;

            popupItem.Background = viewInfo.ContentColor;

            double sizeMin = Settings.Instance.FontSizeTitle - 1;
            double sizeTitle = Settings.Instance.FontSizeTitle;
            double sizeNormal = Settings.Instance.FontSize;
            double indentTitle = Math.Floor(sizeMin * 1.7 + 1);
            double indentNormal = Math.Floor(Settings.Instance.EpgTitleIndent ? indentTitle : 3);
            var fontNormal = new FontFamily(Settings.Instance.FontName);
            var fontTitle = new FontFamily(Settings.Instance.FontNameTitle);
            FontWeight titleWeight = Settings.Instance.FontBoldTitle == true ? FontWeights.Bold : FontWeights.Normal;

            minText.Text = (epgInfo.StartTimeFlag == 0 ? "未定" : epgInfo.start_time.Minute.ToString("d02"));
            minText.FontFamily = fontTitle;
            minText.FontSize = sizeMin;
            minText.FontWeight = titleWeight;
            minText.Foreground = CommonManager.Instance.CustTitle1Color;
            //minText.Margin = new Thickness(0, 0, 0, 0);
            minText.LineHeight = sizeMin + 2;

            if (epgInfo.ShortInfo != null)
            {
                //必ず文字単位で折り返すためにZWSPを挿入 (\\w を使うと記号の間にZWSPが入らない)
                titleText.Text = System.Text.RegularExpressions.Regex.Replace(epgInfo.ShortInfo.event_name, ".", "$0\u200b");
                titleText.FontFamily = fontTitle;
                titleText.FontSize = sizeTitle;
                titleText.FontWeight = titleWeight;
                titleText.Foreground = CommonManager.Instance.CustTitle1Color;
                titleText.Margin = new Thickness(indentTitle, 0, 0, Math.Floor(sizeTitle / 3));
                titleText.LineHeight = sizeTitle + 2;

                string iTxt = epgInfo.ShortInfo.text_char;
                if (Settings.Instance.EpgExtInfoPopup == true && epgInfo.ExtInfo != null)
                {
                    iTxt += "\r\n" + epgInfo.ExtInfo.text_char;
                }
                infoText.Text = System.Text.RegularExpressions.Regex.Replace(iTxt, ".", "$0\u200b");
                infoText.FontFamily = fontNormal;
                infoText.FontSize = sizeNormal;
                //infoText.FontWeight = FontWeights.Normal;
                infoText.Foreground = CommonManager.Instance.CustTitle2Color;
                infoText.Margin = new Thickness(indentNormal, 0, 0, Math.Floor(sizeNormal / 3));
                infoText.LineHeight = sizeNormal + 2;
            }
            else
            {
                titleText.Text = null;
                infoText.Text = null;
            }

            //予約枠の表示
            double marginEpg = 1;
            double marginRes = marginEpg + 3;
            popupItemTextArea.Margin = new Thickness(marginEpg, marginEpg - 2, marginEpg + 3, marginEpg);
            if (popInfoRes != null)
            {
                SetReserveBorder(popupItemBorder, popInfoRes);
                popupItemBorder.Visibility = Visibility.Visible;
                if (Settings.Instance.ReserveRectBackground == false)
                {
                    popupItemTextArea.Margin = new Thickness(marginRes, marginRes - 1, marginRes, marginRes);
                }
            }
            else
            {
                popupItemBorder.Visibility = Visibility.Collapsed;
            }
        }

        protected override PanelItem GetTooltipItem(Point cursorPos)
        {
            return GetProgramViewData(cursorPos);
        }
        protected override void SetTooltip(PanelItem toolInfo)
        {
            var info = toolInfo as ProgramViewItem;
            if (info.TitleDrawErr == false && Settings.Instance.EpgToolTipNoViewOnly == true) return;

            Tooltip.ToolTip = ViewUtil.GetTooltipBlockStandard(CommonManager.ConvertProgramText(info.EventInfo,
                Settings.Instance.EpgExtInfoTooltip == true ? EventInfoTextMode.TextAll : EventInfoTextMode.BasicText));
        }

        public ProgramViewItem GetProgramViewData(Point cursorPos)
        {
            try
            {
                foreach (var childPanel in canvas.Children.OfType<EpgViewPanel>())
                {
                    if (childPanel.Items != null && Canvas.GetLeft(childPanel) <= cursorPos.X && cursorPos.X < Canvas.GetLeft(childPanel) + childPanel.Width)
                    {
                        return childPanel.Items.OfType<ProgramViewItem>().FirstOrDefault(pg => pg.IsPicked(cursorPos));
                    }
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }

            return null;
        }

        private void SetReserveBorder(Rectangle rect, ReserveViewItem info)
        {
            Brush color = info.BorderBrush;

            if (Settings.Instance.ReserveRectBackground == false)
            {
                rect.Opacity = 0.5;
                rect.Effect = new System.Windows.Media.Effects.DropShadowEffect() { BlurRadius = 10 };
                rect.Fill = Brushes.Transparent;
                rect.StrokeThickness = 3;
                rect.Stroke = color;
            }
            else
            {
                rect.Opacity = 0.3;
                rect.Effect = new System.Windows.Media.Effects.DropShadowEffect() { BlurRadius = 6 };
                rect.Fill = color;
            }
        }
        public void SetReserveList(List<ReserveViewItem> resList)
        {
            try
            {
                reserveList = resList;
                rectBorder.ForEach(item => canvas.Children.Remove(item));
                rectBorder.Clear();

                foreach (ReserveViewItem info in reserveList)
                {
                    var rect = new Rectangle();
                    rect.Width = info.Width;
                    rect.Height = info.Height;
                    rect.IsHitTestVisible = false;

                    SetReserveBorder(rect, info);

                    Canvas.SetLeft(rect, info.LeftPos);
                    Canvas.SetTop(rect, info.TopPos);
                    Canvas.SetZIndex(rect, 10);
                    canvas.Children.Add(rect);
                    rectBorder.Add(rect);
                }

                PopUpWork();
                TooltipWork();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void SetProgramList(List<ProgramViewItem> programList, double width, double height)
        {
            SetProgramList(CommonUtil.ToList(new PanelItem<List<ProgramViewItem>>(programList) { Width = width }), height);
        }
        public void SetProgramList(List<PanelItem<List<ProgramViewItem>>> programGroupList, double height)
        {
            try
            {
                for (int i = 0; i < canvas.Children.Count; i++)
                {
                    if (canvas.Children[i] is EpgViewPanel)
                    {
                        canvas.Children.RemoveAt(i--);
                    }
                }
                var itemFontNormal = new EpgViewPanel.ItemFont(Settings.Instance.FontName, false);
                var itemFontTitle = new EpgViewPanel.ItemFont(Settings.Instance.FontNameTitle, Settings.Instance.FontBoldTitle);
                double totalWidth = 0;
                foreach (var programList in programGroupList)
                {
                    EpgViewPanel item = new EpgViewPanel();
                    item.Background = epgViewPanel.Background;
                    item.Height = Math.Ceiling(height);
                    item.Width = programList.Width;
                    item.ItemFontNormal = itemFontNormal;
                    item.ItemFontTitle = itemFontTitle;
                    Canvas.SetLeft(item, totalWidth);
                    item.Items = programList.Data;
                    item.InvalidateVisual();
                    canvas.Children.Add(item);
                    totalWidth += programList.Width;
                }
                canvas.Height = Math.Ceiling(height);
                canvas.Width = totalWidth;
                itemFontNormal.ClearCache();
                itemFontTitle.ClearCache();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
    }
}

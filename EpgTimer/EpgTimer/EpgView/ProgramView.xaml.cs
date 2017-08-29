using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;
using System.Windows.Controls.Primitives;

namespace EpgTimer.EpgView
{
    /// <summary>
    /// ProgramView.xaml の相互作用ロジック
    /// </summary>
    public partial class ProgramView : UserControl
    {
        public delegate void ProgramViewClickHandler(object sender, Point cursorPos);
        public event ScrollChangedEventHandler ScrollChanged = null;
        public event ProgramViewClickHandler LeftDoubleClick = null;
        public event ProgramViewClickHandler RightClick = null;
        private List<Rectangle> reserveBorder = new List<Rectangle>();

        private Point lastDownMousePos;
        private double lastDownHOffset;
        private double lastDownVOffset;
        private bool isDrag = false;
        private bool isDragMoved = false;

        private DispatcherTimer toolTipTimer;
        private DispatcherTimer toolTipOffTimer; 
        private Popup toolTip = new Popup();
        private Point lastPopupPos;
        private ProgramViewItem lastPopupInfo = null;

        public ProgramView()
        {
            InitializeComponent();

            toolTipTimer = new DispatcherTimer(DispatcherPriority.Normal);
            toolTipTimer.Tick += new EventHandler(toolTipTimer_Tick);
            toolTipOffTimer = new DispatcherTimer(DispatcherPriority.Normal);
            toolTipOffTimer.Tick += new EventHandler(toolTipOffTimer_Tick);

            toolTip.Placement = PlacementMode.MousePoint;
            toolTip.PopupAnimation = PopupAnimation.Fade;
            toolTip.PlacementTarget = canvas;
            toolTip.AllowsTransparency = true;
            toolTip.MouseLeftButtonDown += new MouseButtonEventHandler(toolTip_MouseLeftButtonDown);
            toolTip.MouseRightButtonDown += new MouseButtonEventHandler(toolTip_MouseRightButtonDown);
            toolTip.PreviewMouseWheel += new MouseWheelEventHandler(toolTip_PreviewMouseWheel);
        }

        void toolTip_PreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            toolTip.IsOpen = false;
            RaiseEvent(e);
        }

        void toolTip_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (e.ClickCount == 2)
            {
                toolTip.IsOpen = false;
                if (LeftDoubleClick != null)
                {
                    LeftDoubleClick(sender, lastPopupPos);
                }
            }
            else if (Settings.Instance.EpgInfoSingleClick)
            {
                toolTip.IsOpen = false;
            }
        }

        void toolTip_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            toolTip.IsOpen = false;
            if (RightClick != null)
            {
                RightClick(sender, lastPopupPos);
            }
        }

        void toolTipOffTimer_Tick(object sender, EventArgs e)
        {
            toolTipOffTimer.Stop();
            toolTip.IsOpen = false;
        }

        void toolTipTimer_Tick(object sender, EventArgs e)
        {
            toolTipTimer.Stop();
            try
            {
                if (Settings.Instance.EpgPopup || Settings.Instance.EpgToolTip == false)
                {
                    return;
                }
                if (Window.GetWindow(this).IsActive == false)
                {
                    return;
                }
                Point cursorPos2 = Mouse.GetPosition(scrollViewer);
                if (cursorPos2.X < 0 || cursorPos2.Y < 0 ||
                    scrollViewer.ViewportWidth < cursorPos2.X || scrollViewer.ViewportHeight < cursorPos2.Y)
                {
                    return;
                }
                Point cursorPos = Mouse.GetPosition(canvas);
                foreach (UIElement child in canvas.Children)
                {
                    EpgViewPanel childPanel = child as EpgViewPanel;
                    if (childPanel != null && childPanel.Items != null && Canvas.GetLeft(child) <= cursorPos.X && cursorPos.X < Canvas.GetLeft(child) + childPanel.Width)
                    {
                        foreach (ProgramViewItem info in childPanel.Items)
                        {
                            if (info.LeftPos <= cursorPos.X && cursorPos.X < info.LeftPos + info.Width &&
                                info.TopPos <= cursorPos.Y && cursorPos.Y < info.TopPos + info.Height)
                            {
                                if (info.TitleDrawErr == false && Settings.Instance.EpgToolTipNoViewOnly == true)
                                {
                                    break;
                                }
                                String viewTip = "";

                                if (info != null)
                                {
                                    if (info.EventInfo.StartTimeFlag == 1)
                                    {
                                        viewTip += info.EventInfo.start_time.ToString("yyyy/MM/dd(ddd) HH:mm:ss ～ ");
                                    }
                                    else
                                    {
                                        viewTip += "未定 ～ ";
                                    }
                                    if (info.EventInfo.DurationFlag == 1)
                                    {
                                        DateTime endTime = info.EventInfo.start_time + TimeSpan.FromSeconds(info.EventInfo.durationSec);
                                        viewTip += endTime.ToString("yyyy/MM/dd(ddd) HH:mm:ss") + "\r\n";
                                    }
                                    else
                                    {
                                        viewTip += "未定\r\n";
                                    }

                                    if (info.EventInfo.ShortInfo != null)
                                    {
                                        viewTip += info.EventInfo.ShortInfo.event_name + "\r\n\r\n";
                                        viewTip += info.EventInfo.ShortInfo.text_char + "\r\n\r\n";
                                    }
                                    if (info.EventInfo.ExtInfo != null)
                                    {
                                        viewTip += info.EventInfo.ExtInfo.text_char;
                                    }
                                }
                                Border border = new Border();
                                border.Background = Brushes.DarkGray;

                                TextBlock block = new TextBlock();
                                block.Text = viewTip;
                                block.MaxWidth = 400;
                                block.TextWrapping = TextWrapping.Wrap;
                                block.Margin = new Thickness(2);

                                block.Background = CommonManager.Instance.EpgTipsBackColor;
                                block.Foreground = CommonManager.Instance.EpgTipsForeColor;
                                border.Child = block;
                                toolTip.Child = border;
                                toolTip.IsOpen = true;
                                toolTipOffTimer.Stop();
                                toolTipOffTimer.Interval = TimeSpan.FromSeconds(10);
                                toolTipOffTimer.Start();

                                lastPopupInfo = info;
                                lastPopupPos = cursorPos;
                            }
                        }
                        break;
                    }
                }
            }
            catch
            {
                toolTip.IsOpen = false;
            }
        }

        protected void PopupItem()
        {
            if (Settings.Instance.EpgPopup == false) return;

            ProgramViewItem info = null;

            Point cursorPos2 = Mouse.GetPosition(scrollViewer);
            if (cursorPos2.X < 0 || cursorPos2.Y < 0 ||
                scrollViewer.ViewportWidth < cursorPos2.X || scrollViewer.ViewportHeight < cursorPos2.Y)
            {
                return;
            }
            Point cursorPos = Mouse.GetPosition(canvas);
            foreach (UIElement child in canvas.Children)
            {
                EpgViewPanel childPanel = child as EpgViewPanel;
                if (childPanel != null && childPanel.Items != null && Canvas.GetLeft(child) <= cursorPos.X && cursorPos.X < Canvas.GetLeft(child) + childPanel.Width)
                {
                    foreach (ProgramViewItem item in childPanel.Items)
                    {
                        if (item.LeftPos <= cursorPos.X && cursorPos.X < item.LeftPos + item.Width &&
                            item.TopPos <= cursorPos.Y && cursorPos.Y < item.TopPos + item.Height)
                        {
                            if (item == lastPopupInfo) return;

                            info = item;
                            lastPopupInfo = info;
                            break;
                        }
                    }
                    break;
                }
            }

            if (info == null)
            {
                popupItem.Visibility = System.Windows.Visibility.Hidden;
                lastPopupInfo = null;
                return;
            }

            double sizeNormal = Settings.Instance.FontSize;
            double sizeTitle = Settings.Instance.FontSizeTitle;

            FontFamily fontNormal = null;
            FontFamily fontTitle = null;
            try
            {
                if (Settings.Instance.FontName.Length > 0)
                {
                    fontNormal = new FontFamily(Settings.Instance.FontName);
                }
                if (Settings.Instance.FontNameTitle.Length > 0)
                {
                    fontTitle = new FontFamily(Settings.Instance.FontNameTitle);
                }

                if (fontNormal == null)
                {
                    fontNormal = new FontFamily("MS UI Gothic");
                }
                if (fontTitle == null)
                {
                    fontTitle =new FontFamily("MS UI Gothic");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }

            popupItemContainer.Background = info.ContentColor;
            Canvas.SetLeft(popupItem, info.LeftPos);
            Canvas.SetTop(popupItem, info.TopPos);
            popupItemContainer.Width = info.Width;
            popupItemContainer.MinHeight = info.Height;

            FontWeight titleWeight = Settings.Instance.FontBoldTitle ? FontWeights.Bold : FontWeights.Normal;

            string min;
            if (info.EventInfo.StartTimeFlag == 1)
            {
                min = info.EventInfo.start_time.Minute.ToString("d02");
            }
            else
            {
                min = "未定";
            }

            minText.Text = min;
            minText.FontFamily = fontTitle;
            minText.FontSize = sizeTitle - 0.5;
            minText.FontWeight = titleWeight;
            minText.Foreground = CommonManager.Instance.CustTitle1Color;
            minText.Margin = new Thickness(0.5, -0.5, 0, 0);

            minGrid.Width = new GridLength(sizeNormal * 1.7 + 1);

            if (info.EventInfo.ShortInfo != null)
            {
                //必ず文字単位で折り返すためにZWSPを挿入
                titleText.Text = System.Text.RegularExpressions.Regex.Replace(info.EventInfo.ShortInfo.event_name, "\\w", "$0\u200b");
                titleText.FontFamily = fontTitle;
                titleText.FontSize = sizeTitle;
                titleText.FontWeight = titleWeight;
                titleText.Foreground = CommonManager.Instance.CustTitle1Color;
                titleText.Margin = new Thickness(1, 1.5, 5.5, sizeNormal / 2);
                titleText.LineHeight = sizeTitle + 2;

                infoText.Text = System.Text.RegularExpressions.Regex.Replace(info.EventInfo.ShortInfo.text_char, "\\w", "$0\u200b");
                infoText.FontFamily = fontNormal;
                infoText.FontSize = sizeNormal;
                infoText.FontWeight = FontWeights.Normal;
                infoText.Foreground = CommonManager.Instance.CustTitle2Color;
                infoText.Margin = new Thickness(Settings.Instance.EpgTitleIndent ? minGrid.Width.Value + 1 : 1, 0, 9.5, 1);
                infoText.LineHeight = sizeNormal + 2;
            }
            else
            {
                titleText.Text = null;
                infoText.Text = null;
            }

            popupItem.Visibility = System.Windows.Visibility.Visible;
        }

        public void ClearInfo()
        {
            toolTipTimer.Stop();
            toolTip.IsOpen = false;
            lastPopupInfo = null;
            popupItem.Visibility = System.Windows.Visibility.Hidden;

            foreach (Rectangle info in reserveBorder)
            {
                canvas.Children.Remove(info);
            }
            reserveBorder.Clear();
            reserveBorder = null;
            reserveBorder = new List<Rectangle>();

            canvas.ReleaseMouseCapture();
            isDrag = false;

            for (int i = 0; i < canvas.Children.Count; i++)
            {
                if (canvas.Children[i] is EpgViewPanel)
                {
                    canvas.Children.RemoveAt(i--);
                }
            }
            canvas.Height = 0;
            canvas.Width = 0;
        }

        public void SetReserveList(List<ReserveViewItem> reserveList)
        {
            try
            {
                foreach (Rectangle info in reserveBorder)
                {
                    canvas.Children.Remove(info);
                }
                reserveBorder.Clear();

                foreach (ReserveViewItem info in reserveList)
                {
                    Rectangle rect = new Rectangle();
                    Rectangle fillOnlyRect = Settings.Instance.ReserveRectFillWithShadow ? null : new Rectangle();
                    Rectangle fillRect = fillOnlyRect ?? rect;

                    if (info.ReserveInfo.RecSetting.RecMode == 5)
                    {
                        rect.Stroke = CommonManager.Instance.CustContentColorList[19];
                        fillRect.Fill = CommonManager.Instance.CustContentColorList[20];
                    }
                    else if (info.ReserveInfo.OverlapMode == 2)
                    {
                        rect.Stroke = CommonManager.Instance.CustContentColorList[21];
                        fillRect.Fill = CommonManager.Instance.CustContentColorList[22];
                    }
                    else if (info.ReserveInfo.OverlapMode == 1)
                    {
                        rect.Stroke = CommonManager.Instance.CustContentColorList[23];
                        fillRect.Fill = CommonManager.Instance.CustContentColorList[24];
                    }
                    else
                    {
                        rect.Stroke = CommonManager.Instance.CustContentColorList[17];
                        fillRect.Fill = CommonManager.Instance.CustContentColorList[18];
                    }

                    rect.Effect = new System.Windows.Media.Effects.DropShadowEffect() { BlurRadius = 10 };
                    rect.StrokeThickness = 3;
                    rect.Width = info.Width;
                    rect.Height = info.Height;
                    rect.IsHitTestVisible = false;
                    fillRect.Width = info.Width;
                    fillRect.Height = info.Height;
                    fillRect.IsHitTestVisible = false;

                    Canvas.SetLeft(rect, info.LeftPos);
                    Canvas.SetTop(rect, info.TopPos);
                    Canvas.SetZIndex(rect, 10);
                    canvas.Children.Add(rect);
                    reserveBorder.Add(rect);

                    if (fillOnlyRect != null)
                    {
                        Canvas.SetLeft(fillOnlyRect, info.LeftPos);
                        Canvas.SetTop(fillOnlyRect, info.TopPos);
                        Canvas.SetZIndex(fillOnlyRect, 9);
                        canvas.Children.Add(fillOnlyRect);
                        reserveBorder.Add(fillOnlyRect);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void SetProgramList(List<ProgramViewItem> programList, double width, double height)
        {
            var programGroupList = new List<Tuple<double, List<ProgramViewItem>>>();
            programGroupList.Add(new Tuple<double, List<ProgramViewItem>>(width, programList));
            SetProgramList(programGroupList, height);
        }

        public void SetProgramList(List<Tuple<double, List<ProgramViewItem>>> programGroupList, double height)
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
                if (itemFontNormal.GlyphType == null)
                {
                    itemFontNormal = new EpgViewPanel.ItemFont("MS UI Gothic", false);
                }
                var itemFontTitle = new EpgViewPanel.ItemFont(Settings.Instance.FontNameTitle, Settings.Instance.FontBoldTitle);
                if (itemFontTitle.GlyphType == null)
                {
                    itemFontTitle = new EpgViewPanel.ItemFont("MS UI Gothic", Settings.Instance.FontBoldTitle);
                }
                epgViewPanel.Background = CommonManager.Instance.EpgBackColor;
                double totalWidth = 0;
                foreach (var programList in programGroupList)
                {
                    EpgViewPanel item = new EpgViewPanel();
                    item.Background = epgViewPanel.Background;
                    item.Height = Math.Ceiling(height);
                    item.Width = programList.Item1;
                    item.IsTitleIndent = Settings.Instance.EpgTitleIndent;
                    item.ItemFontNormal = itemFontNormal;
                    item.ItemFontTitle = itemFontTitle;
                    Canvas.SetLeft(item, totalWidth);
                    item.Items = programList.Item2;
                    item.InvalidateVisual();
                    canvas.Children.Add(item);
                    totalWidth += programList.Item1;
                }
                canvas.Height = Math.Ceiling(height);
                canvas.Width = totalWidth;
                itemFontNormal.ClearCache();
                itemFontTitle.ClearCache();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void canvas_MouseMove(object sender, MouseEventArgs e)
        {
            try
            {
                {
                    if (e.LeftButton == MouseButtonState.Pressed && isDrag == true)
                    {
                        isDragMoved = true;
                        toolTipTimer.Stop();
                        toolTip.IsOpen = false;

                        Point CursorPos = Mouse.GetPosition(null);
                        double MoveX = lastDownMousePos.X - CursorPos.X;
                        double MoveY = lastDownMousePos.Y - CursorPos.Y;

                        double OffsetH = 0;
                        double OffsetV = 0;
                        MoveX *= Settings.Instance.DragScroll;
                        MoveY *= Settings.Instance.DragScroll;
                        OffsetH = lastDownHOffset + MoveX;
                        OffsetV = lastDownVOffset + MoveY;
                        if (OffsetH < 0)
                        {
                            OffsetH = 0;
                        }
                        if (OffsetV < 0)
                        {
                            OffsetV = 0;
                        }

                        scrollViewer.ScrollToHorizontalOffset(Math.Floor(OffsetH));
                        scrollViewer.ScrollToVerticalOffset(Math.Floor(OffsetV));
                    }
                    else
                    {
                        Point CursorPos = Mouse.GetPosition(canvas);
                        if (lastPopupPos != CursorPos)
                        {
                            toolTipTimer.Stop();
                            if (toolTip.IsOpen == true)
                            {
                                toolTip.IsOpen = false;
                                lastDownMousePos = Mouse.GetPosition(null);
                                lastDownHOffset = scrollViewer.HorizontalOffset;
                                lastDownVOffset = scrollViewer.VerticalOffset;
                                if (e.LeftButton == MouseButtonState.Pressed)
                                {
                                    canvas.CaptureMouse();
                                    isDrag = true;
                                }
                            }

                            toolTipTimer.Interval = TimeSpan.FromMilliseconds(Settings.Instance.EpgToolTipViewWait);
                            toolTipTimer.Start();
                            PopupItem();
                            lastPopupPos = CursorPos;
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void canvas_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            try
            {
                toolTipTimer.Stop();
                toolTip.IsOpen = false;

                lastDownMousePos = Mouse.GetPosition(null);
                lastDownHOffset = scrollViewer.HorizontalOffset;
                lastDownVOffset = scrollViewer.VerticalOffset;
                canvas.CaptureMouse();
                isDrag = true;
                isDragMoved = false;

                if (e.ClickCount == 2)
                {
                    Point cursorPos = Mouse.GetPosition(canvas);
                    if (LeftDoubleClick != null)
                    {
                        LeftDoubleClick(sender, cursorPos);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void canvas_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            try
            {
                canvas.ReleaseMouseCapture();
                isDrag = false;
                if (isDragMoved == false)
                {
                    if (Settings.Instance.EpgInfoSingleClick == true)
                    {
                        Point cursorPos = Mouse.GetPosition(canvas);
                        if (LeftDoubleClick != null)
                        {
                            LeftDoubleClick(sender, cursorPos);
                        }
                    }
                }
                isDragMoved = false;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void scrollViewer_ScrollChanged(object sender, ScrollChangedEventArgs e)
        {
            if (ScrollChanged != null)
            {
                scrollViewer.ScrollToHorizontalOffset(Math.Floor(scrollViewer.HorizontalOffset));
                scrollViewer.ScrollToVerticalOffset(Math.Floor(scrollViewer.VerticalOffset));
                ScrollChanged(this, e);
            }
        }

        private void canvas_MouseRightButtonDown(object sender, MouseButtonEventArgs e)
        {
            toolTipTimer.Stop();
            toolTip.IsOpen = false;

            canvas.ReleaseMouseCapture();
            isDrag = false; 
            lastDownMousePos = Mouse.GetPosition(null);
            lastDownHOffset = scrollViewer.HorizontalOffset;
            lastDownVOffset = scrollViewer.VerticalOffset;
            if (e.ClickCount == 1)
            {
                Point cursorPos = Mouse.GetPosition(canvas);
                if (RightClick != null)
                {
                    RightClick(sender, cursorPos);
                }
            }
        }

        private void canvas_MouseLeave(object sender, MouseEventArgs e)
        {
            if (Settings.Instance.EpgPopup)
            {
                popupItem.Visibility = System.Windows.Visibility.Hidden;
                lastPopupInfo = null;
                lastPopupPos = new Point(-1, -1);
            }
        }
    }
}

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
using System.Windows.Shapes;

namespace EpgTimer
{
    /// <summary>
    /// ColorSetWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class ColorSetWindow : Window
    {
        public ColorSetWindow()
        {
            InitializeComponent();
        }

        public static readonly DependencyProperty RProperty = DependencyProperty.Register(
            "R", typeof(byte), typeof(ColorSetWindow), new PropertyMetadata((byte)255, PropertyChanged));

        public static readonly DependencyProperty GProperty = DependencyProperty.Register(
            "G", typeof(byte), typeof(ColorSetWindow), new PropertyMetadata((byte)255, PropertyChanged));

        public static readonly DependencyProperty BProperty = DependencyProperty.Register(
            "B", typeof(byte), typeof(ColorSetWindow), new PropertyMetadata((byte)255, PropertyChanged));

        public static readonly DependencyProperty AProperty = DependencyProperty.Register(
            "A", typeof(byte), typeof(ColorSetWindow), new PropertyMetadata((byte)0, PropertyChanged));

        public byte R
        {
            get { return (byte)GetValue(RProperty); }
            set { SetValue(RProperty, value); }
        }

        public byte G
        {
            get { return (byte)GetValue(GProperty); }
            set { SetValue(GProperty, value); }
        }

        public byte B
        {
            get { return (byte)GetValue(BProperty); }
            set { SetValue(BProperty, value); }
        }

        public byte A
        {
            get { return (byte)GetValue(AProperty); }
            set { SetValue(AProperty, value); }
        }

        private static void PropertyChanged(DependencyObject sender, DependencyPropertyChangedEventArgs e)
        {
            var win = (ColorSetWindow)sender;
            win.rectangle_color.Fill = new SolidColorBrush(Color.FromArgb(win.A, win.R, win.G, win.B));
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            // 市松模様
            double h = border_color.ActualHeight;
            var drawingGroup = new DrawingGroup();
            drawingGroup.Children.Add(new GeometryDrawing(Brushes.White, null, new RectangleGeometry(new Rect(0, 0, h / 5, h / 5))));
            drawingGroup.Children.Add(new GeometryDrawing(Brushes.Black, null, new RectangleGeometry(new Rect(0, 0, h / 10, h / 10))));
            drawingGroup.Children.Add(new GeometryDrawing(Brushes.Black, null, new RectangleGeometry(new Rect(h / 10, h / 10, h / 10, h / 10))));
            border_color.Background = new DrawingBrush(drawingGroup)
            {
                TileMode = TileMode.Tile,
                Viewport = new Rect(0, 0, h / 5, h / 5),
                ViewportUnits = BrushMappingMode.Absolute
            };
        }

        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
        }
    }
}

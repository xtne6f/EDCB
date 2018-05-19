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

        public void SetColor(Color argb)
        {
            R = argb.R;
            G = argb.G;
            B = argb.B;
            A = argb.A;
        }

        public void GetColor(ref Color argb)
        {
            argb.R = R;
            argb.G = G;
            argb.B = B;
            argb.A = A;
        }

        public static readonly DependencyProperty RProperty = DependencyProperty.Register(
            "R", typeof(byte), typeof(ColorSetWindow), new PropertyMetadata(Colors.White.R, PropertyChanged));

        public static readonly DependencyProperty GProperty = DependencyProperty.Register(
            "G", typeof(byte), typeof(ColorSetWindow), new PropertyMetadata(Colors.White.G, PropertyChanged));

        public static readonly DependencyProperty BProperty = DependencyProperty.Register(
            "B", typeof(byte), typeof(ColorSetWindow), new PropertyMetadata(Colors.White.B, PropertyChanged));

        public static readonly DependencyProperty AProperty = DependencyProperty.Register(
            "A", typeof(byte), typeof(ColorSetWindow), new PropertyMetadata(Colors.White.A, PropertyChanged));

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
            var argb = Colors.White;
            ((ColorSetWindow)sender).GetColor(ref argb);
            ((ColorSetWindow)sender).rectangle_color.Fill = new SolidColorBrush(argb);
        }

        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
    }
}

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

            slider_R.Value = 0xFF;
            slider_G.Value = 0xFF;
            slider_B.Value = 0xFF;
            slider_A.Value = 0xFF;
            textBox_R.Text = "255";
            textBox_G.Text = "255";
            textBox_B.Text = "255";
            textBox_A.Text = "255";

            rectangle_color.Fill = new SolidColorBrush();
            ChgColor();
        }

        public void SetColor(Color argb)
        {
            slider_R.Value = argb.R;
            slider_G.Value = argb.G;
            slider_B.Value = argb.B;
            slider_A.Value = argb.A;
            textBox_R.Text = argb.R.ToString();
            textBox_G.Text = argb.G.ToString();
            textBox_B.Text = argb.B.ToString();
            textBox_A.Text = argb.A.ToString();
            ChgColor();
        }

        public void GetColor(ref Color argb)
        {
            argb.R = (byte)slider_R.Value;
            argb.G = (byte)slider_G.Value;
            argb.B = (byte)slider_B.Value;
            argb.A = (byte)slider_A.Value;
        }

        private void ChgColor()
        {
            if (rectangle_color.Fill != null)
            {
                ((SolidColorBrush)rectangle_color.Fill).Color = Color.FromArgb((byte)slider_A.Value, (byte)slider_R.Value, (byte)slider_G.Value, (byte)slider_B.Value);
            }
        }

        private void slider_R_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            textBox_R.Text = ((byte)slider_R.Value).ToString();
            ChgColor();
        }

        private void slider_G_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            textBox_G.Text = ((byte)slider_G.Value).ToString();
            ChgColor();
        }

        private void slider_B_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            textBox_B.Text = ((byte)slider_B.Value).ToString();
            ChgColor();
        }

        private void slider_A_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            textBox_A.Text = ((byte)slider_A.Value).ToString();
            ChgColor();
        }

        private void textBox_R_TextChanged(object sender, TextChangedEventArgs e)
        {
            byte val;
            slider_R.Value = byte.TryParse(textBox_R.Text, out val) ? val : 255;
            ChgColor();
        }

        private void textBox_G_TextChanged(object sender, TextChangedEventArgs e)
        {
            byte val;
            slider_G.Value = byte.TryParse(textBox_G.Text, out val) ? val : 255;
            ChgColor();
        }

        private void textBox_B_TextChanged(object sender, TextChangedEventArgs e)
        {
            byte val;
            slider_B.Value = byte.TryParse(textBox_B.Text, out val) ? val : 255;
            ChgColor();
        }

        private void textBox_A_TextChanged(object sender, TextChangedEventArgs e)
        {
            byte val;
            slider_A.Value = byte.TryParse(textBox_A.Text, out val) ? val : 255;
            ChgColor();
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

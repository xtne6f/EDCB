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
        SolidColorBrush backColor = new SolidColorBrush();

        public ColorSetWindow()
        {
            InitializeComponent();

            slider_R.Value = 0xFF;
            slider_G.Value = 0xFF;
            slider_B.Value = 0xFF;
            textBox_R.Text = "255";
            textBox_G.Text = "255";
            textBox_B.Text = "255";

            backColor.Color = Color.FromArgb(0xFF, 0xFF, 0xFF, 0xFF);

            rectangle_color.Fill = backColor;
        }

        public void SetColor(Color argb)
        {
            backColor.Color = argb;

            slider_R.Value = argb.R;
            slider_G.Value = argb.G;
            slider_B.Value = argb.B;
            textBox_R.Text = argb.R.ToString();
            textBox_G.Text = argb.G.ToString();
            textBox_B.Text = argb.B.ToString();
        }

        public void GetColor(ref Color argb)
        {
            argb.R = (byte)slider_R.Value;
            argb.G = (byte)slider_G.Value;
            argb.B = (byte)slider_B.Value;
        }

        private void ChgColor()
        {
            backColor.Color = Color.FromArgb(0xFF, (byte)slider_R.Value, (byte)slider_G.Value, (byte)slider_B.Value);
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

        private void textBox_R_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                if (textBox_R.Text.Length == 0)
                {
                    textBox_R.Text = "0";
                }
                else
                {
                    if (Convert.ToInt32(textBox_R.Text) > 255)
                    {
                        textBox_R.Text = "255";
                    }
                } 
                slider_R.Value = Convert.ToByte(textBox_R.Text);
                ChgColor();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void textBox_G_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                if (textBox_G.Text.Length == 0)
                {
                    textBox_G.Text = "0";
                }
                else
                {
                    if (Convert.ToInt32(textBox_G.Text) > 255)
                    {
                        textBox_G.Text = "255";
                    }
                } 
                slider_G.Value = Convert.ToByte(textBox_G.Text);
                ChgColor();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void textBox_B_TextChanged(object sender, TextChangedEventArgs e)
        {
            try
            {
                if (textBox_B.Text.Length == 0)
                {
                    textBox_B.Text = "0";
                }
                else
                {
                    if (Convert.ToInt32(textBox_B.Text) > 255)
                    {
                        textBox_B.Text = "255";
                    }
                }
                slider_B.Value = Convert.ToByte(textBox_B.Text);
                ChgColor();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
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

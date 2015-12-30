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
    /// KeyWordWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class KeyWordWindow : Window
    {
        public String KeyWord = "";

        public KeyWordWindow()
        {
            InitializeComponent();
        }
        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            KeyWord = KeyWord.Replace(" ", "\r\n");
            KeyWord = KeyWord.Replace("　", "\r\n");
            textBox.Text = KeyWord;
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            KeyWord = textBox.Text;
            KeyWord = KeyWord.Replace(" ", "");
            KeyWord = KeyWord.Replace("　", "");
            KeyWord = KeyWord.Replace("\r\n", " ");
        }
    }
}

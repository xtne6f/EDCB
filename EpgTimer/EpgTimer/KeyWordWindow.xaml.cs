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

            if (Settings.Instance.NoStyle == 0)
            {
                ResourceDictionary rd = new ResourceDictionary();
                rd.MergedDictionaries.Add(
                    Application.LoadComponent(new Uri("/PresentationFramework.Aero, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aero.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                    //Application.LoadComponent(new Uri("/PresentationFramework.Classic, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35, ProcessorArchitecture=MSIL;component/themes/Classic.xaml", UriKind.Relative)) as ResourceDictionary
                    );
                this.Resources = rd;
            }
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

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Escape:
                        this.Close();
                        break;
                }
            }
            base.OnKeyDown(e);
        }

    }
}

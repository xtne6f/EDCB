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
    /// AddPresetWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class AddPresetWindow : Window
    {
        private bool chgModeFlag = false;
        public AddPresetWindow()
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
            else
            {
                button_add.Style = null;
                button_cancel.Style = null;
            }
        }

        public void SetMode(bool chgMode)
        {
            if (chgMode == true)
            {
                button_add.Content = "変更";
                label_chgMsg.Visibility = System.Windows.Visibility.Visible;
            }
            else
            {
                button_add.Content = "追加";
            }
            chgModeFlag = chgMode;
        }

        public void SetName(String name)
        {
            textBox_name.Text = name;
        }

        public void GetName(ref String name)
        {
            name = textBox_name.Text;
        }

        private void button_add_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Escape:
                        this.button_cancel.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                }
            }
            base.OnKeyDown(e);
        }

    }
}

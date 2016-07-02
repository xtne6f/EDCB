using System;
using System.Windows;

namespace EpgTimer
{
    /// <summary>
    /// AddPresetWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class AddPresetWindow : Window
    {
        public AddPresetWindow()
        {
            InitializeComponent();
        }

        public void SetMode(bool chgMode)
        {
            button_add.Content = chgMode == true ? "変更" : "追加";
            label_chgMsg.Visibility = chgMode == true ? Visibility.Visible : Visibility.Hidden;
        }

        public void SetName(String name)
        {
            textBox_name.Text = name;
        }

        public String GetName()
        {
            return textBox_name.Text;
        }

        private void button_add_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
    }
}

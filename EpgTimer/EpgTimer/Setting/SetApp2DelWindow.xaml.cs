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
    using BoxExchangeEdit;

    /// <summary>
    /// SetApp2DelWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SetApp2DelWindow : Window
    {
        public List<String> extList = new List<string>();
        public List<String> delChkFolderList = new List<string>();
        public SetApp2DelWindow()
        {
            InitializeComponent();

            if (CommonManager.Instance.NWMode == true)
            {
                button_add.IsEnabled = false;
                textBox_ext.IsEnabled = false;
                label2.IsEnabled = false;
                button_del.IsEnabled = false;
                button_chk_del.IsEnabled = false;
                button_chk_add.IsEnabled = false;
                button_chk_open.IsEnabled = false;
                textBox_chk_folder.IsEnabled = false;
                button_OK.IsEnabled = false;
            }

            var bxe = new BoxExchangeEditor(null, listBox_ext, true);
            var bxc = new BoxExchangeEditor(null, listBox_chk_folder, true);
            if (CommonManager.Instance.NWMode == false)
            {
                bxe.AllowKeyAction();
                bxe.AllowDragDrop();
                button_del.Click += new RoutedEventHandler(bxe.button_Delete_Click);
                bxc.AllowKeyAction();
                bxc.AllowDragDrop();
                button_chk_del.Click += new RoutedEventHandler(bxc.button_Delete_Click);
            }
        }

        private void listBox_ext_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (listBox_ext.SelectedItem is string)
            {
                textBox_ext.Text = listBox_ext.SelectedItem as string;
            }
        }

        private void listBox_chk_folder_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (listBox_chk_folder.SelectedItem is string)
            {
                textBox_chk_folder.Text = listBox_chk_folder.SelectedItem as string;
            }
        }

        private void button_add_Click(object sender, RoutedEventArgs e)
        {
            if (String.IsNullOrEmpty(textBox_ext.Text) == false)
            {
                foreach (String info in listBox_ext.Items)
                {
                    if (String.Compare(textBox_ext.Text, info, true) == 0)
                    {
                        MessageBox.Show("すでに追加されています");
                        return;
                    }
                }
                listBox_ext.Items.Add(textBox_ext.Text);
            }
        }

        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;

            extList = listBox_ext.Items.OfType<string>().ToList();
            delChkFolderList = listBox_chk_folder.Items.OfType<string>().ToList();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            listBox_ext.Items.AddItems(extList);
            listBox_chk_folder.Items.AddItems(delChkFolderList);
        }

        private void button_chk_open_Click(object sender, RoutedEventArgs e)
        {
            CommonManager.GetFolderNameByDialog(textBox_chk_folder, "自動削除対象フォルダの選択");
        }

        private void button_chk_add_Click(object sender, RoutedEventArgs e)
        {
            if (String.IsNullOrEmpty(textBox_chk_folder.Text) == false)
            {
                foreach (String info in listBox_chk_folder.Items)
                {
                    if (String.Compare(textBox_chk_folder.Text, info, true) == 0)
                    {
                        MessageBox.Show("すでに追加されています");
                        return;
                    }
                }
                listBox_chk_folder.Items.Add(textBox_chk_folder.Text);
            }
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

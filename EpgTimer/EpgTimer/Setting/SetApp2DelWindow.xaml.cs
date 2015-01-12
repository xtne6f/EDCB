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
    /// SetApp2DelWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SetApp2DelWindow : Window
    {
        public List<String> extList = new List<string>();
        public List<String> delChkFolderList = new List<string>();
        public SetApp2DelWindow()
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
                button_del.Style = null;
                button_add.Style = null;
                button_chk_del.Style = null;
                button_chk_add.Style = null;
                button_chk_open.Style = null;

            }
        }

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_ext.SelectedItem != null)
            {
                listBox_ext.Items.RemoveAt(listBox_ext.SelectedIndex);
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

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            extList.Clear();
            foreach (string info in listBox_ext.Items)
            {
                extList.Add(info);
            }
            delChkFolderList.Clear();
            foreach (string info in listBox_chk_folder.Items)
            {
                delChkFolderList.Add(info);
            }

        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            foreach (string info in extList)
            {
                listBox_ext.Items.Add(info);
            }
            foreach (string info in delChkFolderList)
            {
                listBox_chk_folder.Items.Add(info);
            }
        }

        private void button_chk_del_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_chk_folder.SelectedItem != null)
            {
                listBox_chk_folder.Items.RemoveAt(listBox_chk_folder.SelectedIndex);
            }
        }

        private void button_chk_open_Click(object sender, RoutedEventArgs e)
        {
            System.Windows.Forms.FolderBrowserDialog dlg = new System.Windows.Forms.FolderBrowserDialog();
            dlg.Description = "自動削除対象フォルダ";
            if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                textBox_chk_folder.Text = dlg.SelectedPath;
            }
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

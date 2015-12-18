using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.IO;
using System.Windows.Interop;

namespace EpgTimer
{
    /// <summary>
    /// RecFolderWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class RecFolderWindow : Window
    {
        private RecFileSetInfo defSet = new RecFileSetInfo();

        public RecFolderWindow()
        {
            InitializeComponent();

            if (CommonManager.Instance.IsConnected == true)
            {
                CommonManager.Instance.DB.ReloadPlugInFile();
            }
            comboBox_writePlugIn.ItemsSource = CommonManager.Instance.DB.WritePlugInList.Values;
            comboBox_writePlugIn.SelectedItem = "Write_Default.dll";

            var list = CommonManager.Instance.DB.RecNamePlugInList.Values.ToList();
            list.Insert(0, "なし");
            comboBox_recNamePlugIn.ItemsSource = list;
            comboBox_recNamePlugIn.SelectedItem = "なし";

            if (CommonManager.Instance.NWMode == true)
            {
                button_folder.IsEnabled = false;
                button_write.IsEnabled = false;
                button_recName.IsEnabled = false;
            }
        }

        public void SetDefSetting(RecFileSetInfo info)
        {
            button_ok.Content = "変更";
            textBox_recFolder.Text = String.Compare(info.RecFolder, "!Default", true) == 0 ? "" : info.RecFolder;
            foreach (string text in comboBox_writePlugIn.Items)
            {
                if (String.Compare(text, info.WritePlugIn, true) == 0)
                {
                    comboBox_writePlugIn.SelectedItem = text;
                    break;
                }
            }
            foreach (string text in comboBox_recNamePlugIn.Items)
            {
                if (String.Compare(text, info.RecNamePlugIn.Substring(0, (info.RecNamePlugIn + '?').IndexOf('?')), true) == 0)
                {
                    comboBox_recNamePlugIn.SelectedItem = text;
                    textBox_recNameOption.Text = info.RecNamePlugIn.IndexOf('?') < 0 ? "" : info.RecNamePlugIn.Substring(info.RecNamePlugIn.IndexOf('?') + 1);
                    break;
                }
            }
        }

        public void GetSetting(ref RecFileSetInfo info)
        {
            info.RecFolder = defSet.RecFolder;
            info.WritePlugIn = defSet.WritePlugIn;
            info.RecNamePlugIn = defSet.RecNamePlugIn;
        }

        private void button_folder_Click(object sender, RoutedEventArgs e)
        {
            string path = CommonManager.Instance.GetFolderNameByDialog(textBox_recFolder.Text, "録画フォルダの選択");
            if (path != null)
            {
                textBox_recFolder.Text = path;
            }
        }

        private void button_write_Click(object sender, RoutedEventArgs e)
        {
            if (comboBox_writePlugIn.SelectedItem != null)
            {
                string name = comboBox_writePlugIn.SelectedItem as string;
                string filePath = SettingPath.ModulePath + "\\Write\\" + name;

                WritePlugInClass plugin = new WritePlugInClass();
                HwndSource hwnd = (HwndSource)HwndSource.FromVisual(this);

                plugin.Setting(filePath, hwnd.Handle);
            }
        }

        private void button_recName_Click(object sender, RoutedEventArgs e)
        {
            if (comboBox_recNamePlugIn.SelectedItem != null)
            {
                string name = comboBox_recNamePlugIn.SelectedItem as string;
                if (String.Compare(name, "なし", true) != 0)
                {
                    string filePath = SettingPath.ModulePath + "\\RecName\\" + name;

                    RecNamePluginClass plugin = new RecNamePluginClass();
                    HwndSource hwnd = (HwndSource)HwndSource.FromVisual(this);

                    plugin.Setting(filePath, hwnd.Handle);
                }
            }
        }

        private void button_ok_Click(object sender, RoutedEventArgs e)
        {
            defSet.RecFolder = textBox_recFolder.Text == "" ? "!Default" : textBox_recFolder.Text;
            defSet.WritePlugIn = (String)comboBox_writePlugIn.SelectedItem;
            defSet.RecNamePlugIn = (String)comboBox_recNamePlugIn.SelectedItem;
            if (String.Compare(defSet.RecNamePlugIn, "なし", true) == 0)
            {
                defSet.RecNamePlugIn = "";
            }
            else if (textBox_recNameOption.Text.Length != 0)
            {
                defSet.RecNamePlugIn += '?' + textBox_recNameOption.Text;
            }
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

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
using System.IO;
using System.Windows.Interop;

namespace EpgTimer
{
    /// <summary>
    /// RecFolderWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class RecFolderWindow : Window
    {
        public RecFolderWindow()
        {
            InitializeComponent();

            string plugInFile = "Write_Default.dll";
            string recNamePlugInFile = "";

            var writeList = new List<string>();
            ErrCode err = CommonManager.CreateSrvCtrl().SendEnumPlugIn(2, ref writeList);
            if (err != ErrCode.CMD_SUCCESS)
            {
                MessageBox.Show(CommonManager.GetErrCodeText(err) ?? "PlugIn一覧の取得でエラーが発生しました。");
            }
            //こちらは空(CMD_ERR)でもよい
            var recNameList = new List<string>();
            CommonManager.CreateSrvCtrl().SendEnumPlugIn(1, ref recNameList);

            int select = 0;
            foreach (string info in writeList)
            {
                int index = comboBox_writePlugIn.Items.Add(info);
                if (info.Equals(plugInFile, StringComparison.OrdinalIgnoreCase))
                {
                    select = index;
                }
            }
            if (comboBox_writePlugIn.Items.Count != 0)
            {
                comboBox_writePlugIn.SelectedIndex = select;
            }

            select = 0;
            comboBox_recNamePlugIn.Items.Add("なし");
            foreach (string info in recNameList)
            {
                int index = comboBox_recNamePlugIn.Items.Add(info);
                if (info.Equals(recNamePlugInFile, StringComparison.OrdinalIgnoreCase))
                {
                    select = index;
                }
            }
            if (comboBox_recNamePlugIn.Items.Count != 0)
            {
                comboBox_recNamePlugIn.SelectedIndex = select;
            }

            if (CommonManager.Instance.NWMode == true)
            {
                button_folder.IsEnabled = false;
                button_write.IsEnabled = false;
                button_recName.IsEnabled = false;
            }
        }

        public void SetDefSetting(RecFileSetInfo info)
        {
            textBox_recFolder.Text = info.RecFolder.Equals("!Default", StringComparison.OrdinalIgnoreCase) ? "" : info.RecFolder;
            foreach (string text in comboBox_writePlugIn.Items)
            {
                if (text.Equals(info.WritePlugIn, StringComparison.OrdinalIgnoreCase))
                {
                    comboBox_writePlugIn.SelectedItem = text;
                    break;
                }
            }
            foreach (string text in comboBox_recNamePlugIn.Items)
            {
                if (text.Equals(info.RecNamePlugIn.Substring(0, (info.RecNamePlugIn + '?').IndexOf('?')), StringComparison.OrdinalIgnoreCase))
                {
                    comboBox_recNamePlugIn.SelectedItem = text;
                    textBox_recNameOption.Text = info.RecNamePlugIn.IndexOf('?') < 0 ? "" : info.RecNamePlugIn.Substring(info.RecNamePlugIn.IndexOf('?') + 1);
                    break;
                }
            }
        }

        public RecFileSetInfo GetSetting()
        {
            var info = new RecFileSetInfo();
            info.RecFolder = textBox_recFolder.Text == "" ? "!Default" : textBox_recFolder.Text;
            info.WritePlugIn = (string)comboBox_writePlugIn.SelectedItem;
            info.RecNamePlugIn = (string)comboBox_recNamePlugIn.SelectedItem;
            if (info.RecNamePlugIn == "なし")
            {
                info.RecNamePlugIn = "";
            }
            else if (textBox_recNameOption.Text != "")
            {
                info.RecNamePlugIn += '?' + textBox_recNameOption.Text;
            }
            return info;
        }

        private void button_folder_Click(object sender, RoutedEventArgs e)
        {
            string path = CommonUtil.BrowseFolder(((HwndSource)PresentationSource.FromVisual(this)).Handle, "フォルダを選択してください");
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
                string filePath = System.IO.Path.Combine(SettingPath.ModulePath, "Write\\" + name);
                CommonUtil.ShowPlugInSetting(filePath, ((HwndSource)PresentationSource.FromVisual(this)).Handle);
            }
        }

        private void button_recName_Click(object sender, RoutedEventArgs e)
        {
            if (comboBox_recNamePlugIn.SelectedItem != null)
            {
                string name = comboBox_recNamePlugIn.SelectedItem as string;
                if (name != "なし")
                {
                    string filePath = System.IO.Path.Combine(SettingPath.ModulePath, "RecName\\" + name);
                    CommonUtil.ShowPlugInSetting(filePath, ((HwndSource)PresentationSource.FromVisual(this)).Handle);
                }
            }
        }

        private void button_ok_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;
        }
    }
}

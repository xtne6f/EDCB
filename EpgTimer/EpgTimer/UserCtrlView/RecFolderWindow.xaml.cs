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

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

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

            if (Settings.Instance.NoStyle == 0)
            {
                ResourceDictionary rd = new ResourceDictionary();
                rd.MergedDictionaries.Add(
                    Application.LoadComponent(new Uri("/PresentationFramework.Aero, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aero.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                    //Application.LoadComponent(new Uri("/PresentationFramework.Classic, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35, ProcessorArchitecture=MSIL;component/themes/Classic.xaml", UriKind.Relative)) as ResourceDictionary
                    );
                this.Resources = rd;
            }


            String plugInFile = "Write_Default.dll";
            String recNamePlugInFile = "";

            ErrCode err = CommonManager.Instance.DB.ReloadPlugInFile();
            if (err == ErrCode.CMD_ERR_CONNECT)
            {
                MessageBox.Show("サーバー または EpgTimerSrv に接続できませんでした。");
            }
            if (err == ErrCode.CMD_ERR_TIMEOUT)
            {
                MessageBox.Show("EpgTimerSrvとの接続にタイムアウトしました。");
            }
            if (err != ErrCode.CMD_SUCCESS)
            {
                MessageBox.Show("PlugIn一覧の取得でエラーが発生しました。");
            }

            int select = 0;
            foreach (string info in CommonManager.Instance.DB.WritePlugInList.Values)
            {
                int index = comboBox_writePlugIn.Items.Add(info);
                if (String.Compare(info, plugInFile, true) == 0)
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
            foreach (string info in CommonManager.Instance.DB.RecNamePlugInList.Values)
            {
                int index = comboBox_recNamePlugIn.Items.Add(info);
                if (String.Compare(info, recNamePlugInFile, true) == 0)
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
            textBox_recFolder.Text = info.RecFolder;
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
            System.Windows.Forms.FolderBrowserDialog dlg = new System.Windows.Forms.FolderBrowserDialog();
            dlg.Description = "フォルダ選択";
            if (dlg.ShowDialog() == System.Windows.Forms.DialogResult.OK)
            {
                textBox_recFolder.Text = dlg.SelectedPath;
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
            if (textBox_recFolder.Text.Length == 0)
            {
                MessageBox.Show("録画フォルダが指定されていません。");
                return;
            }
            defSet.RecFolder = textBox_recFolder.Text;
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
    }
}

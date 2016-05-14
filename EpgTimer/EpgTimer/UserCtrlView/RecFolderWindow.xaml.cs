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
                button_write.IsEnabled = false;
                button_recName.IsEnabled = false;
            }
        }

        public void SetPartialMode(bool partialRec)
        {
            this.Title = "録画フォルダ、使用PlugIn設定" + (partialRec == true ? " (部分受信)" : "");
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
            string path = textBox_recFolder.Text.Trim();

            string base_src = "";
            string base_nw = "";
            if (CommonManager.Instance.NWMode == true && path != "" && path.StartsWith("\\\\") == false)
            {
                //可能ならUNCパスをサーバ側のパスに戻す。
                //複数の共有フォルダ使ってる場合はとりあえず諦める。(サーバ側で要逆変換)
                string path_src = path.TrimEnd('\\');
                string path_nw = CommonManager.Instance.GetRecPath(path_src).TrimEnd('\\');

                if (path_nw != "" && path_nw != path_src)
                {
                    IEnumerable<string> r_src = path_src.Split('\\').Reverse();
                    IEnumerable<string> r = path_nw.Split('\\').Reverse();
                    int length_match = -1;
                    foreach (var item in r.Zip(r_src, (p, ps) => new { nw = p, src = ps }))
                    {
                        if (item.nw != item.src) break;
                        length_match += item.nw.Length + 1;
                    }
                    length_match = Math.Max(0, length_match);
                    base_src = path_src.Substring(0, path_src.Length - length_match).TrimEnd('\\');
                    base_nw = path_nw.Substring(0, path_nw.Length - length_match).TrimEnd('\\');
                }
                if (base_nw != "")
                {
                    path = path_nw;
                }
            }

            path = CommonManager.GetFolderNameByDialog(path, "録画フォルダの選択");
            if (path != null)
            {
                //他のドライブに変ったりしたときは何もしない
                if (base_nw != "" && path.StartsWith(base_nw) == true)
                {
                    path = path.Replace(base_nw, base_src);
                    if (path.EndsWith(":") == true) path += "\\";//EpgTimerSrvに削除されてしまうが‥
                }
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

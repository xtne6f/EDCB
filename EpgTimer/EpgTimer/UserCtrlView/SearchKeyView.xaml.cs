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
using System.Windows.Navigation;
using System.Windows.Shapes;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// SearchKey.xaml の相互作用ロジック
    /// </summary>
    public partial class SearchKey : UserControl
    {
        public SearchKey()
        {
            InitializeComponent();

            try
            {
                foreach (String info in Settings.Instance.AndKeyList)
                {
                    ComboBox_andKey.Items.Add(info);
                }
                foreach (String info in Settings.Instance.NotKeyList)
                {
                    ComboBox_notKey.Items.Add(info);
                }
            }
            catch
            {
            }
        }

        private void button_andIn_Click(object sender, RoutedEventArgs e)
        {
            KeyWordWindow dlg = new KeyWordWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            dlg.KeyWord = ComboBox_andKey.Text;
            dlg.ShowDialog();
            ComboBox_andKey.Text = dlg.KeyWord;
        }

        private void button_notIn_Click(object sender, RoutedEventArgs e)
        {
            KeyWordWindow dlg = new KeyWordWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            dlg.KeyWord = ComboBox_notKey.Text;
            dlg.ShowDialog();
            ComboBox_notKey.Text = dlg.KeyWord;
        }

        public void AddSearchLog()
        {
            AddSerchLog(ComboBox_andKey, Settings.Instance.AndKeyList, 30);
            AddSerchLog(ComboBox_notKey, Settings.Instance.NotKeyList, 30);
        }

        private void AddSerchLog(ComboBox box, List<string> log, byte MaxLog)
        {
            try
            {
                string searchWord = box.Text;
                if (searchWord == null || searchWord == "") return;

                box.Items.Remove(searchWord);
                box.Items.Insert(0, searchWord);
                box.Text = searchWord;

                log.Remove(searchWord);
                log.Insert(0, searchWord);
                if (log.Count > MaxLog)
                {
                    log.RemoveAt(log.Count - 1);
                }
            }
            catch { }
        }

        public void GetSearchKey(ref EpgSearchKeyInfo key)
        {
            key.andKey = ComboBox_andKey.Text;
            key.notKey = ComboBox_notKey.Text;
            searchKeyDescView.GetSearchKey(ref key);
        }

        public void SetSearchKey(EpgSearchKeyInfo key)
        {
            ComboBox_andKey.Text = key.andKey;
            ComboBox_notKey.Text = key.notKey;
            searchKeyDescView.SetSearchKey(key);
        }
    }
}

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
        
        public void SaveSearchLog()
        {
            try
            {
                bool find = false;
                if (ComboBox_andKey.Text.Length > 0)
                {
                    foreach (String info in Settings.Instance.AndKeyList)
                    {
                        if (String.Compare(ComboBox_andKey.Text, info, true) == 0)
                        {
                            find = true;
                            break;
                        }
                    }
                    if (find == false)
                    {
                        Settings.Instance.AndKeyList.Add(ComboBox_andKey.Text);
                        ComboBox_andKey.Items.Add(ComboBox_andKey.Text);
                        if (Settings.Instance.AndKeyList.Count > 30)
                        {
                            Settings.Instance.AndKeyList.RemoveAt(0);
                        }
                    }
                }

                find = false;
                if (ComboBox_notKey.Text.Length > 0)
                {
                    foreach (String info in Settings.Instance.NotKeyList)
                    {
                        if (String.Compare(ComboBox_notKey.Text, info, true) == 0)
                        {
                            find = true;
                            break;
                        }
                    }
                    if (find == false)
                    {
                        Settings.Instance.NotKeyList.Add(ComboBox_notKey.Text);
                        ComboBox_notKey.Items.Add(ComboBox_notKey.Text);
                        if (Settings.Instance.NotKeyList.Count > 30)
                        {
                            Settings.Instance.NotKeyList.RemoveAt(0);
                        }
                    }
                }
                Settings.SaveToXmlFile();

            }
            catch
            {
            }
        }

        public void GetSearchKey(ref EpgSearchKeyInfo key)
        {
            key.andKey = ComboBox_andKey.Text;
            key.notKey = ComboBox_notKey.Text;
            searchKeyDescView.GetSearchKey(ref key);
        }

        public void SetSearchKey(EpgSearchKeyInfo key)
        {
            searchKeyDescView.SetSearchKey(key);
            ComboBox_andKey.Text = System.Text.RegularExpressions.Regex.Replace(
                key.andKey, @"^(?:\^!\{999\})?(?:C!\{999\})?(?:D!\{1[0-9]{8}\})?", "");
            ComboBox_notKey.Text = key.notKey;
        }
    }
}

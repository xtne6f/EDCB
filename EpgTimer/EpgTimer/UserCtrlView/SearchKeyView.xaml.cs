using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

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
                Settings.Instance.AndKeyList.ForEach(s => ComboBox_andKey.Items.Add(s));
                Settings.Instance.NotKeyList.ForEach(s => ComboBox_notKey.Items.Add(s));
            }
            catch { }
        }

        protected virtual void ComboBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            var tb = e.OriginalSource as TextBox;
            if (tb != null)
            {
                ((ComboBox)sender).Text = tb.Text;
            }
        }

        public void AddSearchLog()
        {
            if (Settings.Instance.SaveSearchKeyword == false) return;

            AddSerchLog(ComboBox_andKey, Settings.Instance.AndKeyList, 30);
            AddSerchLog(ComboBox_notKey, Settings.Instance.NotKeyList, 30);
        }

        private void AddSerchLog(ComboBox box, List<string> log, byte MaxLog)
        {
            try
            {
                string searchWord = box.Text;
                if (string.IsNullOrEmpty(searchWord) == true) return;

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

        public EpgSearchKeyInfo GetSearchKey()
        {
            var key = new EpgSearchKeyInfo();
            key.andKey = ComboBox_andKey.Text;
            key.notKey = ComboBox_notKey.Text;
            searchKeyDescView.GetSearchKey(ref key);
            return key;
        }

        public void SetSearchKey(EpgSearchKeyInfo key)
        {
            ComboBox_andKey.Text = key.andKey;
            ComboBox_notKey.Text = key.notKey;
            searchKeyDescView.SetSearchKey(key);
        }
    }
}

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
    /// RecInfoDescWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class RecInfoDescWindow : Window
    {
        private RecFileInfo recInfo = null;

        public RecInfoDescWindow()
        {
            InitializeComponent();
        }

        public void SetRecInfo(RecFileInfo info)
        {
            recInfo = info;
            textBox_pgInfo.Text = info.ProgramInfo;
            textBox_errLog.Text = info.ErrInfo;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            tabItem_pgInfo.Focus();
        }

        private void button_play_Click(object sender, RoutedEventArgs e)
        {
            if (recInfo != null)
            {
                if (recInfo.RecFilePath.Length > 0)
                {
                    CommonManager.Instance.FilePlay(recInfo.RecFilePath);
                }
            }
        }

        private void button_del_Click(object sender, RoutedEventArgs e)
        {
            if (recInfo != null)
            {
                if (Settings.Instance.ConfirmDelRecInfo)
                {
                    if ((recInfo.RecFilePath.Length > 0 || Settings.Instance.ConfirmDelRecInfoAlways) &&
                        MessageBox.Show("削除してよろしいですか?" +
                                        (recInfo.RecFilePath.Length > 0 ? "\r\n\r\n「録画ファイルも削除する」設定が有効な場合、ファイルも削除されます。" : ""), "確認",
                                        MessageBoxButton.OKCancel, MessageBoxImage.Question, MessageBoxResult.OK) != MessageBoxResult.OK)
                    {
                        return;
                    }
                }
                try
                {
                    CommonManager.CreateSrvCtrl().SendDelRecInfo(new List<uint>() { recInfo.ID });
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.ToString());
                }
            }
            DialogResult = false;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
    }
}

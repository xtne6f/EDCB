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

        private void button_play_Click(object sender, RoutedEventArgs e)
        {
            if (recInfo != null)
            {
                CommonManager.Instance.MUtil.FilePlay(recInfo.RecFilePath);
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

        private void Window_Closed(object sender, EventArgs e)
        {
            MainWindow mainWindow = (MainWindow)Application.Current.MainWindow;
            mainWindow.ListFoucsOnVisibleChanged();
        }
    }
}

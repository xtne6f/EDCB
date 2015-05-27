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
    /// SettingWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SettingWindow : Window
    {
        public bool ServiceStop = false;

        public SettingWindow()
        {
            InitializeComponent();
        }

        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (setAppView.ServiceStop == true)
                {
                    ServiceStop = true;
                }
                setBasicView.SaveSetting();
                setAppView.SaveSetting();
                setEpgView.SaveSetting();
                setOtherAppView.SaveSetting();

                Settings.SaveToXmlFile();
                ChSet5.SaveFile();
                CommonManager.Instance.ReloadCustContentColorList();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                MessageBox.Show("不正な入力値によるエラーのため、一部設定のみ更新されました。");
            }

            this.DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            this.DialogResult = false;
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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    /// <summary>
    /// EpgDataViewSettingWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgDataViewSettingWindow : Window
    {
        public EpgDataViewSettingWindow()
        {
            InitializeComponent();
            checkBox_tryEpgSetting.Visibility = Visibility.Hidden;
        }

        public void SetTrySetModeEnable()
        {
            checkBox_tryEpgSetting.Visibility = Visibility.Visible;
            checkBox_tryEpgSetting.IsChecked = (Settings.Instance.TryEpgSetting == true);
        }

        public void SetTrySetModeOnly()
        {
            checkBox_tryEpgSetting.IsEnabled  = false;
            checkBox_tryEpgSetting.IsChecked = true;
            checkBox_tryEpgSetting.ToolTip = "デフォルト表示では一時的な変更のみ可能で設定は保存されません。";
        }

        /// <summary>
        /// デフォルト表示の設定値
        /// </summary>
        /// <param name="setInfo"></param>
        public void SetDefSetting(CustomEpgTabInfo setInfo)
        {
            epgDataViewSetting.SetSetting(setInfo);
        }

        /// <summary>
        /// 設定値の取得
        /// </summary>
        /// <param name="setInfo"></param>
        public void GetSetting(ref CustomEpgTabInfo info)
        {
            epgDataViewSetting.GetSetting(ref info);
        }

        private void checkBox_tryEpgSetting_Click(object sender, RoutedEventArgs e)
        {
            //これはダイアログの設定なので即座に反映
            Settings.Instance.TryEpgSetting = (checkBox_tryEpgSetting.IsChecked == true);
        }

        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
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

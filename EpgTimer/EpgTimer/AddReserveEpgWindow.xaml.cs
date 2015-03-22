using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// AddReserveEpgWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class AddReserveEpgWindow : Window
    {
        private EpgEventInfo eventInfo = null;
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;

        public AddReserveEpgWindow()
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
            else
            {
                button_add_reserve.Style = null;
                button_cancel.Style = null;
            }
        }

        public void SetOpenMode(byte mode)
        {
            tabControl.SelectedIndex = mode;
        }

        public void SetEventInfo(EpgEventInfo eventData)
        {
            eventInfo = eventData;
            textBox_info.Text = CommonManager.Instance.ConvertProgramText(eventData, EventInfoTextMode.BasicOnly);
            richTextBox_descInfo.Document = CommonManager.Instance.ConvertDisplayText(eventInfo);
        }

        private void button_add_reserve_Click(object sender, RoutedEventArgs e)
        {
            if (mutil.IsEnableReserveAdd(eventInfo) == false) return;
            mutil.ReserveAdd(eventInfo, recSettingView);
            DialogResult = true;
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            base.OnKeyDown(e);
            //
            if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control) && Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
            {
                switch (e.Key)
                {
                    case Key.A:
                        if (MessageBox.Show("予約を追加します。\r\nよろしいですか？", "追加の確認", MessageBoxButton.OKCancel) == MessageBoxResult.OK)
                        {
                            this.button_add_reserve.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        }
                        break;
                }
            }
            else if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Escape:
                        this.button_cancel.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                }
            }
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            MainWindow mainWindow = Application.Current.MainWindow as MainWindow;
            mainWindow.ListFoucsOnVisibleChanged();
        }
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    /// <summary>
    /// AddReserveEpgWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class AddReserveEpgWindow : Window
    {
        private EpgEventInfo eventInfo = null;

        public AddReserveEpgWindow()
        {
            InitializeComponent();

            //コマンドの登録
            this.CommandBindings.Add(new CommandBinding(EpgCmds.Cancel, (sender, e) => DialogResult = false));
            this.CommandBindings.Add(new CommandBinding(EpgCmds.AddInDialog, button_add_reserve_Click));

            //ボタンの設定
            var mBinds = new MenuBinds();
            mBinds.SetCommandToButton(button_cancel, EpgCmds.Cancel);
            mBinds.SetCommandToButton(button_add_reserve, EpgCmds.AddInDialog);

            //ショートカットの登録
            mBinds.ResetInputBindings(this);
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

        private void button_add_reserve_Click(object sender, ExecutedRoutedEventArgs e)
        {
            if (CmdExeUtil.IsDisplayKgMessage(e) == true)
            {
                if (MessageBox.Show("予約を追加します。\r\nよろしいですか？", "追加の確認", MessageBoxButton.OKCancel) != MessageBoxResult.OK)
                { return; }
            }
            bool ret = MenuUtil.ReserveAdd(CommonUtil.ToList(eventInfo), recSettingView);
            StatusManager.StatusNotifySet(ret, "録画予約を追加");

            if (ret == false) return;
            DialogResult = true;
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            ViewUtil.MainWindow.ListFoucsOnVisibleChanged();
        }
    }
}

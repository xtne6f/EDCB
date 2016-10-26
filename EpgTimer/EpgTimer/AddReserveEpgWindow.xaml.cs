using System.Windows;
using System.Windows.Input;

namespace EpgTimer
{
    /// <summary>
    /// AddReserveEpgWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class AddReserveEpgWindow : AddReserveEpgWindowBase
    {
        private EpgEventInfo eventInfo = null;

        public AddReserveEpgWindow()
        {
            InitializeComponent();

            base.SetParam(true, checkBox_windowPinned, checkBox_dataReplace);

            //コマンドの登録
            this.CommandBindings.Add(new CommandBinding(EpgCmds.Cancel, (sender, e) => this.Close()));
            this.CommandBindings.Add(new CommandBinding(EpgCmds.AddInDialog, button_add_reserve_Click));

            //ボタンの設定
            var mBinds = new MenuBinds();
            mBinds.SetCommandToButton(button_cancel, EpgCmds.Cancel);
            mBinds.SetCommandToButton(button_add_reserve, EpgCmds.AddInDialog);

            //ショートカットの登録
            mBinds.ResetInputBindings(this);
        }

        public void SetEventInfo(EpgEventInfo info, int? epgInfoOpenMode = null)
        {
            if (info == null) return;
            if (epgInfoOpenMode != null) tabControl.SelectedIndex = epgInfoOpenMode == 1 ? 1 : 0;
            eventInfo = info;
            Title = ViewUtil.WindowTitleText(info.DataTitle, "予約登録");
            textBox_info.Text = CommonManager.ConvertProgramText(info, EventInfoTextMode.BasicOnly);
            richTextBox_descInfo.Document = CommonManager.ConvertDisplayText(eventInfo);
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
            this.Close();
        }
    }
    public class AddReserveEpgWindowBase : ReserveWindowBase<AddReserveEpgWindow> { }
}

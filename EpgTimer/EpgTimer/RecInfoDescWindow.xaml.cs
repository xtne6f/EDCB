using System;
using System.Windows;
using System.Windows.Input;

namespace EpgTimer
{
    /// <summary>
    /// RecInfoDescWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class RecInfoDescWindow : RecInfoDescWindowBase
    {
        protected override UInt64 DataID { get { return recInfo == null ? 0 : recInfo.ID; } }
        protected override UserCtrlView.DataViewBase DataView { get { return ViewUtil.MainWindow.recInfoView; } }

        private RecFileInfo recInfo = null;

        public RecInfoDescWindow()
        {
            InitializeComponent();

            try
            {
                base.SetParam(false, checkBox_windowPinned, checkBox_dataReplace);

                //コマンドの登録
                this.CommandBindings.Add(new CommandBinding(EpgCmds.Cancel, (sender, e) => this.Close()));
                this.CommandBindings.Add(new CommandBinding(EpgCmds.Play, (sender, e) =>
                {
                    if (recInfo != null) CommonManager.Instance.FilePlay(recInfo.RecFilePath);
                }));

                //ボタンの設定
                mBinds.View = CtxmCode.RecInfoView;
                mBinds.SetCommandToButton(button_play, EpgCmds.Play);
                mBinds.AddInputCommand(EpgCmds.Cancel);//ショートカット登録
                base.RefreshMenu();
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void SetRecInfo(RecFileInfo info)
        {
            if (info == null) return;
            //
            recInfo = info;
            this.Title = ViewUtil.WindowTitleText(info.Title, "録画情報");
            textBox_pgInfo.Text = info.ProgramInfo;
            textBox_errLog.Text = info.ErrInfo;
        }
    }
    public class RecInfoDescWindowBase : AttendantDataWindow<RecInfoDescWindow> { }
}

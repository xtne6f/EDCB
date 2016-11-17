using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    /// <summary>
    /// AddManualAutoAddWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class AddManualAutoAddWindow : AddManualAutoAddWindowBase
    {
        protected override UserCtrlView.DataViewBase DataView { get { return ViewUtil.MainWindow.autoAddView.manualAutoAddView; } }
        protected override string AutoAddString { get { return "プログラム予約登録"; } }

        private List<CheckBox> chbxList;

        public AddManualAutoAddWindow()
        {
            InitializeComponent();

            try
            {
                base.SetParam(false, checkBox_windowPinned, checkBox_dataReplace);

                //コマンドの登録
                this.CommandBindings.Add(new CommandBinding(EpgCmds.Cancel, (sender, e) => this.Close()));
                this.CommandBindings.Add(new CommandBinding(EpgCmds.AddInDialog, autoadd_add));
                this.CommandBindings.Add(new CommandBinding(EpgCmds.ChangeInDialog, autoadd_chg, (sender, e) => e.CanExecute = winMode == AutoAddMode.Change));
                this.CommandBindings.Add(new CommandBinding(EpgCmds.DeleteInDialog, autoadd_del1, (sender, e) => e.CanExecute = winMode == AutoAddMode.Change));
                this.CommandBindings.Add(new CommandBinding(EpgCmds.Delete2InDialog, autoadd_del2, (sender, e) => e.CanExecute = winMode == AutoAddMode.Change));
                this.CommandBindings.Add(new CommandBinding(EpgCmds.UpItem, (sender, e) => button_up_down_Click(-1)));
                this.CommandBindings.Add(new CommandBinding(EpgCmds.DownItem, (sender, e) => button_up_down_Click(1)));

                //ボタンの設定
                mBinds.SetCommandToButton(button_cancel, EpgCmds.Cancel);
                mBinds.SetCommandToButton(button_chg, EpgCmds.ChangeInDialog);
                mBinds.SetCommandToButton(button_add, EpgCmds.AddInDialog);
                mBinds.SetCommandToButton(button_del, EpgCmds.DeleteInDialog);
                mBinds.SetCommandToButton(button_del2, EpgCmds.Delete2InDialog);
                mBinds.SetCommandToButton(button_up, EpgCmds.UpItem);
                mBinds.SetCommandToButton(button_down, EpgCmds.DownItem);
                mBinds.ResetInputBindings(this);

                //ステータスバーの登録
                this.statusBar.Status.Visibility = Visibility.Collapsed;
                StatusManager.RegisterStatusbar(this.statusBar, this);

                //その他設定
                chbxList = CommonManager.DayOfWeekArray.Select(wd => 
                    new CheckBox { Content = wd, Margin = new Thickness(0, 0, 6, 0) }).ToList();
                chbxList.ForEach(chbx => stackPanel_week.Children.Add(chbx));

                comboBox_startHH.ItemsSource = CommonManager.CustomHourList;
                comboBox_startHH.SelectedIndex = 0;
                comboBox_startMM.ItemsSource = Enumerable.Range(0, 60);
                comboBox_startMM.SelectedIndex = 0;
                comboBox_startSS.ItemsSource = Enumerable.Range(0, 60);
                comboBox_startSS.SelectedIndex = 0;
                comboBox_endHH.ItemsSource = CommonManager.CustomHourList;
                comboBox_endHH.SelectedIndex = 0;
                comboBox_endMM.ItemsSource = Enumerable.Range(0, 60);
                comboBox_endMM.SelectedIndex = 0;
                comboBox_endSS.ItemsSource = Enumerable.Range(0, 60);
                comboBox_endSS.SelectedIndex = 0;

                comboBox_service.ItemsSource = ChSet5.ChList.Values;
                comboBox_service.SelectedIndex = 0;

                recSettingView.SetViewMode(false);
                SetViewMode(AutoAddMode.NewAdd);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public override void SetWindowTitle()
        {
            this.Title = ViewUtil.WindowTitleText(textBox_title.Text, "プログラム自動予約登録");
        }

        public override AutoAddData GetAutoAddData()
        {
            try
            {
                var data = new ManualAutoAddData();
                data.dataID = autoAddID;

                UInt32 startTime = ((UInt32)comboBox_startHH.SelectedIndex * 60 * 60) + ((UInt32)comboBox_startMM.SelectedIndex * 60) + (UInt32)comboBox_startSS.SelectedIndex;
                UInt32 endTime = ((UInt32)comboBox_endHH.SelectedIndex * 60 * 60) + ((UInt32)comboBox_endMM.SelectedIndex * 60) + (UInt32)comboBox_endSS.SelectedIndex;
                while (endTime < startTime) endTime += 24 * 60 * 60;
                UInt32 duration = endTime - startTime;
                if (duration >= 24 * 60 * 60)
                {
                    //深夜時間帯の処理の関係で、不可条件が新たに発生しているため、その対応。
                    MessageBox.Show("24時間以上の録画時間は設定出来ません。", "録画時間長の確認", MessageBoxButton.OK);
                    return null;
                }

                data.startTime = startTime;
                data.durationSecond = duration;

                //曜日の処理、0～6bit目:日～土
                data.dayOfWeekFlag = 0;
                int val = 0;
                chbxList.ForEach(chbx => data.dayOfWeekFlag |= (byte)((chbx.IsChecked == true ? 0x01 : 0x00) << val++));

                //開始時刻を0～24時に調整する。
                data.RegulateData();

                data.IsEnabled = checkBox_keyDisabled.IsChecked != true;

                data.title = textBox_title.Text;

                ChSet5Item chItem = comboBox_service.SelectedItem as ChSet5Item;
                data.stationName = chItem.ServiceName;
                data.originalNetworkID = chItem.ONID;
                data.transportStreamID = chItem.TSID;
                data.serviceID = chItem.SID;
                data.recSetting = recSettingView.GetRecSetting();

                return data;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
            return null;
        }
        public override bool SetAutoAddData(AutoAddData setdata)
        {
            if (setdata as ManualAutoAddData == null) return false;
            var data = setdata as ManualAutoAddData;

            data = data.Clone();

            autoAddID = data.dataID;

            //深夜時間帯の処理
            if (Settings.Instance.LaterTimeUse == true && DateTime28.IsLateHour(data.PgStartTime.Hour) == true)
            {
                data.ShiftRecDay(-1);
            }

            //曜日の処理、0～6bit目:日～土
            int val = 0;
            chbxList.ForEach(chbx => chbx.IsChecked = (data.dayOfWeekFlag & (0x01 << val++)) != 0);

            checkBox_keyDisabled.IsChecked = data.IsEnabled == false;

            UInt32 hh = data.startTime / (60 * 60);
            UInt32 mm = (data.startTime % (60 * 60)) / 60;
            UInt32 ss = data.startTime % 60;

            comboBox_startHH.SelectedIndex = (int)hh;
            comboBox_startMM.SelectedIndex = (int)mm;
            comboBox_startSS.SelectedIndex = (int)ss;

            //深夜時間帯の処理も含む
            UInt32 endTime = data.startTime + data.durationSecond;
            if (endTime >= comboBox_endHH.Items.Count * 60 * 60 || endTime >= 24 * 60 * 60
                && DateTime28.JudgeLateHour(data.PgStartTime.AddSeconds(data.durationSecond), data.PgStartTime) == false)
            {
                //正規のデータであれば、必ず0～23時台かつstartTimeより小さくなる。
                endTime -= 24 * 60 * 60;
            }
            hh = endTime / (60 * 60);
            mm = (endTime % (60 * 60)) / 60;
            ss = endTime % 60;

            comboBox_endHH.SelectedIndex = (int)hh;
            comboBox_endMM.SelectedIndex = (int)mm;
            comboBox_endSS.SelectedIndex = (int)ss;

            textBox_title.Text = data.title;

            UInt64 key = data.Create64Key();

            if (ChSet5.ChList.ContainsKey(key) == true)
            {
                comboBox_service.SelectedItem = ChSet5.ChList[key];
            }
            recSettingView.SetDefSetting(data.recSetting, true);

            return true;
        }
    }
    public class AddManualAutoAddWindowBase : AutoAddWindow<AddManualAutoAddWindow, ManualAutoAddData> { }
}

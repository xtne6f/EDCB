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
    public partial class AddManualAutoAddWindow : Window
    {
        private ManualAutoAddData defKey = null;
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private MenuBinds mBinds = new MenuBinds();

        private bool chgMode = false;

        public AddManualAutoAddWindow()
        {
            InitializeComponent();

            try
            {
                //コマンドの登録
                this.CommandBindings.Add(new CommandBinding(EpgCmds.Cancel, (sender, e) => DialogResult = false));
                this.CommandBindings.Add(new CommandBinding(EpgCmds.AddInDialog, button_add_click));
                this.CommandBindings.Add(new CommandBinding(EpgCmds.ChangeInDialog, button_chg_click, (sender, e) => e.CanExecute = chgMode));
                this.CommandBindings.Add(new CommandBinding(EpgCmds.DeleteInDialog, button_del_click, (sender, e) => e.CanExecute = chgMode));
                this.CommandBindings.Add(new CommandBinding(EpgCmds.Delete2InDialog, button_del2_click, (sender, e) => e.CanExecute = chgMode));

                //ボタンの設定
                mBinds.SetCommandToButton(button_cancel, EpgCmds.Cancel);
                mBinds.SetCommandToButton(button_chg, EpgCmds.ChangeInDialog);
                mBinds.SetCommandToButton(button_add, EpgCmds.AddInDialog);
                mBinds.SetCommandToButton(button_del, EpgCmds.DeleteInDialog);
                mBinds.SetCommandToButton(button_del2, EpgCmds.Delete2InDialog);
                mBinds.ResetInputBindings(this);

                //その他設定
                comboBox_startHH.DataContext = CommonManager.Instance.HourDictionarySelect.Values;
                comboBox_startHH.SelectedIndex = 0;
                comboBox_startMM.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_startMM.SelectedIndex = 0;
                comboBox_startSS.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_startSS.SelectedIndex = 0;
                comboBox_endHH.DataContext = CommonManager.Instance.HourDictionarySelect.Values;
                comboBox_endHH.SelectedIndex = 0;
                comboBox_endMM.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_endMM.SelectedIndex = 0;
                comboBox_endSS.DataContext = CommonManager.Instance.MinDictionary.Values;
                comboBox_endSS.SelectedIndex = 0;

                comboBox_service.ItemsSource = ChSet5.Instance.ChList.Values;
                comboBox_service.SelectedIndex = 0;

                recSettingView.SetViewMode(false);
                SetChangeMode(false);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void SetChangeMode(bool chgFlag)
        {
            chgMode = chgFlag;
            button_chg.Visibility = (chgFlag == true ? Visibility.Visible : Visibility.Hidden);
            button_del.Visibility = button_chg.Visibility;
            button_del2.Visibility = button_chg.Visibility;
        }

        public void SetDefaultSetting(ManualAutoAddData item)
        {
            defKey = item.Clone();
        }

        //proc 0:追加、1:変更、2:削除、3:予約ごと削除
        private bool CheckAutoAddChange(ExecutedRoutedEventArgs e, int proc)
        {
            if (proc != 3)
            {
                if (CmdExeUtil.IsDisplayKgMessage(e) == true)
                {
                    var strMode = new string[] { "追加", "変更", "削除" }[proc];
                    if (MessageBox.Show("プログラム予約登録を" + strMode + "します。\r\nよろしいですか？", strMode + "の確認", MessageBoxButton.OKCancel) != MessageBoxResult.OK)
                    { return false; }
                }
            }
            else
            {
                if (CmdExeUtil.CheckAllProcCancel(e, CommonUtil.ToList(defKey), true) == true)
                { return false; }
            }

            if (proc != 0)
            {
                if (CommonManager.Instance.DB.ManualAutoAddList.ContainsKey(this.defKey.dataID) == false)
                {
                    MessageBox.Show("項目がありません。\r\n" +
                        "既に削除されています。\r\n" +
                        "(別のEpgtimerによる操作など)");

                    //追加モードに変更
                    SetChangeMode(false);
                    defKey = null;
                    return false;
                }
            }

            return true;
        }

        private void button_add_click(object sender, ExecutedRoutedEventArgs e)
        {
            button_add_chg(sender, e, false);
        }
        private void button_chg_click(object sender, ExecutedRoutedEventArgs e)
        {
            button_add_chg(sender, e, true);
        }
        private void button_add_chg(object sender, ExecutedRoutedEventArgs e, bool chgFlag)
        {
            try
            {
                UInt32 startTime = ((UInt32)comboBox_startHH.SelectedIndex * 60 * 60) + ((UInt32)comboBox_startMM.SelectedIndex * 60) + (UInt32)comboBox_startSS.SelectedIndex;
                UInt32 endTime = ((UInt32)comboBox_endHH.SelectedIndex * 60 * 60) + ((UInt32)comboBox_endMM.SelectedIndex * 60) + (UInt32)comboBox_endSS.SelectedIndex;
                while (endTime < startTime) endTime += 24 * 60 * 60;
                UInt32 duration = endTime - startTime;
                if (duration >= 24 * 60 * 60)
                {
                    //深夜時間帯の処理の関係で、不可条件が新たに発生しているため、その対応。
                    MessageBox.Show("24時間以上の録画時間は設定出来ません。", "録画時間長の確認", MessageBoxButton.OK);
                    return;
                }

                if (CheckAutoAddChange(e, chgFlag == false ? 0 : 1) == false) return;
                //
                if (defKey == null)
                {
                    defKey = new ManualAutoAddData();
                }

                defKey.startTime = startTime;
                defKey.durationSecond = duration;
                
                defKey.dayOfWeekFlag = 0;
                if (checkBox_week0.IsChecked == true)
                {
                    defKey.dayOfWeekFlag |= 0x01;
                }
                if (checkBox_week1.IsChecked == true)
                {
                    defKey.dayOfWeekFlag |= 0x02;
                }
                if (checkBox_week2.IsChecked == true)
                {
                    defKey.dayOfWeekFlag |= 0x04;
                }
                if (checkBox_week3.IsChecked == true)
                {
                    defKey.dayOfWeekFlag |= 0x08;
                }
                if (checkBox_week4.IsChecked == true)
                {
                    defKey.dayOfWeekFlag |= 0x10;
                }
                if (checkBox_week5.IsChecked == true)
                {
                    defKey.dayOfWeekFlag |= 0x20;
                }
                if (checkBox_week6.IsChecked == true)
                {
                    defKey.dayOfWeekFlag |= 0x40;
                }

                //開始時刻を0～24時に調整する。
                defKey.RegulateData();
                
                defKey.IsEnabled = checkBox_keyDisabled.IsChecked != true;

                defKey.title = textBox_title.Text;

                ChSet5Item chItem = comboBox_service.SelectedItem as ChSet5Item;
                defKey.stationName = chItem.ServiceName;
                defKey.originalNetworkID = chItem.ONID;
                defKey.transportStreamID = chItem.TSID;
                defKey.serviceID = chItem.SID;
                defKey.recSetting = recSettingView.GetRecSetting();

                if (chgFlag == true)
                {
                    mutil.AutoAddChange(CommonUtil.ToList(defKey));
                }
                else
                {
                    mutil.AutoAddAdd(CommonUtil.ToList(defKey));
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            DialogResult = true;
        }

        private void button_del_click(object sender, ExecutedRoutedEventArgs e)
        {
            if (CheckAutoAddChange(e, 2) == false) return;
            //
            mutil.AutoAddDelete(CommonUtil.ToList(defKey));
            DialogResult = true;
        }

        private void button_del2_click(object sender, ExecutedRoutedEventArgs e)
        {
            if (CheckAutoAddChange(e, 3) == false) return;
            //
            mutil.AutoAddDelete(CommonUtil.ToList(defKey), true, true, false);
            DialogResult = true;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            if (defKey != null)
            {
                //深夜時間帯の処理
                if (Settings.Instance.LaterTimeUse == true && DateTime28.IsLateHour(defKey.PgStartTime.Hour) == true)
                {
                    defKey.ShiftRecDay(-1);
                }

                if ((defKey.dayOfWeekFlag & 0x01) != 0)
                {
                    checkBox_week0.IsChecked = true;
                }
                if ((defKey.dayOfWeekFlag & 0x02) != 0)
                {
                    checkBox_week1.IsChecked = true;
                }
                if ((defKey.dayOfWeekFlag & 0x04) != 0)
                {
                    checkBox_week2.IsChecked = true;
                }
                if ((defKey.dayOfWeekFlag & 0x08) != 0)
                {
                    checkBox_week3.IsChecked = true;
                }
                if ((defKey.dayOfWeekFlag & 0x10) != 0)
                {
                    checkBox_week4.IsChecked = true;
                }
                if ((defKey.dayOfWeekFlag & 0x20) != 0)
                {
                    checkBox_week5.IsChecked = true;
                }
                if ((defKey.dayOfWeekFlag & 0x40) != 0)
                {
                    checkBox_week6.IsChecked = true;
                }

                checkBox_keyDisabled.IsChecked = defKey.IsEnabled == false;

                UInt32 hh = defKey.startTime / (60 * 60);
                UInt32 mm = (defKey.startTime % (60 * 60)) / 60;
                UInt32 ss = defKey.startTime % 60;

                comboBox_startHH.SelectedIndex = (int)hh;
                comboBox_startMM.SelectedIndex = (int)mm;
                comboBox_startSS.SelectedIndex = (int)ss;

                //深夜時間帯の処理も含む
                UInt32 endTime = defKey.startTime + defKey.durationSecond;
                if (endTime >= comboBox_endHH.Items.Count * 60 * 60 || endTime >= 24 * 60 * 60
                    && DateTime28.JudgeLateHour(defKey.PgStartTime.AddSeconds(defKey.durationSecond), defKey.PgStartTime) == false)
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

                textBox_title.Text = defKey.title;

                UInt64 key = defKey.Create64Key();

                if (ChSet5.Instance.ChList.ContainsKey(key) == true)
                {
                    comboBox_service.SelectedItem = ChSet5.Instance.ChList[key];
                }
                recSettingView.SetDefSetting(defKey.recSetting, true);
            }
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            MainWindow mainWindow = (MainWindow)Application.Current.MainWindow;
            mainWindow.ListFoucsOnVisibleChanged();
        }
    }
}

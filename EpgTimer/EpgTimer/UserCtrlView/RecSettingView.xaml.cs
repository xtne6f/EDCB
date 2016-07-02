using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Input;

namespace EpgTimer
{
    /// <summary>
    /// RecSettingView.xaml の相互作用ロジック
    /// </summary>
    public partial class RecSettingView : UserControl
    {
        public virtual event EventHandler SelectedPresetChanged = null;
        
        private RecSettingData recSetting;
        private List<TunerSelectInfo> tunerList = new List<TunerSelectInfo>();
        private static CtrlCmdUtil cmd { get { return CommonManager.Instance.CtrlCmd; } }

        private bool initLoad = false;
        public RecSettingView()
        {
            InitializeComponent();

            try
            {
                recSetting = Settings.Instance.RecPresetList[0].RecPresetData.Clone();

                comboBox_recMode.DataContext = CommonManager.Instance.RecModeDictionary.Values;
                comboBox_tuijyu.DataContext = CommonManager.Instance.YesNoDictionary.Values;
                comboBox_pittari.DataContext = CommonManager.Instance.YesNoDictionary.Values;
                comboBox_priority.DataContext = CommonManager.Instance.PriorityDictionary.Values;

                tunerList.Add(new TunerSelectInfo("自動", 0));
                foreach (TunerReserveInfo info in CommonManager.Instance.DB.TunerReserveList.Values)
                {
                    if (info.tunerID != 0xFFFFFFFF)
                    {
                        tunerList.Add(new TunerSelectInfo(info.tunerName, info.tunerID));
                    }
                }
                comboBox_tuner.ItemsSource = tunerList;
                comboBox_tuner.SelectedIndex = 0;

                Settings.Instance.RecPresetList.ForEach(info => info.LoadRecPresetData());//iniファイルから録画設定をロード
                Settings.Instance.RecPresetList.ForEach(info => comboBox_preSet.Items.Add(info.Clone()));//現在の処理ならClone()無くても大丈夫
                comboBox_preSet.SelectedIndex = 0;

                var bx = new BoxExchangeEdit.BoxExchangeEditor(null, listView_recFolder, true, true, true);
                bx.targetBoxAllowDoubleClick(bx.TargetBox, (sender, e) => button_recFolderChg.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)));
                button_recFolderDel.Click += new RoutedEventHandler(bx.button_Delete_Click);

                if (CommonManager.Instance.NWMode == true)
                {
                    button_bat.IsEnabled = false;
                }
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void SavePreset()
        {
            string saveID = "";
            for (int i = 0; i < comboBox_preSet.Items.Count; i++)
            {
                var preItem = comboBox_preSet.Items[i] as RecPresetItem;
                if (preItem.IsCustom == true) continue;

                preItem.ID = (UInt32)i;
                preItem.SaveRecPresetData();

                if (preItem.ID != 0)
                {
                    saveID += preItem.ID.ToString();
                    saveID += ",";
                }
            }
            comboBox_preSet.Items.Refresh();
            IniFileHandler.WritePrivateProfileString("SET", "PresetID", saveID, SettingPath.TimerSrvIniPath);
            Settings.SaveToXmlFile();
            Settings.Instance.RecPresetList = null;

            if (CommonManager.Instance.NWMode == false)
            {
                CommonManager.Instance.CtrlCmd.SendNotifyProfileUpdate();
                ViewUtil.MainWindow.RefreshAllViewsReserveInfo(MainWindow.UpdateViewMode.ReserveInfoNoTuner);
            }
        }

        public void SetViewMode(bool epgMode)
        {
            comboBox_tuijyu.IsEnabled = (epgMode == true);
            comboBox_pittari.IsEnabled = (epgMode == true);
        }
 
        public void SetChangeMode(int chgMode)
        {
            switch (chgMode)
            {
                case 0:
                    ViewUtil.SetSpecificChgAppearance(textBox_margineStart);
                    textBox_margineStart.Focus();
                    textBox_margineStart.SelectAll();
                    break;
                case 1:
                    ViewUtil.SetSpecificChgAppearance(textBox_margineEnd);
                    textBox_margineEnd.Focus();
                    textBox_margineEnd.SelectAll();
                    break;
            }
        }

        //予約データがプリセットに該当するときに、プリセットではなく予約データを画面に読むようにするフラグ
        bool loadingDefSetting = false;
        public void SetDefSetting(RecSettingData set, bool isDisplayManual = false)
        {
            recSetting = set.Clone();

            //"登録時"を追加する。既存があれば追加前に削除する。検索ダイアログの上下ボタンの移動用のコード。
            comboBox_preSet.Items.Remove(FindPresetItem(RecPresetItem.CustomID));
            comboBox_preSet.Items.Add(new RecPresetItem("登録時", RecPresetItem.CustomID, set.Clone()));

            //該当するものがあれば選択、無ければ"登録時"。一応特定条件下で齟齬が出ないように2回検索にしておく。
            object target = FindPresetItem(set.LookUpPreset(isDisplayManual).ID);
            if (target == null) target = comboBox_preSet.Items[comboBox_preSet.Items.Count - 1];

            //強制更新
            comboBox_preSet.SelectedItem = null;
            loadingDefSetting = true;
            comboBox_preSet.SelectedItem = target;
        }

        //未使用。プリセット指定で予約追加ダイアログを立ち上げるときに使えると思うが、そのような処理がない。
        public void SetDefSetting(UInt32 presetID)
        {
            RecPresetItem target = FindPresetItem(presetID);
            if (target == null) target = comboBox_preSet.Items[0] as RecPresetItem;

            recSetting = target.RecPresetData.Clone();

            //強制更新
            comboBox_preSet.SelectedItem = null;
            comboBox_preSet.SelectedItem = target;
        }

        private RecPresetItem FindPresetItem(UInt32 presetID)
        {
            return comboBox_preSet.Items.OfType<RecPresetItem>().FirstOrDefault(item => item.ID == presetID);
        }

        public RecPresetItem SelectedPreset(bool isCheckData = false, bool isDisplayManual = false)
        {
            RecPresetItem preset = comboBox_preSet.SelectedItem as RecPresetItem;
            if (isCheckData == true)
            {
                var preset_back = preset;
                preset = GetRecSetting().LookUpPreset(comboBox_preSet.Items.OfType<RecPresetItem>(), isDisplayManual);
                if (preset != null & comboBox_preSet.Items.Contains(preset) == false)
                {
                    preset.DisplayName = (preset_back != null ? preset_back.DisplayName : "カスタム") + "*";
                }
            }
            return preset;
        }

        public RecSettingData GetRecSetting()
        {
            if (initLoad == false)
            {
                return recSetting.Clone();
            }

            var setInfo = new RecSettingData();

            setInfo.RecMode = ((RecModeInfo)comboBox_recMode.SelectedItem).Value;
            setInfo.Priority = ((PriorityInfo)comboBox_priority.SelectedItem).Value;
            setInfo.TuijyuuFlag = ((YesNoInfo)comboBox_tuijyu.SelectedItem).Value;

            setInfo.ServiceMode = (uint)(checkBox_serviceMode.IsChecked == true ? 0 : 1);
            if (checkBox_serviceCaption.IsChecked == true)
            {
                setInfo.ServiceMode |= 0x10;
            }
            if (checkBox_serviceData.IsChecked == true)
            {
                setInfo.ServiceMode |= 0x20;
            }

            setInfo.PittariFlag = ((YesNoInfo)comboBox_pittari.SelectedItem).Value;
            setInfo.BatFilePath = textBox_bat.Text;
            setInfo.RecFolderList.Clear();
            setInfo.PartialRecFolder.Clear();
            foreach (RecFileSetInfoView view in listView_recFolder.Items)
            {
                (view.PartialRec ? setInfo.PartialRecFolder : setInfo.RecFolderList).Add(view.Info);
            }

            if (checkBox_suspendDef.IsChecked == true)
            {
                setInfo.SuspendMode = 0;
            }
            else if (radioButton_standby.IsChecked == true)
            {
                setInfo.SuspendMode = 1;
            }
            else if (radioButton_suspend.IsChecked == true)
            {
                setInfo.SuspendMode = 2;
            }
            else if (radioButton_shutdown.IsChecked == true)
            {
                setInfo.SuspendMode = 3;
            }
            else if (radioButton_non.IsChecked == true)
            {
                setInfo.SuspendMode = 4;
            }
            setInfo.RebootFlag = (byte)(checkBox_reboot.IsChecked == true ? 1 : 0);
            
            setInfo.UseMargineFlag = (byte)(checkBox_margineDef.IsChecked == true ? 0 : 1);
            Func<string, int> GetMargin = (text) =>
            {
                if (text.Length == 0) return 0;

                int marginSec = 0;
                int marginMinus = 1;
                if (text.IndexOf("-") == 0)
                {
                    marginMinus = -1;
                    text = text.Substring(1);
                }
                string[] startArray = text.Split(':');
                startArray = startArray.Take(Math.Min(startArray.Length, 3)).Reverse().ToArray();
                for (int i = 0; i < startArray.Length; i++)
                {
                    marginSec += Convert.ToInt32(startArray[i]) * (int)Math.Pow(60, i);
                }

                return marginMinus * marginSec;
            };
            setInfo.StartMargine = GetMargin(textBox_margineStart.Text);
            setInfo.EndMargine = GetMargin(textBox_margineEnd.Text);

            setInfo.PartialRecFlag = (byte)(checkBox_partial.IsChecked == true ? 1 : 0);
            setInfo.ContinueRecFlag = (byte)(checkBox_continueRec.IsChecked == true ? 1 : 0);

            TunerSelectInfo tuner = comboBox_tuner.SelectedItem as TunerSelectInfo;
            setInfo.TunerID = tuner.ID;

            return setInfo;
        }

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (initLoad == false)
            {
                UpdateView();
                initLoad = true;
            }
        }

        private void comboBox_preSet_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (comboBox_preSet.SelectedItem != null)
                {
                    if (SelectedPresetChanged != null) SelectedPresetChanged(this, new EventArgs());
                    if (loadingDefSetting != true)
                    {
                        recSetting = (comboBox_preSet.SelectedItem as RecPresetItem).RecPresetData;
                    }
                    UpdateView();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
            loadingDefSetting = false;
        }

        private void UpdateView()
        {
            try
            {
                if (CommonManager.Instance.NWMode == true)
                {
                    button_add_preset.IsEnabled = false;
                    button_chg_preset.IsEnabled = false;
                    button_del_preset.IsEnabled = false;
                }
                foreach (RecModeInfo info in comboBox_recMode.Items)
                {
                    if (info.Value == recSetting.RecMode)
                    {
                        comboBox_recMode.SelectedItem = info;
                    }
                }
                foreach (PriorityInfo info in comboBox_priority.Items)
                {
                    if (info.Value == recSetting.Priority)
                    {
                        comboBox_priority.SelectedItem = info;
                    }
                }
                foreach (YesNoInfo info in comboBox_tuijyu.Items)
                {
                    if (info.Value == recSetting.TuijyuuFlag)
                    {
                        comboBox_tuijyu.SelectedItem = info;
                    }
                }

                checkBox_serviceMode.IsChecked = null;//切り替え時のイベント発生のために必要
                checkBox_serviceMode.IsChecked = ((recSetting.ServiceMode & 0x0F) == 0);

                foreach (YesNoInfo info in comboBox_pittari.Items)
                {
                    if (info.Value == recSetting.PittariFlag)
                    {
                        comboBox_pittari.SelectedItem = info;
                    }
                }


                textBox_bat.Text = recSetting.BatFilePath;

                listView_recFolder.Items.Clear();
                foreach (RecFileSetInfo info in recSetting.RecFolderList)
                {
                    listView_recFolder.Items.Add(new RecFileSetInfoView(info.Clone(), false));
                }
                foreach (RecFileSetInfo info in recSetting.PartialRecFolder)
                {
                    listView_recFolder.Items.Add(new RecFileSetInfoView(info.Clone(), true));
                }

                checkBox_suspendDef.IsChecked = null;//切り替え時のイベント発生のために必要
                checkBox_suspendDef.IsChecked = (recSetting.SuspendMode == 0);
                checkBox_margineDef.IsChecked = null;//切り替え時のイベント発生のために必要
                checkBox_margineDef.IsChecked = recSetting.UseMargineFlag == 0;
                checkBox_continueRec.IsChecked = (recSetting.ContinueRecFlag == 1);
                checkBox_partial.IsChecked = (recSetting.PartialRecFlag == 1);

                foreach (TunerSelectInfo info in comboBox_tuner.Items)
                {
                    if (info.ID == recSetting.TunerID)
                    {
                        comboBox_tuner.SelectedItem = info;
                        break;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void checkBox_suspendDef_Checked(object sender, RoutedEventArgs e)
        {
            int recEndMode = 0;
            bool reboot = false;
            if (checkBox_suspendDef.IsChecked == true)
            {
                recEndMode = IniFileHandler.GetPrivateProfileInt("SET", "RecEndMode", 2, SettingPath.TimerSrvIniPath);
                reboot = IniFileHandler.GetPrivateProfileInt("SET", "Reboot", 0, SettingPath.TimerSrvIniPath) == 1;
            }
            else
            {
                recEndMode = recSetting.SuspendMode == 4 ? 0 : recSetting.SuspendMode;
                reboot = (recSetting.RebootFlag == 1);
            }
            checkBox_reboot.IsChecked = reboot;
            switch (recEndMode)
            {
                case 0:
                    radioButton_non.IsChecked = true;
                    break;
                case 1:
                    radioButton_standby.IsChecked = true;
                    break;
                case 2:
                    radioButton_suspend.IsChecked = true;
                    break;
                case 3:
                    radioButton_shutdown.IsChecked = true;
                    break;
            }
        }

        private class RecFileSetInfoView
        {
            public RecFileSetInfoView(RecFileSetInfo info, bool partialRec) { Info = info; PartialRec = partialRec; }
            public string RecFileName { get { return Info.RecFileName; } }
            public string RecFolder { get { return Info.RecFolder; } }
            public string RecNamePlugIn { get { return Info.RecNamePlugIn; } }
            public string WritePlugIn { get { return Info.WritePlugIn; } }
            public RecFileSetInfo Info { get; private set; }
            public bool PartialRec { get; private set; }
            public string PartialRecYesNo { get { return PartialRec ? "はい" : "いいえ"; } } 
        }

        private void checkBox_margineDef_Checked(object sender, RoutedEventArgs e)
        {
            RecSettingData recSet = recSetting.Clone();
            recSet.UseMargineFlag = (byte)(checkBox_margineDef.IsChecked == true ? 0 : 1);
            textBox_margineStart.Text = recSet.GetTrueMargin(true).ToString();
            textBox_margineEnd.Text = recSet.GetTrueMargin(false).ToString();
        }

        private void checkBox_serviceMode_Checked(object sender, RoutedEventArgs e)
        {
            if (checkBox_serviceMode.IsChecked == true)
            {
                checkBox_serviceCaption.IsChecked =
                    IniFileHandler.GetPrivateProfileInt("SET", "Caption", 1, SettingPath.EdcbIniPath) != 0;
                checkBox_serviceData.IsChecked =
                    IniFileHandler.GetPrivateProfileInt("SET", "Data", 0, SettingPath.EdcbIniPath) != 0;
            }
            else
            {
                checkBox_serviceCaption.IsChecked = ((recSetting.ServiceMode & 0x10) > 0);
                checkBox_serviceData.IsChecked = ((recSetting.ServiceMode & 0x20) > 0);
            }
        }

        private void button_bat_Click(object sender, RoutedEventArgs e)
        {
            CommonManager.GetFileNameByDialog(textBox_bat, false, "", ".bat");
        }

        private void button_recFolderAdd_Click(object sender, RoutedEventArgs e)
        {
            recFolderAdd(false);
        }

        private void button_recFolderChg_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recFolder.SelectedItem == null)
            {
                if (listView_recFolder.Items.Count != 0)
                {
                    listView_recFolder.SelectedIndex = 0;
                }
            }
            if (listView_recFolder.SelectedItem != null)
            {
                var setting = new RecFolderWindow();
                setting.Owner = CommonUtil.GetTopWindow(this);
                var selectInfo = ((RecFileSetInfoView)listView_recFolder.SelectedItem).Info;
                setting.SetDefSetting(selectInfo);
                setting.SetPartialMode(((RecFileSetInfoView)listView_recFolder.SelectedItem).PartialRec);
                if (setting.ShowDialog() == true)
                {
                    setting.GetSetting(ref selectInfo);
                }
                listView_recFolder.Items.Refresh();
            }
            else
            {
                recFolderAdd(false);
            }
        }

        private void recFolderAdd(bool partialRec)
        {
            var setting = new RecFolderWindow();
            setting.Owner = CommonUtil.GetTopWindow(this);
            setting.SetPartialMode(partialRec);
            if (setting.ShowDialog() == true)
            {
                var setInfo = new RecFileSetInfo();
                setting.GetSetting(ref setInfo);
                foreach (RecFileSetInfoView info in listView_recFolder.Items)
                {
                    if (info.PartialRec == partialRec &&
                        String.Compare(setInfo.RecFolder, info.RecFolder, true) == 0 &&
                        String.Compare(setInfo.WritePlugIn, info.WritePlugIn, true) == 0 &&
                        String.Compare(setInfo.RecNamePlugIn, info.RecNamePlugIn, true) == 0)
                    {
                        MessageBox.Show("すでに追加されています");
                        return;
                    }
                }
                listView_recFolder.Items.Add(new RecFileSetInfoView(setInfo, partialRec));
            }
        }
        
        private void button_del_preset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (comboBox_preSet.SelectedItem != null)
                {
                    var item = comboBox_preSet.SelectedItem as RecPresetItem;
                    if (item.ID == 0)
                    {
                        MessageBox.Show("デフォルトは削除できません");
                        return;
                    }
                    else if (item.IsCustom == true)
                    {
                        MessageBox.Show("このプリセットは変更できません");
                        return;
                    }
                    else
                    {
                        int newIndex = Math.Max(0, Math.Min(comboBox_preSet.SelectedIndex, comboBox_preSet.Items.Count - 2));
                        comboBox_preSet.Items.Remove(item);
                        comboBox_preSet.SelectedIndex = newIndex;
                        SavePreset();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_reload_preset_Click(object sender, RoutedEventArgs e)
        {
            comboBox_preSet_SelectionChanged(null, null);
        }

        private void button_add_preset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                var setting = new AddPresetWindow();
                setting.Owner = CommonUtil.GetTopWindow(this);
                if (setting.ShowDialog() == true)
                {
                    RecPresetItem preCust = FindPresetItem(RecPresetItem.CustomID);
                    int insertIndex = comboBox_preSet.Items.Count + (preCust == null ? 0 : -1);
                    var newInfo = new RecPresetItem(setting.GetName(), 0, GetRecSetting());//IDはSavePresetですぐ割り振られる。
                    comboBox_preSet.Items.Insert(insertIndex, newInfo);
                    comboBox_preSet.SelectedIndex = insertIndex;
                    SavePreset();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_chg_preset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (comboBox_preSet.SelectedItem != null)
                {
                    var item = comboBox_preSet.SelectedItem as RecPresetItem;

                    if (item.IsCustom == true)
                    {
                        MessageBox.Show("このプリセットは変更できません");
                        return;
                    }

                    var setting = new AddPresetWindow();
                    setting.Owner = CommonUtil.GetTopWindow(this);
                    setting.SetMode(true);
                    setting.SetName(item.DisplayName);
                    if (setting.ShowDialog() == true)
                    {
                        item.DisplayName = setting.GetName();
                        item.RecPresetData = GetRecSetting();
                        comboBox_preSet.SelectedItem = null;
                        comboBox_preSet.SelectedItem = item;
                        SavePreset();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_recFolderAdd_1seg_Click(object sender, RoutedEventArgs e)
        {
            recFolderAdd(true);
        }
    }

    public class RecSettingViewInverter : IValueConverter
    {
        public object Convert(object v, Type t, object p, System.Globalization.CultureInfo c)
        {
            return !(v is bool && (bool)v);
        }
        public object ConvertBack(object v, Type t, object p, System.Globalization.CultureInfo c)
        {
            return !(v is bool && (bool)v);
        }
    }
}

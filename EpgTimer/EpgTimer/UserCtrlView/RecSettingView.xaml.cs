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
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO;
using System.Windows.Interop;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer
{
    /// <summary>
    /// RecSettingView.xaml の相互作用ロジック
    /// </summary>
    public partial class RecSettingView : UserControl
    {
        private RecSettingData recSetting = new RecSettingData();
        private RecSettingData setDefSetting = new RecSettingData();
        private List<TunerSelectInfo> tunerList = new List<TunerSelectInfo>();
        private CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;

        private int lastSelectIndex = 0;
        private Dictionary<UInt32, RecSettingData> presetList = new Dictionary<UInt32, RecSettingData>();

        private bool initLoad = false;
        public RecSettingView()
        {
            InitializeComponent();

            try
            {
                Settings.GetDefRecSetting(0, ref recSetting);

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

                RecPresetItem preDef = new RecPresetItem();
                preDef.DisplayName = "デフォルト";
                preDef.ID = 0;
                RecSettingData defSet = new RecSettingData();
                Settings.GetDefRecSetting(0, ref defSet);
                presetList.Add(0, defSet);

                comboBox_preSet.Items.Add(preDef);
                comboBox_preSet.SelectedIndex = 0;
                lastSelectIndex = 0;

                foreach (RecPresetItem info in Settings.Instance.RecPresetList)
                {
                    if (presetList.ContainsKey(info.ID) == false)
                    {
                        RecSettingData setDatat = new RecSettingData();
                        Settings.GetDefRecSetting(info.ID, ref setDatat);

                        presetList.Add(info.ID, setDatat);

                        comboBox_preSet.Items.Add(info);
                    }
                }

                if (CommonManager.Instance.NWMode == true)
                {
                    button_bat.IsEnabled = false;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void AddPreset(String name)
        {
            RecSettingData newSet = new RecSettingData();
            Settings.GetDefRecSetting(0, ref newSet);

            RecPresetItem newInfo = new RecPresetItem();
            newInfo.DisplayName = name;
            newInfo.ID = 0;

            while (presetList.ContainsKey(newInfo.ID) == true)
            {
                newInfo.ID++;
            }

            presetList.Add(newInfo.ID, newSet);
            int index = comboBox_preSet.Items.Add(newInfo);
            SavePreset();
            comboBox_preSet.SelectedIndex = index;

        }

        public void SavePreset()
        {
            Settings.Instance.RecPresetList.Clear();
            string saveID = "";
            for (int i = 0; i < comboBox_preSet.Items.Count; i++)
            {
                RecPresetItem preItem = comboBox_preSet.Items[i] as RecPresetItem;
                if (preItem.ID == 0xFFFFFFFF)
                {
                    continue;
                }
                String defName = "REC_DEF";
                String defFolderName = "REC_DEF_FOLDER";
                String defFolder1SegName = "REC_DEF_FOLDER_1SEG";
                RecSettingData info = presetList[preItem.ID];

                preItem.ID = (UInt32)i;
                if (preItem.ID != 0)
                {
                    defName += preItem.ID.ToString();
                    defFolderName += preItem.ID.ToString();
                    defFolder1SegName += preItem.ID.ToString();
                    saveID += preItem.ID.ToString();
                    saveID += ",";
                }

                IniFileHandler.WritePrivateProfileString(defName, "SetName", preItem.DisplayName, SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "RecMode", info.RecMode.ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "Priority", info.Priority.ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "TuijyuuFlag", info.TuijyuuFlag.ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "ServiceMode", info.ServiceMode.ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "PittariFlag", info.PittariFlag.ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "BatFilePath", info.BatFilePath, SettingPath.TimerSrvIniPath);

                IniFileHandler.WritePrivateProfileString(defFolderName, "Count", info.RecFolderList.Count.ToString(), SettingPath.TimerSrvIniPath);
                for (int j = 0; j < info.RecFolderList.Count; j++)
                {
                    IniFileHandler.WritePrivateProfileString(defFolderName, j.ToString(), info.RecFolderList[j].RecFolder, SettingPath.TimerSrvIniPath);
                    IniFileHandler.WritePrivateProfileString(defFolderName, "WritePlugIn" + j.ToString(), info.RecFolderList[j].WritePlugIn, SettingPath.TimerSrvIniPath);
                    IniFileHandler.WritePrivateProfileString(defFolderName, "RecNamePlugIn" + j.ToString(), info.RecFolderList[j].RecNamePlugIn, SettingPath.TimerSrvIniPath);
                }
                IniFileHandler.WritePrivateProfileString(defFolder1SegName, "Count", info.PartialRecFolder.Count.ToString(), SettingPath.TimerSrvIniPath);
                for (int j = 0; j < info.PartialRecFolder.Count; j++)
                {
                    IniFileHandler.WritePrivateProfileString(defFolder1SegName, j.ToString(), info.PartialRecFolder[j].RecFolder, SettingPath.TimerSrvIniPath);
                    IniFileHandler.WritePrivateProfileString(defFolder1SegName, "WritePlugIn" + j.ToString(), info.PartialRecFolder[j].WritePlugIn, SettingPath.TimerSrvIniPath);
                    IniFileHandler.WritePrivateProfileString(defFolder1SegName, "RecNamePlugIn" + j.ToString(), info.PartialRecFolder[j].RecNamePlugIn, SettingPath.TimerSrvIniPath);
                }

                IniFileHandler.WritePrivateProfileString(defName, "SuspendMode", info.SuspendMode.ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "RebootFlag", info.RebootFlag.ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "UseMargineFlag", info.UseMargineFlag.ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "StartMargine", info.StartMargine.ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "EndMargine", info.EndMargine.ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "ContinueRec", info.ContinueRecFlag.ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "PartialRec", info.PartialRecFlag.ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "TunerID", info.TunerID.ToString(), SettingPath.TimerSrvIniPath);

                Settings.Instance.RecPresetList.Add(preItem);
            }
            IniFileHandler.WritePrivateProfileString("SET", "PresetID", saveID, SettingPath.TimerSrvIniPath);
            Settings.SaveToXmlFile();

            if (CommonManager.Instance.NWMode == false) CommonManager.Instance.CtrlCmd.SendNotifyProfileUpdate();
        }

        public void SetViewMode(bool epgMode)
        {
            if (epgMode == true)
            {
                comboBox_tuijyu.IsEnabled = true;
                comboBox_pittari.IsEnabled = true;
            }
            else
            {
                comboBox_tuijyu.IsEnabled = false;
                comboBox_pittari.IsEnabled = false;
            }
        }

        public void SetChangeMode(int chgMode)
        {
            switch (chgMode)
            {
                case 0:
                    CommonManager.Instance.VUtil.SetSpecificChgAppearance(textBox_margineStart);
                    textBox_margineStart.Focus();
                    textBox_margineStart.SelectAll();
                    break;
                case 1:
                    CommonManager.Instance.VUtil.SetSpecificChgAppearance(textBox_margineEnd);
                    textBox_margineEnd.Focus();
                    textBox_margineEnd.SelectAll();
                    break;
            }
        }

        public void SetDefSetting(RecSettingData set)
        {
            RecPresetItem preCust = null;

            foreach (RecPresetItem item in comboBox_preSet.Items)
            {
                if (item.ID == 0xFFFFFFFF)
                {
                    preCust = item;
                    break;
                }
            }

            if (preCust == null)
            {
                preCust = new RecPresetItem();
                preCust.DisplayName = "登録時";
                preCust.ID = 0xFFFFFFFF;
                comboBox_preSet.Items.Add(preCust);
            }
            comboBox_preSet.SelectedItem = preCust;

            setDefSetting = set.Clone();
            recSetting = setDefSetting;

            UpdateView();
        }

        public void SetDefSetting(UInt32 presetID)
        {
            Settings.GetDefRecSetting(presetID, ref recSetting);
            setDefSetting = recSetting;
            foreach(RecPresetItem info in comboBox_preSet.Items)
            {
                if (info.ID == presetID)
                {
                    comboBox_preSet.SelectedItem = info;
                    break;
                }
            }

            UpdateView();
        }

        public void GetRecSetting(ref RecSettingData setInfo)
        {
            if (initLoad == false)
            {
                setInfo = recSetting.Clone();
                return;
            }

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
            foreach (RecFileSetInfoView view in listView_recFolder.Items)
            {
                setInfo.RecFolderList.Add(view.Info);
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
            setInfo.PartialRecFolder.Clear();
            foreach (RecFileSetInfoView view in listView_recFolder_1seg.Items)
            {
                setInfo.PartialRecFolder.Add(view.Info);
            }
            setInfo.ContinueRecFlag = (byte)(checkBox_continueRec.IsChecked == true ? 1 : 0);

            TunerSelectInfo tuner = comboBox_tuner.SelectedItem as TunerSelectInfo;
            setInfo.TunerID = tuner.ID;
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
                    RecPresetItem item = comboBox_preSet.SelectedItem as RecPresetItem;
                    if (item.ID == 0xFFFFFFFF)
                    {
                        recSetting = setDefSetting;
                    }
                    else
                    {
                        recSetting = null;
                        recSetting = new RecSettingData();
                        Settings.GetDefRecSetting(item.ID, ref recSetting);
                    }
                    lastSelectIndex = comboBox_preSet.SelectedIndex;
                    UpdateView();
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
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
                    listView_recFolder.Items.Add(new RecFileSetInfoView(info.Clone()));
                }

                checkBox_suspendDef.IsChecked = null;//切り替え時のイベント発生のために必要
                checkBox_suspendDef.IsChecked = (recSetting.SuspendMode == 0);
                checkBox_margineDef.IsChecked = null;//切り替え時のイベント発生のために必要
                checkBox_margineDef.IsChecked = recSetting.UseMargineFlag == 0;
                checkBox_continueRec.IsChecked = (recSetting.ContinueRecFlag == 1);
                checkBox_partial.IsChecked = (recSetting.PartialRecFlag == 1);

                listView_recFolder_1seg.Items.Clear();
                foreach (RecFileSetInfo info in recSetting.PartialRecFolder)
                {
                    listView_recFolder_1seg.Items.Add(new RecFileSetInfoView(info.Clone()));
                }

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
            radioButton_non.IsEnabled = checkBox_suspendDef.IsChecked != true;
            radioButton_standby.IsEnabled = checkBox_suspendDef.IsChecked != true;
            radioButton_suspend.IsEnabled = checkBox_suspendDef.IsChecked != true;
            radioButton_shutdown.IsEnabled = checkBox_suspendDef.IsChecked != true;
            checkBox_reboot.IsEnabled = checkBox_suspendDef.IsChecked != true;

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
            public RecFileSetInfoView(RecFileSetInfo info) { Info = info; }
            public string RecFileName { get { return Info.RecFileName; } }
            public string RecFolder { get { return Info.RecFolder; } }
            public string RecNamePlugIn { get { return Info.RecNamePlugIn; } }
            public string WritePlugIn { get { return Info.WritePlugIn; } }
            public RecFileSetInfo Info { get; private set; }
        }

        private void checkBox_margineDef_Checked(object sender, RoutedEventArgs e)
        {
            textBox_margineStart.IsEnabled = checkBox_margineDef.IsChecked != true;
            textBox_margineEnd.IsEnabled = checkBox_margineDef.IsChecked != true;
            
            RecSettingData recSet = recSetting.Clone();
            recSet.UseMargineFlag = (byte)(checkBox_margineDef.IsChecked == true ? 0 : 1);
            textBox_margineStart.Text = CommonManager.Instance.MUtil.GetMargin(recSet, true).ToString();
            textBox_margineEnd.Text = CommonManager.Instance.MUtil.GetMargin(recSet, false).ToString();
        }

        private void checkBox_serviceMode_Checked(object sender, RoutedEventArgs e)
        {
            checkBox_serviceCaption.IsEnabled = checkBox_serviceMode.IsChecked != true;
            checkBox_serviceData.IsEnabled = checkBox_serviceMode.IsChecked != true;

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
            string path = CommonManager.Instance.GetFileNameByDialog(textBox_bat.Text, "", ".bat");
            if (path != null)
            {
                textBox_bat.Text = path;
            }
        }

        private void button_recFolderAdd_Click(object sender, RoutedEventArgs e)
        {
            recFolderAdd(listView_recFolder);
        }

        private void button_recFolderChg_Click(object sender, RoutedEventArgs e)
        {
            recFolderChange(listView_recFolder);
        }

        private void button_recFolderDel_Click(object sender, RoutedEventArgs e)
        {
            recFolderDelete(listView_recFolder);
        }

        private void listView_recFolder_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            recFolderChange(listView_recFolder);
        }

        private void recFolderAdd(ListBox listbox)
        {
            var setting = new RecFolderWindow();
            PresentationSource topWindow = PresentationSource.FromVisual(this);
            if (topWindow != null)
            {
                setting.Owner = (Window)topWindow.RootVisual;
            }
            if (setting.ShowDialog() == true)
            {
                var setInfo = new RecFileSetInfo();
                setting.GetSetting(ref setInfo);
                foreach (RecFileSetInfoView info in listbox.Items)
                {
                    if (String.Compare(setInfo.RecFolder, info.RecFolder, true) == 0 &&
                        String.Compare(setInfo.WritePlugIn, info.WritePlugIn, true) == 0 &&
                        String.Compare(setInfo.RecNamePlugIn, info.RecNamePlugIn, true) == 0)
                    {
                        MessageBox.Show("すでに追加されています");
                        return;
                    }
                }
                listbox.Items.Add(new RecFileSetInfoView(setInfo));
            }
        }

        private void recFolderChange(ListBox listbox)
        {
            if (listbox.SelectedItem == null)
            {
                if (listbox.Items.Count != 0)
                {
                    listbox.SelectedIndex = 0;
                }
            }
            if (listbox.SelectedItem != null)
            {
                var setting = new RecFolderWindow();
                PresentationSource topWindow = PresentationSource.FromVisual(this);
                if (topWindow != null)
                {
                    setting.Owner = (Window)topWindow.RootVisual;
                }
                var selectInfo = ((RecFileSetInfoView)listbox.SelectedItem).Info;
                setting.SetDefSetting(selectInfo);
                if (setting.ShowDialog() == true)
                {
                    setting.GetSetting(ref selectInfo);
                }
                listbox.Items.Refresh();
            }
            else
            {
                recFolderAdd(listbox);
            }
        }

        private void recFolderDelete(ListBox listbox)
        {
            if (listbox.SelectedItem != null)
            {
                listbox.Items.RemoveAt(listbox.SelectedIndex);
            }
        }

        private void button_del_preset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (comboBox_preSet.SelectedItem != null)
                {
                    RecPresetItem item = comboBox_preSet.SelectedItem as RecPresetItem;
                    if (item.ID == 0)
                    {
                        MessageBox.Show("デフォルトは削除できません");
                        return;
                    }
                    else if (item.ID == 0xFFFFFFFF)
                    {
                        MessageBox.Show("このプリセットは変更できません");
                        return;
                    }
                    else
                    {
                        presetList.Remove(item.ID);
                        comboBox_preSet.Items.Remove(item);

                        lastSelectIndex = -1;
                        comboBox_preSet.SelectedIndex = 0;
                        SavePreset();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_add_preset_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                AddPresetWindow setting = new AddPresetWindow();
                PresentationSource topWindow = PresentationSource.FromVisual(this);
                if (topWindow != null)
                {
                    setting.Owner = (Window)topWindow.RootVisual;
                }
                if (setting.ShowDialog() == true)
                {
                    String name = "";
                    setting.GetName(ref name);
                    AddPreset(name);
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
                    RecPresetItem item = comboBox_preSet.SelectedItem as RecPresetItem;

                    if (item.ID == 0xFFFFFFFF)
                    {
                        MessageBox.Show("このプリセットは変更できません");
                        return;
                    }

                    AddPresetWindow setting = new AddPresetWindow();
                    PresentationSource topWindow = PresentationSource.FromVisual(this);
                    if (topWindow != null)
                    {
                        setting.Owner = (Window)topWindow.RootVisual;
                    }
                    setting.SetMode(true);
                    setting.SetName(item.DisplayName);
                    if (setting.ShowDialog() == true)
                    {
                        String name = "";
                        setting.GetName(ref name);

                        RecSettingData newSet = new RecSettingData();
                        GetRecSetting(ref newSet);
                        item.DisplayName = name;
                        presetList[item.ID] = newSet;

                        SavePreset();

                        comboBox_preSet.Items.Refresh();
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_recFolderChg_1seg_Click(object sender, RoutedEventArgs e)
        {
            recFolderChange(listView_recFolder_1seg);
        }

        private void button_recFolderAdd_1seg_Click(object sender, RoutedEventArgs e)
        {
            recFolderAdd(listView_recFolder_1seg);
        }

        private void button_recFolderDel_1seg_Click(object sender, RoutedEventArgs e)
        {
            recFolderDelete(listView_recFolder_1seg);
        }

        private void listView_recFolder_1seg_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            recFolderChange(listView_recFolder_1seg);
        }

    }
}

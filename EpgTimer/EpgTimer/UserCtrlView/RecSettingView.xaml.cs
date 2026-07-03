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

namespace EpgTimer
{
    /// <summary>
    /// RecSettingView.xaml の相互作用ロジック
    /// </summary>
    public partial class RecSettingView : UserControl
    {
        private RecSettingData setDefSetting;

        public RecSettingView()
        {
            InitializeComponent();

            comboBox_tuner.Items.Add(new TunerSelectInfo("自動", 0));
            foreach (TunerReserveInfo info in CommonManager.Instance.DB.TunerReserveList.Values)
            {
                if (info.tunerID != 0xFFFFFFFF)
                {
                    comboBox_tuner.Items.Add(new TunerSelectInfo(info.tunerName, info.tunerID));
                }
            }
            comboBox_tuner.SelectedIndex = 0;

            foreach (RecPresetItem info in Settings.GetRecPresetList())
            {
                comboBox_preSet.Items.Add(info);
            }
            comboBox_preSet.SelectedIndex = 0;

            if (CommonManager.Instance.NWMode)
            {
                button_bat.IsEnabled = false;
            }
            setDefSetting = Settings.CreateRecSetting(0);
            UpdateView(setDefSetting);
        }

        private void AddPreset(string name)
        {
            RecPresetItem newInfo = new RecPresetItem();
            newInfo.DisplayName = name;
            newInfo.ID = 0;

            int index = comboBox_preSet.Items.Add(newInfo);
            SavePreset(newInfo, GetRecSetting());
            comboBox_preSet.SelectedIndex = index;

        }

        private void SavePreset(object addOrChgTarget, RecSettingData addOrChgInfo)
        {
            var saveList = new List<RecSettingData>();
            for (int i = 0; i < comboBox_preSet.Items.Count; i++)
            {
                RecPresetItem preItem = comboBox_preSet.Items[i] as RecPresetItem;
                if (preItem == addOrChgTarget)
                {
                    // 追加または変更
                    saveList.Add(addOrChgInfo);
                    // IDを振りなおす
                    preItem.ID = (uint)(saveList.Count - 1);
                }
                else if (preItem.ID != 0xFFFFFFFF)
                {
                    // 現在設定を維持
                    saveList.Add(Settings.CreateRecSetting(preItem.ID));
                    // IDを振りなおす
                    preItem.ID = (uint)(saveList.Count - 1);
                }
            }

            if (CommonManager.Instance.NWMode)
            {
                IniFileHandler.TouchFileAsUnicode(SettingPath.TimerSrvIniPath);
            }

            string saveID = "";
            for (int i = 0; i < saveList.Count; i++)
            {
                string defName = "REC_DEF";
                string defFolderName = "REC_DEF_FOLDER";
                string defFolder1SegName = "REC_DEF_FOLDER_1SEG";
                RecSettingData info = saveList[i];

                RecPresetItem preItem = comboBox_preSet.Items.OfType<RecPresetItem>().First(a => a.ID == i);
                if (preItem.ID != 0)
                {
                    defName += preItem.ID.ToString();
                    defFolderName += preItem.ID.ToString();
                    defFolder1SegName += preItem.ID.ToString();
                    saveID += preItem.ID.ToString();
                    saveID += ",";
                }

                IniFileHandler.WritePrivateProfileString(defName, "SetName", preItem.DisplayName, SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "RecMode", (info.IsNoRec() ? 5 : info.GetRecMode()).ToString(), SettingPath.TimerSrvIniPath);
                IniFileHandler.WritePrivateProfileString(defName, "NoRecMode", info.GetRecMode().ToString(), SettingPath.TimerSrvIniPath);
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
            }
            IniFileHandler.WritePrivateProfileString("SET", "PresetID", saveID, SettingPath.TimerSrvIniPath);
        }

        public void SetViewMode(bool epgMode)
        {
            checkBox_tuijyu.IsEnabled = epgMode;
            checkBox_pittari.IsEnabled = epgMode;
        }

        public void SetDefSetting(RecSettingData set)
        {
            RecPresetItem preCust = new RecPresetItem();
            preCust.DisplayName = "登録時";
            preCust.ID = 0xFFFFFFFF;
            int index = comboBox_preSet.Items.Add(preCust);

            setDefSetting = set;
            comboBox_preSet.SelectedIndex = index;

            UpdateView(set);
        }

        public RecSettingData GetRecSetting()
        {
            var setInfo = new RecSettingData();
            setInfo.RecMode = CommonManager.Instance.DB.CombineRecModeAndNoRec((byte)comboBox_recMode.SelectedIndex, checkBox_enabled.IsChecked != true);
            setInfo.Priority = (byte)(comboBox_priority.SelectedIndex + 1);
            setInfo.TuijyuuFlag = (byte)(checkBox_tuijyu.IsChecked == true ? 1 : 0);
            if (checkBox_serviceMode.IsChecked == true)
            {
                setInfo.ServiceMode = 0;
            }
            else
            {
                setInfo.ServiceMode = 1;
                if (checkBox_serviceCaption.IsChecked == true)
                {
                    setInfo.ServiceMode |= 0x10;
                }
                if (checkBox_serviceData.IsChecked == true)
                {
                    setInfo.ServiceMode |= 0x20;
                }
            }
            setInfo.PittariFlag = (byte)(checkBox_pittari.IsChecked == true ? 1 : 0);
            setInfo.BatFilePath = textBox_bat.Text;
            foreach (RecFileSetInfoView view in listView_recFolder.Items)
            {
                (view.PartialRec ? setInfo.PartialRecFolder : setInfo.RecFolderList).Add(view.Info);
            }
            if (checkBox_suspendDef.IsChecked == true)
            {
                setInfo.SuspendMode = 0;
                setInfo.RebootFlag = 0;
            }
            else
            {
                setInfo.SuspendMode = 0;
                if (radioButton_standby.IsChecked == true)
                {
                    setInfo.SuspendMode = 1;
                }
                else if (radioButton_supend.IsChecked == true)
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

                if (checkBox_reboot.IsChecked == true)
                {
                    setInfo.RebootFlag = 1;
                }
                else
                {
                    setInfo.RebootFlag = 0;
                }
            }
            if (checkBox_margineDef.IsChecked == true)
            {
                setInfo.UseMargineFlag = 0;
            }
            else
            {
                setInfo.UseMargineFlag = 1;
                if (textBox_margineStart.Text.Length == 0 || textBox_margineEnd.Text.Length == 0)
                {
                    setInfo.StartMargine = 0;
                    setInfo.EndMargine = 0;
                }
                else
                {
                    int startSec = 0;
                    int startMinus = 1;
                    if (textBox_margineStart.Text.StartsWith("-", StringComparison.Ordinal))
                    {
                        startMinus = -1;
                    }
                    string[] startArray = textBox_margineStart.Text.Split(':');
                    if (startArray.Length == 2)
                    {
                        startSec = Convert.ToInt32(startArray[0]) * 60;
                        startSec += Convert.ToInt32(startArray[1]) * startMinus;
                    }
                    else if (startArray.Length == 3)
                    {
                        startSec = Convert.ToInt32(startArray[0]) * 60 * 60;
                        startSec += Convert.ToInt32(startArray[1]) * 60 * startMinus;
                        startSec += Convert.ToInt32(startArray[2]) * startMinus;
                    }
                    else
                    {
                        startSec = Convert.ToInt32(startArray[0]);
                    }

                    int endSec = 0;
                    int endMinus = 1;
                    if (textBox_margineEnd.Text.StartsWith("-", StringComparison.Ordinal))
                    {
                        endMinus = -1;
                    }
                    string[] endArray = textBox_margineEnd.Text.Split(':');
                    if (endArray.Length == 2)
                    {
                        endSec = Convert.ToInt32(endArray[0]) * 60;
                        endSec += Convert.ToInt32(endArray[1]) * endMinus;
                    }
                    else if (endArray.Length == 3)
                    {
                        endSec = Convert.ToInt32(endArray[0]) * 60 * 60;
                        endSec += Convert.ToInt32(endArray[1]) * 60 * endMinus;
                        endSec += Convert.ToInt32(endArray[2]) * endMinus;
                    }
                    else
                    {
                        endSec = Convert.ToInt32(endArray[0]);
                    }

                    setInfo.StartMargine = startSec;
                    setInfo.EndMargine = endSec;
                }
            }
            if (checkBox_partial.IsChecked == true)
            {
                setInfo.PartialRecFlag = 1;
            }
            else
            {
                setInfo.PartialRecFlag = 0;
            }
            if (checkBox_continueRec.IsChecked == true)
            {
                setInfo.ContinueRecFlag = 1;
            }
            else
            {
                setInfo.ContinueRecFlag = 0;
            }

            TunerSelectInfo tuner = comboBox_tuner.SelectedItem as TunerSelectInfo;
            setInfo.TunerID = tuner.ID;
            return setInfo;
        }

        private void comboBox_preSet_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (comboBox_preSet.SelectedItem != null)
                {
                    RecPresetItem item = comboBox_preSet.SelectedItem as RecPresetItem;
                    UpdateView(item.ID == 0xFFFFFFFF ? setDefSetting : Settings.CreateRecSetting(item.ID));
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void UpdateView(RecSettingData recSetting)
        {
            try
            {
                checkBox_enabled.IsChecked = recSetting.IsNoRec() == false;
                comboBox_recMode.SelectedIndex = recSetting.GetRecMode();
                comboBox_priority.SelectedIndex = Math.Min(Math.Max((int)recSetting.Priority, 1), 5) - 1;
                checkBox_tuijyu.IsChecked = recSetting.TuijyuuFlag != 0;

                if ((recSetting.ServiceMode & 0x01) == 0)
                {
                    checkBox_serviceMode.IsChecked = true;
                    if (CommonManager.Instance.DB.DefaultRecSetting != null)
                    {
                        checkBox_serviceCaption.IsChecked = (CommonManager.Instance.DB.DefaultRecSetting.ServiceMode & 0x10) != 0;
                        checkBox_serviceData.IsChecked = (CommonManager.Instance.DB.DefaultRecSetting.ServiceMode & 0x20) != 0;
                    }
                }
                else
                {
                    checkBox_serviceMode.IsChecked = false;
                    checkBox_serviceCaption.IsChecked = (recSetting.ServiceMode & 0x10) != 0;
                    checkBox_serviceData.IsChecked = (recSetting.ServiceMode & 0x20) != 0;
                }

                checkBox_pittari.IsChecked = recSetting.PittariFlag != 0;

                textBox_bat.Text = recSetting.BatFilePath;

                listView_recFolder.Items.Clear();
                foreach (RecFileSetInfo info in recSetting.RecFolderList)
                {
                    listView_recFolder.Items.Add(new RecFileSetInfoView(GetCopyRecFileSetInfo(info), false));
                }
                foreach (RecFileSetInfo info in recSetting.PartialRecFolder)
                {
                    listView_recFolder.Items.Add(new RecFileSetInfoView(GetCopyRecFileSetInfo(info), true));
                }

                if (recSetting.SuspendMode == 0)
                {
                    checkBox_suspendDef.IsChecked = true;
                    checkBox_reboot.IsChecked = false;
                    if (CommonManager.Instance.DB.DefaultRecSetting != null)
                    {
                        radioButton_standby.IsChecked = CommonManager.Instance.DB.DefaultRecSetting.SuspendMode == 1;
                        radioButton_supend.IsChecked = CommonManager.Instance.DB.DefaultRecSetting.SuspendMode == 2;
                        radioButton_shutdown.IsChecked = CommonManager.Instance.DB.DefaultRecSetting.SuspendMode == 3;
                        radioButton_non.IsChecked = CommonManager.Instance.DB.DefaultRecSetting.SuspendMode == 4;
                        checkBox_reboot.IsChecked = CommonManager.Instance.DB.DefaultRecSetting.RebootFlag != 0;
                    }
                }
                else
                {
                    checkBox_suspendDef.IsChecked = false;
                    radioButton_standby.IsChecked = recSetting.SuspendMode == 1;
                    radioButton_supend.IsChecked = recSetting.SuspendMode == 2;
                    radioButton_shutdown.IsChecked = recSetting.SuspendMode == 3;
                    radioButton_non.IsChecked = recSetting.SuspendMode == 4;
                    checkBox_reboot.IsChecked = recSetting.RebootFlag != 0;
                }
                if (recSetting.UseMargineFlag == 0)
                {
                    checkBox_margineDef.IsChecked = true;
                    if (CommonManager.Instance.DB.DefaultRecSetting != null)
                    {
                        textBox_margineStart.Text = CommonManager.Instance.DB.DefaultRecSetting.StartMargine.ToString();
                        textBox_margineEnd.Text = CommonManager.Instance.DB.DefaultRecSetting.EndMargine.ToString();
                    }
                }
                else
                {
                    checkBox_margineDef.IsChecked = false;
                    textBox_margineStart.Text = recSetting.StartMargine.ToString();
                    textBox_margineEnd.Text = recSetting.EndMargine.ToString();
                }

                if (recSetting.ContinueRecFlag == 1)
                {
                    checkBox_continueRec.IsChecked = true;
                }
                else
                {
                    checkBox_continueRec.IsChecked = false;
                }
                if (recSetting.PartialRecFlag == 1)
                {
                    checkBox_partial.IsChecked = true;
                }
                else
                {
                    checkBox_partial.IsChecked = false;
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
                MessageBox.Show(ex.ToString());
            }
        }

        private RecFileSetInfo GetCopyRecFileSetInfo(RecFileSetInfo info)
        {
            var info_copy = new RecFileSetInfo();
            info_copy.RecFileName = info.RecFileName;
            info_copy.RecFolder = info.RecFolder;
            info_copy.RecNamePlugIn = info.RecNamePlugIn;
            info_copy.WritePlugIn = info.WritePlugIn;
            return info_copy;
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

        private void button_bat_Click(object sender, RoutedEventArgs e)
        {
            Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();
            dlg.DefaultExt = ".bat";
            dlg.Filter = "batch Files(*.bat;*.ps1;*.lua)|*.bat;*.ps1;*.lua|all Files(*.*)|*.*";

            Nullable<bool> result = dlg.ShowDialog();
            if (result == true)
            {
                textBox_bat.Text = dlg.FileName;
            }
        }

        private void button_recFolderAdd_Click(object sender, RoutedEventArgs e)
        {
            RecFolderWindow setting = new RecFolderWindow();
            PresentationSource topWindow = PresentationSource.FromVisual(this);
            if (topWindow != null)
            {
                setting.Owner = (Window)topWindow.RootVisual;
            }
            if (setting.ShowDialog() == true)
            {
                RecFileSetInfo setInfo = setting.GetSetting();
                foreach (RecFileSetInfoView info in listView_recFolder.Items)
                {
                    if (info.PartialRec == false &&
                        setInfo.RecFolder.Equals(info.RecFolder, StringComparison.OrdinalIgnoreCase) &&
                        setInfo.WritePlugIn.Equals(info.WritePlugIn, StringComparison.OrdinalIgnoreCase) &&
                        setInfo.RecNamePlugIn.Equals(info.RecNamePlugIn, StringComparison.OrdinalIgnoreCase))
                    {
                        MessageBox.Show("すでに追加されています");
                        return;
                    }
                }
                listView_recFolder.Items.Add(new RecFileSetInfoView(setInfo, false));
            }

        }

        private void button_recFolderChg_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recFolder.SelectedItem != null)
            {
                RecFolderWindow setting = new RecFolderWindow();
                PresentationSource topWindow = PresentationSource.FromVisual(this);
                if (topWindow != null)
                {
                    setting.Owner = (Window)topWindow.RootVisual;
                }
                var item = (RecFileSetInfoView)listView_recFolder.SelectedItem;
                setting.SetDefSetting(item.Info);
                if (setting.ShowDialog() == true)
                {
                    listView_recFolder.SelectedItem =
                        listView_recFolder.Items[listView_recFolder.SelectedIndex] = new RecFileSetInfoView(setting.GetSetting(), item.PartialRec);
                }
            }

        }
        
        private void button_recFolderDel_Click(object sender, RoutedEventArgs e)
        {
            if (listView_recFolder.SelectedItem != null)
            {
                listView_recFolder.Items.RemoveAt(listView_recFolder.SelectedIndex);
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
                        comboBox_preSet.Items.Remove(item);
                        comboBox_preSet.SelectedIndex = 0;
                        SavePreset(null, null);
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
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
                    AddPreset(setting.PresetName);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
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
                    setting.PresetName = item.DisplayName;
                    if (setting.ShowDialog() == true)
                    {
                        item.DisplayName = setting.PresetName;
                        SavePreset(item, GetRecSetting());

                        comboBox_preSet.Items.Refresh();
                        comboBox_preSet.SelectedItem = null;
                        comboBox_preSet.SelectedItem = item;
                    }
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.ToString());
            }
        }

        private void button_recFolderAdd_1seg_Click(object sender, RoutedEventArgs e)
        {
            RecFolderWindow setting = new RecFolderWindow();
            PresentationSource topWindow = PresentationSource.FromVisual(this);
            if (topWindow != null)
            {
                setting.Owner = (Window)topWindow.RootVisual;
            }
            if (setting.ShowDialog() == true)
            {
                RecFileSetInfo setInfo = setting.GetSetting();
                foreach (RecFileSetInfoView info in listView_recFolder.Items)
                {
                    if (info.PartialRec &&
                        setInfo.RecFolder.Equals(info.RecFolder, StringComparison.OrdinalIgnoreCase) &&
                        setInfo.WritePlugIn.Equals(info.WritePlugIn, StringComparison.OrdinalIgnoreCase) &&
                        setInfo.RecNamePlugIn.Equals(info.RecNamePlugIn, StringComparison.OrdinalIgnoreCase))
                    {
                        MessageBox.Show("すでに追加されています");
                        return;
                    }
                }
                listView_recFolder.Items.Add(new RecFileSetInfoView(setInfo, true));
            }
        }


    }
}

using System;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    /// <summary>
    /// ConnectWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class ConnectWindow : Window
    {
        private MenuUtil mutil = CommonManager.Instance.MUtil;
        private const string DefPresetStr = "前回接続時";

        public ConnectWindow()
        {
            InitializeComponent();

            try
            {
                var nowSet = new NWPresetItem(DefPresetStr, Settings.Instance.NWServerIP, Settings.Instance.NWServerPort, Settings.Instance.NWWaitPort, Settings.Instance.NWMacAdd);
                cmb_preset.Items.Add(nowSet);
                Settings.Instance.NWPerset.ForEach(item => cmb_preset.Items.Add(item.Clone()));
                cmb_preset.SelectedIndex = FindCmbPresetItem(nowSet, true);
            }
            catch { }
        }

        private void button_connect_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                NWPresetItem data = GetSetting();
                Settings.Instance.NWServerIP = data.NWServerIP;
                Settings.Instance.NWServerPort = data.NWServerPort;
                Settings.Instance.NWWaitPort = data.NWWaitPort;
                Settings.Instance.NWMacAdd = data.NWMacAdd;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                return;
            }

            DialogResult = true;
        }

        private void button_wake_Click(object sender, RoutedEventArgs e)
        {
            byte[] macAddress = Enumerable.Repeat<byte>(0xFF, 6).ToArray();

            try
            {
                string[] mac = textBox_mac.Text.Split('-');
                for (int i = 0; i < Math.Max(mac.Length, 6); i++)
                {
                    macAddress[i] = Convert.ToByte(mac[i], 16);
                }

            }
            catch
            {
                MessageBox.Show("書式が間違っているか、16進アドレスの数値が読み取れません。");
                return;
            }

            Settings.Instance.NWMacAdd = textBox_mac.Text;
            NWConnect.SendMagicPacket(macAddress);
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Escape:
                        this.Close();
                        break;
                }
            }
            base.OnKeyDown(e);
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            button_connect.Focus();
        }

        private void SetSetting(NWPresetItem data)
        {
            if (data != null)
            {
                textBox_srvIP.Text = data.NWServerIP;
                textBox_srvPort.Text = data.NWServerPort.ToString();
                textBox_clientPort.Text = data.NWWaitPort.ToString();
                textBox_mac.Text = data.NWMacAdd;
            }
        }
        private NWPresetItem GetSetting()
        {
            var preset = new NWPresetItem();
            preset.NWServerIP = textBox_srvIP.Text;
            preset.NWServerPort = mutil.MyToNumerical(textBox_srvPort, Convert.ToUInt32, Settings.Instance.NWServerPort);
            preset.NWWaitPort = mutil.MyToNumerical(textBox_clientPort, Convert.ToUInt32, Settings.Instance.NWWaitPort);
            preset.NWMacAdd = textBox_mac.Text;
            return preset;
        }

        private void cmb_preset_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            SetSetting(cmb_preset.SelectedItem as NWPresetItem);
        }
        private int FindCmbPresetItem(NWPresetItem preset, bool ignoreName = true)
        {
            return cmb_preset.Items.OfType<NWPresetItem>().ToList().FindLastIndex(item => item.EqualsTo(preset, ignoreName));
        }
        private int FindCmbPresetItem(string name)
        {
            return cmb_preset.Items.OfType<NWPresetItem>().ToList().FindLastIndex(item => item.Name == name);
        }

        private void btn_reload_Click(object sender, RoutedEventArgs e)
        {
            SetSetting(cmb_preset.SelectedItem as NWPresetItem);
        }
        private void btn_add_Click(object sender, RoutedEventArgs e)
        {
            if (IsCmbTextInvalid() == true) return;//デフォルトアイテム

            NWPresetItem newitem = GetSetting();
            newitem.Name = cmb_preset.Text.Trim();

            int pos = FindCmbPresetItem(cmb_preset.Text.Trim());
            if (pos >= 0)
            {
                cmb_preset.Items[pos] = newitem;
                cmb_preset.SelectedIndex = pos;
            }
            else
            {
                cmb_preset.Items.Add(newitem);
                cmb_preset.SelectedIndex = cmb_preset.Items.Count - 1;
            }
        }
        private void btn_delete_Click(object sender, RoutedEventArgs e)
        {
            if (IsCmbTextInvalid() == true) return;//デフォルトアイテム

            int pos = FindCmbPresetItem(cmb_preset.Text.Trim());
            if (pos >= 0)
            {
                cmb_preset.Items.RemoveAt(pos);
                cmb_preset.SelectedIndex = Math.Min(pos, cmb_preset.Items.Count - 1);
            }
        }
        private bool IsCmbTextInvalid()
        {
            return cmb_preset.Text == null || cmb_preset.Text.Trim() == "" || cmb_preset.Text.Trim() == DefPresetStr;
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            Settings.Instance.NWPerset = cmb_preset.Items.OfType<NWPresetItem>().Skip(1).ToList();
            Settings.SaveToXmlFile();
        }

    }
}

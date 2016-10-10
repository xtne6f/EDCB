using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer.Setting
{
    using BoxExchangeEdit;

    /// <summary>
    /// SetDefRecSettingWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SetDefRecSettingWindow : Window
    {
        public SetDefRecSettingWindow()
        {
            InitializeComponent();

            button_ok.IsEnabled = !CommonManager.Instance.NWMode;

            //ビュー側の簡易編集用のバーを非表示に
            this.recSettingView.SetChangeMode(2);

            //リストボックスの設定
            var bx = new BoxExchangeEditor(null, listBox_preset, true, true, true);
            button_up.Click += bx.button_Up_Click;
            button_down.Click += bx.button_Down_Click;
            button_top.Click += bx.button_Top_Click;
            button_bottom.Click += bx.button_Bottom_Click;
            button_del.Click += bx.button_Delete_Click;
        }

        public void SetSettingMode(string title, int chgMode = -1)
        {
            Title = title;
            grid_PresetEdit.Visibility = System.Windows.Visibility.Collapsed;
            button_iniLoad.Visibility = System.Windows.Visibility.Collapsed;
            this.recSettingView.SetChangeMode(chgMode);
        }

        public void SetPresetList(List<RecPresetItem> srclist)
        {
            listBox_preset.Items.Clear();
            listBox_preset.Items.AddItems(srclist.Clone());
            listBox_preset.SelectedIndex = 0;
        }
        public List<RecPresetItem> GetPresetList()
        {
            List<RecPresetItem> list = listBox_preset.Items.OfType<RecPresetItem>().ToList().Clone();
            RecPresetItem.ChecknFixRecPresetList(ref list);
            return list;
        }
        private void button_iniLoad_Click(object sender, RoutedEventArgs e)
        {
            SetPresetList(Settings.Instance.RecPresetList);
        }

        RecPresetItem currentItem = null;
        private void listBox_preset_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (currentItem != null)
            {
                currentItem.RecPresetData = recSettingView.GetRecSetting();
            }

            currentItem = listBox_preset.SelectedItem as RecPresetItem;
            if (currentItem == null) return;

            recSettingView.SetDefSetting(currentItem.RecPresetData);
            noTextChange = true;//リストボックス内をキーボードで操作しているときのフォーカス対策
            textBox_preset.Text = currentItem.DisplayName;
            noTextChange = false;
        }

        bool noTextChange = false;
        private void textBox_preset_TextChanged(object sender, TextChangedEventArgs e)
        {
            if (currentItem == null || noTextChange == true) return;
            currentItem.DisplayName = textBox_preset.Text;
            listBox_preset.Items.Refresh();
        }

        private void button_add_Click(object sender, RoutedEventArgs e)
        {
            listBox_preset.Items.Add(new RecPresetItem(textBox_preset.Text, 0, recSettingView.GetRecSetting()));
            listBox_preset.SelectedIndex = listBox_preset.Items.Count - 1;
            listBox_preset.ScrollIntoView(listBox_preset.SelectedItem);
        }

        private void button_ok_Click(object sender, RoutedEventArgs e)
        {
            listBox_preset.SelectedIndex = -1;
            DialogResult = true;
        }
        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
    }
}

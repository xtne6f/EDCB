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
using System.Windows.Markup;

namespace EpgTimer.Setting
{
    /// <summary>
    /// SetEpgView.xaml の相互作用ロジック
    /// </summary>
    public partial class SetEpgView : UserControl
    {
        private bool initializingColor;

        public SetEpgView()
        {
            InitializeComponent();

            {
                foreach (FontFamily family in Fonts.SystemFontFamilies)
                {
                    string s;
                    if (family.FamilyNames.TryGetValue(XmlLanguage.GetLanguage("ja-JP"), out s))
                    {
                        comboBox_font.Items.Add(s);
                        comboBox_fontTitle.Items.Add(s);
                    }
                }
                //ローカル名がなくても一応候補に加える
                foreach (FontFamily family in Fonts.SystemFontFamilies)
                {
                    if (family.FamilyNames.ContainsKey(XmlLanguage.GetLanguage("ja-JP")) == false)
                    {
                        comboBox_font.Items.Add(family.Source);
                        comboBox_fontTitle.Items.Add(family.Source);
                    }
                }
            }
        }

        private void UserControl_DataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            var settings = (Settings)DataContext;
            if (settings == null)
            {
                grid_basic.DataContext = null;
                grid_color.DataContext = null;
            }
            else
            {
                grid_basic.DataContext = settings.EpgSettingList[radioButton_basicDesignSub1.IsChecked == true ? 1 :
                                                                 radioButton_basicDesignSub2.IsChecked == true ? 2 : 0];
                grid_color.DataContext = settings.EpgSettingList[radioButton_colorDesignSub1.IsChecked == true ? 1 :
                                                                 radioButton_colorDesignSub2.IsChecked == true ? 2 : 0];
                listBox_tab.Items.Clear();
                foreach (CustomEpgTabInfo info in settings.CustomEpgTabList)
                {
                    listBox_tab.Items.Add(info);
                }
                if (listBox_tab.Items.Count > 0)
                {
                    listBox_tab.SelectedIndex = 0;
                }
            }
        }

        private void grid_basic_DataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            var epgSetting = (EpgSetting)grid_basic.DataContext;
            if (epgSetting != null)
            {
                initializingColor = true;
                comboBox_colorTitle1.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.TitleColor1));
                comboBox_colorTitle2.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.TitleColor2));
                initializingColor = false;
                button_colorTitle1.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.TitleCustColor1));
                button_colorTitle2.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.TitleCustColor2));
            }
        }

        private void grid_color_DataContextChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            var epgSetting = (EpgSetting)grid_color.DataContext;
            if (epgSetting != null)
            {
                initializingColor = true;
                comboBox0.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x00]));
                comboBox1.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x01]));
                comboBox2.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x02]));
                comboBox3.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x03]));
                comboBox4.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x04]));
                comboBox5.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x05]));
                comboBox6.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x06]));
                comboBox7.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x07]));
                comboBox8.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x08]));
                comboBox9.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x09]));
                comboBox10.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x0A]));
                comboBox11.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x0B]));
                comboBox12.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x0F]));
                comboBox13.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ContentColorList[0x10]));
                comboBox_service.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ServiceColor));
                comboBox_reserveNormal.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ReserveRectColorNormal));
                comboBox_reserveNo.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ReserveRectColorNo));
                comboBox_reserveNoTuner.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ReserveRectColorNoTuner));
                comboBox_reserveWarning.SelectedIndex = Math.Max(0, ColorDef.BrushNames.IndexOfKey(epgSetting.ReserveRectColorWarning));
                initializingColor = false;
                button0.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x00]));
                button1.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x01]));
                button2.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x02]));
                button3.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x03]));
                button4.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x04]));
                button5.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x05]));
                button6.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x06]));
                button7.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x07]));
                button8.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x08]));
                button9.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x09]));
                button10.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x0A]));
                button11.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x0B]));
                button12.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x0F]));
                button13.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x10]));
                button_service.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ServiceCustColor));
                button_epgBack.Background = new SolidColorBrush(Color.FromRgb(epgSetting.EpgBackColorR, epgSetting.EpgBackColorG, epgSetting.EpgBackColorB));
                button14.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x11]));
                button15.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x12]));
                button16.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x13]));
                button17.Background = new SolidColorBrush(ColorDef.FromUInt(epgSetting.ContentCustColorList[0x14]));
            }
        }

        private void OnUpdateColor()
        {
            var epgSetting = (EpgSetting)grid_basic.DataContext;
            if (epgSetting != null && initializingColor == false)
            {
                epgSetting.TitleColor1 = ((KeyValuePair<string, SolidColorBrush>)comboBox_colorTitle1.SelectedItem).Key;
                epgSetting.TitleColor2 = ((KeyValuePair<string, SolidColorBrush>)comboBox_colorTitle2.SelectedItem).Key;
                epgSetting.TitleCustColor1 = ColorDef.ToUInt(((SolidColorBrush)button_colorTitle1.Background).Color);
                epgSetting.TitleCustColor2 = ColorDef.ToUInt(((SolidColorBrush)button_colorTitle2.Background).Color);
            }
            epgSetting = (EpgSetting)grid_color.DataContext;
            if (epgSetting != null && initializingColor == false)
            {
                epgSetting.ContentColorList[0x00] = ((KeyValuePair<string, SolidColorBrush>)comboBox0.SelectedItem).Key;
                epgSetting.ContentColorList[0x01] = ((KeyValuePair<string, SolidColorBrush>)comboBox1.SelectedItem).Key;
                epgSetting.ContentColorList[0x02] = ((KeyValuePair<string, SolidColorBrush>)comboBox2.SelectedItem).Key;
                epgSetting.ContentColorList[0x03] = ((KeyValuePair<string, SolidColorBrush>)comboBox3.SelectedItem).Key;
                epgSetting.ContentColorList[0x04] = ((KeyValuePair<string, SolidColorBrush>)comboBox4.SelectedItem).Key;
                epgSetting.ContentColorList[0x05] = ((KeyValuePair<string, SolidColorBrush>)comboBox5.SelectedItem).Key;
                epgSetting.ContentColorList[0x06] = ((KeyValuePair<string, SolidColorBrush>)comboBox6.SelectedItem).Key;
                epgSetting.ContentColorList[0x07] = ((KeyValuePair<string, SolidColorBrush>)comboBox7.SelectedItem).Key;
                epgSetting.ContentColorList[0x08] = ((KeyValuePair<string, SolidColorBrush>)comboBox8.SelectedItem).Key;
                epgSetting.ContentColorList[0x09] = ((KeyValuePair<string, SolidColorBrush>)comboBox9.SelectedItem).Key;
                epgSetting.ContentColorList[0x0A] = ((KeyValuePair<string, SolidColorBrush>)comboBox10.SelectedItem).Key;
                epgSetting.ContentColorList[0x0B] = ((KeyValuePair<string, SolidColorBrush>)comboBox11.SelectedItem).Key;
                epgSetting.ContentColorList[0x0F] = ((KeyValuePair<string, SolidColorBrush>)comboBox12.SelectedItem).Key;
                epgSetting.ContentColorList[0x10] = ((KeyValuePair<string, SolidColorBrush>)comboBox13.SelectedItem).Key;
                epgSetting.ServiceColor = ((KeyValuePair<string, SolidColorBrush>)comboBox_service.SelectedItem).Key;
                epgSetting.ReserveRectColorNormal = ((KeyValuePair<string, SolidColorBrush>)comboBox_reserveNormal.SelectedItem).Key;
                epgSetting.ReserveRectColorNo = ((KeyValuePair<string, SolidColorBrush>)comboBox_reserveNo.SelectedItem).Key;
                epgSetting.ReserveRectColorNoTuner = ((KeyValuePair<string, SolidColorBrush>)comboBox_reserveNoTuner.SelectedItem).Key;
                epgSetting.ReserveRectColorWarning = ((KeyValuePair<string, SolidColorBrush>)comboBox_reserveWarning.SelectedItem).Key;
                epgSetting.ContentCustColorList[0x00] = ColorDef.ToUInt(((SolidColorBrush)button0.Background).Color);
                epgSetting.ContentCustColorList[0x01] = ColorDef.ToUInt(((SolidColorBrush)button1.Background).Color);
                epgSetting.ContentCustColorList[0x02] = ColorDef.ToUInt(((SolidColorBrush)button2.Background).Color);
                epgSetting.ContentCustColorList[0x03] = ColorDef.ToUInt(((SolidColorBrush)button3.Background).Color);
                epgSetting.ContentCustColorList[0x04] = ColorDef.ToUInt(((SolidColorBrush)button4.Background).Color);
                epgSetting.ContentCustColorList[0x05] = ColorDef.ToUInt(((SolidColorBrush)button5.Background).Color);
                epgSetting.ContentCustColorList[0x06] = ColorDef.ToUInt(((SolidColorBrush)button6.Background).Color);
                epgSetting.ContentCustColorList[0x07] = ColorDef.ToUInt(((SolidColorBrush)button7.Background).Color);
                epgSetting.ContentCustColorList[0x08] = ColorDef.ToUInt(((SolidColorBrush)button8.Background).Color);
                epgSetting.ContentCustColorList[0x09] = ColorDef.ToUInt(((SolidColorBrush)button9.Background).Color);
                epgSetting.ContentCustColorList[0x0A] = ColorDef.ToUInt(((SolidColorBrush)button10.Background).Color);
                epgSetting.ContentCustColorList[0x0B] = ColorDef.ToUInt(((SolidColorBrush)button11.Background).Color);
                epgSetting.ContentCustColorList[0x0F] = ColorDef.ToUInt(((SolidColorBrush)button12.Background).Color);
                epgSetting.ContentCustColorList[0x10] = ColorDef.ToUInt(((SolidColorBrush)button13.Background).Color);
                epgSetting.ServiceCustColor = ColorDef.ToUInt(((SolidColorBrush)button_service.Background).Color);
                epgSetting.EpgBackColorR = ((SolidColorBrush)button_epgBack.Background).Color.R;
                epgSetting.EpgBackColorG = ((SolidColorBrush)button_epgBack.Background).Color.G;
                epgSetting.EpgBackColorB = ((SolidColorBrush)button_epgBack.Background).Color.B;
                epgSetting.ContentCustColorList[0x11] = ColorDef.ToUInt(((SolidColorBrush)button14.Background).Color);
                epgSetting.ContentCustColorList[0x12] = ColorDef.ToUInt(((SolidColorBrush)button15.Background).Color);
                epgSetting.ContentCustColorList[0x13] = ColorDef.ToUInt(((SolidColorBrush)button16.Background).Color);
                epgSetting.ContentCustColorList[0x14] = ColorDef.ToUInt(((SolidColorBrush)button17.Background).Color);
            }
        }

        private void OnUpdateTabListBox()
        {
            ((Settings)DataContext).CustomEpgTabList.Clear();
            ((Settings)DataContext).CustomEpgTabList.AddRange(listBox_tab.Items.OfType<CustomEpgTabInfo>());
        }

        private void button_tab_add_Click(object sender, RoutedEventArgs e)
        {
            EpgDataViewSettingWindow dlg = new EpgDataViewSettingWindow();
            PresentationSource topWindow = PresentationSource.FromVisual(this);
            if (topWindow != null)
            {
                dlg.Owner = (Window)topWindow.RootVisual;
            }
            if (dlg.ShowDialog() == true)
            {
                listBox_tab.Items.Add(dlg.GetSetting());
                OnUpdateTabListBox();
            }
        }

        private void button_tab_chg_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_tab.SelectedItem != null)
            {
                EpgDataViewSettingWindow dlg = new EpgDataViewSettingWindow();
                PresentationSource topWindow = PresentationSource.FromVisual(this);
                if (topWindow != null)
                {
                    dlg.Owner = (Window)topWindow.RootVisual;
                }
                dlg.SetDefSetting((CustomEpgTabInfo)listBox_tab.SelectedItem);
                if (dlg.ShowDialog() == true)
                {
                    listBox_tab.SelectedItem = listBox_tab.Items[listBox_tab.SelectedIndex] = dlg.GetSetting();
                    OnUpdateTabListBox();
                }
            }
            else
            {
                MessageBox.Show("アイテムが選択されていません");
            }
        }

        private void button_tab_del_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_tab.SelectedItem != null)
            {
                listBox_tab.Items.RemoveAt(listBox_tab.SelectedIndex);
                if (listBox_tab.Items.Count > 0)
                {
                    listBox_tab.SelectedIndex = 0;
                }
                OnUpdateTabListBox();
            }
            else
            {
                MessageBox.Show("アイテムが選択されていません");
            }
        }

        private void button_tab_up_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_tab.SelectedItem != null)
            {
                if (listBox_tab.SelectedIndex >= 1)
                {
                    object temp = listBox_tab.SelectedItem;
                    int index = listBox_tab.SelectedIndex;
                    listBox_tab.Items.RemoveAt(listBox_tab.SelectedIndex);
                    listBox_tab.Items.Insert(index - 1, temp);
                    listBox_tab.SelectedIndex = index - 1;
                    OnUpdateTabListBox();
                }
            }
        }

        private void buttontab_down_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_tab.SelectedItem != null)
            {
                if (listBox_tab.SelectedIndex < listBox_tab.Items.Count - 1)
                {
                    object temp = listBox_tab.SelectedItem;
                    int index = listBox_tab.SelectedIndex;
                    listBox_tab.Items.RemoveAt(listBox_tab.SelectedIndex);
                    listBox_tab.Items.Insert(index + 1, temp);
                    listBox_tab.SelectedIndex = index + 1;
                    OnUpdateTabListBox();
                }
            }
        }

        private void button_Color_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = ((SolidColorBrush)((Button)sender).Background).Color;
            dlg.A = item.A;
            dlg.R = item.R;
            dlg.G = item.G;
            dlg.B = item.B;
            if (dlg.ShowDialog() == true)
            {
                ((SolidColorBrush)((Button)sender).Background).Color = Color.FromArgb(dlg.A, dlg.R, dlg.G, dlg.B);
                OnUpdateColor();
            }
        }

        private void comboBox_color_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            OnUpdateColor();
        }

        private void radioButton_basicDesign_Checked(object sender, RoutedEventArgs e)
        {
            Settings settings = (Settings)DataContext;
            if (settings != null)
            {
                grid_basic.DataContext = settings.EpgSettingList[radioButton_basicDesignSub1.IsChecked == true ? 1 :
                                                                 radioButton_basicDesignSub2.IsChecked == true ? 2 : 0];
                if (radioButton_colorDesignDefault.IsChecked != radioButton_basicDesignDefault.IsChecked ||
                    radioButton_colorDesignSub1.IsChecked != radioButton_basicDesignSub1.IsChecked ||
                    radioButton_colorDesignSub2.IsChecked != radioButton_basicDesignSub2.IsChecked)
                {
                    radioButton_colorDesignDefault.IsChecked = radioButton_basicDesignDefault.IsChecked;
                    radioButton_colorDesignSub1.IsChecked = radioButton_basicDesignSub1.IsChecked;
                    radioButton_colorDesignSub2.IsChecked = radioButton_basicDesignSub2.IsChecked;
                }
            }
        }

        private void radioButton_colorDesign_Checked(object sender, RoutedEventArgs e)
        {
            Settings settings = (Settings)DataContext;
            if (settings != null)
            {
                grid_color.DataContext = settings.EpgSettingList[radioButton_colorDesignSub1.IsChecked == true ? 1 :
                                                                 radioButton_colorDesignSub2.IsChecked == true ? 2 : 0];
                if (radioButton_basicDesignDefault.IsChecked != radioButton_colorDesignDefault.IsChecked ||
                    radioButton_basicDesignSub1.IsChecked != radioButton_colorDesignSub1.IsChecked ||
                    radioButton_basicDesignSub2.IsChecked != radioButton_colorDesignSub2.IsChecked)
                {
                    radioButton_basicDesignDefault.IsChecked = radioButton_colorDesignDefault.IsChecked;
                    radioButton_basicDesignSub1.IsChecked = radioButton_colorDesignSub1.IsChecked;
                    radioButton_basicDesignSub2.IsChecked = radioButton_colorDesignSub2.IsChecked;
                }
            }
        }
    }
}

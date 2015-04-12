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
        public SetEpgView()
        {
            InitializeComponent();

            try
            {
                var colorList = new SortedList<string, SolidColorBrush>();
                foreach (var prop in typeof(Brushes).GetProperties())
                {
                    colorList.Add(prop.Name, (SolidColorBrush)prop.GetValue(null, null));
                }
                colorList.Add("カスタム", Brushes.Transparent);
                comboBox0.DataContext = colorList;
                comboBox1.DataContext = colorList;
                comboBox2.DataContext = colorList;
                comboBox3.DataContext = colorList;
                comboBox4.DataContext = colorList;
                comboBox5.DataContext = colorList;
                comboBox6.DataContext = colorList;
                comboBox7.DataContext = colorList;
                comboBox8.DataContext = colorList;
                comboBox9.DataContext = colorList;
                comboBox10.DataContext = colorList;
                comboBox11.DataContext = colorList;
                comboBox12.DataContext = colorList;
                comboBox13.DataContext = colorList;
                comboBox_reserveNormal.DataContext = colorList;
                comboBox_reserveNo.DataContext = colorList;
                comboBox_reserveNoTuner.DataContext = colorList;
                comboBox_reserveWarning.DataContext = colorList;
                comboBox_colorTitle1.DataContext = colorList;
                comboBox_colorTitle2.DataContext = colorList;

                comboBox0.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x00]));
                comboBox1.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x01]));
                comboBox2.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x02]));
                comboBox3.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x03]));
                comboBox4.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x04]));
                comboBox5.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x05]));
                comboBox6.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x06]));
                comboBox7.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x07]));
                comboBox8.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x08]));
                comboBox9.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x09]));
                comboBox10.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x0A]));
                comboBox11.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x0B]));
                comboBox12.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x0F]));
                comboBox13.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ContentColorList[0x10]));

                comboBox_reserveNormal.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ReserveRectColorNormal));
                comboBox_reserveNo.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ReserveRectColorNo));
                comboBox_reserveNoTuner.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ReserveRectColorNoTuner));
                comboBox_reserveWarning.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.ReserveRectColorWarning));
                checkBox_reserveBackground.IsChecked = Settings.Instance.ReserveRectBackground;

                comboBox_colorTitle1.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.TitleColor1));
                comboBox_colorTitle2.SelectedIndex = Math.Max(0, colorList.IndexOfKey(Settings.Instance.TitleColor2));

                foreach (FontFamily family in Fonts.SystemFontFamilies)
                {
                    LanguageSpecificStringDictionary dictionary = family.FamilyNames;

                    XmlLanguage FLanguage = XmlLanguage.GetLanguage("ja-JP");
                    if (dictionary.ContainsKey(FLanguage) == true)
                    {
                        string s = dictionary[FLanguage] as string;
                        comboBox_font.Items.Add(s);
                        if (String.Compare(s, Settings.Instance.FontName) == 0)
                        {
                            comboBox_font.SelectedItem = s;
                        }
                        comboBox_fontTitle.Items.Add(s);
                        if (String.Compare(s, Settings.Instance.FontNameTitle) == 0)
                        {
                            comboBox_fontTitle.SelectedItem = s;
                        }
                    }
                }
                if (comboBox_font.SelectedItem == null)
                {
                    comboBox_font.SelectedIndex = 0;
                }
                if (comboBox_fontTitle.SelectedItem == null)
                {
                    comboBox_fontTitle.SelectedIndex = 0;
                }
                textBox_fontSize.Text = Settings.Instance.FontSize.ToString();
                textBox_fontSizeTitle.Text = Settings.Instance.FontSizeTitle.ToString();
                checkBox_fontBoldTitle.IsChecked = Settings.Instance.FontBoldTitle;

                textBox_mouse_scroll.Text = Settings.Instance.ScrollSize.ToString();
                textBox_minHeight.Text = Settings.Instance.MinHeight.ToString();
                textBox_service_width.Text = Settings.Instance.ServiceWidth.ToString();
                textBox_dragScroll.Text = Settings.Instance.DragScroll.ToString();
                textBox_minimumHeight.Text = Settings.Instance.MinimumHeight.ToString();
                checkBox_descToolTip.IsChecked = Settings.Instance.EpgToolTip;
                checkBox_title_indent.IsChecked = Settings.Instance.EpgTitleIndent;
                checkBox_toolTip_noView_only.IsChecked = Settings.Instance.EpgToolTipNoViewOnly;
                textBox_toolTipWait.Text = Settings.Instance.EpgToolTipViewWait.ToString();
                checkBox_epg_popup.IsChecked = Settings.Instance.EpgPopup;
                checkBox_gradation.IsChecked = Settings.Instance.EpgGradation;
                checkBox_gradationHeader.IsChecked = Settings.Instance.EpgGradationHeader;

                if (Settings.Instance.UseCustomEpgView == false)
                {
                    radioButton_1_def.IsChecked = true;
                    radioButton_1_cust.IsChecked = false;
                }
                else
                {
                    radioButton_1_def.IsChecked = false;
                    radioButton_1_cust.IsChecked = true;
                }
                foreach (CustomEpgTabInfo info in Settings.Instance.CustomEpgTabList)
                {
                    listBox_tab.Items.Add(info);
                }
                if (listBox_tab.Items.Count > 0)
                {
                    listBox_tab.SelectedIndex = 0;
                }

                button0.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x00]));
                button1.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x01]));
                button2.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x02]));
                button3.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x03]));
                button4.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x04]));
                button5.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x05]));
                button6.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x06]));
                button7.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x07]));
                button8.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x08]));
                button9.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x09]));
                button10.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x0A]));
                button11.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x0B]));
                button12.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x0F]));
                button13.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x10]));
                button14.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x11]));
                button15.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x12]));
                button16.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x13]));
                button17.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.ContentCustColorList[0x14]));

                button_colorTitle1.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.TitleCustColor1));
                button_colorTitle2.Background = new SolidColorBrush(ColorDef.FromUInt(Settings.Instance.TitleCustColor2));

                checkBox_singleOpen.IsChecked = Settings.Instance.EpgInfoSingleClick;
                if (Settings.Instance.EpgInfoOpenMode == 0)
                {
                    checkBox_openInfo.IsChecked = false;
                }
                else
                {
                    checkBox_openInfo.IsChecked = true;
                }

                checkBox_scrollAuto.IsChecked = Settings.Instance.MouseScrollAuto;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        public void SaveSetting()
        {
            try
            {
                Settings.Instance.ScrollSize = Convert.ToDouble(textBox_mouse_scroll.Text);
                Settings.Instance.MinHeight = Convert.ToDouble(textBox_minHeight.Text);
                Settings.Instance.ServiceWidth = Convert.ToDouble(textBox_service_width.Text);
                Settings.Instance.DragScroll = Convert.ToDouble(textBox_dragScroll.Text);
                Settings.Instance.MinimumHeight = Convert.ToDouble(textBox_minimumHeight.Text);
                if (checkBox_title_indent.IsChecked == true)
                {
                    Settings.Instance.EpgTitleIndent = true;
                }
                else
                {
                    Settings.Instance.EpgTitleIndent = false;
                }
                Settings.Instance.EpgToolTip = (checkBox_descToolTip.IsChecked == true);
                Settings.Instance.EpgToolTipNoViewOnly = (checkBox_toolTip_noView_only.IsChecked == true);
                Settings.Instance.EpgToolTipViewWait = Convert.ToInt32(textBox_toolTipWait.Text);
                Settings.Instance.EpgPopup = (checkBox_epg_popup.IsChecked == true);
                if (checkBox_gradation.IsChecked == true)
                {
                    Settings.Instance.EpgGradation = true;
                }
                else
                {
                    Settings.Instance.EpgGradation = false;
                }
                if (checkBox_gradationHeader.IsChecked == true)
                {
                    Settings.Instance.EpgGradationHeader = true;
                }
                else
                {
                    Settings.Instance.EpgGradationHeader = false;
                }

                Settings.Instance.ContentColorList[0x00] = ((KeyValuePair<string, SolidColorBrush>)comboBox0.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x01] = ((KeyValuePair<string, SolidColorBrush>)comboBox1.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x02] = ((KeyValuePair<string, SolidColorBrush>)comboBox2.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x03] = ((KeyValuePair<string, SolidColorBrush>)comboBox3.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x04] = ((KeyValuePair<string, SolidColorBrush>)comboBox4.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x05] = ((KeyValuePair<string, SolidColorBrush>)comboBox5.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x06] = ((KeyValuePair<string, SolidColorBrush>)comboBox6.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x07] = ((KeyValuePair<string, SolidColorBrush>)comboBox7.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x08] = ((KeyValuePair<string, SolidColorBrush>)comboBox8.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x09] = ((KeyValuePair<string, SolidColorBrush>)comboBox9.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x0A] = ((KeyValuePair<string, SolidColorBrush>)comboBox10.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x0B] = ((KeyValuePair<string, SolidColorBrush>)comboBox11.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x0F] = ((KeyValuePair<string, SolidColorBrush>)comboBox12.SelectedItem).Key;
                Settings.Instance.ContentColorList[0x10] = ((KeyValuePair<string, SolidColorBrush>)comboBox13.SelectedItem).Key;
                Settings.Instance.ReserveRectColorNormal = ((KeyValuePair<string, SolidColorBrush>)comboBox_reserveNormal.SelectedItem).Key;
                Settings.Instance.ReserveRectColorNo = ((KeyValuePair<string, SolidColorBrush>)comboBox_reserveNo.SelectedItem).Key;
                Settings.Instance.ReserveRectColorNoTuner = ((KeyValuePair<string, SolidColorBrush>)comboBox_reserveNoTuner.SelectedItem).Key;
                Settings.Instance.ReserveRectColorWarning = ((KeyValuePair<string, SolidColorBrush>)comboBox_reserveWarning.SelectedItem).Key;
                Settings.Instance.TitleColor1 = ((KeyValuePair<string, SolidColorBrush>)comboBox_colorTitle1.SelectedItem).Key;
                Settings.Instance.TitleColor2 = ((KeyValuePair<string, SolidColorBrush>)comboBox_colorTitle2.SelectedItem).Key;
                if (checkBox_reserveBackground.IsChecked == true)
                {
                    Settings.Instance.ReserveRectBackground = true;
                }
                else
                {
                    Settings.Instance.ReserveRectBackground = false;
                }

                if (comboBox_font.SelectedItem != null)
                {
                    Settings.Instance.FontName = comboBox_font.SelectedItem as string;
                }
                Settings.Instance.FontSize = Convert.ToDouble(textBox_fontSize.Text);
                if (comboBox_fontTitle.SelectedItem != null)
                {
                    Settings.Instance.FontNameTitle = comboBox_fontTitle.SelectedItem as string;
                }
                Settings.Instance.FontSizeTitle = Convert.ToDouble(textBox_fontSizeTitle.Text);
                if (checkBox_fontBoldTitle.IsChecked == true)
                {
                    Settings.Instance.FontBoldTitle = true;
                }
                else
                {
                    Settings.Instance.FontBoldTitle = false;
                }

                if (radioButton_1_cust.IsChecked == true)
                {
                    Settings.Instance.UseCustomEpgView = true;
                    IniFileHandler.WritePrivateProfileString("HTTP", "HttpCustEpg", "1", SettingPath.TimerSrvIniPath);
                }
                else
                {
                    Settings.Instance.UseCustomEpgView = false;
                    IniFileHandler.WritePrivateProfileString("HTTP", "HttpCustEpg", "0", SettingPath.TimerSrvIniPath);
                }

                Settings.Instance.CustomEpgTabList.Clear();
                int custCount = listBox_tab.Items.Count;
                IniFileHandler.WritePrivateProfileString("HTTP", "HttpCustCount", custCount.ToString(), SettingPath.TimerSrvIniPath);
                custCount = 0;
                foreach (CustomEpgTabInfo info in listBox_tab.Items)
                {
                    Settings.Instance.CustomEpgTabList.Add(info);

                    IniFileHandler.WritePrivateProfileString("HTTP_CUST" + custCount.ToString(), "Name", info.TabName, SettingPath.TimerSrvIniPath);
                    IniFileHandler.WritePrivateProfileString("HTTP_CUST" + custCount.ToString(), "ViewServiceCount", info.ViewServiceList.Count.ToString(), SettingPath.TimerSrvIniPath);
                    int serviceCount = 0;
                    foreach (Int64 id in info.ViewServiceList)
                    {
                        IniFileHandler.WritePrivateProfileString("HTTP_CUST" + custCount.ToString(), "ViewService" + serviceCount.ToString(), id.ToString(), SettingPath.TimerSrvIniPath);
                        serviceCount++;
                    }

                    IniFileHandler.WritePrivateProfileString("HTTP_CUST" + custCount.ToString(), "ContentCount", info.ViewContentKindList.Count.ToString(), SettingPath.TimerSrvIniPath);
                    int contentCount = 0;
                    foreach (UInt16 id in info.ViewContentKindList)
                    {
                        IniFileHandler.WritePrivateProfileString("HTTP_CUST" + custCount.ToString(), "Content" + contentCount.ToString(), id.ToString(), SettingPath.TimerSrvIniPath);
                        contentCount++;
                    }
                    IniFileHandler.WritePrivateProfileString("HTTP_CUST" + custCount.ToString(), "ViewMode", info.ViewMode.ToString(), SettingPath.TimerSrvIniPath);
                    if (info.NeedTimeOnlyBasic == true)
                    {
                        IniFileHandler.WritePrivateProfileString("HTTP_CUST" + custCount.ToString(), "NeedTimeOnlyBasic", "1", SettingPath.TimerSrvIniPath);
                    }
                    else
                    {
                        IniFileHandler.WritePrivateProfileString("HTTP_CUST" + custCount.ToString(), "NeedTimeOnlyBasic", "0", SettingPath.TimerSrvIniPath);
                    }
                    if (info.NeedTimeOnlyWeek == true)
                    {
                        IniFileHandler.WritePrivateProfileString("HTTP_CUST" + custCount.ToString(), "NeedTimeOnlyWeek", "1", SettingPath.TimerSrvIniPath);
                    }
                    else
                    {
                        IniFileHandler.WritePrivateProfileString("HTTP_CUST" + custCount.ToString(), "NeedTimeOnlyWeek", "0", SettingPath.TimerSrvIniPath);
                    }
                    if (info.SearchMode == true)
                    {
                        IniFileHandler.WritePrivateProfileString("HTTP_CUST" + custCount.ToString(), "SearchMode", "1", SettingPath.TimerSrvIniPath);
                    }
                    else
                    {
                        IniFileHandler.WritePrivateProfileString("HTTP_CUST" + custCount.ToString(), "SearchMode", "0", SettingPath.TimerSrvIniPath);
                    }
                    
                    custCount++;
                }

                Settings.Instance.ContentCustColorList[0x00] = ColorDef.ToUInt(((SolidColorBrush)button0.Background).Color);
                Settings.Instance.ContentCustColorList[0x01] = ColorDef.ToUInt(((SolidColorBrush)button1.Background).Color);
                Settings.Instance.ContentCustColorList[0x02] = ColorDef.ToUInt(((SolidColorBrush)button2.Background).Color);
                Settings.Instance.ContentCustColorList[0x03] = ColorDef.ToUInt(((SolidColorBrush)button3.Background).Color);
                Settings.Instance.ContentCustColorList[0x04] = ColorDef.ToUInt(((SolidColorBrush)button4.Background).Color);
                Settings.Instance.ContentCustColorList[0x05] = ColorDef.ToUInt(((SolidColorBrush)button5.Background).Color);
                Settings.Instance.ContentCustColorList[0x06] = ColorDef.ToUInt(((SolidColorBrush)button6.Background).Color);
                Settings.Instance.ContentCustColorList[0x07] = ColorDef.ToUInt(((SolidColorBrush)button7.Background).Color);
                Settings.Instance.ContentCustColorList[0x08] = ColorDef.ToUInt(((SolidColorBrush)button8.Background).Color);
                Settings.Instance.ContentCustColorList[0x09] = ColorDef.ToUInt(((SolidColorBrush)button9.Background).Color);
                Settings.Instance.ContentCustColorList[0x0A] = ColorDef.ToUInt(((SolidColorBrush)button10.Background).Color);
                Settings.Instance.ContentCustColorList[0x0B] = ColorDef.ToUInt(((SolidColorBrush)button11.Background).Color);
                Settings.Instance.ContentCustColorList[0x0F] = ColorDef.ToUInt(((SolidColorBrush)button12.Background).Color);
                Settings.Instance.ContentCustColorList[0x10] = ColorDef.ToUInt(((SolidColorBrush)button13.Background).Color);
                Settings.Instance.ContentCustColorList[0x11] = ColorDef.ToUInt(((SolidColorBrush)button14.Background).Color);
                Settings.Instance.ContentCustColorList[0x12] = ColorDef.ToUInt(((SolidColorBrush)button15.Background).Color);
                Settings.Instance.ContentCustColorList[0x13] = ColorDef.ToUInt(((SolidColorBrush)button16.Background).Color);
                Settings.Instance.ContentCustColorList[0x14] = ColorDef.ToUInt(((SolidColorBrush)button17.Background).Color);
                Settings.Instance.TitleCustColor1 = ColorDef.ToUInt(((SolidColorBrush)button_colorTitle1.Background).Color);
                Settings.Instance.TitleCustColor2 = ColorDef.ToUInt(((SolidColorBrush)button_colorTitle2.Background).Color);

                if (checkBox_singleOpen.IsChecked == true)
                {
                    Settings.Instance.EpgInfoSingleClick = true;
                }
                else
                {
                    Settings.Instance.EpgInfoSingleClick = false;
                }
                if (checkBox_openInfo.IsChecked == true)
                {
                    Settings.Instance.EpgInfoOpenMode = 1;
                }
                else
                {
                    Settings.Instance.EpgInfoOpenMode = 0;
                }

                if (checkBox_scrollAuto.IsChecked == true)
                {
                    Settings.Instance.MouseScrollAuto = true;
                }
                else
                {
                    Settings.Instance.MouseScrollAuto = false;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
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
                CustomEpgTabInfo info = new CustomEpgTabInfo();
                dlg.GetSetting(ref info);
                listBox_tab.Items.Add(info);
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
                CustomEpgTabInfo setInfo = listBox_tab.SelectedItem as CustomEpgTabInfo;
                dlg.SetDefSetting(setInfo);
                if (dlg.ShowDialog() == true)
                {
                    dlg.GetSetting(ref setInfo);
                    listBox_tab.Items.Refresh();
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
                }
            }
        }

        private void button_Color_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = ((SolidColorBrush)((Button)sender).Background).Color;
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                ((SolidColorBrush)((Button)sender).Background).Color = item;
            }
        }
    }
}

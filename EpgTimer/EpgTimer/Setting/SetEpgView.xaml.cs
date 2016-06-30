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
        private Dictionary<string, ColorSelectionItem> colorList = new Dictionary<string, ColorSelectionItem>();
        private List<Color> custColorList = new List<Color>();
        private Color custTitleColor1;
        private Color custTitleColor2;

        SolidColorBrush backColor0 = new SolidColorBrush();
        SolidColorBrush backColor1 = new SolidColorBrush();
        SolidColorBrush backColor2 = new SolidColorBrush();
        SolidColorBrush backColor3 = new SolidColorBrush();
        SolidColorBrush backColor4 = new SolidColorBrush();
        SolidColorBrush backColor5 = new SolidColorBrush();
        SolidColorBrush backColor6 = new SolidColorBrush();
        SolidColorBrush backColor7 = new SolidColorBrush();
        SolidColorBrush backColor8 = new SolidColorBrush();
        SolidColorBrush backColor9 = new SolidColorBrush();
        SolidColorBrush backColor10 = new SolidColorBrush();
        SolidColorBrush backColor11 = new SolidColorBrush();
        SolidColorBrush backColor12 = new SolidColorBrush();
        SolidColorBrush backColor13 = new SolidColorBrush();
        SolidColorBrush backColor14 = new SolidColorBrush();
        SolidColorBrush backColor15 = new SolidColorBrush();
        SolidColorBrush backColor16 = new SolidColorBrush();
        SolidColorBrush backColor17 = new SolidColorBrush();
        SolidColorBrush titleColor1 = new SolidColorBrush();
        SolidColorBrush titleColor2 = new SolidColorBrush();

        public SetEpgView()
        {
            InitializeComponent();

            try
            {
                foreach (string name in ColorDef.ColorNames)
                {
                    colorList.Add(name, new ColorSelectionItem(name, ColorDef.Instance.ColorTable[name]));
                }
                comboBox0.DataContext = colorList.Values;
                comboBox1.DataContext = colorList.Values;
                comboBox2.DataContext = colorList.Values;
                comboBox3.DataContext = colorList.Values;
                comboBox4.DataContext = colorList.Values;
                comboBox5.DataContext = colorList.Values;
                comboBox6.DataContext = colorList.Values;
                comboBox7.DataContext = colorList.Values;
                comboBox8.DataContext = colorList.Values;
                comboBox9.DataContext = colorList.Values;
                comboBox10.DataContext = colorList.Values;
                comboBox11.DataContext = colorList.Values;
                comboBox12.DataContext = colorList.Values;
                comboBox13.DataContext = colorList.Values;
                comboBox_reserveNormal.DataContext = colorList.Values;
                comboBox_reserveNo.DataContext = colorList.Values;
                comboBox_reserveNoTuner.DataContext = colorList.Values;
                comboBox_reserveWarning.DataContext = colorList.Values;
                comboBox_colorTitle1.DataContext = colorList.Values;
                comboBox_colorTitle2.DataContext = colorList.Values;

                if (Settings.Instance.ContentColorList.Count == 0)
                {
                    Settings.LoadFromXmlFile();
                }

                comboBox0.SelectedItem = colorList[Settings.Instance.ContentColorList[0x00]];
                comboBox1.SelectedItem = colorList[Settings.Instance.ContentColorList[0x01]];
                comboBox2.SelectedItem = colorList[Settings.Instance.ContentColorList[0x02]];
                comboBox3.SelectedItem = colorList[Settings.Instance.ContentColorList[0x03]];
                comboBox4.SelectedItem = colorList[Settings.Instance.ContentColorList[0x04]];
                comboBox5.SelectedItem = colorList[Settings.Instance.ContentColorList[0x05]];
                comboBox6.SelectedItem = colorList[Settings.Instance.ContentColorList[0x06]];
                comboBox7.SelectedItem = colorList[Settings.Instance.ContentColorList[0x07]];
                comboBox8.SelectedItem = colorList[Settings.Instance.ContentColorList[0x08]];
                comboBox9.SelectedItem = colorList[Settings.Instance.ContentColorList[0x09]];
                comboBox10.SelectedItem = colorList[Settings.Instance.ContentColorList[0x0A]];
                comboBox11.SelectedItem = colorList[Settings.Instance.ContentColorList[0x0B]];
                comboBox12.SelectedItem = colorList[Settings.Instance.ContentColorList[0x0F]];
                comboBox13.SelectedItem = colorList[Settings.Instance.ContentColorList[0x10]];

                comboBox_reserveNormal.SelectedItem = colorList[Settings.Instance.ReserveRectColorNormal];
                comboBox_reserveNo.SelectedItem = colorList[Settings.Instance.ReserveRectColorNo];
                comboBox_reserveNoTuner.SelectedItem = colorList[Settings.Instance.ReserveRectColorNoTuner];
                comboBox_reserveWarning.SelectedItem = colorList[Settings.Instance.ReserveRectColorWarning];
                checkBox_reserveBackground.IsChecked = Settings.Instance.ReserveRectBackground;

                comboBox_colorTitle1.SelectedItem = colorList[Settings.Instance.TitleColor1];
                comboBox_colorTitle2.SelectedItem = colorList[Settings.Instance.TitleColor2];

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
                checkBox_title_indent.IsChecked = Settings.Instance.EpgTitleIndent;
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

                byte r = 0;
                byte g = 0;
                byte b = 0;
                foreach (UInt32 argb in Settings.Instance.ContentCustColorList)
                {
                    r = (byte)((argb&0x00FF0000)>>16);
                    g = (byte)((argb&0x0000FF00)>>8);
                    b = (byte)(argb&0x000000FF);

                    Color item = Color.FromArgb(0xFF, r, g, b);
                    custColorList.Add(item);
                }
                r = (byte)((Settings.Instance.TitleCustColor1 & 0x00FF0000) >> 16);
                g = (byte)((Settings.Instance.TitleCustColor1 & 0x0000FF00) >> 8);
                b = (byte)(Settings.Instance.TitleCustColor1 & 0x000000FF);
                custTitleColor1 = Color.FromArgb(0xFF, r, g, b);
                r = (byte)((Settings.Instance.TitleCustColor2 & 0x00FF0000) >> 16);
                g = (byte)((Settings.Instance.TitleCustColor2 & 0x0000FF00) >> 8);
                b = (byte)(Settings.Instance.TitleCustColor2 & 0x000000FF);
                custTitleColor2 = Color.FromArgb(0xFF, r, g, b);

                titleColor1.Color = custTitleColor1;
                titleColor2.Color = custTitleColor2;

                backColor0.Color = custColorList[0x00];
                backColor1.Color = custColorList[0x01];
                backColor2.Color = custColorList[0x02];
                backColor3.Color = custColorList[0x03];
                backColor4.Color = custColorList[0x04];
                backColor5.Color = custColorList[0x05];
                backColor6.Color = custColorList[0x06];
                backColor7.Color = custColorList[0x07];
                backColor8.Color = custColorList[0x08];
                backColor9.Color = custColorList[0x09];
                backColor10.Color = custColorList[0x0a];
                backColor11.Color = custColorList[0x0b];
                backColor12.Color = custColorList[0x0f];
                backColor13.Color = custColorList[0x10];
                backColor14.Color = custColorList[0x11];
                backColor15.Color = custColorList[0x12];
                backColor16.Color = custColorList[0x13];
                backColor17.Color = custColorList[0x14];
                button0.Background = backColor0;
                button1.Background = backColor1;
                button2.Background = backColor2;
                button3.Background = backColor3;
                button4.Background = backColor4;
                button5.Background = backColor5;
                button6.Background = backColor6;
                button7.Background = backColor7;
                button8.Background = backColor8;
                button9.Background = backColor9;
                button10.Background = backColor10;
                button11.Background = backColor11;
                button12.Background = backColor12;
                button13.Background = backColor13;
                button14.Background = backColor14;
                button15.Background = backColor15;
                button16.Background = backColor16;
                button17.Background = backColor17;

                button_colorTitle1.Background = titleColor1;
                button_colorTitle2.Background = titleColor2;

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

                Settings.Instance.ContentColorList[0x00] = ((ColorSelectionItem)(comboBox0.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x01] = ((ColorSelectionItem)(comboBox1.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x02] = ((ColorSelectionItem)(comboBox2.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x03] = ((ColorSelectionItem)(comboBox3.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x04] = ((ColorSelectionItem)(comboBox4.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x05] = ((ColorSelectionItem)(comboBox5.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x06] = ((ColorSelectionItem)(comboBox6.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x07] = ((ColorSelectionItem)(comboBox7.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x08] = ((ColorSelectionItem)(comboBox8.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x09] = ((ColorSelectionItem)(comboBox9.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x0A] = ((ColorSelectionItem)(comboBox10.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x0B] = ((ColorSelectionItem)(comboBox11.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x0F] = ((ColorSelectionItem)(comboBox12.SelectedItem)).ColorName;
                Settings.Instance.ContentColorList[0x10] = ((ColorSelectionItem)(comboBox13.SelectedItem)).ColorName;
                Settings.Instance.ReserveRectColorNormal = ((ColorSelectionItem)(comboBox_reserveNormal.SelectedItem)).ColorName;
                Settings.Instance.ReserveRectColorNo = ((ColorSelectionItem)(comboBox_reserveNo.SelectedItem)).ColorName;
                Settings.Instance.ReserveRectColorNoTuner = ((ColorSelectionItem)(comboBox_reserveNoTuner.SelectedItem)).ColorName;
                Settings.Instance.ReserveRectColorWarning = ((ColorSelectionItem)(comboBox_reserveWarning.SelectedItem)).ColorName;
                Settings.Instance.TitleColor1 = ((ColorSelectionItem)(comboBox_colorTitle1.SelectedItem)).ColorName;
                Settings.Instance.TitleColor2 = ((ColorSelectionItem)(comboBox_colorTitle2.SelectedItem)).ColorName;
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

                Settings.Instance.ContentCustColorList.Clear();
                UInt32 argb = 0;
                foreach (Color info in custColorList)
                {
                    argb = CommonManager.CreateARGBKey(0xFF, info.R, info.G, info.B);
                    Settings.Instance.ContentCustColorList.Add(argb);
                }

                argb = CommonManager.CreateARGBKey(0xFF, custTitleColor1.R, custTitleColor1.G, custTitleColor1.B);
                Settings.Instance.TitleCustColor1 = argb;
                argb = CommonManager.CreateARGBKey(0xFF, custTitleColor2.R, custTitleColor2.G, custTitleColor2.B);
                Settings.Instance.TitleCustColor2 = argb;

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

        private void button0_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[0];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[0] = item;
                backColor0.Color = custColorList[0x00];
            }
        }

        private void button1_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[1];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[1] = item;
                backColor1.Color = custColorList[0x01];
            }
        }

        private void button2_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[2];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[2] = item;
                backColor2.Color = custColorList[0x02];
            }
        }

        private void button3_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[3];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[3] = item;
                backColor3.Color = custColorList[0x03];
            }
        }

        private void button4_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[4];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[4] = item;
                backColor4.Color = custColorList[0x04];
            }
        }

        private void button5_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[5];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[5] = item;
                backColor5.Color = custColorList[0x05];
            }
        }

        private void button6_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[6];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[6] = item;
                backColor6.Color = custColorList[0x06];
            }
        }

        private void button7_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[7];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[7] = item;
                backColor7.Color = custColorList[0x07];
            }
        }

        private void button8_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[8];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[8] = item;
                backColor8.Color = custColorList[0x08];
            }
        }

        private void button9_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[9];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[9] = item;
                backColor9.Color = custColorList[0x09];
            }
        }

        private void button10_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[0x0a];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[0x0a] = item;
                backColor10.Color = custColorList[0x0a];
            }
        }

        private void button11_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[0x0b];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[0x0b] = item;
                backColor11.Color = custColorList[0x0b];
            }
        }

        private void button12_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[0x0f];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[0x0f] = item;
                backColor12.Color = custColorList[0x0f];
            }
        }

        private void button13_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[0x10];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[0x10] = item;
                backColor13.Color = custColorList[0x10];
            }
        }

        private void button14_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[0x11];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[0x11] = item;
                backColor14.Color = custColorList[0x11];
            }
        }

        private void button15_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[0x12];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[0x12] = item;
                backColor15.Color = custColorList[0x12];
            }
        }

        private void button16_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[0x13];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[0x13] = item;
                backColor16.Color = custColorList[0x13];
            }
        }

        private void button17_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custColorList[0x14];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custColorList[0x14] = item;
                backColor17.Color = custColorList[0x14];
            }

        }

        private void button_colorTitle1_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custTitleColor1;
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custTitleColor1 = item;
                titleColor1.Color = custTitleColor1;
            }
        }

        private void button_colorTitle2_Click(object sender, RoutedEventArgs e)
        {
            ColorSetWindow dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = custTitleColor2;
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                custTitleColor2 = item;
                titleColor2.Color = custTitleColor2;
            }
        }



    }
}

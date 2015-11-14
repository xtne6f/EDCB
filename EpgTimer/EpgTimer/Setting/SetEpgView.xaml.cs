using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Markup;

namespace EpgTimer.Setting
{
    /// <summary>
    /// SetEpgView.xaml の相互作用ロジック
    /// </summary>
    public partial class SetEpgView : UserControl
    {
        private Dictionary<string, ColorSelectionItem> colorList = new Dictionary<string, ColorSelectionItem>();
        private List<Color> custTitleColorList = new List<Color>();
        private List<Color> custColorList;
        private BoxExchangeEditor bx = new BoxExchangeEditor();

        public SetEpgView()
        {
            InitializeComponent();

            try
            {
                foreach (string name in ColorDef.ColorNames)
                {
                    colorList.Add(name, new ColorSelectionItem(name, ColorDef.Instance.ColorTable[name]));
                }

                foreach (var obj in grid_Epg.Children.OfType<ComboBox>())
                {
                    obj.DataContext = colorList.Values;
                    int idx = int.Parse((string)obj.Tag);
                    obj.SelectedItem = colorList[Settings.Instance.ContentColorList[idx]];
                }
                foreach (var obj in grid_Reserve.Children.OfType<ComboBox>())
                {
                    obj.DataContext = colorList.Values;
                }
                comboBox_reserveNormal.SelectedItem = colorList[Settings.Instance.ReserveRectColorNormal];
                comboBox_reserveNo.SelectedItem = colorList[Settings.Instance.ReserveRectColorNo];
                comboBox_reserveNoTuner.SelectedItem = colorList[Settings.Instance.ReserveRectColorNoTuner];
                comboBox_reserveWarning.SelectedItem = colorList[Settings.Instance.ReserveRectColorWarning];
                comboBox_reserveAutoAddMissing.SelectedItem = colorList[Settings.Instance.ReserveRectColorAutoAddMissing];
                checkBox_reserveBackground.IsChecked = Settings.Instance.ReserveRectBackground;

                comboBox_colorTitle1.DataContext = colorList.Values;
                comboBox_colorTitle2.DataContext = colorList.Values;
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
                checkBox_epg_popup_resOnly.IsEnabled = Settings.Instance.EpgPopup;
                checkBox_epg_popup_resOnly.IsChecked = Settings.Instance.EpgPopupResOnly;
                checkBox_gradation.IsChecked = Settings.Instance.EpgGradation;
                checkBox_gradationHeader.IsChecked = Settings.Instance.EpgGradationHeader;

                radioButton_1_def.IsChecked = (Settings.Instance.UseCustomEpgView == false);
                radioButton_1_cust.IsChecked = (Settings.Instance.UseCustomEpgView != false);

                foreach (CustomEpgTabInfo info in Settings.Instance.CustomEpgTabList)
                {
                    listBox_tab.Items.Add(info);
                }
                if (listBox_tab.Items.Count > 0)
                {
                    listBox_tab.SelectedIndex = 0;
                }

                custColorList = Settings.Instance.ContentCustColorList.Select(c => ColorDef.FromUInt(c)).ToList();

                new List<Grid> { grid_Epg, grid_Reserve }.ForEach(grd =>
                {
                    foreach (var obj in grd.Children.OfType<Button>())
                    {
                        int idx = int.Parse((string)obj.Tag);
                        obj.Background = ColorDef.SolidBrush(custColorList[idx]);
                    }
                });

                custTitleColorList.Add(ColorDef.FromUInt(Settings.Instance.TitleCustColor1));
                custTitleColorList.Add(ColorDef.FromUInt(Settings.Instance.TitleCustColor2));
                button_colorTitle1.Background = ColorDef.SolidBrush(custTitleColorList[0]);
                button_colorTitle2.Background = ColorDef.SolidBrush(custTitleColorList[1]);

                checkBox_singleOpen.IsChecked = Settings.Instance.EpgInfoSingleClick;
                checkBox_openInfo.IsChecked = (Settings.Instance.EpgInfoOpenMode != 0);

                checkBox_scrollAuto.IsChecked = Settings.Instance.MouseScrollAuto;
                checkBox_displayNotifyChange.IsChecked = Settings.Instance.DisplayNotifyEpgChange;

                bx.TargetBox = this.listBox_tab;
                button_tab_del.Click += new RoutedEventHandler(bx.button_del_Click);
                button_tab_up.Click += new RoutedEventHandler(bx.button_up_Click);
                button_tab_down.Click += new RoutedEventHandler(bx.button_down_Click);
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
                Settings.Instance.EpgTitleIndent = (checkBox_title_indent.IsChecked == true);
                Settings.Instance.EpgPopup = (checkBox_epg_popup.IsChecked == true);
                Settings.Instance.EpgPopupResOnly = (checkBox_epg_popup_resOnly.IsChecked == true);
                Settings.Instance.EpgGradation = (checkBox_gradation.IsChecked == true);
                Settings.Instance.EpgGradationHeader = (checkBox_gradationHeader.IsChecked == true);

                foreach (var obj in grid_Epg.Children.OfType<ComboBox>())
                {
                    obj.DataContext = colorList.Values;
                    int idx = int.Parse((string)obj.Tag);
                    Settings.Instance.ContentColorList[idx] = ((ColorSelectionItem)(obj.SelectedItem)).ColorName;
                }

                Settings.Instance.ReserveRectColorNormal = ((ColorSelectionItem)(comboBox_reserveNormal.SelectedItem)).ColorName;
                Settings.Instance.ReserveRectColorNo = ((ColorSelectionItem)(comboBox_reserveNo.SelectedItem)).ColorName;
                Settings.Instance.ReserveRectColorNoTuner = ((ColorSelectionItem)(comboBox_reserveNoTuner.SelectedItem)).ColorName;
                Settings.Instance.ReserveRectColorWarning = ((ColorSelectionItem)(comboBox_reserveWarning.SelectedItem)).ColorName;
                Settings.Instance.ReserveRectColorAutoAddMissing = ((ColorSelectionItem)(comboBox_reserveAutoAddMissing.SelectedItem)).ColorName;
                Settings.Instance.TitleColor1 = ((ColorSelectionItem)(comboBox_colorTitle1.SelectedItem)).ColorName;
                Settings.Instance.TitleColor2 = ((ColorSelectionItem)(comboBox_colorTitle2.SelectedItem)).ColorName;

                Settings.Instance.ReserveRectBackground = (checkBox_reserveBackground.IsChecked == true);

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
                Settings.Instance.FontBoldTitle = (checkBox_fontBoldTitle.IsChecked == true);

                Settings.Instance.UseCustomEpgView = (radioButton_1_cust.IsChecked == true);
                Settings.Instance.CustomEpgTabList = listBox_tab.Items.OfType<CustomEpgTabInfo>().ToList();
                Settings.SetCustomEpgTabInfoID();

                Settings.Instance.ContentCustColorList = custColorList.Select(c => ColorDef.ToUInt(c)).ToList();
                Settings.Instance.TitleCustColor1 = ColorDef.ToUInt(custTitleColorList[0]);
                Settings.Instance.TitleCustColor2 = ColorDef.ToUInt(custTitleColorList[1]);

                Settings.Instance.EpgInfoSingleClick = (checkBox_singleOpen.IsChecked == true);
                Settings.Instance.EpgInfoOpenMode = (byte)(checkBox_openInfo.IsChecked == true ? 1 : 0);
                Settings.Instance.MouseScrollAuto = (checkBox_scrollAuto.IsChecked == true);
                Settings.Instance.DisplayNotifyEpgChange = (checkBox_displayNotifyChange.IsChecked == true);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_tab_add_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new EpgDataViewSettingWindow();
            var topWindow = PresentationSource.FromVisual(this);
            if (topWindow != null)
            {
                dlg.Owner = (Window)topWindow.RootVisual;
            }
            if (dlg.ShowDialog() == true)
            {
                var info = new CustomEpgTabInfo();
                dlg.GetSetting(ref info);
                listBox_tab.Items.Add(info);
                listBox_tab.SelectedItem = info;
                listBox_tab.ScrollIntoView(info);
            }
        }
        private void button_tab_chg_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_tab.SelectedItem == null)
            {
                if (listBox_tab.Items.Count != 0)
                {
                    listBox_tab.SelectedIndex = 0;
                }
            }
            var setInfo = listBox_tab.SelectedItem as CustomEpgTabInfo;
            if (setInfo != null)
            {
                listBox_tab.UnselectAll();
                listBox_tab.SelectedItem = setInfo;
                var dlg = new EpgDataViewSettingWindow();
                var topWindow = PresentationSource.FromVisual(this);
                if (topWindow != null)
                {
                    dlg.Owner = (Window)topWindow.RootVisual;
                }
                dlg.SetDefSetting(setInfo);
                if (dlg.ShowDialog() == true)
                {
                    dlg.GetSetting(ref setInfo);
                    listBox_tab.Items.Refresh();
                }
            }
            else
            {
                button_tab_add_Click(null, null);
            }
        }

        private void button_tab_clone_Click(object sender, RoutedEventArgs e)
        {
            if (listBox_tab.SelectedItem != null)
            {
                List<CustomEpgTabInfo> items = listBox_tab.SelectedItems.OfType<CustomEpgTabInfo>().ToList().Clone();
                items.ForEach(info => info.TabName += "～コピー");
                button_tab_copyAdd(items);
            }
        }

        private void button_tab_defaultCopy_Click(object sender, RoutedEventArgs e)
        {
            button_tab_copyAdd(CommonManager.Instance.CreateDefaultTabInfo());
        }

        private void button_tab_copyAdd(List<CustomEpgTabInfo> items)
        {
            if (items.Count != 0)
            {
                items.ForEach(info => listBox_tab.Items.Add(info));
                listBox_tab.ScrollIntoView(listBox_tab.Items[listBox_tab.Items.Count - 1]);
                listBox_tab.SelectedItem = items[0];
                items.ForEach(info => listBox_tab.SelectedItems.Add(info));
            }
        }

        private void listBox_tab_MouseDoubleClick(object sender, System.Windows.Input.MouseButtonEventArgs e)
        {
            button_tab_chg_Click(null, null);
        }

        private void button_colorTitle_Click(object sender, RoutedEventArgs e)
        {
            button_Color_Click(sender, e, ref custTitleColorList);
        }
        private void button_colorEpg_Click(object sender, RoutedEventArgs e)
        {
            button_Color_Click(sender, e, ref custColorList);
        }
        private void button_Color_Click(object sender, RoutedEventArgs e, ref List<Color> colorList)
        {
            var btn = sender as Button;
            int idx = int.Parse((string)btn.Tag);

            var dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = colorList[idx];
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                colorList[idx] = item;
                btn.Background = ColorDef.SolidBrush(item);
            }
        }

        private void checkBox_epg_popup_Click(object sender, RoutedEventArgs e)
        {
            checkBox_epg_popup_resOnly.IsEnabled = (checkBox_epg_popup.IsChecked == true);
        }

    }
}

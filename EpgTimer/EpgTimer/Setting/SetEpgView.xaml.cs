using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using System.Windows.Markup;
using System.Windows.Shapes;

namespace EpgTimer.Setting
{
    /// <summary>
    /// SetEpgView.xaml の相互作用ロジック
    /// </summary>
    public partial class SetEpgView : UserControl
    {
        private class ColorReferenceViewItem
        {
            public ColorReferenceViewItem(string name, Brush c) { ColorName = name; Color = c; }
            public string ColorName { get; private set; }
            public Brush Color { get; private set; }
        }

        private BoxExchangeEditor bx = new BoxExchangeEditor();

        public SetEpgView()
        {
            InitializeComponent();

            try
            {
                textBox_mouse_scroll.Text = Settings.Instance.ScrollSize.ToString();
                textBox_service_width.Text = Settings.Instance.ServiceWidth.ToString();
                textBox_minHeight.Text = Settings.Instance.MinHeight.ToString();
                textBox_dragScroll.Text = Settings.Instance.DragScroll.ToString();
                textBox_minimumHeight.Text = Settings.Instance.MinimumHeight.ToString();
                checkBox_epg_popup.IsChecked = Settings.Instance.EpgPopup;
                checkBox_epg_popup_resOnly.IsEnabled = Settings.Instance.EpgPopup;
                checkBox_epg_popup_resOnly.IsChecked = Settings.Instance.EpgPopupResOnly;
                checkBox_title_indent.IsChecked = Settings.Instance.EpgTitleIndent;
                checkBox_singleOpen.IsChecked = Settings.Instance.EpgInfoSingleClick;
                checkBox_scrollAuto.IsChecked = Settings.Instance.MouseScrollAuto;
                checkBox_gradation.IsChecked = Settings.Instance.EpgGradation;
                checkBox_gradationHeader.IsChecked = Settings.Instance.EpgGradationHeader;
                checkBox_openInfo.IsChecked = (Settings.Instance.EpgInfoOpenMode != 0);
                checkBox_displayNotifyChange.IsChecked = Settings.Instance.DisplayNotifyEpgChange;
                checkBox_reserveBackground.IsChecked = Settings.Instance.ReserveRectBackground;

                textBox_tuner_mouse_scroll.Text = Settings.Instance.TunerScrollSize.ToString();
                textBox_tuner_width.Text = Settings.Instance.TunerWidth.ToString();
                textBox_tuner_minHeight.Text = Settings.Instance.TunerMinHeight.ToString();
                textBox_tunerDdragScroll.Text = Settings.Instance.TunerDragScroll.ToString();
                textBox_tunerMinLineHeight.Text = Settings.Instance.TunerMinimumLine.ToString();
                checkBox_tuner_popup.IsChecked = Settings.Instance.TunerPopup;
                checkBox_tuner_popup_recInfo.IsEnabled = Settings.Instance.TunerPopup;
                checkBox_tuner_popup_recInfo.IsChecked = Settings.Instance.TunerPopupRecinfo;
                checkBox_tuner_title_indent.IsChecked = Settings.Instance.TunerTitleIndent;
                checkBox_tunerSingleOpen.IsChecked = Settings.Instance.TunerInfoSingleClick;
                checkBox_tuner_scrollAuto.IsChecked = Settings.Instance.TunerMouseScrollAuto;
                checkBox_tuner_service_nowrap.IsChecked = Settings.Instance.TunerServiceNoWrap;
                checkBox_tunerColorModeUse.IsChecked = Settings.Instance.TunerColorModeUse;
                comboBox_tunerFontColorService.IsEnabled = !Settings.Instance.TunerColorModeUse;
                button_tunerFontCustColorService.IsEnabled = !Settings.Instance.TunerColorModeUse;

                var setTunerColorModeUseTooltip = new Action<Control>((obj) =>
                {
                    obj.ToolTip = "'サービスのフォント色を優先度で変える'オプションが有効です。('色'タブの設定が使用されます。)";
                    obj.ToolTipOpening += new ToolTipEventHandler((sender, e) => e.Handled = (sender as Control).IsEnabled);
                    ToolTipService.SetShowOnDisabled(obj, true);
                });
                setTunerColorModeUseTooltip(comboBox_tunerFontColorService);
                setTunerColorModeUseTooltip(button_tunerFontCustColorService);

                bx.TargetBox = this.listBox_tab;
                button_tab_del.Click += new RoutedEventHandler(bx.button_del_Click);
                button_tab_up.Click += new RoutedEventHandler(bx.button_up_Click);
                button_tab_down.Click += new RoutedEventHandler(bx.button_down_Click);

                radioButton_1_def.IsChecked = (Settings.Instance.UseCustomEpgView == false);
                radioButton_1_cust.IsChecked = (Settings.Instance.UseCustomEpgView != false);

                Settings.Instance.CustomEpgTabList.ForEach(info => listBox_tab.Items.Add(info));
                if (listBox_tab.Items.Count > 0) listBox_tab.SelectedIndex = 0;

                XmlLanguage FLanguage = XmlLanguage.GetLanguage("ja-JP");
                List<string> fontList = Fonts.SystemFontFamilies
                    .Where(f => f.FamilyNames.ContainsKey(FLanguage) == true)
                    .Select(f => f.FamilyNames[FLanguage]).ToList();

                var setCmboFont = new Action<string, ComboBox>((name, cmb) =>
                {
                    cmb.ItemsSource = fontList;
                    cmb.SelectedItem = name;
                    if (cmb.SelectedItem == null) cmb.SelectedIndex = 0;
                });
                setCmboFont(Settings.Instance.FontNameTitle, comboBox_fontTitle);
                setCmboFont(Settings.Instance.FontName, comboBox_font);
                setCmboFont(Settings.Instance.TunerFontNameService, comboBox_fontTunerService);
                setCmboFont(Settings.Instance.TunerFontName, comboBox_fontTuner);

                textBox_fontSize.Text = Settings.Instance.FontSize.ToString();
                textBox_fontSizeTitle.Text = Settings.Instance.FontSizeTitle.ToString();
                checkBox_fontBoldTitle.IsChecked = Settings.Instance.FontBoldTitle;
                textBox_fontTunerSize.Text = Settings.Instance.TunerFontSize.ToString();
                textBox_fontTunerSizeService.Text = Settings.Instance.TunerFontSizeService.ToString();
                checkBox_fontTunerBoldService.IsChecked = Settings.Instance.TunerFontBoldService;

                var colorReference = ColorDef.ColorNames.ToDictionary
                    (name => name, name => new ColorReferenceViewItem(name, ColorDef.Instance.ColorTable[name]));
                colorReference["カスタム"] = new ColorReferenceViewItem("カスタム", this.Resources["HatchBrush"] as VisualBrush);

                var setComboColor1 = new Action<string, ComboBox>((name, cmb) =>
                {
                    cmb.ItemsSource = colorReference.Values;
                    cmb.SelectedItem = colorReference[name];
                });
                var setComboColors = new Action<List<string>, Panel>((list, pnl) =>
                {
                    foreach (var cmb in pnl.Children.OfType<ComboBox>())
                    {
                        int idx = int.Parse((string)cmb.Tag);
                        setComboColor1(list[idx], cmb);
                    }
                });

                //番組表のフォント色と予約枠色はSettingsが個別のため個別処理。
                //これをまとめて出来るようにSettingsを変えると以前の設定が消える。
                setComboColor1(Settings.Instance.TitleColor1, comboBox_colorTitle1);
                setComboColor1(Settings.Instance.TitleColor2, comboBox_colorTitle2);
                setComboColors(Settings.Instance.ContentColorList, grid_EpgColors);
                setComboColor1(Settings.Instance.ReserveRectColorNormal, comboBox_reserveNormal);
                setComboColor1(Settings.Instance.ReserveRectColorNo, comboBox_reserveNo);
                setComboColor1(Settings.Instance.ReserveRectColorNoTuner, comboBox_reserveNoTuner);
                setComboColor1(Settings.Instance.ReserveRectColorWarning, comboBox_reserveWarning);
                setComboColor1(Settings.Instance.ReserveRectColorAutoAddMissing, comboBox_reserveAutoAddMissing);
                setComboColors(Settings.Instance.TunerServiceColors, grid_TunerFontColor);
                setComboColors(Settings.Instance.TunerServiceColors, grid_TunerColors);

                var setButtonColor1 = new Action<uint, Button>((clr, btn) => btn.Background = new SolidColorBrush(ColorDef.FromUInt(clr)));
                var setButtonColors = new Action<List<uint>, Panel>((list, pnl) =>
                {
                    foreach (var btn in pnl.Children.OfType<Button>())
                    {
                        int idx = int.Parse((string)btn.Tag);
                        setButtonColor1(list[idx], btn);
                    }
                });
                setButtonColor1(Settings.Instance.TitleCustColor1, button_colorTitle1);
                setButtonColor1(Settings.Instance.TitleCustColor2, button_colorTitle2);
                setButtonColors(Settings.Instance.ContentCustColorList, grid_EpgColors);
                setButtonColors(Settings.Instance.ContentCustColorList, grid_EpgColorsReserve);
                setButtonColors(Settings.Instance.TunerServiceCustColors, grid_TunerFontColor);
                setButtonColors(Settings.Instance.TunerServiceCustColors, grid_TunerColors);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
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
                Settings.Instance.EpgInfoSingleClick = (checkBox_singleOpen.IsChecked == true);
                Settings.Instance.EpgInfoOpenMode = (byte)(checkBox_openInfo.IsChecked == true ? 1 : 0);
                Settings.Instance.MouseScrollAuto = (checkBox_scrollAuto.IsChecked == true);
                Settings.Instance.DisplayNotifyEpgChange = (checkBox_displayNotifyChange.IsChecked == true);
                Settings.Instance.ReserveRectBackground = (checkBox_reserveBackground.IsChecked == true);
                Settings.Instance.TunerMinHeight = Convert.ToDouble(textBox_tuner_minHeight.Text);
                Settings.Instance.TunerMinimumLine = Convert.ToDouble(textBox_tunerMinLineHeight.Text);
                Settings.Instance.TunerDragScroll = Convert.ToDouble(textBox_tunerDdragScroll.Text);
                Settings.Instance.TunerScrollSize = Convert.ToDouble(textBox_tuner_mouse_scroll.Text);
                Settings.Instance.TunerMouseScrollAuto = (checkBox_tuner_scrollAuto.IsChecked == true);
                Settings.Instance.TunerWidth = Convert.ToDouble(textBox_tuner_width.Text);
                Settings.Instance.TunerServiceNoWrap = (checkBox_tuner_service_nowrap.IsChecked == true);
                Settings.Instance.TunerTitleIndent = (checkBox_tuner_title_indent.IsChecked == true);
                Settings.Instance.TunerPopup = (checkBox_tuner_popup.IsChecked == true);
                Settings.Instance.TunerPopupRecinfo = (checkBox_tuner_popup_recInfo.IsChecked == true);
                Settings.Instance.TunerInfoSingleClick = (checkBox_tunerSingleOpen.IsChecked == true);
                Settings.Instance.TunerColorModeUse = (checkBox_tunerColorModeUse.IsChecked == true);

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

                if (comboBox_fontTuner.SelectedItem != null)
                {
                    Settings.Instance.TunerFontName = comboBox_fontTuner.SelectedItem as string;
                }
                Settings.Instance.TunerFontSize = Convert.ToDouble(textBox_fontTunerSize.Text);
                if (comboBox_fontTunerService.SelectedItem != null)
                {
                    Settings.Instance.TunerFontNameService = comboBox_fontTunerService.SelectedItem as string;
                }
                Settings.Instance.TunerFontSizeService = Convert.ToDouble(textBox_fontTunerSizeService.Text);
                Settings.Instance.TunerFontBoldService = (checkBox_fontTunerBoldService.IsChecked == true);

                Settings.Instance.UseCustomEpgView = (radioButton_1_cust.IsChecked == true);
                Settings.Instance.CustomEpgTabList = listBox_tab.Items.OfType<CustomEpgTabInfo>().ToList();
                Settings.SetCustomEpgTabInfoID();

                var getComboColor1 = new Func<ComboBox, string>((cmb) => ((ColorReferenceViewItem)(cmb.SelectedItem)).ColorName);
                var getComboColors = new Action<List<string>, Panel>((list, pnl) =>
                {
                    foreach (var cmb in pnl.Children.OfType<ComboBox>())
                    {
                        int idx = int.Parse((string)cmb.Tag);
                        list[idx] = getComboColor1(cmb);
                    }
                });
                Settings.Instance.TitleColor1 = getComboColor1(comboBox_colorTitle1);
                Settings.Instance.TitleColor2 = getComboColor1(comboBox_colorTitle2);
                getComboColors(Settings.Instance.ContentColorList, grid_EpgColors);
                Settings.Instance.ReserveRectColorNormal = getComboColor1(comboBox_reserveNormal);
                Settings.Instance.ReserveRectColorNo = getComboColor1(comboBox_reserveNo);
                Settings.Instance.ReserveRectColorNoTuner = getComboColor1(comboBox_reserveNoTuner);
                Settings.Instance.ReserveRectColorWarning = getComboColor1(comboBox_reserveWarning);
                Settings.Instance.ReserveRectColorAutoAddMissing = getComboColor1(comboBox_reserveAutoAddMissing);
                getComboColors(Settings.Instance.TunerServiceColors, grid_TunerFontColor);
                getComboColors(Settings.Instance.TunerServiceColors, grid_TunerColors);

                var getButtonColor1 = new Func<Button, uint>((btn) => ColorDef.ToUInt((btn.Background as SolidColorBrush).Color));
                var getButtonColors = new Action<List<uint>, Panel>((list, pnl) =>
                {
                    foreach (var btm in pnl.Children.OfType<Button>())
                    {
                        int idx = int.Parse((string)btm.Tag);
                        list[idx] = getButtonColor1(btm);
                    }
                });

                Settings.Instance.TitleCustColor1 = getButtonColor1(button_colorTitle1);
                Settings.Instance.TitleCustColor2 = getButtonColor1(button_colorTitle2);
                getButtonColors(Settings.Instance.ContentCustColorList, grid_EpgColors);
                getButtonColors(Settings.Instance.ContentCustColorList, grid_EpgColorsReserve);
                getButtonColors(Settings.Instance.TunerServiceCustColors, grid_TunerFontColor);
                getButtonColors(Settings.Instance.TunerServiceCustColors, grid_TunerColors);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
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

        private void button_Color_Click(object sender, RoutedEventArgs e)
        {
            var btn = sender as Button;

            var dlg = new ColorSetWindow();
            dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
            Color item = (btn.Background as SolidColorBrush).Color;
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                (btn.Background as SolidColorBrush).Color = item;
            }
        }

        private void checkBox_epg_popup_Click(object sender, RoutedEventArgs e)
        {
            checkBox_epg_popup_resOnly.IsEnabled = (checkBox_epg_popup.IsChecked == true);
        }

        private void checkBox_tuner_popup_Click(object sender, RoutedEventArgs e)
        {
            checkBox_tuner_popup_recInfo.IsEnabled = (checkBox_tuner_popup.IsChecked == true);
        }

        private void checkBox_tunerColorModeUse_Click(object sender, RoutedEventArgs e)
        {
            comboBox_tunerFontColorService.IsEnabled = (checkBox_tunerColorModeUse.IsChecked == false);
            button_tunerFontCustColorService.IsEnabled = (checkBox_tunerColorModeUse.IsChecked == false);
        }
    }
}

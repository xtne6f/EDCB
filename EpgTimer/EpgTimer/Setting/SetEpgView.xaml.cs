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
    using BoxExchangeEdit;

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

        private MenuSettingData ctxmSetInfo;
        private RadioBtnSelect epgPopupRadioBtns;
        private RadioBtnSelect tunerPopupRadioBtns;

        public bool IsChangeEpgArcLoadSetting { get; private set; }

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
                epgPopupRadioBtns = new RadioBtnSelect(radioButton_epg_popup_onOver, radioButton_epg_popup_onClick, radioButton_epg_popup_resOnly);
                epgPopupRadioBtns.Value = Settings.Instance.EpgPopupMode;
                textBox_epg_popup_Width.Text = Settings.Instance.EpgPopupWidth.ToString();
                checkBox_title_indent.IsChecked = Settings.Instance.EpgTitleIndent;
                checkBox_descToolTip.IsChecked = Settings.Instance.EpgToolTip;
                checkBox_toolTip_noView_only.IsChecked = Settings.Instance.EpgToolTipNoViewOnly;
                textBox_toolTipWait.Text = Settings.Instance.EpgToolTipViewWait.ToString();
                checkBox_epg_ExtInfo_table.IsChecked = Settings.Instance.EpgExtInfoTable;
                checkBox_epg_ExtInfo_popup.IsChecked = Settings.Instance.EpgExtInfoPopup;
                checkBox_epg_ExtInfo_tooltip.IsChecked = Settings.Instance.EpgExtInfoTooltip;
                checkBox_singleOpen.IsChecked = Settings.Instance.EpgInfoSingleClick;
                checkBox_scrollAuto.IsChecked = Settings.Instance.MouseScrollAuto;
                checkBox_gradation.IsChecked = Settings.Instance.EpgGradation;
                checkBox_gradationHeader.IsChecked = Settings.Instance.EpgGradationHeader;
                checkBox_openInfo.IsChecked = (Settings.Instance.EpgInfoOpenMode != 0);
                checkBox_displayNotifyChange.IsChecked = Settings.Instance.DisplayNotifyEpgChange;
                checkBox_reserveBackground.IsChecked = Settings.Instance.ReserveRectBackground;
                checkBox_epgLoadArcInfo.IsChecked = Settings.Instance.EpgLoadArcInfo;
                checkBox_epgNoDisplayOld.IsChecked = Settings.Instance.EpgNoDisplayOld;
                textBox_epgNoDisplayOldDays.Text = Settings.Instance.EpgNoDisplayOldDays.ToString();

                textBox_tuner_mouse_scroll.Text = Settings.Instance.TunerScrollSize.ToString();
                textBox_tuner_width.Text = Settings.Instance.TunerWidth.ToString();
                textBox_tuner_minHeight.Text = Settings.Instance.TunerMinHeight.ToString();
                textBox_tunerDdragScroll.Text = Settings.Instance.TunerDragScroll.ToString();
                textBox_tunerMinLineHeight.Text = Settings.Instance.TunerMinimumLine.ToString();
                checkBox_tuner_popup.IsChecked = Settings.Instance.TunerPopup;
                tunerPopupRadioBtns = new RadioBtnSelect(radioButton_tuner_popup_onOver, radioButton_tuner_popup_onClick);
                tunerPopupRadioBtns.Value = Settings.Instance.TunerPopupMode;
                checkBox_tuner_popup_recInfo.IsChecked = Settings.Instance.TunerPopupRecinfo;
                textBox_tuner_popup_Width.Text = Settings.Instance.TunerPopupWidth.ToString();
                checkBox_tuner_title_indent.IsChecked = Settings.Instance.TunerTitleIndent;
                checkBox_tunerDescToolTip.IsChecked = Settings.Instance.TunerToolTip;
                textBox_tunerToolTipWait.Text = Settings.Instance.TunerToolTipViewWait.ToString();
                checkBox_tunerSingleOpen.IsChecked = Settings.Instance.TunerInfoSingleClick;
                checkBox_tuner_scrollAuto.IsChecked = Settings.Instance.TunerMouseScrollAuto;
                checkBox_tuner_service_nowrap.IsChecked = Settings.Instance.TunerServiceNoWrap;
                checkBox_tunerColorModeUse.IsChecked = Settings.Instance.TunerColorModeUse;
                comboBox_tunerFontColorService.IsEnabled = !Settings.Instance.TunerColorModeUse;
                button_tunerFontCustColorService.IsEnabled = !Settings.Instance.TunerColorModeUse;
                checkBox_tuner_display_offres.IsChecked = Settings.Instance.TunerDisplayOffReserve;

                this.listBox_tab.KeyDown += ViewUtil.KeyDown_Enter(button_tab_chg);
                var bx = new BoxExchangeEditor(null, this.listBox_tab, true, true, true);
                bx.targetBoxAllowDoubleClick(bx.TargetBox, (sender, e) => button_tab_chg.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)));
                button_tab_del.Click += new RoutedEventHandler(bx.button_Delete_Click);
                button_tab_up.Click += new RoutedEventHandler(bx.button_Up_Click);
                button_tab_down.Click += new RoutedEventHandler(bx.button_Down_Click);
                button_tab_top.Click += new RoutedEventHandler(bx.button_Top_Click);
                button_tab_bottom.Click += new RoutedEventHandler(bx.button_Bottom_Click);

                radioButton_1_def.IsChecked = (Settings.Instance.UseCustomEpgView == false);
                radioButton_1_cust.IsChecked = (Settings.Instance.UseCustomEpgView != false);

                listBox_tab.Items.AddItems(Settings.Instance.CustomEpgTabList);
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

                var colorReference = ColorDef.ColorTable.ToDictionary
                    (item => item.Key, item => new ColorReferenceViewItem(item.Key, item.Value));
                colorReference["カスタム"] = new ColorReferenceViewItem("カスタム", this.Resources["HatchBrush"] as VisualBrush);

                var setComboColor1 = new Action<string, ComboBox>((name, cmb) =>
                {
                    cmb.ItemsSource = colorReference.Values;
                    try 
                    {
                        cmb.SelectedItem = colorReference[name];
                    }
                    catch
                    {
                        cmb.SelectedItem = colorReference["カスタム"];
                    }
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
                setComboColors(Settings.Instance.EpgEtcColors, grid_EpgTimeColors);
                setComboColors(Settings.Instance.EpgEtcColors, grid_EpgEtcColors);
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
                setButtonColors(Settings.Instance.EpgEtcCustColors, grid_EpgTimeColors);
                setButtonColors(Settings.Instance.EpgEtcCustColors, grid_EpgEtcColors);
                setButtonColors(Settings.Instance.TunerServiceCustColors, grid_TunerFontColor);
                setButtonColors(Settings.Instance.TunerServiceCustColors, grid_TunerColors);

                //録画済み一覧画面
                textBox_dropErrIgnore.Text = Settings.Instance.RecInfoDropErrIgnore.ToString();
                textBox_dropWrnIgnore.Text = Settings.Instance.RecInfoDropWrnIgnore.ToString();
                textBox_scrambleIgnore.Text = Settings.Instance.RecInfoScrambleIgnore.ToString();
                checkBox_playDClick.IsChecked = Settings.Instance.PlayDClick;
                checkBox_recinfo_errCritical.IsChecked = Settings.Instance.RecinfoErrCriticalDrops;
                checkBox_recNoYear.IsChecked = Settings.Instance.RecInfoNoYear;
                checkBox_recNoSecond.IsChecked = Settings.Instance.RecInfoNoSecond;
                checkBox_recNoDurSecond.IsChecked = Settings.Instance.RecInfoNoDurSecond;
                checkBox_ChacheOn.IsChecked = Settings.Instance.RecInfoExtraDataCache;
                checkBox_CacheOptimize.IsChecked = Settings.Instance.RecInfoExtraDataCacheOptimize;
                checkBox_CacheKeepConnect.IsChecked = Settings.Instance.RecInfoExtraDataCacheKeepConnect;
                if (CommonManager.Instance.NWMode == false)
                {
                    checkBox_CacheKeepConnect.IsEnabled = false;//{Binding}を破棄しているので注意
                }
                setComboColors(Settings.Instance.RecEndColors, grid_RecInfoBackColors);
                setButtonColors(Settings.Instance.RecEndCustColors, grid_RecInfoBackColors);

                //予約一覧・共通画面
                this.ctxmSetInfo = Settings.Instance.MenuSet.Clone();
                checkBox_displayAutoAddMissing.IsChecked = Settings.Instance.DisplayReserveAutoAddMissing;
                checkBox_resNoYear.IsChecked = Settings.Instance.ResInfoNoYear;
                checkBox_resNoSecond.IsChecked = Settings.Instance.ResInfoNoSecond;
                checkBox_resNoDurSecond.IsChecked = Settings.Instance.ResInfoNoDurSecond;

                setComboColor1(Settings.Instance.ListDefColor, cmb_ListDefFontColor);
                setComboColors(Settings.Instance.RecModeFontColors, grid_ReserveRecModeColors);
                setComboColors(Settings.Instance.ResBackColors, grid_ReserveBackColors);
                setComboColors(Settings.Instance.StatColors, grid_StatColors);

                setButtonColor1(Settings.Instance.ListDefCustColor, btn_ListDefFontColor);
                setButtonColors(Settings.Instance.RecModeFontCustColors, grid_ReserveRecModeColors);
                setButtonColors(Settings.Instance.ResBackCustColors, grid_ReserveBackColors);
                setButtonColors(Settings.Instance.StatCustColors, grid_StatColors);

                textBox_DisplayJumpTime.Text = Settings.Instance.DisplayNotifyJumpTime.ToString();
                checkBox_LaterTimeUse.IsChecked = Settings.Instance.LaterTimeUse;
                textBox_LaterTimeHour.Text = (Settings.Instance.LaterTimeHour + 24).ToString();
                checkBox_displayPresetOnSearch.IsChecked = Settings.Instance.DisplayPresetOnSearch;
                checkBox_toolTips.IsChecked = !Settings.Instance.NoToolTip;
                textBox_ToolTipsWidth.Text = Settings.Instance.ToolTipWidth.ToString();
                checkBox_NotNoStyle.ToolTip = string.Format("チェック時、テーマファイル「{0}」があればそれを、無ければ既定のテーマ(Aero)を適用します。", System.IO.Path.GetFileName(System.Reflection.Assembly.GetEntryAssembly().Location) + ".rd.xaml");
                checkBox_NotNoStyle.IsChecked = Settings.Instance.NoStyle == 0;
                checkBox_displayStatus.IsChecked = Settings.Instance.DisplayStatus;
                checkBox_displayStatusNotify.IsChecked = Settings.Instance.DisplayStatusNotify;
                checkBox_IsVisibleReserveView.IsChecked = Settings.Instance.IsVisibleReserveView;
                checkBox_IsVisibleRecInfoView.IsChecked = Settings.Instance.IsVisibleRecInfoView;
                checkBox_IsVisibleAutoAddView.IsChecked = Settings.Instance.IsVisibleAutoAddView;
                checkBox_IsVisibleAutoAddViewMoveOnly.IsChecked = Settings.Instance.IsVisibleAutoAddViewMoveOnly;

                foreach (var item in new Dictionary<object, string> {
                            { CtxmCode.ReserveView, "予約一覧" },{ CtxmCode.TunerReserveView, "使用予定チューナ" },
                            { CtxmCode.RecInfoView, "録画済み一案" },{ CtxmCode.EpgAutoAddView, "キーワード予約登録" },
                            { CtxmCode.ManualAutoAddView, "プログラム予約登録" },{ CtxmCode.EpgView, "番組表" } })
                {
                    wrapPanel_StartTab.Children.Add(new RadioButton { Tag = item.Key, Content = item.Value });
                }
                var rbtn = wrapPanel_StartTab.Children.OfType<RadioButton>()
                    .FirstOrDefault(item => item.Tag as CtxmCode? == Settings.Instance.StartTab);
                if (rbtn != null) rbtn.IsChecked = true;

                foreach (var item in new Dictionary<object, string> {
                            { Dock.Bottom, "下" },{ Dock.Top, "上" },{ Dock.Left, "左" },{ Dock.Right, "右" } })
                {
                    wrapPanel_MainViewButtonsDock.Children.Add(new RadioButton { Tag = item.Key, Content = item.Value });
                }
                rbtn = wrapPanel_MainViewButtonsDock.Children.OfType<RadioButton>()
                    .FirstOrDefault(item => item.Tag as Dock? == Settings.Instance.MainViewButtonsDock);
                if (rbtn != null) rbtn.IsChecked = true;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        public void SaveSetting()
        {
            try
            {
                Settings.Instance.ScrollSize = MenuUtil.MyToNumerical(textBox_mouse_scroll, Convert.ToDouble, 240);
                Settings.Instance.ServiceWidth = MenuUtil.MyToNumerical(textBox_service_width, Convert.ToDouble, double.MaxValue, 16, 16);//小さいと描画で落ちる
                Settings.Instance.MinHeight = MenuUtil.MyToNumerical(textBox_minHeight, Convert.ToDouble, double.MaxValue, 0.1, 2);
                Settings.Instance.MinimumHeight = MenuUtil.MyToNumerical(textBox_minimumHeight, Convert.ToDouble, double.MaxValue, 0, 0);
                Settings.Instance.DragScroll = MenuUtil.MyToNumerical(textBox_dragScroll, Convert.ToDouble, 1.5);
                Settings.Instance.EpgTitleIndent = (checkBox_title_indent.IsChecked == true);
                Settings.Instance.EpgToolTip = (checkBox_descToolTip.IsChecked == true);
                Settings.Instance.EpgToolTipNoViewOnly = (checkBox_toolTip_noView_only.IsChecked == true);
                Settings.Instance.EpgToolTipViewWait = MenuUtil.MyToNumerical(textBox_toolTipWait, Convert.ToInt32, Int32.MaxValue, Int32.MinValue, 1500);
                Settings.Instance.EpgPopup = (checkBox_epg_popup.IsChecked == true);
                Settings.Instance.EpgPopupMode = epgPopupRadioBtns.Value;
                Settings.Instance.EpgPopupWidth = MenuUtil.MyToNumerical(textBox_epg_popup_Width, Convert.ToDouble, double.MaxValue, 0, 1);
                Settings.Instance.EpgExtInfoTable = (checkBox_epg_ExtInfo_table.IsChecked == true);
                Settings.Instance.EpgExtInfoPopup = (checkBox_epg_ExtInfo_popup.IsChecked == true);
                Settings.Instance.EpgExtInfoTooltip = (checkBox_epg_ExtInfo_tooltip.IsChecked == true);
                Settings.Instance.EpgGradation = (checkBox_gradation.IsChecked == true);
                Settings.Instance.EpgGradationHeader = (checkBox_gradationHeader.IsChecked == true);
                Settings.Instance.EpgInfoSingleClick = (checkBox_singleOpen.IsChecked == true);
                Settings.Instance.EpgInfoOpenMode = (byte)(checkBox_openInfo.IsChecked == true ? 1 : 0);
                Settings.Instance.MouseScrollAuto = (checkBox_scrollAuto.IsChecked == true);
                Settings.Instance.DisplayNotifyEpgChange = (checkBox_displayNotifyChange.IsChecked == true);
                Settings.Instance.ReserveRectBackground = (checkBox_reserveBackground.IsChecked == true);
                IsChangeEpgArcLoadSetting = Settings.Instance.EpgLoadArcInfo != (checkBox_epgLoadArcInfo.IsChecked == true);
                Settings.Instance.EpgLoadArcInfo = (checkBox_epgLoadArcInfo.IsChecked == true);
                Settings.Instance.EpgNoDisplayOld = (checkBox_epgNoDisplayOld.IsChecked == true);
                Settings.Instance.EpgNoDisplayOldDays = MenuUtil.MyToNumerical(textBox_epgNoDisplayOldDays, Convert.ToDouble, double.MaxValue, double.MinValue, 1);

                Settings.Instance.TunerScrollSize = MenuUtil.MyToNumerical(textBox_tuner_mouse_scroll, Convert.ToDouble, 240);
                Settings.Instance.TunerWidth = MenuUtil.MyToNumerical(textBox_tuner_width, Convert.ToDouble, double.MaxValue, 16, 150);//小さいと描画で落ちる
                Settings.Instance.TunerMinHeight = MenuUtil.MyToNumerical(textBox_tuner_minHeight, Convert.ToDouble, double.MaxValue, 0.1, 2);
                Settings.Instance.TunerMinimumLine = MenuUtil.MyToNumerical(textBox_tunerMinLineHeight, Convert.ToDouble, double.MaxValue, 0, 0);
                Settings.Instance.TunerDragScroll = MenuUtil.MyToNumerical(textBox_tunerDdragScroll, Convert.ToDouble, 1.5);
                Settings.Instance.TunerMouseScrollAuto = (checkBox_tuner_scrollAuto.IsChecked == true);
                Settings.Instance.TunerServiceNoWrap = (checkBox_tuner_service_nowrap.IsChecked == true);
                Settings.Instance.TunerTitleIndent = (checkBox_tuner_title_indent.IsChecked == true);
                Settings.Instance.TunerToolTip = (checkBox_tunerDescToolTip.IsChecked == true);
                Settings.Instance.TunerToolTipViewWait = MenuUtil.MyToNumerical(textBox_tunerToolTipWait, Convert.ToInt32, Int32.MaxValue, Int32.MinValue, 1500);
                Settings.Instance.TunerPopup = (checkBox_tuner_popup.IsChecked == true);
                Settings.Instance.TunerPopupMode = tunerPopupRadioBtns.Value;
                Settings.Instance.TunerPopupRecinfo = (checkBox_tuner_popup_recInfo.IsChecked == true);
                Settings.Instance.TunerPopupWidth = MenuUtil.MyToNumerical(textBox_tuner_popup_Width, Convert.ToDouble, double.MaxValue, 0, 1);
                Settings.Instance.TunerInfoSingleClick = (checkBox_tunerSingleOpen.IsChecked == true);
                Settings.Instance.TunerColorModeUse = (checkBox_tunerColorModeUse.IsChecked == true);
                Settings.Instance.TunerDisplayOffReserve = (checkBox_tuner_display_offres.IsChecked == true);

                if (comboBox_font.SelectedItem != null)
                {
                    Settings.Instance.FontName = comboBox_font.SelectedItem as string;
                }
                Settings.Instance.FontSize = MenuUtil.MyToNumerical(textBox_fontSize, Convert.ToDouble, 72, 1, 12);
                if (comboBox_fontTitle.SelectedItem != null)
                {
                    Settings.Instance.FontNameTitle = comboBox_fontTitle.SelectedItem as string;
                }
                Settings.Instance.FontSizeTitle = MenuUtil.MyToNumerical(textBox_fontSizeTitle, Convert.ToDouble, 72, 1, 12);
                Settings.Instance.FontBoldTitle = (checkBox_fontBoldTitle.IsChecked == true);

                if (comboBox_fontTuner.SelectedItem != null)
                {
                    Settings.Instance.TunerFontName = comboBox_fontTuner.SelectedItem as string;
                }
                Settings.Instance.TunerFontSize = MenuUtil.MyToNumerical(textBox_fontTunerSize, Convert.ToDouble, 72, 1, 12);
                if (comboBox_fontTunerService.SelectedItem != null)
                {
                    Settings.Instance.TunerFontNameService = comboBox_fontTunerService.SelectedItem as string;
                }
                Settings.Instance.TunerFontSizeService = MenuUtil.MyToNumerical(textBox_fontTunerSizeService, Convert.ToDouble, 72, 1, 12);
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
                getComboColors(Settings.Instance.EpgEtcColors, grid_EpgTimeColors);
                getComboColors(Settings.Instance.EpgEtcColors, grid_EpgEtcColors);
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
                getButtonColors(Settings.Instance.EpgEtcCustColors, grid_EpgTimeColors);
                getButtonColors(Settings.Instance.EpgEtcCustColors, grid_EpgEtcColors);
                getButtonColors(Settings.Instance.TunerServiceCustColors, grid_TunerFontColor);
                getButtonColors(Settings.Instance.TunerServiceCustColors, grid_TunerColors);

                //録画済み一覧画面
                Settings.Instance.PlayDClick = (checkBox_playDClick.IsChecked == true);
                Settings.Instance.RecInfoDropErrIgnore = MenuUtil.MyToNumerical(textBox_dropErrIgnore, Convert.ToInt64, Settings.Instance.RecInfoDropErrIgnore);
                Settings.Instance.RecInfoDropWrnIgnore = MenuUtil.MyToNumerical(textBox_dropWrnIgnore, Convert.ToInt64, Settings.Instance.RecInfoDropWrnIgnore);
                Settings.Instance.RecInfoScrambleIgnore = MenuUtil.MyToNumerical(textBox_scrambleIgnore, Convert.ToInt64, Settings.Instance.RecInfoScrambleIgnore);
                Settings.Instance.RecinfoErrCriticalDrops = (checkBox_recinfo_errCritical.IsChecked == true);
                Settings.Instance.RecInfoNoYear = (checkBox_recNoYear.IsChecked == true);
                Settings.Instance.RecInfoNoSecond = (checkBox_recNoSecond.IsChecked == true);
                Settings.Instance.RecInfoNoDurSecond = (checkBox_recNoDurSecond.IsChecked == true);
                getComboColors(Settings.Instance.RecEndColors, grid_RecInfoBackColors);
                getButtonColors(Settings.Instance.RecEndCustColors, grid_RecInfoBackColors);
                Settings.Instance.RecInfoExtraDataCache = (checkBox_ChacheOn.IsChecked == true);
                Settings.Instance.RecInfoExtraDataCacheOptimize = (checkBox_CacheOptimize.IsChecked == true);
                Settings.Instance.RecInfoExtraDataCacheKeepConnect = (checkBox_CacheKeepConnect.IsChecked == true);

                //予約一覧画面
                Settings.Instance.MenuSet = this.ctxmSetInfo.Clone();
                Settings.Instance.DisplayReserveAutoAddMissing = (checkBox_displayAutoAddMissing.IsChecked != false);
                Settings.Instance.ResInfoNoYear = (checkBox_resNoYear.IsChecked == true);
                Settings.Instance.ResInfoNoSecond = (checkBox_resNoSecond.IsChecked == true);
                Settings.Instance.ResInfoNoDurSecond = (checkBox_resNoDurSecond.IsChecked == true);

                Settings.Instance.ListDefColor = getComboColor1(cmb_ListDefFontColor);
                getComboColors(Settings.Instance.RecModeFontColors, grid_ReserveRecModeColors);
                getComboColors(Settings.Instance.ResBackColors, grid_ReserveBackColors);
                getComboColors(Settings.Instance.StatColors, grid_StatColors);

                Settings.Instance.ListDefCustColor = getButtonColor1(btn_ListDefFontColor);
                getButtonColors(Settings.Instance.RecModeFontCustColors, grid_ReserveRecModeColors);
                getButtonColors(Settings.Instance.ResBackCustColors, grid_ReserveBackColors);
                getButtonColors(Settings.Instance.StatCustColors, grid_StatColors);

                Settings.Instance.DisplayNotifyJumpTime = MenuUtil.MyToNumerical(textBox_DisplayJumpTime, Convert.ToDouble, Double.MaxValue, 0, 3);
                Settings.Instance.LaterTimeUse = (checkBox_LaterTimeUse.IsChecked == true);
                Settings.Instance.LaterTimeHour = MenuUtil.MyToNumerical(textBox_LaterTimeHour, Convert.ToInt32, 36, 24, 28) - 24;
                Settings.Instance.DisplayPresetOnSearch = (checkBox_displayPresetOnSearch.IsChecked == true);
                Settings.Instance.NoStyle = (checkBox_NotNoStyle.IsChecked == true ? 0 : 1);
                Settings.Instance.NoToolTip = checkBox_toolTips.IsChecked == false;
                Settings.Instance.ToolTipWidth = MenuUtil.MyToNumerical(textBox_ToolTipsWidth, Convert.ToDouble, Double.MaxValue, 16, 400);
                Settings.Instance.DisplayStatus = (checkBox_displayStatus.IsChecked == true);
                Settings.Instance.DisplayStatusNotify = (checkBox_displayStatusNotify.IsChecked == true);
                Settings.Instance.IsVisibleReserveView = (checkBox_IsVisibleReserveView.IsChecked == true);
                Settings.Instance.IsVisibleRecInfoView = (checkBox_IsVisibleRecInfoView.IsChecked == true);
                Settings.Instance.IsVisibleAutoAddView = (checkBox_IsVisibleAutoAddView.IsChecked == true);
                Settings.Instance.IsVisibleAutoAddViewMoveOnly = (checkBox_IsVisibleAutoAddViewMoveOnly.IsChecked == true);

                CtxmCode? code = wrapPanel_StartTab.Children.OfType<RadioButton>()
                    .Where(btn => btn.IsChecked == true).Select(btn => btn.Tag as CtxmCode?).FirstOrDefault();
                if (code != null) Settings.Instance.StartTab = (CtxmCode)code;

                Dock? dock = wrapPanel_MainViewButtonsDock.Children.OfType<RadioButton>()
                    .Where(btn => btn.IsChecked == true).Select(btn => btn.Tag as Dock?).FirstOrDefault();
                if (dock != null) Settings.Instance.MainViewButtonsDock = (Dock)dock;
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }

        private void button_tab_add_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new EpgDataViewSettingWindow();
            dlg.Owner = CommonUtil.GetTopWindow(this);
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
                dlg.Owner = CommonUtil.GetTopWindow(this);
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
                listBox_tab.Items.AddItems(items);
                listBox_tab.UnselectAll();
                listBox_tab.SelectedItemsAdd(items);
                listBox_tab.ScrollIntoViewIndex(int.MaxValue);
            }
        }

        private void button_Color_Click(object sender, RoutedEventArgs e)
        {
            var btn = sender as Button;

            var dlg = new ColorSetWindow();
            dlg.Owner = CommonUtil.GetTopWindow(this);
            Color item = (btn.Background as SolidColorBrush).Color;
            dlg.SetColor(item);
            if (dlg.ShowDialog() == true)
            {
                dlg.GetColor(ref item);
                (btn.Background as SolidColorBrush).Color = item;
            }
        }

        private void checkBox_tunerColorModeUse_Click(object sender, RoutedEventArgs e)
        {
            comboBox_tunerFontColorService.IsEnabled = (checkBox_tunerColorModeUse.IsChecked == false);
            button_tunerFontCustColorService.IsEnabled = (checkBox_tunerColorModeUse.IsChecked == false);
        }

        private void button_set_cm_Click(object sender, RoutedEventArgs e)
        {
            var dlg = new SetContextMenuWindow();
            dlg.Owner = CommonUtil.GetTopWindow(this);
            dlg.info = this.ctxmSetInfo.Clone();
            if (dlg.ShowDialog() == true)
            {
                this.ctxmSetInfo = dlg.info.Clone();
            }
        }
    }
}

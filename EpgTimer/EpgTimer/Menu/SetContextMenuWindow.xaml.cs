using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace EpgTimer
{
    using ComboItem = KeyValuePair<CtxmCode, string>;

    /// <summary>
    /// SetContextMenuWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SetContextMenuWindow : Window
    {
        private MenuManager mm = CommonManager.Instance.MM;
        private BoxExchangeEditor bx = new BoxExchangeEditor();

        private static ComboItem[] MenuCodeToTitle = new ComboItem[]{
            new ComboItem(CtxmCode.ReserveView, "予約一覧"),
            new ComboItem(CtxmCode.TunerReserveView, "使用予定チューナ"),
            new ComboItem(CtxmCode.RecInfoView, "録画済み一覧"),
            new ComboItem(CtxmCode.EpgAutoAddView, "EPG自動予約登録"),
            new ComboItem(CtxmCode.ManualAutoAddView, "プログラム自動予約登録"),
            new ComboItem(CtxmCode.EpgView, "番組表(共通)"),
            new ComboItem(CtxmCode.SearchWindow, "検索/EPG自動登録ダイアログ"),
            new ComboItem(CtxmCode.EditChgMenu, "[編集]サブメニュー")
        };

        private static List<List<ICommand>> SettingTable = new List<List<ICommand>>{
            new List<ICommand>{EpgCmds.ChgOnOff},
            new List<ICommand>{EpgCmds.Delete},
            new List<ICommand>{EpgCmds.Delete2},
            new List<ICommand>{EpgCmds.ProtectChange},
            new List<ICommand>{EpgCmds.ShowAddDialog},

            new List<ICommand>{EpgCmds.JumpTable},
            new List<ICommand>{EpgCmds.ToAutoadd},
            new List<ICommand>{EpgCmds.ReSearch},
            new List<ICommand>{EpgCmds.ReSearch2},
            new List<ICommand>{EpgCmds.Play},
            new List<ICommand>{EpgCmds.OpenFolder, EpgCmdsEx.OpenFolderMenu},

            new List<ICommand>{EpgCmds.CopyTitle},
            new List<ICommand>{EpgCmds.CopyContent},
            new List<ICommand>{EpgCmds.SearchTitle},
            new List<ICommand>{EpgCmds.CopyNotKey},
            new List<ICommand>{EpgCmds.SetNotKey},

            new List<ICommand>{EpgCmds.MenuSetting}
        };

        public MenuSettingData info;
        List<CtxmSetting> defaultMenu;
        List<CtxmSetting> editMenu;

        List<CheckBox> stackItems_menu;
        List<CheckBox> stackItems_ges1;
        List<CheckBox> stackItems_ges2;

        public SetContextMenuWindow()
        {
            InitializeComponent();
            try
	        {
                //共通設定画面用の設定
                Action<StackPanel> SetCheckBox = (trg) =>
                {
                    foreach (var item in trg.Children.OfType<CheckBox>())
                    {
                        item.Height = 15;
                        item.Margin = new Thickness(6, 0, 0, 0);
                    }
                };
                SetCheckBox(stackPanel_menu);
                SetCheckBox(stackPanel_option);

                Action<StackPanel, StackPanel> CopyStackItem = (src, trg) =>
                {
                    foreach (var item in src.Children.OfType<Control>())
                    {
                        Control newItem = null;
                        if (item is Separator)
                        {
                            newItem = new Separator();
                        }
                        else if (item is CheckBox)
                        {
                            newItem = new CheckBox();
                        }
                        if (newItem != null)
                        {
                            newItem.Height = item.Height;
                            newItem.Margin = item.Margin;
                            trg.Children.Add(newItem);
                        }
                    }
                };
                CopyStackItem(stackPanel_menu, stackPanel_gesture);
                CopyStackItem(stackPanel_menu, stackPanel_gesture2);
                
                stackItems_menu = stackPanel_menu.Children.OfType<CheckBox>().ToList();
                stackItems_ges1 = stackPanel_gesture.Children.OfType<CheckBox>().ToList();
                stackItems_ges2 = stackPanel_gesture2.Children.OfType<CheckBox>().ToList();

                //個別設定画面用の設定
                this.comboBoxViewSelect.DisplayMemberPath = "Value";
                this.comboBoxViewSelect.SelectedValuePath = "Key";

                bx.SourceBox = this.listBox_Default;
                bx.TargetBox = this.listBox_Setting;
                bx.DuplicationSpecific = new List<object> { EpgCmdsEx.SeparatorString };
                button_reset.Click += new RoutedEventHandler(bx.button_reset_Click);
                button_add.Click += new RoutedEventHandler(bx.button_add_Click);
                button_del.Click += new RoutedEventHandler(bx.button_del_Click);
                button_delAll.Click += new RoutedEventHandler(bx.button_delAll_Click);
                button_top.Click += new RoutedEventHandler(bx.button_top_Click);
                button_up.Click += new RoutedEventHandler(bx.button_up_Click);
                button_down.Click += new RoutedEventHandler(bx.button_down_Click);
                button_bottom.Click += new RoutedEventHandler(bx.button_bottom_Click);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            SetData();
        }

        private void button_Initialize_Click(object sender, RoutedEventArgs e)
        {
            info = mm.GetDefaultMenuSettingData();
            SetData();
        }

        private void SetData()
        {
            try
            {
                checkBox_ManualMenu.IsChecked = info.IsManualMenu;
                checkBox_NoMessageKeyGesture.IsChecked = info.NoMessageKeyGesture;
                checkBox_NoMessageDeleteAll.IsChecked = info.NoMessageDeleteAll;
                checkBox_NoMessageDelete2.IsChecked = info.NoMessageDelete2;
                checkBox_EpgKeyword_Trim.IsChecked = info.Keyword_Trim;
                checkBox_CopyTitle_Trim.IsChecked = info.CopyTitle_Trim;
                checkBox_CopyContentBasic.IsChecked = info.CopyContentBasic;
                checkBox_SearchTtile_Trim.IsChecked = info.SearchTitle_Trim;
                textBox_SearchURI.Text = info.SearchURI;
                checkBox_NoMessageNotKEY.IsChecked = info.NoMessageNotKEY;

                defaultMenu = mm.GetDefaultCtxmSettingForEditor();
                editMenu = info.ManualMenuItems.Clone();

                for (int i = 0; i < SettingTable.Count; i++)
                {
                    MenuSettingData.CmdSaveData src = info.EasyMenuItems.Find(item => 
                        item.GetCommand() == SettingTable[i][0]);

                    stackItems_menu[i].IsChecked = src.IsMenuEnabled;
                    stackItems_ges1[i].IsChecked = src.IsGestureEnabled;
                    stackItems_ges2[i].IsChecked = !src.IsGesNeedMenu;

                    stackItems_menu[i].Content = src.GetCommand().Text;
                    stackItems_ges1[i].Content = MenuBinds.GetInputGestureText(src.GetGestuers());
                    stackItems_ges2[i].Content = "使用する";

                    stackItems_ges1[i].Visibility = (stackItems_ges1[i].Content == null) ? Visibility.Hidden : Visibility.Visible;
                    stackItems_ges2[i].Visibility = stackItems_ges1[i].Visibility;
                }

                this.listBox_Default.ItemsSource = null;
                this.listBox_Setting.ItemsSource = null;//初期化ボタンでSetData()使うとき用のリセット。
                this.comboBoxViewSelect.ItemsSource = MenuCodeToTitle;
                this.comboBoxViewSelect.SelectedIndex = -1; //初期化ボタンでSetData()使うとき用のリセット。
                this.comboBoxViewSelect.SelectedIndex = 0; //これでSelectionChanged発生する
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (comboBoxViewSelect.SelectedValue != null)
                {
                    var code = (CtxmCode)this.comboBoxViewSelect.SelectedValue;
                    editMenu.FindData(code).Items = this.listBox_Setting.Items.Cast<string>().ToList();
                }

                for (int i = 0; i < SettingTable.Count; i++)
                {
                    SettingTable[i].ForEach(icmd =>
                    {
                        MenuSettingData.CmdSaveData trg = info.EasyMenuItems.Find(item => item.GetCommand() == icmd);
                        trg.IsMenuEnabled = stackItems_menu[i].IsChecked == true;
                        trg.IsGestureEnabled = stackItems_ges1[i].IsChecked == true;
                        trg.IsGesNeedMenu = stackItems_ges2[i].IsChecked == false;
                    });
                }

                info.IsManualMenu = (checkBox_ManualMenu.IsChecked == true);
                info.NoMessageKeyGesture = (checkBox_NoMessageKeyGesture.IsChecked == true);
                info.NoMessageDeleteAll = (checkBox_NoMessageDeleteAll.IsChecked == true);
                info.NoMessageDelete2 = (checkBox_NoMessageDelete2.IsChecked == true);
                info.ManualMenuItems = editMenu.Clone();
                info.Keyword_Trim = (checkBox_EpgKeyword_Trim.IsChecked == true);
                info.CopyTitle_Trim = (checkBox_CopyTitle_Trim.IsChecked == true);
                info.CopyContentBasic = (checkBox_CopyContentBasic.IsChecked == true);
                info.SearchTitle_Trim = (checkBox_SearchTtile_Trim.IsChecked == true);
                info.SearchURI = textBox_SearchURI.Text;
                info.NoMessageNotKEY = (checkBox_NoMessageNotKEY.IsChecked == true);

                DialogResult = true;
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                DialogResult = false;
            }
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }

        protected override void OnKeyDown(KeyEventArgs e)
        {
            if (Keyboard.Modifiers == ModifierKeys.None)
            {
                switch (e.Key)
                {
                    case Key.Escape:
                        this.button_cancel.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        break;
                }
            }
            base.OnKeyDown(e);
        }

        private void comboBoxViewSelect_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (comboBoxViewSelect.SelectedIndex == -1) return;

                if (this.listBox_Default.ItemsSource != null)
                {
                    CtxmCode oldcode = ((ComboItem)e.RemovedItems[0]).Key;
                    editMenu.FindData(oldcode).Items = this.listBox_Setting.Items.Cast<string>().ToList();
                }
                CtxmCode newcode = ((ComboItem)e.AddedItems[0]).Key;
                this.listBox_Default.ItemsSource = defaultMenu.FindData(newcode).Items;
                this.listBox_Setting.Items.Clear();
                editMenu.FindData(newcode).Items.ForEach(item => this.listBox_Setting.Items.Add(item));

                switch (newcode)
                {
                    case CtxmCode.EpgView:
                        textblocExp.Text = "・「番組表(標準モード)へジャンプ」は標準モードでは非表示になります。";
                        break;
                    case CtxmCode.EditChgMenu:
                        textblocExp.Text = "・[編集]サブメニューは「表示項目は個別設定を使用する」の設定に関わらず、常にこの個別設定が反映されます。\r\n\r\n"
                            + "・「まとめてジャンル絞り込みを変更」は、EPG自動予約登録画面のみ表示されます。\r\n\r\n"
                            + "・「イベントリレー追従」「ぴったり（？）録画」は、プログラム自動予約登録画面では表示されません。";
                            break;
                    default:
                        textblocExp.Text = "";
                        break;
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void checkBox_EpgKeyword_Trim_ValueChanged(object sender, RoutedEventArgs e)
        {
            checkBox_EpgKeyword_Trim2.IsChecked = checkBox_EpgKeyword_Trim.IsChecked;
            checkBox_EpgKeyword_Trim3.IsChecked = checkBox_EpgKeyword_Trim.IsChecked;
        }

        private void button_allDefault_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                editMenu = mm.GetDefaultMenuSettingData().ManualMenuItems;
                if (comboBoxViewSelect.SelectedValue != null)
                {
                    var code = (CtxmCode)this.comboBoxViewSelect.SelectedValue;
                    this.listBox_Setting.Items.Clear();
                    editMenu.FindData(code).Items.ForEach(item => this.listBox_Setting.Items.Add(item));
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            }
        }

        private void button_separator_Click(object sender, RoutedEventArgs e)
        {
            listBox_Setting.Items.Add(EpgCmdsEx.SeparatorString);
            listBox_Setting.SelectedIndex = listBox_Setting.Items.Count - 1;
            listBox_Setting.ScrollIntoView(listBox_Setting.SelectedItem);
        }
        
    }
}

using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

namespace EpgTimer
{
    using BoxExchangeEdit;

    /// <summary>
    /// SetApp2DelWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SetApp2DelWindow : Window
    {
        public List<String> extList = new List<string>();
        public List<String> delChkFolderList = new List<string>();
        public SetApp2DelWindow()
        {
            InitializeComponent();

            if (CommonManager.Instance.NWMode == true)
            {
                ViewUtil.ChangeChildren(grid_main, false);
                listBox_ext.IsEnabled = true;
                textBox_ext.SetReadOnlyWithEffect(true);
                listBox_chk_folder.IsEnabled = true;
                textBox_chk_folder.SetReadOnlyWithEffect(true);
                button_chk_open.IsEnabled = true;
                button_cancel.IsEnabled = true;
            }

            var bxe = new BoxExchangeEditor(null, listBox_ext, true);
            var bxc = new BoxExchangeEditor(null, listBox_chk_folder, true);
            listBox_ext.SelectionChanged += ViewUtil.ListBox_TextBoxSyncSelectionChanged(listBox_ext, textBox_ext);
            bxc.TargetBox.SelectionChanged += ViewUtil.ListBox_TextBoxSyncSelectionChanged(bxc.TargetBox, textBox_chk_folder);
            bxc.TargetBox.KeyDown += ViewUtil.KeyDown_Enter(button_chk_open);
            bxc.targetBoxAllowDoubleClick(bxc.TargetBox, (sender, e) => button_chk_open.RaiseEvent(new RoutedEventArgs(Button.ClickEvent)));
            if (CommonManager.Instance.NWMode == false)
            {
                bxe.AllowKeyAction();
                bxe.AllowDragDrop();
                button_del.Click += new RoutedEventHandler(bxe.button_Delete_Click);
                button_add.Click += ViewUtil.ListBox_TextCheckAdd(listBox_ext, textBox_ext);
                bxc.AllowKeyAction();
                bxc.AllowDragDrop();
                button_chk_del.Click += new RoutedEventHandler(bxc.button_Delete_Click);
                button_chk_add.Click += ViewUtil.ListBox_TextCheckAdd(listBox_chk_folder, textBox_chk_folder);

                textBox_ext.KeyDown += ViewUtil.KeyDown_Enter(button_add);
                textBox_chk_folder.KeyDown += ViewUtil.KeyDown_Enter(button_chk_add);
            }
        }

        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;

            extList = listBox_ext.Items.OfType<string>().ToList();
            delChkFolderList = listBox_chk_folder.Items.OfType<string>().ToList();
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            listBox_ext.Items.AddItems(extList);
            listBox_chk_folder.Items.AddItems(delChkFolderList);
        }

        private void button_chk_open_Click(object sender, RoutedEventArgs e)
        {
            CommonManager.GetFolderNameByDialog(textBox_chk_folder, "自動削除対象フォルダの選択", true);
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = false;
        }
    }
}

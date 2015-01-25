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
using System.Windows.Shapes;

namespace EpgTimer
{
    /// <summary>
    /// SetContextMenuWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SetContextMenuWindow : Window
    {
        public bool cmEpgKeyword_Trim = false;
        public bool cmAppendMenu = false;
        public bool cmCopyTitle = false;
        public bool cmCopyTitle_Trim = false;
        public bool cmCopyContent = false;
        public bool cmCopyContentBasic = false;
        public bool cmSearchTitle = false;
        public bool cmSearchTitle_Trim = false;
        public String cmSearchURI = "";

        public SetContextMenuWindow()
        {
            InitializeComponent();

            if (Settings.Instance.NoStyle == 0)
            {
                ResourceDictionary rd = new ResourceDictionary();
                rd.MergedDictionaries.Add(
                    Application.LoadComponent(new Uri("/PresentationFramework.Aero, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35;component/themes/aero.normalcolor.xaml", UriKind.Relative)) as ResourceDictionary
                    //Application.LoadComponent(new Uri("/PresentationFramework.Classic, Version=4.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35, ProcessorArchitecture=MSIL;component/themes/Classic.xaml", UriKind.Relative)) as ResourceDictionary
                    );
                this.Resources = rd;
            }

        }

        private void button_OK_Click(object sender, RoutedEventArgs e)
        {
            DialogResult = true;

            cmEpgKeyword_Trim = (checkBox_EpgKeyword_Trim.IsChecked == true);
            cmAppendMenu = (checkBox_AppendMenu.IsChecked == true);
            cmCopyTitle = (checkBox_CopyTitle.IsChecked == true);
            cmCopyTitle_Trim = (checkBox_CopyTitle_Trim.IsChecked == true);
            cmCopyContent = (checkBox_CopyContent.IsChecked == true);
            cmCopyContentBasic = (checkBox_CopyContentBasic.IsChecked == true);
            cmSearchTitle = (checkBox_SearchTtile.IsChecked == true);
            cmSearchTitle_Trim = (checkBox_SearchTtile_Trim.IsChecked == true);
            cmSearchURI = textBox_SearchURI.Text;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            checkBox_EpgKeyword_Trim.IsChecked = cmEpgKeyword_Trim;
            checkBox_AppendMenu.IsChecked = cmAppendMenu;
            checkBox_CopyTitle.IsChecked = cmCopyTitle;
            checkBox_CopyTitle_Trim.IsChecked = cmCopyTitle_Trim;
            checkBox_CopyContent.IsChecked = cmCopyContent;
            checkBox_CopyContentBasic.IsChecked = cmCopyContentBasic;
            checkBox_SearchTtile.IsChecked = cmSearchTitle;
            checkBox_SearchTtile_Trim.IsChecked = cmSearchTitle_Trim;
            textBox_SearchURI.Text=cmSearchURI;
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

    }
}

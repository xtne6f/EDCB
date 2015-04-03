using System;
using System.Diagnostics;
using System.Text.RegularExpressions;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Documents;
using System.Windows.Input;

namespace EpgTimer {
    /// <summary>
    /// 1. 予約詳細や番組詳細をポップアップ
    /// 2. キーワードでググる
    /// </summary>
    public partial class PopupWindow : Window {

        Process proc_Browser = null;

        public PopupWindow(Window owner0) {
            InitializeComponent();
            //
            this.Owner = owner0;
        }

        public void google(string keyword0) {
            //
            // 前後の記号を取り除く
            //
            // [二][字]NHKニュース7
            // ５．１[SS][字]NHK歌謡コンサート「人生の旅路に　この歌を」
            // 午後のロードショー「私がウォシャウスキー」魅惑のヒロイン特集[字][S][二]
            // 【Ｍｏｔｈｅｒ’ｓ　Ｄａｙ　Ｓｐｅｃｉａｌ】【映】バンガー・シスターズ
            string markExp1 =
                "(" +
                    "(\\[[^\\]]+\\])+" +
                    "|" +
                    "(【[^】]+】)+" +
                ")";
            foreach (string str1 in new string[] { "^((５．１)|" + markExp1 + ")+", markExp1 + "$" }) {
                keyword0 = Regex.Replace(keyword0, str1, string.Empty);
            }
            //
            this.proc_Browser = Process.Start("http://www.google.co.jp/#hl=ja&sclient=psy-ab&q=" + Uri.EscapeUriString(keyword0));
        }

        public void show(string text0) {
            //
            Point pnt_Client1 = Mouse.GetPosition(this.Owner);
            Point pnt_Screen1 = this.Owner.PointToScreen(pnt_Client1);
            this.Left = pnt_Screen1.X;
            if ((this.Left + this.Width) > SystemParameters.WorkArea.Width) {
                this.Left -= this.Width;
            }
            this.Top = pnt_Screen1.Y;
            if ((this.Top + this.Height) > SystemParameters.WorkArea.Height) {
                this.Top -= this.Height;
            }
            //
            this.myTextBlock.Text = text0;
            //
            this.richTextBox.Document =
                 new FlowDocument(
                     new Paragraph(
                         new Run(text0)));
            //
            base.Show();
        }

        protected override void OnPreviewMouseDown(MouseButtonEventArgs e) {
            this.border_TextBlock.Visibility = System.Windows.Visibility.Hidden;
            this.border_RichTextBox.Visibility = System.Windows.Visibility.Visible;
            //
            base.OnPreviewMouseDown(e);
        }

        protected override void OnMouseLeave(MouseEventArgs e) {
            base.OnMouseLeave(e);
            //
            Point cursor1 = Mouse.GetPosition(this);
            if (cursor1.X <= 0 || cursor1.X >= this.Width
                || cursor1.Y <= 0 || cursor1.Y >= this.Height) {
                base.Hide();
                this.border_TextBlock.Visibility = System.Windows.Visibility.Visible;
                this.border_RichTextBox.Visibility = System.Windows.Visibility.Hidden;
            }
        }

        protected override void OnKeyDown(KeyEventArgs e) {
            switch (e.Key) {
                case Key.F2:
                    if (string.IsNullOrWhiteSpace(this.richTextBox.Selection.Text)) {
                        MessageBox.Show("テキストが選択されていません！", "エラー (PopupWindow)");
                    } else {
                        this.google(this.richTextBox.Selection.Text);
                    }
                    break;
            }
            //
            base.OnKeyDown(e);
        }

        private void MenuItem_Google_Click(object sender, RoutedEventArgs e) {
            this.google(this.richTextBox.Selection.Text);
        }

        private void MenuItem_Google_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e) {
            MenuItem item1 = (MenuItem)sender;
            if (item1.IsVisible) {
                if (this.richTextBox.Selection.IsEmpty) {
                    item1.IsEnabled = false;
                } else {
                    item1.IsEnabled = true;
                }
            }
        }

    }

}

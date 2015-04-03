using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.ComponentModel;
using System.Windows.Controls;

namespace EpgTimer {
    /// <summary>
    /// Interaction logic for BlackoutWindow.xaml
    /// </summary>
    public partial class BlackoutWindow : Window {

        /// <summary>
        /// 番組表への受け渡し
        /// </summary>
        public static ReserveItem selectedReserveItem = null;

        /// <summary>
        /// 
        /// </summary>
        public static SearchItem selectedSearchItem = null;

        /// <summary>
        /// 番組表へジャンプした際に非表示にしたSearchWindow
        /// </summary>
        public static SearchWindow unvisibleSearchWindow = null;

        public BlackoutWindow(Window owner0) {
            InitializeComponent();
            //
            this.Owner = owner0;
            //
            this.WindowState = this.Owner.WindowState;
            this.Width = this.Owner.Width;
            this.Height = this.Owner.Height;
            //this.Topmost = true;
        }

        public void showWindow(string message0) {
            this.messageLabel.Content = message0 + "...";
            this.Show();
            //
            BackgroundWorker bgw1 = new BackgroundWorker();
            bgw1.DoWork += (object sender, DoWorkEventArgs e) => {
                System.Threading.Thread.Sleep(1000);
            };
            bgw1.RunWorkerCompleted += (object sender, RunWorkerCompletedEventArgs e) => {
                //this.Owner.Focus();
                this.Close();
            };
            bgw1.RunWorkerAsync();
        }

        /// <summary>
        /// Hide()した検索ウィンドウがあることを知らせるため、検索ボタンを点滅させる
        /// </summary>
        /// <param name="button0"></param>
        /// <param name="isBlink0"></param>
        public static void blinkSearchButton_Start(SearchWindow searchWindow0, Button button0) {
            BlackoutWindow.unvisibleSearchWindow = searchWindow0;
            //
            button0.Effect = new System.Windows.Media.Effects.DropShadowEffect();
            var animation = new System.Windows.Media.Animation.DoubleAnimation {
                From = 1.0,
                To = 0.7,
                RepeatBehavior = System.Windows.Media.Animation.RepeatBehavior.Forever,
                AutoReverse = true
            };
            button0.BeginAnimation(Button.OpacityProperty, animation);
        }

        public static void blinkSearchButton_Stop(SearchWindow searchWindow0, Button button0) {
            if (searchWindow0 == BlackoutWindow.unvisibleSearchWindow) {
                BlackoutWindow.unvisibleSearchWindow = null;
                //
                button0.BeginAnimation(Button.OpacityProperty, null);
                button0.Opacity = 1;
                button0.Effect = null;
            }
        }

    }
}

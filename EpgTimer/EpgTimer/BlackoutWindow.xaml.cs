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
        public static ReserveData selectedReserve = null;

        /// <summary>
        /// 
        /// </summary>
        public static EpgEventInfo selectedEventInfo = null;

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

    }
}

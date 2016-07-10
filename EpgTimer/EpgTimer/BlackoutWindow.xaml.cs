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
        public static object SelectedData = null;
        public static bool HasData { get { return SelectedData != null; } }

        public static SearchItem SelectedItem { get { return SelectedData as SearchItem; } }
        public static bool HasReserveData { get { return SelectedItem != null && SelectedItem.ReserveInfo != null; } }
        public static bool HasProgramData { get { return SelectedItem != null && SelectedItem.EventInfo != null; } }

        //番組表へジャンプ中
        public static bool NowJumpTable = false;

        public static void Clear()
        {
            SelectedData = null;
            NowJumpTable = false;
        }

        public static ulong Create64Key()
        {
            if (HasReserveData == true) return SelectedItem.ReserveInfo.Create64Key();
            if (HasProgramData == true) return SelectedItem.EventInfo.Create64Key();
            return 0;
        }

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

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
using System.Windows.Threading;

namespace EpgTimer
{
    /// <summary>
    /// SuspendCheckWindow.xaml の相互作用ロジック
    /// </summary>
    public partial class SuspendCheckWindow : Window
    {
        private DispatcherTimer countTimer;

        public SuspendCheckWindow()
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

            countTimer = new DispatcherTimer(DispatcherPriority.Normal);
            countTimer.Tick += new EventHandler(CountTimer);
            countTimer.Interval = TimeSpan.FromSeconds(1);
        }

        public void SetMode(Byte reboot, Byte suspendMode)
        {
            if (reboot == 1)
            {
                label1.Content = "再起動します。";
            }
            else
            {
                if (suspendMode == 1)
                {
                    label1.Content = "スタンバイに移行します。";
                }
                else if (suspendMode == 2)
                {
                    label1.Content = "休止に移行します。";
                }
                else if (suspendMode == 3)
                {
                    label1.Content = "シャットダウンします。";
                }
            }
        }

        private void CountTimer(object sender, EventArgs e)
        {
            if (progressBar.Value != 0)
            {
                progressBar.Value--;
            }
            else
            {
                countTimer.Stop();
                DialogResult = false;
            }
        }

        private void button_cancel_Click(object sender, RoutedEventArgs e)
        {
            countTimer.Stop();
            DialogResult = true;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            progressBar.Value = 20;
            countTimer.Start();
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            countTimer.Stop();
            if (DialogResult == null)
            {
                DialogResult = false;
            }
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

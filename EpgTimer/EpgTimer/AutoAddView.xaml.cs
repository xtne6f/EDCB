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
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace EpgTimer
{
    /// <summary>
    /// AutoAddView.xaml の相互作用ロジック
    /// </summary>
    public partial class AutoAddView : UserControl
    {
        public AutoAddView()
        {
            InitializeComponent();
        }

        public void RefreshMenu()
        {
            epgAutoAddView.RefreshMenu();
            manualAutoAddView.RefreshMenu();
        }

        public void SaveSize()
        {
            epgAutoAddView.SaveSize();
            manualAutoAddView.SaveSize();
        }

        public void UpdateAutoAddInfo()
        {
            epgAutoAddView.UpdateInfo();
            manualAutoAddView.UpdateInfo();
        }

    }
}

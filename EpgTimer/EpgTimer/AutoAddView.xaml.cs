using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Controls;

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

        public void SaveViewData()
        {
            epgAutoAddView.SaveViewData();
            manualAutoAddView.SaveViewData();
        }

        public void UpdateInfo()
        {
            epgAutoAddView.UpdateInfo();
            manualAutoAddView.UpdateInfo();
        }

    }
}

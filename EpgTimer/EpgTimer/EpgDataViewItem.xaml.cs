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

    interface IEpgDataViewItem
    {
        event ViewSettingClickHandler ViewSettingClick;
        bool ClearInfo();
        void RefreshMenu();
        void SetViewMode(CustomEpgTabInfo setInfo);
        void UpdateReserveData();
        void UpdateEpgData();
    }

    /// <summary>
    /// EpgDataViewItem.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgDataViewItem : UserControl
    {
        public event ViewSettingClickHandler ViewSettingClick = null;

        private CustomEpgTabInfo viewInfo = null;
        private IEpgDataViewItem viewCtrl = null;
        public EpgDataViewItem()
        {
            InitializeComponent();
        }

        public void ClearInfo()
        {
            //情報クリア
            if (viewCtrl != null)
            {
                viewCtrl.ClearInfo();
            }
            grid_main.Children.Clear();
            viewCtrl = null;
        }

        /// <summary>
        /// 現在のEPGデータ表示モードの設定を取得する
        /// </summary>
        /// <param name="setInfo">[OUT]表示モードの設定値</param>
        /// <returns></returns>
        public bool GetViewMode(ref CustomEpgTabInfo setInfo)
        {
            if (viewInfo == null) return false;

            viewInfo.CopyTo(ref setInfo);

            return true;
        }

        /// <summary>
        /// EPGデータの表示モードを設定する
        /// </summary>
        /// <param name="setInfo">[IN]表示モードの設定値</param>
        public void SetViewMode(CustomEpgTabInfo setInfo)
        {
            //表示モード一緒で、絞り込み内容変化のみ。
            if (viewInfo != null && viewCtrl != null)
            {
                if (viewInfo.ViewMode == setInfo.ViewMode)
                {
                    viewCtrl.SetViewMode(setInfo);
                    viewInfo = setInfo;
                    return;
                }
            }

            //切り替える場合
            viewInfo = setInfo;

            //情報クリア
            ClearInfo();

            switch (setInfo.ViewMode)
            {
                case 1://1週間表示
                    viewCtrl = new EpgWeekMainView();
                    break;
                case 2://リスト表示
                    viewCtrl = new EpgListMainView();
                    break;
                default://標準ラテ欄表示
                    viewCtrl = new EpgMainView();
                    break;
            }

            viewCtrl.ViewSettingClick += new ViewSettingClickHandler(item_ViewSettingClick);
            viewCtrl.SetViewMode(setInfo);
            grid_main.Children.Add(viewCtrl as UIElement);

        }

        public void UpdateReserveData()
        {
            if (viewCtrl == null) return;

            viewCtrl.UpdateReserveData();
        }

        public void UpdateEpgData()
        {
            if (viewCtrl == null) return;

            viewCtrl.UpdateEpgData();
        }

        private void item_ViewSettingClick(object sender, object param)
        {
            if (ViewSettingClick == null) return;

            ViewSettingClick(this, param);
        }

        public void RefreshMenu()
        {
            if (viewCtrl == null) return;

            viewCtrl.RefreshMenu();
        }

        public CustomEpgTabInfo ViewInfo 
        { 
            get { return this.viewInfo; }
        }
    }
}

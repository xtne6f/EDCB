using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

namespace EpgTimer
{

    interface IEpgDataViewItem
    {
        event ViewSettingClickHandler ViewSettingClick;
        void RefreshMenu();
        CustomEpgTabInfo GetViewMode();
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

        private IEpgDataViewItem viewCtrl = null;
        public EpgDataViewItem()
        {
            InitializeComponent();
        }

        /// <summary>
        /// 現在のEPGデータ表示モードの設定を取得する
        /// </summary>
        public CustomEpgTabInfo GetViewMode()
        {
            return viewCtrl == null ? null : viewCtrl.GetViewMode();
        }

        /// <summary>
        /// EPGデータの表示モードを設定する
        /// </summary>
        /// <param name="setInfo">[IN]表示モードの設定値</param>
        public void SetViewMode(CustomEpgTabInfo setInfo)
        {
            //表示モード一緒で、絞り込み内容変化のみ。
            if (viewCtrl != null)
            {
                CustomEpgTabInfo viewInfo = viewCtrl.GetViewMode();
                if (viewInfo != null && viewInfo.ViewMode == setInfo.ViewMode)
                {
                    viewInfo = setInfo.Clone();
                    viewCtrl.SetViewMode(viewInfo);
                    return;
                }
            }

            //切り替える場合
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
            grid_main.Children.Clear();
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
    }
}

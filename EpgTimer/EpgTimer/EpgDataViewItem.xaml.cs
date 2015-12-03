using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

using EpgTimer.EpgView;

namespace EpgTimer
{
    /// <summary>
    /// EpgDataViewItem.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgDataViewItem : UserControl
    {
        private EpgViewBase viewCtrl = null;
        public EpgDataViewItem()
        {
            InitializeComponent();
        }

        public void RefreshMenu()
        {
            if (viewCtrl != null) viewCtrl.RefreshMenu();
        }

        /// <summary>現在のEPGデータ表示モードの設定を取得する</summary>
        public CustomEpgTabInfo GetViewMode()
        {
            return viewCtrl == null ? null : viewCtrl.GetViewMode();
        }

        public void UpdateReserveData()
        {
            if (viewCtrl != null) viewCtrl.UpdateReserveData();
        }

        public void UpdateEpgData()
        {
            if (viewCtrl != null) viewCtrl.UpdateEpgData();
        }

        /// <summary>EPGデータの表示モードを設定する</summary>
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

        private void item_ViewSettingClick(object sender, object param)
        {
            try
            {
                if (param == null)
                {
                    var dlg = new EpgDataViewSettingWindow();
                    var topWindow = PresentationSource.FromVisual(this);
                    if (topWindow != null)
                    {
                        dlg.Owner = (Window)topWindow.RootVisual;
                    }
                    dlg.SetDefSetting(this.GetViewMode());
                    dlg.SetTrySetModeEnable();
                    if (Settings.Instance.UseCustomEpgView == false)
                    {
                        dlg.SetTrySetModeOnly();
                    }
                    if (dlg.ShowDialog() == true)
                    {
                        var setInfo = new CustomEpgTabInfo(); ;
                        dlg.GetSetting(ref setInfo);

                        if (Settings.Instance.UseCustomEpgView == true && Settings.Instance.TryEpgSetting == false)
                        {
                            if (setInfo.ID >= 0 && setInfo.ID <= Settings.Instance.CustomEpgTabList.Count)
                            {
                                Settings.Instance.CustomEpgTabList[setInfo.ID] = setInfo;
                                Settings.SaveToXmlFile();
                            }
                        }

                        this.SetViewMode(setInfo);
                    }
                }
                else
                {
                    CustomEpgTabInfo setInfo = param as CustomEpgTabInfo;
                    this.SetViewMode(setInfo);
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
            } 
        }

    }
}

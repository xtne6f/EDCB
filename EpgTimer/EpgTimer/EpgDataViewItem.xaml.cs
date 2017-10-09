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
    /// EpgDataViewItem.xaml の相互作用ロジック
    /// </summary>
    public partial class EpgDataViewItem : UserControl
    {
        private CustomEpgTabInfo viewInfo = null;
        private Object viewCtrl = null;
        public EpgDataViewItem()
        {
            InitializeComponent();
        }

        public void ClearInfo()
        {
            //情報クリア
            if (viewCtrl != null)
            {
                if (viewCtrl.GetType() == typeof(EpgListMainView))
                {
                    EpgListMainView item = viewCtrl as EpgListMainView;
                    item.ClearInfo();
                }
                else if (viewCtrl.GetType() == typeof(EpgMainView))
                {
                    EpgMainView item = viewCtrl as EpgMainView;
                    item.ClearInfo();
                }
                else if (viewCtrl.GetType() == typeof(EpgWeekMainView))
                {
                    EpgWeekMainView item = viewCtrl as EpgWeekMainView;
                    item.ClearInfo();
                }
            }
            grid_main.Children.Clear();
            viewCtrl = null;
        }

        /// <summary>
        /// EPGデータの表示モードを設定する
        /// </summary>
        /// <param name="setInfo">[IN]表示モードの設定値</param>
        public void SetViewMode(CustomEpgTabInfo setInfo)
        {
            if (viewInfo != null && viewCtrl != null)
            {
                if (viewInfo.ViewMode == setInfo.ViewMode)
                {
                    //表示モード一緒で、絞り込み内容変化
                    if (viewCtrl.GetType() == typeof(EpgListMainView))
                    {
                        EpgListMainView item = viewCtrl as EpgListMainView;
                        item.SetViewMode(setInfo);
                    }
                    else if (viewCtrl.GetType() == typeof(EpgMainView))
                    {
                        EpgMainView item = viewCtrl as EpgMainView;
                        item.SetViewMode(setInfo);
                    }
                    else if (viewCtrl.GetType() == typeof(EpgWeekMainView))
                    {
                        EpgWeekMainView item = viewCtrl as EpgWeekMainView;
                        item.SetViewMode(setInfo);
                    }

                    viewInfo = setInfo;
                    return;
                }
            }
            viewInfo = setInfo;

            //情報クリア
            if (viewCtrl != null)
            {
                if (viewCtrl.GetType() == typeof(EpgListMainView))
                {
                    EpgListMainView item = viewCtrl as EpgListMainView;
                    item.ClearInfo();
                }
                else if (viewCtrl.GetType() == typeof(EpgMainView))
                {
                    EpgMainView item = viewCtrl as EpgMainView;
                    item.ClearInfo();
                }
                else if (viewCtrl.GetType() == typeof(EpgWeekMainView))
                {
                    EpgWeekMainView item = viewCtrl as EpgWeekMainView;
                    item.ClearInfo();
                }
            }
            grid_main.Children.Clear();
            viewCtrl = null;


            switch (setInfo.ViewMode)
            {
                case 1:
                    //1週間表示
                    {
                        EpgWeekMainView item = new EpgWeekMainView();
                        item.ViewSettingClick += item_ViewSettingClick;
                        item.SetViewMode(setInfo);
                        grid_main.Children.Add(item);
                        viewCtrl = item;
                    }
                    break;
                case 2:
                    //リスト表示
                    {
                        EpgListMainView item = new EpgListMainView();
                        item.ViewSettingClick += item_ViewSettingClick;
                        item.SetViewMode(setInfo);
                        grid_main.Children.Add(item);
                        viewCtrl = item;
                    }
                    break;
                default:
                    //標準ラテ欄表示
                    {
                        EpgMainView item = new EpgMainView();
                        item.ViewSettingClick += item_ViewSettingClick;
                        item.SetViewMode(setInfo);
                        grid_main.Children.Add(item);
                        viewCtrl = item;
                    }
                    break;
            }
        }

        public void UpdateReserveData()
        {
            if (viewCtrl != null)
            {
                if (viewCtrl.GetType() == typeof(EpgListMainView))
                {
                    EpgListMainView item = viewCtrl as EpgListMainView;
                    item.UpdateReserveData();
                }
                else if (viewCtrl.GetType() == typeof(EpgMainView))
                {
                    EpgMainView item = viewCtrl as EpgMainView;
                    item.UpdateReserveData();
                }
                else if (viewCtrl.GetType() == typeof(EpgWeekMainView))
                {
                    EpgWeekMainView item = viewCtrl as EpgWeekMainView;
                    item.UpdateReserveData();
                }
            }
        }

        public void UpdateEpgData()
        {
            if (viewCtrl != null)
            {
                if (viewCtrl.GetType() == typeof(EpgListMainView))
                {
                    EpgListMainView item = viewCtrl as EpgListMainView;
                    item.UpdateEpgData();
                }
                else if (viewCtrl.GetType() == typeof(EpgMainView))
                {
                    EpgMainView item = viewCtrl as EpgMainView;
                    item.UpdateEpgData();
                }
                else if (viewCtrl.GetType() == typeof(EpgWeekMainView))
                {
                    EpgWeekMainView item = viewCtrl as EpgWeekMainView;
                    item.UpdateEpgData();
                }
            }
        }

        private void item_ViewSettingClick(object sender, CustomEpgTabInfo param)
        {
            if (param == null)
            {
                if (Settings.Instance.UseCustomEpgView == false)
                {
                    MessageBox.Show("デフォルト表示では設定を変更することはできません。");
                }
                else
                {
                    var dlg = new EpgDataViewSettingWindow();
                    dlg.Owner = (Window)PresentationSource.FromVisual(this).RootVisual;
                    dlg.SetDefSetting(viewInfo);
                    if (dlg.ShowDialog() == true)
                    {
                        param = new CustomEpgTabInfo();
                        dlg.GetSetting(ref param);
                        SetViewMode(param);
                    }
                }
            }
            else
            {
                SetViewMode(param);
            }
        }

        public CustomEpgTabInfo ViewInfo 
        { 
            get { return this.viewInfo; }
        }
    }
}

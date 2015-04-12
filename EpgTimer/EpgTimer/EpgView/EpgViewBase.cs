using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;

using CtrlCmdCLI;
using CtrlCmdCLI.Def;

namespace EpgTimer.EpgView
{
    public class EpgViewBase : UserControl, IEpgDataViewItem
    {
        protected CtrlCmdUtil cmd = CommonManager.Instance.CtrlCmd;
        protected MenuUtil mutil = CommonManager.Instance.MUtil;
        protected ViewUtil vutil = CommonManager.Instance.VUtil;

        protected CustomEpgTabInfo setViewInfo = null;
        protected List<UInt64> viewCustServiceList = null;
        protected Dictionary<UInt16, UInt16> viewCustContentKindList = new Dictionary<UInt16, UInt16>();

        protected bool updateEpgData = true;
        protected bool updateReserveData = true;

        protected Dictionary<UInt64, EpgServiceEventInfo> searchEventList = new Dictionary<UInt64, EpgServiceEventInfo>();

        public virtual event ViewSettingClickHandler ViewSettingClick = null;
        protected bool EnableViewSetting() { return ViewSettingClick != null; }
        protected void ViewSetting(object sender, object param)
        {
            if (EnableViewSetting() == false) return;
            ViewSettingClick(this, param);
        }

        public virtual bool ClearInfo()
        {
            searchEventList = new Dictionary<UInt64, EpgServiceEventInfo>();
            return true; 
        }

        public virtual void SetViewMode(CustomEpgTabInfo setInfo)
        {
            setViewInfo = setInfo;

            this.viewCustServiceList = setInfo.ViewServiceList;
            this.viewCustContentKindList.Clear();
            if (setInfo.ViewContentKindList != null)
            {
                setInfo.ViewContentKindList.ForEach(val => this.viewCustContentKindList.Add(val, val));
            }

            updateEpgData = !ReloadViewData();
        }

        protected virtual void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            if (this.IsVisible == true)
            {
                if (updateEpgData == true)
                {
                    updateEpgData = !ReloadViewData();
                }
                if (updateReserveData == true)
                {
                    updateReserveData = !ReloadReserveData();
                }
            }
        }

        /// <summary>
        /// 予約情報更新通知
        /// </summary>
        public void UpdateReserveData()
        {
            updateReserveData = true;
            if (this.IsVisible == true)
            {
                updateReserveData = !ReloadReserveData();
            }
        }

        protected virtual bool ReloadReserveData()
        {
            return vutil.ReloadReserveData();
        }

        /// <summary>
        /// EPGデータ更新通知
        /// </summary>
        public void UpdateEpgData()
        {
            updateEpgData = true;
            if (this.IsVisible == true || CommonManager.Instance.NWMode == false)
            {
                updateEpgData = !ReloadViewData();
            }
        }

        protected virtual bool ReloadViewData()
        {
            ClearInfo();
            if (ReloadEpgData() == false) return false;
            updateReserveData = !ReloadReserveData();
            return true;
        }

        protected virtual bool ReloadEpgData()
        {
            try
            {
                if (setViewInfo == null) return true;
                if (vutil.EpgTimerNWNotConnect() == true) return false;

                if (setViewInfo.SearchMode == false)
                {
                    ErrCode err = CommonManager.Instance.DB.ReloadEpgData();
                    if (CommonManager.CmdErrMsgTypical(err, "EPGデータの取得", this) == false) return false;
                }
                else
                {
                    //番組情報の検索
                    List<EpgSearchKeyInfo> keyList = mutil.GetList(setViewInfo.SearchKey);
                    var list = new List<EpgEventInfo>();

                    ErrCode err = (ErrCode)cmd.SendSearchPg(keyList, ref list);
                    if (CommonManager.CmdErrMsgTypical(err, "EPGデータの取得", this) == false) return false;

                    //サービス毎のリストに変換
                    var serviceEventList = new Dictionary<UInt64, EpgServiceEventInfo>();
                    foreach (EpgEventInfo eventInfo in list)
                    {
                        UInt64 id = eventInfo.Create64Key();
                        EpgServiceEventInfo serviceInfo;
                        if (serviceEventList.TryGetValue(id, out serviceInfo) == false)
                        {
                            if (ChSet5.Instance.ChList.ContainsKey(id) == false)
                            {
                                //サービス情報ないので無効
                                continue;
                            }
                            serviceInfo = new EpgServiceEventInfo();
                            serviceInfo.serviceInfo = CommonManager.ConvertChSet5To(ChSet5.Instance.ChList[id]);

                            serviceEventList.Add(id, serviceInfo);
                        }
                        serviceInfo.eventList.Add(eventInfo);

                    }
                    searchEventList = serviceEventList;
                }

                return true;
            }
            catch (Exception ex)
            {
                this.Dispatcher.BeginInvoke(new Action(() =>
                {
                    MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace);
                }), null);
                return false;
            }
        }

        protected virtual void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (this.IsVisible == false) return;

            if (BlackoutWindow.SelectedReserveItem != null)
            {
                MoveToReserveItem(BlackoutWindow.SelectedReserveItem);
            }
            else if (BlackoutWindow.SelectedSearchItem != null)
            {
                MoveToProgramItem(BlackoutWindow.SelectedSearchItem);
            }

            BlackoutWindow.Clear();
        }

        protected virtual void MoveToReserveItem(ReserveItem target) { }
        protected virtual void MoveToProgramItem(SearchItem target) { }

    }
}

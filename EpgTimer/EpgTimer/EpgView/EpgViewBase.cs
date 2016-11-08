using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows;
using System.Windows.Input;

namespace EpgTimer.EpgView
{
    public delegate void ViewSettingClickHandler(object sender, object param);

    public class EpgViewBase : EpgTimer.UserCtrlView.DataViewBase
    {
        protected static CtrlCmdUtil cmd { get { return CommonManager.Instance.CtrlCmd; } }

        protected CustomEpgTabInfo setViewInfo = null;
        protected Dictionary<UInt16, UInt16> viewCustContentKindList = new Dictionary<UInt16, UInt16>();
        protected List<EpgServiceEventInfo> serviceEventList = new List<EpgServiceEventInfo>();

        protected CmdExeReserve mc; //予約系コマンド集
        protected bool ReloadReserveInfoFlg = true;

        protected object restoreData = null;
        public virtual object GetViewState() { return null; }
        public virtual void SetViewState(object data) { restoreData = data; }

        protected virtual void InitCommand()
        {
            //ビューコードの登録
            mBinds.View = CtxmCode.EpgView;

            //コマンド集の初期化
            mc = new CmdExeReserve(this);

            //コマンド集にないものを登録
            mc.AddReplaceCommand(EpgCmds.ViewChgSet, (sender, e) => ViewSetting(this, null));
            mc.AddReplaceCommand(EpgCmds.ViewChgMode, mc_ViewChgMode);

            //コマンド集を振り替えるもの
            mc.AddReplaceCommand(EpgCmds.JumpTable, mc_JumpTable);
        }

        public virtual event ViewSettingClickHandler ViewSettingClick = null;
        protected bool EnableViewSetting() { return ViewSettingClick != null; }
        protected void ViewSetting(object sender, object param)
        {
            if (EnableViewSetting() == false) return;
            ViewSettingClick(sender, param);
        }

        /// <summary>右クリックメニュー 表示モードイベント呼び出し</summary>
        protected void mc_ViewChgMode(object sender, ExecutedRoutedEventArgs e)
        {
            try
            {
                var param = e.Parameter as EpgCmdParam;
                if (param == null || param.ID == setViewInfo.ViewMode) return;

                //BlackWindowに状態を登録。
                //コマンド集の機能による各ビューの共用メソッド。
                BlackoutWindow.SelectedData = mc.GetJumpTabItem();

                CustomEpgTabInfo setInfo = setViewInfo.Clone();
                setInfo.ViewMode = param.ID;
                ViewSetting(this, setInfo);
            }
            catch (Exception ex) { MessageBox.Show(ex.Message + "\r\n" + ex.StackTrace); }
        }
        protected void mc_JumpTable(object sender, ExecutedRoutedEventArgs e)
        {
            var param = e.Parameter as EpgCmdParam;
            if (param == null) return;

            param.ID = 0;//実際は設定するまでもなく、初期値0。
            BlackoutWindow.NowJumpTable = true;
            new BlackoutWindow(ViewUtil.MainWindow).showWindow(ViewUtil.MainWindow.tabItem_epg.Header.ToString());

            mc_ViewChgMode(sender, e);
        }

        public virtual void RefreshMenu()
        {
            mc.EpgInfoOpenMode = Settings.Instance.EpgInfoOpenMode;
        }

        public virtual CustomEpgTabInfo GetViewMode()
        {
            return setViewInfo == null ? null : setViewInfo.Clone();
        }
        public virtual void SetViewMode(CustomEpgTabInfo setInfo)
        {
            setViewInfo = setInfo.Clone();
            this.viewCustContentKindList = setViewInfo.ViewContentKindList.ToDictionary(id => id, id => id);

            ReloadInfoFlg = true;
        }

        /// 保存関係
        public virtual void SaveViewData() { }

        /// <summary>
        /// 予約情報更新通知
        /// </summary>
        public void UpdateReserveInfo(bool reload = true)
        {
            ReloadReserveInfoFlg |= reload;
            ReloadReserveInfo();
        }
        protected void ReloadReserveInfo()
        {
            if (ReloadReserveInfoFlg == true && this.IsVisible == true)
            {
                ReloadReserveInfoFlg = !ReloadReserveInfoData();
                UpdateStatus();
            }
        }
        protected bool ReloadReserveInfoData()
        {
            if (ViewUtil.ReloadReserveData() == false) return false;
            ReloadReserveViewItem();
            return true;
        }
        protected virtual void ReloadReserveViewItem() { }

        /// <summary>EPGデータ更新</summary>
        protected override bool ReloadInfoData()
        {
            if (ReloadEpgData() == false) return false;
            ReloadReserveInfoFlg = true;
            ReloadProgramViewItem();
            if (ReloadReserveInfoFlg == true) ReloadReserveInfoFlg = !ReloadReserveInfoData();
            restoreData = null;
            return true;
        }
        protected virtual void ReloadProgramViewItem() { }

        protected bool ReloadEpgData()
        {
            try
            {
                if (setViewInfo == null) return true;
                if (CommonManager.Instance.IsConnected == false) return false;

                Dictionary<UInt64, EpgServiceAllEventInfo> serviceDic;
                if (setViewInfo.SearchMode == false)
                {
                    ErrCode err = CommonManager.Instance.DB.ReloadEpgData();
                    if (CommonManager.CmdErrMsgTypical(err, "EPGデータの取得") == false) return false;
                    serviceDic = CommonManager.Instance.DB.ServiceEventList;
                }
                else
                {
                    //番組情報の検索
                    var list = new List<EpgEventInfo>();
                    ErrCode err = cmd.SendSearchPg(CommonUtil.ToList(setViewInfo.GetSearchKeyReloadEpg()), ref list);
                    if (CommonManager.CmdErrMsgTypical(err, "EPGデータの取得") == false) return false;

                    //サービス毎のリストに変換
                    serviceDic = list.GroupBy(info => info.Create64Key())
                        .Where(gr => ChSet5.ChList.ContainsKey(gr.Key) == true)
                        .ToDictionary(gr => gr.Key, gr => new EpgServiceAllEventInfo(ChSet5.ChList[gr.Key].ToInfo(), gr.ToList()));
                }

                //並び順はViewServiceListによる。eventListはこの後すぐ作り直すのでとりあえずそのままもらう。
                serviceEventList = setViewInfo.ViewServiceList.Distinct()
                    .Where(id => serviceDic.ContainsKey(id) == true).Select(id => serviceDic[id])
                    .Select(info => new EpgServiceEventInfo { serviceInfo = info.serviceInfo, eventList = info.eventMergeList }).ToList();

                var keyTime = DateTime.Now.AddDays(-Settings.Instance.EpgNoDisplayOldDays);
                foreach (EpgServiceEventInfo item in serviceEventList)
                {
                    item.eventList = item.eventList.FindAll(eventInfo =>
                        //開始時間未定を除外
                        (eventInfo.StartTimeFlag != 0)

                        //自動登録されたりするので、サービス別番組表では表示させる
                        //&& (eventInfo.IsGroupMainEvent == true)

                        //過去番組表示抑制
                        && (Settings.Instance.EpgNoDisplayOld == false || eventInfo.IsOver(keyTime) == false)

                        //ジャンル絞り込み
                        && (ViewUtil.ContainsContent(eventInfo, viewCustContentKindList, setViewInfo.ViewNotContentFlag) == true)
                    );
                }

                return true;
            }
            catch (Exception ex) { CommonUtil.DispatcherMsgBoxShow(ex.Message + "\r\n" + ex.StackTrace); }
            return false;
        }

        protected override void UserControl_IsVisibleChanged(object sender, DependencyPropertyChangedEventArgs e)
        {
            if (this.IsVisible == false) return;

            ReloadInfo();
            ReloadReserveInfo();//ReloadInfo()が実行された場合は、こちらは素通りになる。

            //「番組表へジャンプ」の場合、またはオプションで指定のある場合に強調表示する。
            bool isMarking = BlackoutWindow.NowJumpTable || Settings.Instance.DisplayNotifyEpgChange;
            if (BlackoutWindow.HasReserveData == true)
            {
                MoveToReserveItem(BlackoutWindow.SelectedItem.ReserveInfo, isMarking);
            }
            else if (BlackoutWindow.HasProgramData == true)
            {
                MoveToProgramItem(BlackoutWindow.SelectedItem.EventInfo, isMarking);
            }
            BlackoutWindow.Clear();

            RefreshStatus();
        }

        protected virtual void MoveToReserveItem(ReserveData target, bool IsMarking) { }
        protected virtual void MoveToProgramItem(EpgEventInfo target, bool IsMarking) { }
    }
}

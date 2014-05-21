#pragma once

#define MUTEX_TIME_SYNC_NAME			TEXT("Global\\EpgDataCap_Bon_TIME_SYNC")

//GUIの表示状態
#define GUI_NORMAL		0
#define GUI_CANCEL_ONLY	1
#define GUI_OPEN_FAIL	3
#define GUI_REC			4
#define GUI_REC_SET_TIME	5
#define GUI_OTHER_CTRL	6
#define GUI_REC_STANDBY	7

//内部タイマー
#define TIMER_STATUS_UPDATE		1000
#define TIMER_CHSCAN_STATSU		1001
#define TIMER_EPGCAP_STATSU		1002
#define TIMER_REC_END			1003
#define RETRY_ADD_TRAY			1006
#define TIMER_INIT_DLG			1007

#define WM_RESERVE_REC_START	(WM_USER + 50)
#define WM_RESERVE_REC_STOP		(WM_USER + 51)
#define WM_RESERVE_EPGCAP_START	(WM_USER + 52)
#define WM_RESERVE_EPGCAP_STOP	(WM_USER + 53)
#define WM_CHG_TUNER			(WM_USER + 54)
#define WM_CHG_CH				(WM_USER + 55)
#define WM_BACK_EPGCAP_START	(WM_USER + 56)
#define WM_BACK_EPGCAP_STOP		(WM_USER + 57)
#define WM_RESERVE_REC_STANDBY	(WM_USER + 58)
#define WM_INVOKE_CTRL_CMD		(WM_USER + 59)

#define WM_TRAY_PUSHICON (WM_USER+101) //トレイアイコン押された
#define TRAYICON_ID 200


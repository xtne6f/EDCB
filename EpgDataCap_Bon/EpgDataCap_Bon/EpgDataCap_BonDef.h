#pragma once

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
#define TIMER_REC_END			1003
#define TIMER_CHG_TRAY			1005
#define RETRY_ADD_TRAY			1006
#define TIMER_INIT_DLG			1007
#define TIMER_TRY_STOP_SERVER	1008

#define WM_INVOKE_CTRL_CMD		(WM_APP + 59)
#define WM_VIEW_APP_OPEN		(WM_APP + 60)

#define WM_TRAY_PUSHICON (WM_APP+101) //トレイアイコン押された
#define TRAYICON_ID 200


#pragma once

//サービス動作用のメイン
void WINAPI service_main(DWORD dwArgc, LPWSTR* lpszArgv);

//サービスからのコマンドのコールバック
DWORD WINAPI service_ctrl(DWORD dwControl, DWORD dwEventType, LPVOID lpEventData, LPVOID lpContext);

//サービスのステータス通知用
void ReportServiceStatus(DWORD dwCurrentState, DWORD dwControlsAccepted, DWORD dwCheckPoint, DWORD dwWaitHint);

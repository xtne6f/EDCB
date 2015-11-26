#include "StdAfx.h"
#include "resource.h"
#include "SettingDlg.h"

CDShowCtrl ctrl;
wstring grfPath = L"";
wstring ext = L"";

CSettingDlg::CSettingDlg(void)
{
}


CSettingDlg::~CSettingDlg(void)
{
}

DWORD CSettingDlg::CreateSettingDialog(HINSTANCE hInstance)
{
	DWORD dwRet = 0;

	WCHAR filePath[512] = L"";
	GetModuleFileName(hInstance, filePath, 512);

	wstring folder = L"";
	wstring fileTitle = L"";
	wstring iniPath = L"";

	GetFileFolder(filePath, folder);
	GetFileTitle(filePath, fileTitle);

	Format(grfPath, L"%s\\%s.grf", folder.c_str(), fileTitle.c_str());
	Format(iniPath, L"%s\\%s.ini", folder.c_str(), fileTitle.c_str());

	WCHAR buff[256] = L"";
	GetPrivateProfileString(L"SET", L"ext", L"ts", buff, 256, iniPath.c_str());
	ext = buff;


	HANDLE file = _CreateDirectoryAndFile( grfPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( file == INVALID_HANDLE_VALUE ){
		//設定ファイルないので新規作成
		ctrl.CreateNewGraph();
	}else{
		CloseHandle(file);
		//設定ファイルあるのでロード
		ctrl.LoadGraph(grfPath.c_str());
	}

	dwRet = (DWORD)DialogBox(hInstance,MAKEINTRESOURCE(IDD_DIALOG_SET), NULL, (DLGPROC)DlgProc );

	if( dwRet == IDOK ){
		WritePrivateProfileString( L"SET", L"ext", ext.c_str(), iniPath.c_str() );
	}
	return dwRet;
}

LRESULT CALLBACK CSettingDlg::DlgProc(HWND hDlgWnd, UINT msg, WPARAM wp, LPARAM lp)
{
	switch (msg) {
		case WM_KEYDOWN:
			if(wp == VK_RETURN){
				{
					WCHAR buff[256] = L"";
					GetWindowText(GetDlgItem(hDlgWnd, IDC_EDIT_EXT), buff, 256);
					ext = buff;
				}
				EndDialog(hDlgWnd, IDOK);
			}
			break;
		case WM_INITDIALOG:
			SetWindowText(GetDlgItem(hDlgWnd, IDC_EDIT_EXT), ext.c_str());
			return FALSE;
        case WM_COMMAND:
			switch (LOWORD(wp)) {
				case IDOK:
					{
						WCHAR buff[256] = L"";
						GetWindowText(GetDlgItem(hDlgWnd, IDC_EDIT_EXT), buff, 256);
						ext = buff;
						ctrl.SaveGraph(grfPath.c_str());
					}
					EndDialog(hDlgWnd, IDOK);
					break;
				case IDCANCEL:
					EndDialog(hDlgWnd, IDCANCEL);
					break;
				case IDC_BUTTON_VIDEO:
					ctrl.SetVideoEncProp();
					break;
				case IDC_BUTTON_VIDEO_SIZE:
					ctrl.SetVideoScaleProp();
					break;
				case IDC_BUTTON_AUDIO:
					ctrl.SetAudioEncProp();
					break;
				case IDC_BUTTON_MUX:
					ctrl.SetMuxerProp();
					break;
				default:
					return FALSE;
				}
		default:
			return FALSE;
	}
	return TRUE;
}


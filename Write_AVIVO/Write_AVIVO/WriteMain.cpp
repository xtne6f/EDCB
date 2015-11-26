#include "StdAfx.h"
#include "WriteMain.h"


CWriteMain::CWriteMain(void)
{

}


CWriteMain::~CWriteMain(void)
{

}

//ファイル保存を開始する
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// fileName				[IN]保存ファイルフルパス（必要に応じて拡張子変えたりなど行う）
// overWriteFlag		[IN]同一ファイル名存在時に上書きするかどうか（TRUE：する、FALSE：しない）
// createSize			[IN]入力予想容量（188バイトTSでの容量。即時録画時など総時間未定の場合は0。延長などの可能性もあるので目安程度）
BOOL CWriteMain::_StartSave(
	LPCWSTR fileName,
	BOOL overWriteFlag,
	ULONGLONG createSize,
	HINSTANCE hInstance
	)
{
	if( fileName == NULL ){
		return FALSE;
	}
	this->savePath = L"";

	this->savePath = fileName;

	WCHAR filePath[512] = L"";
	GetModuleFileName(hInstance, filePath, 512);

	wstring folder = L"";
	wstring fileTitle = L"";
	wstring iniPath = L"";

	GetFileFolder(filePath, folder);
	GetFileTitle(filePath, fileTitle);

	wstring grfPath = L"";
	Format(grfPath, L"%s\\%s.grf", folder.c_str(), fileTitle.c_str());
	Format(iniPath, L"%s\\%s.ini", folder.c_str(), fileTitle.c_str());

	WCHAR buff[256] = L"";
	GetPrivateProfileString(L"SET", L"ext", L"ts", buff, 256, iniPath.c_str());
	wstring ext = buff;
	if( ext.find(L".") == string::npos ){
		Format(ext, L".%s", buff);
	}

	HANDLE file = _CreateDirectoryAndFile( grfPath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( file == INVALID_HANDLE_VALUE ){
		//設定ファイルないので新規作成
		dshowCtrl.CreateNewGraph();
	}else{
		CloseHandle(file);
		//設定ファイルあるのでロード
		dshowCtrl.LoadGraph(grfPath.c_str());
	}

	wstring saveFileName = L"";
	wstring saveFolder = L"";
	GetFileFolder(fileName, saveFolder);
	GetFileTitle(fileName, saveFileName);
	wstring savePath = L"";
	Format(savePath, L"%s\\%s%s", saveFolder.c_str(), saveFileName.c_str(), ext.c_str());

	BOOL ret = dshowCtrl.CreateRunGraph(savePath.c_str());
	if( ret == TRUE ){
		this->savePath = savePath;
		//dshowCtrl.ConnectRunGraph();
	}
	return ret;
}

//ファイル保存を終了する
//戻り値：
// TRUE（成功）、FALSE（失敗）
BOOL CWriteMain::_StopSave(
	)
{
	if( dshowCtrl.IsRun() == TRUE ){
		dshowCtrl.StopGraph();
	}

	return TRUE;
}

//実際に保存しているファイルパスを取得する（再生やバッチ処理に利用される）
//filePathがNULL時は必要なサイズをfilePathSizeで返す
//通常filePathSize=512で呼び出し
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// filePath				[OUT]保存ファイルフルパス
// filePathSize			[IN/OUT]filePathのサイズ(WCHAR単位)
BOOL CWriteMain::_GetSaveFilePath(
	WCHAR* filePath,
	DWORD* filePathSize
	)
{
	if( filePath == NULL ){
		if( filePathSize == NULL ){
			return FALSE;
		}else{
			*filePathSize = (DWORD)this->savePath.size()+1;
		}
	}else{
		if( filePathSize == NULL ){
			return FALSE;
		}else{
			if( *filePathSize < (DWORD)this->savePath.size()+1 ){
				*filePathSize = (DWORD)this->savePath.size()+1;
				return FALSE;
			}else{
				wcscpy_s(filePath, *filePathSize, this->savePath.c_str());
			}
		}
	}
	return TRUE;
}

//保存用TSデータを送る
//空き容量不足などで書き出し失敗した場合、writeSizeの値を元に
//再度保存処理するときの送信開始地点を決める
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// data					[IN]TSデータ
// size					[IN]dataのサイズ
// writeSize			[OUT]保存に利用したサイズ
BOOL CWriteMain::_AddTSBuff(
	BYTE* data,
	DWORD size,
	DWORD* writeSize
	)
{
	BOOL ret = FALSE;

	if( dshowCtrl.IsRun() == FALSE ){
		dshowCtrl.RunGraph();
	}
	ret = dshowCtrl.AddTS(data, size);
	if( writeSize != NULL ){
		*writeSize = 0;
	}
	return ret;
}

BOOL CWriteMain::GetNextFileName(wstring filePath, wstring& newPath)
{
	WCHAR szPath[_MAX_PATH];
	WCHAR szDrive[_MAX_DRIVE];
	WCHAR szDir[_MAX_DIR];
	WCHAR szFname[_MAX_FNAME];
	WCHAR szExt[_MAX_EXT];
	_wsplitpath_s( filePath.c_str(), szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFname, _MAX_FNAME, szExt, _MAX_EXT );
	_wmakepath_s(  szPath, _MAX_PATH, szDrive, szDir, NULL, NULL );

	BOOL findFlag = FALSE;
	for( int i=1; i<1000; i++ ){
		WIN32_FIND_DATA findData;
		HANDLE find;

		wstring name;
		Format(name, L"%s%s-(%d)%s", szPath, szFname, i, szExt);
		newPath = name;
		
		find = FindFirstFile( newPath.c_str(), &findData);
		if ( find == INVALID_HANDLE_VALUE ) {
			findFlag = TRUE;
			break;
		}
		FindClose(find);
	}
	return findFlag;
}

#include "stdafx.h"
#include "BonDriverUtil.h"

CBonDriverUtil::CBonDriverUtil(void)
{
	this->lockEvent = CreateEvent(NULL, FALSE, TRUE, NULL );

	this->loadDllPath = L"";
	this->loadTunerName = L"";
	this->initChSetFlag = FALSE;
	this->bonIF = NULL;
	this->bon2IF = NULL;
	this->module = NULL;
}

CBonDriverUtil::~CBonDriverUtil(void)
{
	_CloseBonDriver();

	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}
}

BOOL CBonDriverUtil::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED){
		return FALSE;
	}
	return TRUE;
}

void CBonDriverUtil::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

//BonDriverフォルダを指定
//引数：
// bonDriverFolderPath		[IN]BonDriverフォルダパス
void CBonDriverUtil::SetBonDriverFolder(
	LPCWSTR bonDriverFolderPath
)
{
	if( Lock() == FALSE ) return ;

	wstring strBonDriverFolderPath = bonDriverFolderPath;

	ChkFolderPath(strBonDriverFolderPath);

	this->bonDllList.clear();

	WIN32_FIND_DATA findData;
	HANDLE find;

	//指定フォルダのファイル一覧取得
	find = FindFirstFile( (strBonDriverFolderPath + L"\\BonDriver*.dll").c_str(), &findData);
	if ( find == INVALID_HANDLE_VALUE ) {
		UnLock();
		return ;
	}
	do{
		if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ){
			//見つかったDLLを一覧に追加
			this->bonDllList.push_back(strBonDriverFolderPath + L"\\" + findData.cFileName);
		}
	}while(FindNextFile(find, &findData));

	FindClose(find);

	UnLock();
}

//BonDriverフォルダのBonDriver_*.dllを列挙
//戻り値：
// エラーコード
//引数：
// bonList			[OUT]検索できたBonDriver一覧
DWORD CBonDriverUtil::EnumBonDriver(
	vector<wstring>* bonList
)
{
	if( Lock() == FALSE ) return ERR_FALSE;

	for( size_t i = 0; i < this->bonDllList.size(); i++ ){
		bonList->push_back(L"");
		GetFileName(this->bonDllList[i], bonList->back());
	}

	UnLock();

	return NO_ERR;
}

//BonDriverをロードしてチャンネル情報などを取得（ファイル名で指定）
//戻り値：
// エラーコード
//引数：
// bonDriverFile	[IN]EnumBonDriverで取得されたBonDriverのファイル名
DWORD CBonDriverUtil::OpenBonDriver(
	LPCWSTR bonDriverFile,
	int openWait
	)
{
	if( Lock() == FALSE ) return ERR_OPEN_TUNER;
	DWORD err = ERR_FIND_TUNER;
	for( size_t i = 0; i < this->bonDllList.size(); i++ ){
		wstring fileName;
		GetFileName(this->bonDllList[i], fileName);
		if( CompareNoCase(bonDriverFile, fileName) == 0 ){
			err = _OpenBonDriver(this->bonDllList[i].c_str(), openWait);
			break;
		}
	}
	if( err == ERR_FIND_TUNER ){
		_OutputDebugString(L"★OpenするBonDriverが見つかりません");
		OutputDebugString(bonDriverFile);
	}

	UnLock();
	return err;
}

//BonDriverをロード時の本体
//戻り値：
// エラーコード
//引数：
// bonDriverFilePath		[IN] ロードするBonDriverのファイルパス
DWORD CBonDriverUtil::_OpenBonDriver(
	LPCWSTR bonDriverFilePath,
	int openWait
	)
{
	_CloseBonDriver();

	this->module = ::LoadLibrary(bonDriverFilePath);
	if( this->module == NULL ){
		OutputDebugString(L"★BonDriverがロードできません");
		OutputDebugString(bonDriverFilePath);
		return ERR_LOAD_MODULE;
	}
	IBonDriver* (*func)();
	func = (IBonDriver* (*)())::GetProcAddress( this->module , "CreateBonDriver");
	if( !func ){
		OutputDebugString(L"★GetProcAddressに失敗しました");
		_CloseBonDriver();
		return ERR_INIT;
	}
	this->bonIF = func();
	try{
		this->bon2IF = dynamic_cast<IBonDriver2 *>(bonIF);
		BOOL open = this->bonIF->OpenTuner();
		if( open == FALSE ){
			Sleep(1000);
			open = this->bonIF->OpenTuner();
		}

		if( open == FALSE ){
			//Open失敗
			OutputDebugString(L"★OpenTunerに失敗しました");
			_CloseBonDriver();
			return ERR_OPEN_TUNER;
		}else{
			//Open成功
			//チューナー名の取得
			this->loadTunerName = this->bon2IF->GetTunerName();
			Replace(this->loadTunerName, L"(",L"（");
			Replace(this->loadTunerName, L")",L"）");
			//チャンネル一覧の取得
			DWORD countSpace = 0;
			while(1){
				if( this->bon2IF->EnumTuningSpace(countSpace) != NULL ){
					BON_SPACE_INFO spaceItem;
					spaceItem.spaceName = this->bon2IF->EnumTuningSpace(countSpace);
					DWORD countCh = 0;

					while(1){
						LPCWSTR chName = this->bon2IF->EnumChannelName(countSpace, countCh);
						if( chName != NULL ){
							if( chName[0] != L'\0' ){
								spaceItem.chMap.insert(pair<DWORD,wstring>(countCh, chName));
							}
						}else{
							break;
						}
						countCh++;
					}
					this->loadChMap.insert(pair<DWORD, BON_SPACE_INFO>(countSpace, spaceItem));
				}else{
					break;
				}
				countSpace++;
			}
			Sleep(openWait);
			this->loadDllPath = bonDriverFilePath;
		}
	}catch(...){
		_CloseBonDriver();
		return ERR_OPEN_TUNER;
	}

	return NO_ERR;
}

//ロードしているBonDriverの開放
//戻り値：
// エラーコード
DWORD CBonDriverUtil::CloseBonDriver()
{
	if( Lock() == FALSE ) return ERR_FALSE;
	DWORD err = NO_ERR;
	err = _CloseBonDriver();
	UnLock();
	return err;
}

DWORD CBonDriverUtil::_CloseBonDriver()
{
	DWORD err = NO_ERR;
	if( this->bonIF != NULL ){
		this->bonIF->CloseTuner();
		this->bonIF->Release();
		this->bonIF = NULL;
		this->bon2IF = NULL;
	}
	if( this->module != NULL ){
		::FreeLibrary( this->module );
		this->module = NULL;
	}
	this->loadDllPath = L"";
	this->loadTunerName = L"";
	this->loadChMap.clear();
	this->initChSetFlag = FALSE;
	return err;
}

//ロードしたBonDriverの情報取得
//SpaceとChの一覧を取得する
//戻り値：
// エラーコード
//引数：
// spaceMap			[OUT] SpaceとChの一覧（mapのキー Space）
DWORD CBonDriverUtil::GetOriginalChList(
	map<DWORD, BON_SPACE_INFO>* spaceMap
)
{
	if( Lock() == FALSE ) return ERR_FALSE;

	if( spaceMap == NULL || this->bon2IF == NULL ){
		UnLock();
		return ERR_INVALID_ARG;
	}

	*spaceMap = this->loadChMap;

	UnLock();
	return NO_ERR;
}

//BonDriverのチューナー名を取得
//戻り値：
// チューナー名
wstring CBonDriverUtil::GetTunerName()
{
	wstring name = L"";
	if( Lock() == FALSE ) return name;

	name = this->loadTunerName;

	UnLock();
	return name;
}

//チャンネル変更
//戻り値：
// エラーコード
//引数：
// space			[IN]変更チャンネルのSpace
// ch				[IN]変更チャンネルの物理Ch
DWORD CBonDriverUtil::SetCh(
	DWORD space,
	DWORD ch
	)
{
	if( Lock() == FALSE ) return FALSE;
	if( this->bon2IF == NULL ){
		UnLock();
		return ERR_NOT_INIT;
	}
	//初回は常にチャンネル設定行う
	if( this->initChSetFlag == TRUE ){
		//２回目以降は変更あった場合に行う
		if( space == this->bon2IF->GetCurSpace() &&
			ch == this->bon2IF->GetCurChannel() )
		{
			UnLock();
			return NO_ERR;
		}
	}
	if( this->bon2IF->SetChannel(space, ch) == FALSE ){
		Sleep(500);
		if( this->bon2IF->SetChannel(space, ch) == FALSE ){
			UnLock();
			return ERR_FALSE;
		}
	}
	this->initChSetFlag = TRUE;
	UnLock();
	return NO_ERR;
}

//現在のチャンネル取得
//戻り値：
// エラーコード
//引数：
// space			[IN]現在のチャンネルのSpace
// ch				[IN]現在のチャンネルの物理Ch
DWORD CBonDriverUtil::GetNowCh(
	DWORD* space,
	DWORD* ch
	)
{
	if( Lock() == FALSE ) return FALSE;
	if( this->bon2IF == NULL || this->initChSetFlag == FALSE ){
		UnLock();
		return FALSE;
	}
	*space = this->bon2IF->GetCurSpace();
	*ch = this->bon2IF->GetCurChannel();
	UnLock();
	return TRUE;
}

//TSストリームを取得
//戻り値：
// TRUE（成功）、FALSE（失敗）
//引数：
// data				[OUT]BonDriver内部バッファのポインタ
// size				[OUT]取得バッファのサイズ
// remain			[OUT]未取得バッファのサイズ
BOOL CBonDriverUtil::GetTsStream(
	BYTE **data,
	DWORD *size,
	DWORD *remain
	)
{
	if( Lock() == FALSE ) return FALSE;

	BOOL ret = FALSE;
	if( this->bonIF == NULL ){
		UnLock();
		return FALSE;
	}
	try{
		ret = this->bonIF->GetTsStream(data, size, remain);
	}catch(...){
		ret = FALSE;
	}
	UnLock();
	return ret;
}

//シグナルレベルの取得
//戻り値：
// シグナルレベル
float CBonDriverUtil::GetSignalLevel()
{
	if( Lock() == FALSE ) return 0;
	if( this->bonIF == NULL ){
		UnLock();
		return 0;
	}
	float fLevel = this->bonIF->GetSignalLevel();
	UnLock();
	return fLevel;
}

//OpenしたBonDriverのファイル名を取得
//戻り値：
// BonDriverのファイル名（拡張子含む）（emptyで未Open）
wstring CBonDriverUtil::GetOpenBonDriverFileName()
{
	wstring ret = L"";
	if( Lock() == FALSE ) return ret;

	GetFileName(this->loadDllPath, ret);

	UnLock();
	return ret;
}

#include "StdAfx.h"
#include "RecInfoDBManager.h"

#include "../../Common/CtrlCmdUtil.h"
#include "../../Common/TimeUtil.h"

CRecInfoDBManager::CRecInfoDBManager(void)
{
	this->lockEvent = _CreateEvent(FALSE, TRUE, NULL);
	::CoInitialize( NULL );
	HRESULT hr=regExp.CreateInstance(CLSID_RegExp);
	if(FAILED(hr)){
		regExp = NULL;
	}
}


CRecInfoDBManager::~CRecInfoDBManager(void)
{
	Clear();

	if( this->lockEvent != NULL ){
		UnLock();
		CloseHandle(this->lockEvent);
		this->lockEvent = NULL;
	}

	if( regExp != NULL ){
		regExp.Release();
	}

	::CoUninitialize();
}

BOOL CRecInfoDBManager::Lock(LPCWSTR log, DWORD timeOut)
{
	if( this->lockEvent == NULL ){
		return FALSE;
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
	DWORD dwRet = WaitForSingleObject(this->lockEvent, timeOut);
	if( dwRet == WAIT_ABANDONED || 
		dwRet == WAIT_FAILED ||
		dwRet == WAIT_TIMEOUT){
			OutputDebugString(L"ŸCRecInfoDBManager::Lock FALSE");
		return FALSE;
	}
	return TRUE;
}

void CRecInfoDBManager::UnLock(LPCWSTR log)
{
	if( this->lockEvent != NULL ){
		SetEvent(this->lockEvent);
	}
	if( log != NULL ){
		OutputDebugString(log);
	}
}

void CRecInfoDBManager::Clear()
{
	map<wstring, RECINFO_LIST_ITEM*>::iterator itrTitle;
	for( itrTitle = this->titleMap.begin(); itrTitle != this->titleMap.end(); itrTitle++ ){
		SAFE_DELETE(itrTitle->second);
	}
	this->titleMap.clear();

	for( size_t i=0 ;i<this->recInfoList.size(); i++ ){
		SAFE_DELETE(this->recInfoList[i]);
	}
	this->recInfoList.clear();
}

void CRecInfoDBManager::LoadRecInfo()
{
	if( Lock() == FALSE ) return;
	Clear();

	wstring filePath = L""; 
	GetSettingPath(filePath);
	filePath += L"\\RecInfo2Data.bin";

	HANDLE hFile = _CreateFile2( filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		UnLock();
		return ;
	}
	DWORD dwFileSize = GetFileSize( hFile, NULL );
	if( dwFileSize == 0 ){
		CloseHandle(hFile);
		UnLock();
		return ;
	}
	BYTE* pszBuff = new BYTE[dwFileSize+1];
	if( pszBuff == NULL ){
		CloseHandle(hFile);
		UnLock();
		return ;
	}
	ZeroMemory(pszBuff,dwFileSize+1);
	DWORD dwRead=0;
	ReadFile( hFile, pszBuff, dwFileSize, &dwRead, NULL );
		
	if( ReadVALUE( &recInfoList, pszBuff, dwRead, NULL ) == FALSE){
		CloseHandle(hFile);
		SAFE_DELETE_ARRAY(pszBuff);
		UnLock();
		return;
	}

	CloseHandle(hFile);
	SAFE_DELETE_ARRAY(pszBuff);

	CreateKeyMap();

	UnLock();
}

void CRecInfoDBManager::CreateKeyMap()
{
	map<wstring, RECINFO_LIST_ITEM*>::iterator itrTitle;
	for( itrTitle = this->titleMap.begin(); itrTitle != this->titleMap.end(); itrTitle++ ){
		SAFE_DELETE(itrTitle->second);
	}
	this->titleMap.clear();

	wstring iniAppPath = L"";
	GetModuleIniPath(iniAppPath);
	WCHAR buff[1024] = L"";
	GetPrivateProfileString(L"SET", L"RecInfo2RegExp", L"", buff, 1024, iniAppPath.c_str());
	wstring strReg = buff;

	for( size_t i=0 ;i<this->recInfoList.size(); i++ ){
		wstring title = L"";
		if( this->recInfoList[i]->shortInfo != NULL ){
			title = this->recInfoList[i]->shortInfo->event_name;
			ReplaceRegExp(title, strReg, L"");
		}
		map<wstring, RECINFO_LIST_ITEM*>::iterator itrTitle;
		RECINFO_LIST_ITEM* itemTitle = NULL;
		itrTitle = this->titleMap.find(title);
		if( itrTitle == this->titleMap.end() ){
			itemTitle = new RECINFO_LIST_ITEM;
			this->titleMap.insert(pair<wstring, RECINFO_LIST_ITEM*>(title, itemTitle));
		}else{
			itemTitle = itrTitle->second;
		}
		itemTitle->infoList.push_back(this->recInfoList[i]);
	}
}

void CRecInfoDBManager::SaveRecInfo()
{
	if( Lock() == FALSE ) return;

	wstring filePath = L""; 
	GetSettingPath(filePath);
	filePath += L"\\RecInfo2Data.bin";

	HANDLE hFile = _CreateFile2( filePath.c_str(), GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		UnLock();
		return ;
	}
	DWORD dwFileSize = GetVALUESize( &recInfoList);
	BYTE* pszBuff = new BYTE[dwFileSize];
	if( pszBuff == NULL ){
		CloseHandle(hFile);
		UnLock();
		return ;
	}
	WriteVALUE(&recInfoList, pszBuff, dwFileSize, NULL);
	DWORD dwWrite=0;
	WriteFile( hFile, pszBuff, dwFileSize, &dwWrite, NULL );

	CloseHandle(hFile);
	SAFE_DELETE_ARRAY(pszBuff);

	UnLock();
}

void CRecInfoDBManager::AddInfo(EPGDB_EVENT_INFO* info)
{
	if( Lock() == FALSE ) return;

	wstring iniAppPath = L"";
	GetModuleIniPath(iniAppPath);
	UINT maxSize = GetPrivateProfileInt(L"SET", L"RecInfo2Max", 1000, iniAppPath.c_str());

	EPGDB_EVENT_INFO* item = new EPGDB_EVENT_INFO;

	item->original_network_id = info->original_network_id;
	item->transport_stream_id = info->transport_stream_id;
	item->service_id = info->service_id;
	item->event_id = info->event_id;
	item->StartTimeFlag = info->StartTimeFlag;
	item->start_time = info->start_time;
	item->DurationFlag = info->DurationFlag;
	item->durationSec = info->durationSec;
	if( info->shortInfo != NULL ){
		item->shortInfo = new EPGDB_SHORT_EVENT_INFO;
		*item->shortInfo = *info->shortInfo;
	}
	if( info->extInfo != NULL ){
		item->extInfo = new EPGDB_EXTENDED_EVENT_INFO;
		*item->extInfo = *info->extInfo;
	}
	if( info->contentInfo != NULL ){
		item->contentInfo = new EPGDB_CONTEN_INFO;
		*item->contentInfo = *info->contentInfo;
	}
	if( info->componentInfo != NULL ){
		item->componentInfo = new EPGDB_COMPONENT_INFO;
		*item->componentInfo = *info->componentInfo;
	}
	if( info->audioInfo != NULL ){
		item->audioInfo = new EPGDB_AUDIO_COMPONENT_INFO;
		*item->audioInfo = *info->audioInfo;
	}
	if( info->eventGroupInfo != NULL ){
		item->eventGroupInfo = new EPGDB_EVENTGROUP_INFO;
		*item->eventGroupInfo = *info->eventGroupInfo;
	}
	if( info->eventRelayInfo != NULL ){
		item->eventRelayInfo = new EPGDB_EVENTGROUP_INFO;
		*item->eventRelayInfo = *info->eventRelayInfo;
	}

	recInfoList.push_back(item);
	while( recInfoList.size() > maxSize ){
		SAFE_DELETE(recInfoList[0]);
		recInfoList.erase(recInfoList.begin());
	}
	CreateKeyMap();

	UnLock();
}

BOOL CRecInfoDBManager::IsFindTitleInfo(EPGDB_EVENT_INFO* info, WORD chkDay)
{
	if( Lock() == FALSE ) return FALSE;
	BOOL ret = FALSE;

	wstring iniAppPath = L"";
	GetModuleIniPath(iniAppPath);
	WCHAR buff[1024] = L"";
	GetPrivateProfileString(L"SET", L"RecInfo2RegExp", L"", buff, 1024, iniAppPath.c_str());
	wstring strReg = buff;

	wstring title = L"";
	if( info->shortInfo != NULL ){
		title = info->shortInfo->event_name;
		ReplaceRegExp(title, strReg, L"");
	}
	__int64 weekSec = chkDay*24*60*60;
	weekSec *= I64_1SEC;

	map<wstring, RECINFO_LIST_ITEM*>::iterator itrTitle;
	itrTitle = this->titleMap.find(title);
	if( itrTitle != this->titleMap.end() ){
		__int64 startTime = ConvertI64Time(info->start_time);
		for( size_t i=0; i<itrTitle->second->infoList.size(); i++){
			EPGDB_EVENT_INFO* recInfo = itrTitle->second->infoList[i];
			__int64 recStartTime = ConvertI64Time(recInfo->start_time);
			if( info->original_network_id == recInfo->original_network_id &&
				info->transport_stream_id == recInfo->transport_stream_id &&
				info->service_id == recInfo->service_id )
			{
				if( recStartTime + weekSec > startTime ){
					ret = TRUE;
					break;
				}
			}
		}
	}

	UnLock();
	return ret;
}

void CRecInfoDBManager::ReplaceRegExp(wstring &strBuff, wstring strReg, wstring strNew)
{
	if( this->regExp != NULL && strBuff.size() > 0 && strReg.size() > 0 ){
		try{
			_bstr_t target( strBuff.c_str() );
			_bstr_t pattern( strReg.c_str() );
			_bstr_t replace( strNew.c_str() );

			this->regExp->PutGlobal( VARIANT_TRUE );
			this->regExp->PutPattern( pattern );

			strBuff = (wstring)this->regExp->Replace( target, replace );
		}catch(...){
			strBuff = L"";
		}
	}
}

#include "StdAfx.h"
#include "DLNAManager.h"

#include "../../Common/ErrDef.h"
#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"

CDLNAManager::CDLNAManager(void)
{
	this->upnpCtrl = NULL;
	this->startDMS = FALSE;
	this->httpPort = 8081;

	wstring filePath = L"";
	GetModuleFolderPath(filePath);
	filePath += L"\\dlna\\dms\\protocolInfo.txt";
	protocolInfo.ParseText(filePath.c_str());

	DLNA_DMS_CONTAINER_META_INFO info;
	info.title = L"RecFile";
	info.uploadSupportFlag = 0;
	dms.CreateContainer(L"0", &info, this->recFolderObjectID);

	info.title = L"PublicFile";
	info.uploadSupportFlag = 0;
	dms.CreateContainer(L"0", &info, this->publicFolderObjectID);
}


CDLNAManager::~CDLNAManager(void)
{
	if( this->upnpCtrl != NULL ){
		UPNP_SERVER_Stop(this->upnpCtrl);
		UPNP_SERVER_CloseHandle(&this->upnpCtrl);
	}
}

int CDLNAManager::StartSSDPServer(DWORD httpPort)
{
	if( this->upnpCtrl == NULL ){
		this->httpPort = httpPort;
		this->upnpCtrl = UPNP_SERVER_CreateHandle(UpnpSSDPCallback, this, UpnpMSearchReqCallback, this);
		UPNP_SERVER_Start(this->upnpCtrl);
	}
	return NO_ERR;
}

int CDLNAManager::StopSSDPServer()
{
	if( this->upnpCtrl != NULL ){
		UPNP_SERVER_Stop(this->upnpCtrl);
		UPNP_SERVER_CloseHandle(&this->upnpCtrl);
	}
	return NO_ERR;
}

int CDLNAManager::LoadPublicFolder()
{
	wstring filePath = L"";
	GetModuleFolderPath(filePath);
	filePath += L"\\dlna\\dms\\publicFolder.txt";
	publicFolder.ParseText(filePath.c_str());

	DLNA_DMS_CONTAINER_META_INFO info;

	map<wstring, DLNA_PUBLIC_FOLDER_INFO>::iterator itrFolder;
	for( itrFolder = publicFolder.folderList.begin(); itrFolder != publicFolder.folderList.end(); itrFolder++ ){
		wstring buff1 = itrFolder->second.virtualPath;
		wstring buff2=L"";
		do{
			Separate(buff1, L"/", buff2, buff1);
		}while(buff1.size() > 0 );

		info.title = buff2;
		info.uploadFolderPath = itrFolder->second.folderPath;
		info.uploadSupportFlag = 0;

		wstring virtualPath = L"PublicFile/";
		virtualPath += itrFolder->second.virtualPath;

		wstring objectID;
		if( this->dms.CreateContainer2(virtualPath, &info, objectID) != NO_ERR){
			continue;
		}

		AddFolderItem(itrFolder->second.folderPath, objectID);
	}

	return NO_ERR;
}

int CDLNAManager::AddFolderItem(wstring folderPath, wstring parentObjectID)
{
	wstring searchKey = folderPath;
	searchKey += L"\\*";

	WIN32_FIND_DATA findData;
	HANDLE find;

	//Žw’èƒtƒHƒ‹ƒ_‚Ìƒtƒ@ƒCƒ‹ˆê——Žæ“¾
	find = FindFirstFile( searchKey.c_str(), &findData);
	if ( find == INVALID_HANDLE_VALUE ) {
		return FALSE;
	}
	do{
		if( (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ){
			wstring filePath;
			Format(filePath, L"%s\\%s", folderPath.c_str(), findData.cFileName);

			wstring objectID;
			AddFileItem(parentObjectID, filePath, objectID);
		}else{
			if(CompareNoCase(findData.cFileName, L".") != 0 && CompareNoCase(findData.cFileName, L"..") != 0 ){
				wstring subFolderPath;
				Format(subFolderPath, L"%s\\%s", folderPath.c_str(), findData.cFileName);

				DLNA_DMS_CONTAINER_META_INFO info;
				info.title = findData.cFileName;
				info.uploadFolderPath = subFolderPath;
				info.uploadSupportFlag = 0;

				wstring objectID;
				if( this->dms.CreateContainer(parentObjectID, &info, objectID) == NO_ERR){
					AddFolderItem(subFolderPath, objectID);
				}
			}
		}
	}while(FindNextFile(find, &findData));

	FindClose(find);

	return NO_ERR;
}

int CDLNAManager::AddFileItem(wstring parentObjectID, wstring filePath, wstring& objectID)
{
	wstring fileName;
	wstring fileExt;

	GetFileName(filePath, fileName);
	GetFileExt(filePath, fileExt);

	map<wstring, DLNA_PROTOCOL_INFO>::iterator itrInfo;
	itrInfo = protocolInfo.supportList.find(fileExt);
	if( itrInfo == protocolInfo.supportList.end() ){
		return ERR_FALSE;
	}

	DLNA_DMS_CONTENT_META_INFO itemInfo;
	itemInfo.title = fileName;
	itemInfo.filePath = filePath;
	itemInfo.upnpClass = itrInfo->second.upnpClass;
	itemInfo.mimeType = itrInfo->second.mimeType;
	itemInfo.protocolInfo = itrInfo->second.info;

	HANDLE hFile = CreateFile( filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		return 0;
	}
	DWORD sizeH = 0;
	DWORD sizeL = GetFileSize( hFile, &sizeH );
	__int64 fileSize = 0;
	itemInfo.fileSize = ((__int64)sizeH)<<32 | sizeL;
	CloseHandle(hFile);

	dms.AddContent(parentObjectID, &itemInfo, objectID);

	return NO_ERR;
}

int CDLNAManager::AddDMSRecFile(wstring filePath)
{
	map<wstring, wstring>::iterator itr;
	itr = recFileList.find(filePath);
	if( itr != recFileList.end() ){
		return NO_ERR;
	}

	wstring objectID;
	if( AddFileItem(this->recFolderObjectID, filePath, objectID) == NO_ERR ){
		recFileList.insert(pair<wstring, wstring>(filePath, objectID));
		return NO_ERR;
	}else{
		return ERR_FALSE;
	}
}

int CDLNAManager::StartDMS()
{
	this->startDMS = TRUE;

	if( this->upnpCtrl != NULL ){
		//Notify“o˜^
		string uuid;
		this->dms.GetUuid(uuid);
		vector<string> devUrnList;
		this->dms.GetDevicesType(&devUrnList);
		vector<string> srvUrnList;
		this->dms.GetServicesType(&srvUrnList);
		string uri;
		this->dms.GetDDDUri(uuid, uri);
		for( size_t i=0; i<devUrnList.size(); i++ ){
			UPNP_SERVER_AddNotifyInfo(this->upnpCtrl, uuid.c_str(), devUrnList[i].c_str(), this->httpPort, uri.c_str());
		}
		for( size_t i=0; i<srvUrnList.size(); i++ ){
			UPNP_SERVER_AddNotifyInfo(this->upnpCtrl, uuid.c_str(), srvUrnList[i].c_str(), this->httpPort, uri.c_str());
		}
	}

	return NO_ERR;
}

int CDLNAManager::StopDMS()
{
	this->startDMS = FALSE;

	if( this->upnpCtrl != NULL ){
		//Notify“o˜^
		string uuid;
		this->dms.GetUuid(uuid);
		UPNP_SERVER_RemoveNotifyInfo(this->upnpCtrl, uuid.c_str());
	}

	return NO_ERR;
}

int CDLNAManager::HttpRequest(string method, string uri, nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent)
{
	if(uri.find("/dlna/dms/") == 0 ){
		dms.HttpRequest(method, uri, headerList, reqReader, clientSock, stopEvent);
	}
	return NO_ERR;
}

void CDLNAManager::UpnpSSDPCallback( SSDP_NOTIFY_STATUS notifyType, void* notifyParam, void* param)
{
}

int CDLNAManager::UpnpMSearchReqCallback( UPNP_MSEARCH_REQUEST_INFO* requestParam, void* param, SORT_LIST_HANDLE resDeviceList)
{
	int ret = -1;
	CDLNAManager* sys = (CDLNAManager*)param;
	UPNP_MSEARCH_RES_DEV_INFO* resDevInfo = NULL;
	vector<string> urnList;
	string buff = "";
	string uuid = "";
	string server = "Windows/5.1 UPnP/1.1 EpgTimerSrv/1.0";

	if(requestParam == NULL || sys == NULL ){
		return ret;
	}

	if( CompareNoCase(requestParam->man, "\"ssdp:discover\"") != 0 ){
		return ret;
	}

	if( CompareNoCase(requestParam->st, "upnp:rootdevice") == 0 || CompareNoCase(requestParam->st, "ssdp:all") == 0){
		if( sys->startDMS == TRUE ){
			sys->dms.GetDevicesType(&urnList);
			sys->dms.GetUuid(uuid);
			for( size_t i=0; i<urnList.size(); i++ ){
				resDevInfo = UPNP_MSEARCH_RES_DEV_INFO_New();
				resDevInfo->max_age = 1800;
				resDevInfo->port = sys->httpPort;

				sys->dms.GetDDDUri(uuid, buff);
				resDevInfo->uri = (char*)malloc(buff.size()+1);
				strcpy_s(resDevInfo->uri, buff.size()+1, buff.c_str());


				resDevInfo->uuid = (char*)malloc(uuid.size()+1);
				strcpy_s(resDevInfo->uuid, uuid.size()+1, uuid.c_str());


				resDevInfo->usn = (char*)malloc(strlen("uuid:::urn:rootdevice")+uuid.size()+1);
				sprintf_s(resDevInfo->usn, strlen("uuid:::urn:rootdevice")+uuid.size()+1, "uuid:%s::urn:rootdevice", uuid.c_str());

				resDevInfo->server = (char*)malloc(server.size()+1);
				strcpy_s(resDevInfo->server, server.size()+1, server.c_str());

				SORT_LIST_AddItem(resDeviceList, resDevInfo->uuid, resDevInfo);
			}
		}
	}
	string st = requestParam->st;
	if( st.find("uuid:") == 0 || CompareNoCase(requestParam->st, "ssdp:all") == 0){
		if( sys->startDMS == TRUE ){
			sys->dms.GetUuid(uuid);
			if(  st.find(uuid) != string::npos ){
				resDevInfo = UPNP_MSEARCH_RES_DEV_INFO_New();
				resDevInfo->max_age = 1800;
				resDevInfo->port = sys->httpPort;

				sys->dms.GetDDDUri(uuid, buff);
				resDevInfo->uri = (char*)malloc(buff.size()+1);
				strcpy_s(resDevInfo->uri, buff.size()+1, buff.c_str());


				resDevInfo->uuid = (char*)malloc(uuid.size()+1);
				strcpy_s(resDevInfo->uuid, uuid.size()+1, uuid.c_str());


				resDevInfo->usn = (char*)malloc(strlen("uuid:")+uuid.size()+1);
				sprintf_s(resDevInfo->usn, strlen("uuid:")+uuid.size()+1, "uuid:%s", uuid.c_str());

				resDevInfo->server = (char*)malloc(server.size()+1);
				strcpy_s(resDevInfo->server, server.size()+1, server.c_str());


				SORT_LIST_AddItem(resDeviceList, resDevInfo->uuid, resDevInfo);
			}
		}
	}
	if(st.find("urn:") == 0 || CompareNoCase(requestParam->st, "ssdp:all") == 0){
		if( sys->startDMS == TRUE ){
			sys->dms.GetUuid(uuid);
			if( sys->dms.IsSupportUrn(st) >0){
				resDevInfo = UPNP_MSEARCH_RES_DEV_INFO_New();
				resDevInfo->max_age = 1800;
				resDevInfo->port = sys->httpPort;

				sys->dms.GetDDDUri(uuid, buff);
				resDevInfo->uri = (char*)malloc(buff.size()+1);
				strcpy_s(resDevInfo->uri, buff.size()+1, buff.c_str());

				resDevInfo->uuid = (char*)malloc(uuid.size()+1);
				strcpy_s(resDevInfo->uuid, uuid.size()+1, uuid.c_str());

				resDevInfo->usn = (char*)malloc(strlen("uuid:::")+uuid.size()+st.size()+1);
				sprintf_s(resDevInfo->usn, strlen("uuid:::")+uuid.size()+st.size()+1, "uuid:%s::%s", uuid.c_str(), st.c_str());

				resDevInfo->server = (char*)malloc(server.size()+1);
				strcpy_s(resDevInfo->server, server.size()+1, server.c_str());

				SORT_LIST_AddItem(resDeviceList, resDevInfo->uuid, resDevInfo);
			}
		}
	}

	return ret;
}


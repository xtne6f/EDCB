#pragma once

#include "../../Common/CommonDef.h"
#include "../../UPnPCtrl/UpnpSsdpUtil.h"

#include "DLNADmsManager.h"
#include "DLNAParseProtocolInfo.h"
#include "DLNAParsePublicFolder.h"

class CDLNAManager
{
public:
	CDLNAManager(void);
	~CDLNAManager(void);

	int StartSSDPServer(DWORD);
	int StopSSDPServer();

	int LoadPublicFolder();
	int AddDMSRecFile(wstring filePath);
	int StartDMS();
	int StopDMS();

	int HttpRequest(string method, string uri, nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent);

protected:
	UPNP_SERVER_HANDLE upnpCtrl;

	CDLNAParseProtocolInfo protocolInfo;
	DLNAParsePublicFolder publicFolder;

	CDLNADmsManager dms;
	BOOL startDMS;
    DWORD httpPort;
	wstring recFolderObjectID;
	wstring publicFolderObjectID;

	map<wstring, wstring> recFileList;
protected:
	int AddFolderItem(wstring folderPath, wstring parentObjectID);
	int AddFileItem(wstring parentObjectID, wstring filePath, wstring& objectID);

	static void UpnpSSDPCallback( SSDP_NOTIFY_STATUS notifyType, void* notifyParam, void* param);
	static int UpnpMSearchReqCallback( UPNP_MSEARCH_REQUEST_INFO* requestParam, void* param, SORT_LIST_HANDLE resDeviceList);
};


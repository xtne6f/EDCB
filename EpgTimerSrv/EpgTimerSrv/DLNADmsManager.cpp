#include "StdAfx.h"
#include "DLNADmsManager.h"

#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../UPnPCtrl/UpnpUtil.h"
#include "../../UPnPCtrl/CharUtil.h"
#include "../../Common/TimeUtil.h"

#include <shlwapi.h>

static WCHAR DIDLStart[] = L"<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" >";
static WCHAR DIDLEnd[] = L"</DIDL-Lite>";


CDLNADmsManager::CDLNADmsManager(void)
{
	this->rootUri = L"/dlna/dms";
	this->fileRootUri = this->rootUri;
	this->fileRootUri+=L"/file";
	this->dmsDB.SetRootUri(this->fileRootUri);
	this->subscribeCount = 0;

	wstring filePath = L"";
	GetModuleFolderPath(filePath);
	filePath += L"\\dlna\\dms\\protocolInfo.txt";
	protocolInfo.ParseText(filePath.c_str());

	
}


CDLNADmsManager::~CDLNADmsManager(void)
{
}

void CDLNADmsManager::SetRootUri(wstring& uri)
{
	this->rootUri = uri;
	this->fileRootUri = this->rootUri;
	this->fileRootUri+=L"/file";
	this->dmsDB.SetRootUri(this->fileRootUri);
}

void CDLNADmsManager::GetUuid(string& uuid)
{
	if( this->uuid.size() == 0 ){
		char uuidVal[37] = "";
		if( UPNP_UTIL_CreateUUID(uuidVal, 37, UPNP_DEVICE_TYPE_DMS, 0, 0) >= 0){
			this->uuid = uuidVal;
		}else{
			this->uuid = "cdcdcdcd-cdcd-cdcd-cdcd-cdcdcdcdcdcd";
		}
	}

	uuid = this->uuid;
}

void CDLNADmsManager::GetDevicesType(vector<string>* urnList)
{
	if( urnList != NULL ){
		urnList->push_back("urn:schemas-upnp-org:device:MediaServer:1");
	}
}

void CDLNADmsManager::GetServicesType(vector<string>* urnList)
{
	if( urnList != NULL ){
		urnList->push_back(UPNP_URN_AVT_1);
		urnList->push_back(UPNP_URN_CDS_1);
		urnList->push_back(UPNP_URN_CMS_1);
	}
}

int CDLNADmsManager::IsSupportUrn(string urn)
{
	int find = 0;
	if( CompareNoCase(urn, "urn:schemas-upnp-org:device:MediaServer:1") == 0 ){
		find = 1;
	}
	if( CompareNoCase(urn, UPNP_URN_AVT_1) == 0 ){
		find = 1;
	}
	if( CompareNoCase(urn, UPNP_URN_CDS_1) == 0 ){
		find = 1;
	}
	if( CompareNoCase(urn, UPNP_URN_CMS_1) == 0 ){
		find = 1;
	}

	return find;
}

void CDLNADmsManager::GetDDDUri(string uuid, string& uri)
{
	WtoA(rootUri, uri);
	uri += "/ddd.xml";
}

int CDLNADmsManager::CreateContainer(wstring parentObjectID, DLNA_DMS_CONTAINER_META_INFO* info, wstring& objectID)
{
	return dmsDB.CreateContainer(parentObjectID, info, objectID);
}

int CDLNADmsManager::CreateContainer2(wstring virtualPath, DLNA_DMS_CONTAINER_META_INFO* info, wstring& objectID)
{
	return dmsDB.CreateContainer2(virtualPath, info, objectID);
}

int CDLNADmsManager::DeleteContainer(wstring objectID)
{
	return dmsDB.DeleteContainer(objectID);
}

int CDLNADmsManager::AddContent(wstring containerObjectID, DLNA_DMS_CONTENT_META_INFO* info, wstring& objectID)
{
	return dmsDB.AddContent(containerObjectID, info, objectID);
}

int CDLNADmsManager::RemoveContent(wstring objectID)
{
	return dmsDB.RemoveContent(objectID);
}

int CDLNADmsManager::HttpRequest(string method, string uri, nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	string dddUri = "";
	string sddCDSUri = "";
	string sddAVTUri = "";
	string sddCMSUri = "";
	string soapCDSUri = "";
	string soapAVTUri = "";
	string soapCMSUri = "";
	string subscribeCDSUri = "";
	string subscribeAVTUri = "";
	string subscribeCMSUri = "";
	wstring filePath = L"";
	nocase::map<string, string> httpResHeader;
	string xml = "";
	string fileUri = "";
	wstring getfilePath = L"";

	if( this->uuid.size() == 0 ){
		char uuidVal[37] = "";
		if( UPNP_UTIL_CreateUUID(uuidVal, 37, UPNP_DEVICE_TYPE_DMS, 0, 0) >= 0){
			this->uuid = uuidVal;
		}else{
			this->uuid = "cdcdcdcd-cdcd-cdcd-cdcd-cdcdcdcdcdcd";
		}
	}

	WtoA(rootUri, dddUri);
	dddUri += "/ddd.xml";
	WtoA(rootUri, sddCDSUri);
	sddCDSUri += "/cds/sdd.xml";
	WtoA(rootUri, sddAVTUri);
	sddAVTUri += "/avt/sdd.xml";
	WtoA(rootUri, sddCMSUri);
	sddCMSUri += "/cms/sdd.xml";

	WtoA(rootUri, soapCDSUri);
	soapCDSUri += "/cds/soap";
	WtoA(rootUri, soapAVTUri);
	soapAVTUri += "/avt/soap";
	WtoA(rootUri, soapCMSUri);
	soapCMSUri += "/cms/soap";

	WtoA(rootUri, subscribeCDSUri);
	subscribeCDSUri += "/cds/event";
	WtoA(rootUri, subscribeAVTUri);
	subscribeAVTUri += "/avt/event";
	WtoA(rootUri, subscribeCMSUri);
	subscribeCMSUri += "/cms/event";

	GetModuleFolderPath(filePath);
	filePath += L"\\dlna\\dms\\";

	if( method.compare("GET") == 0 ){
		if( dddUri.compare(uri) == 0 ){
			filePath += L"MediaServer1.xml";
			if( PathFileExists(filePath.c_str()) == FALSE ){
				ret = 404;
				goto Err_End;
			}
			ret = SendDDD(filePath, clientSock, stopEvent);
		}else if( sddCDSUri.compare(uri) == 0 ){
			filePath += L"ContentDirectory1.xml";
			if( PathFileExists(filePath.c_str()) == FALSE ){
				ret = 404;
				goto Err_End;
			}
			ret = SendSDD(filePath, clientSock, stopEvent);
		}else if( sddAVTUri.compare(uri) == 0 ){
			filePath += L"AVTransport1.xml";
			if( PathFileExists(filePath.c_str()) == FALSE ){
				ret = 404;
				goto Err_End;
			}
			ret = SendSDD(filePath, clientSock, stopEvent);
		}else if( sddCMSUri.compare(uri) == 0 ){
			filePath += L"ConnectionManager1.xml";
			if( PathFileExists(filePath.c_str()) == FALSE ){
				ret = 404;
				goto Err_End;
			}
			ret = SendSDD(filePath, clientSock, stopEvent);
		}else{
			WtoA(this->fileRootUri, fileUri);
			fileUri += "/";
			if( uri.find(fileUri) == 0 ){
				if( dmsDB.SearchFilePath(uri, getfilePath) == NO_ERR ){
					SendFile(getfilePath, headerList, clientSock, stopEvent);
				}else{
					ret = 404;
					goto Err_End;
				}
			}else{
				ret = 404;
				goto Err_End;
			}
		}
	}else if( method.compare("POST") == 0 ){
		if(  uri.find(soapCDSUri) == 0 ){
			ret = SoapCDS(headerList, reqReader, clientSock, stopEvent);
		}else if(  uri.find(soapAVTUri) == 0 ){
			ret = SoapAVT(headerList, reqReader, clientSock, stopEvent);
		}else if(  uri.find(soapCMSUri) == 0 ){
			ret = SoapCMS(headerList, reqReader, clientSock, stopEvent);
		}else{
			ret = UploadFile(headerList, reqReader, clientSock, stopEvent);
			//ret = 404;
			//goto Err_End;
		}
	}else if( method.compare("SUBSCRIBE") == 0 ){
		if(  uri.find(subscribeCDSUri) == 0 ){
			ret = Subscribe_CDS(headerList, reqReader, clientSock, stopEvent);
		}else if(  uri.find(subscribeAVTUri) == 0 ){
			ret = Subscribe_AVT(headerList, reqReader, clientSock, stopEvent);
		}else if(  uri.find(subscribeCMSUri) == 0 ){
			ret = Subscribe_CMS(headerList, reqReader, clientSock, stopEvent);
		}else{
			ret = 400;
			goto Err_End;
		}
	}else if( method.compare("UNSUBSCRIBE") == 0 ){
		if(  uri.find(subscribeCDSUri) == 0 ){
			ret = UnSubscribe_CDS(headerList, reqReader, clientSock, stopEvent);
		}else if(  uri.find(subscribeAVTUri) == 0 ){
			ret = UnSubscribe_AVT(headerList, reqReader, clientSock, stopEvent);
		}else if(  uri.find(subscribeCMSUri) == 0 ){
			ret = UnSubscribe_CMS(headerList, reqReader, clientSock, stopEvent);
		}else{
			ret = 400;
			goto Err_End;
		}
	}else{
		ret = 400;
		goto Err_End;
	}

	return ret;

Err_End:
	httpSend.SendResponseHeader(ret, &httpResHeader, clientSock, stopEvent);
	return ret;
}

int CDLNADmsManager::SendDDD(wstring filePath, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	string buff = "";
	__int64 fileSize = 0;
	nocase::map<string, string> httpResHeader;

	HANDLE hFile = CreateFile( filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		return 404;
	}
	DWORD dwFileSize = GetFileSize( hFile, NULL );
	if( dwFileSize == 0 ){
		CloseHandle(hFile);
		return 404;
	}
	char* pszBuff = new char[dwFileSize+1];
	if( pszBuff == NULL ){
		CloseHandle(hFile);
		return 404;
	}
	ZeroMemory(pszBuff,dwFileSize+1);
	DWORD dwRead=0;
	ReadFile( hFile, pszBuff, dwFileSize, &dwRead, NULL );

	string strRead = pszBuff;

	CloseHandle(hFile);
	SAFE_DELETE_ARRAY(pszBuff);

	Replace(strRead, "$uuid$", this->uuid);
	Replace(strRead, "$friendlyName$", "EpgTimerSrv MediaServer");
	Replace(strRead, "$manufacturer$", "test");
	Replace(strRead, "$manufacturerURL$", "");
	Replace(strRead, "$modelName$", "test");
	Replace(strRead, "$modelNumber$", "test");
	Replace(strRead, "$modelURL$", "");
	Replace(strRead, "$serialNumber$", "");

	fileSize = strRead.size();

	Format(buff, "%I64d", fileSize);
	httpResHeader.insert(pair<string, string>("Content-Length", buff));
	httpResHeader.insert(pair<string, string>("Content-Type", "text/xml"));

	httpSend.SendResponseHeader(200, &httpResHeader, clientSock, stopEvent);
	httpSend.SendData((BYTE*)strRead.c_str(), (DWORD)fileSize, clientSock, stopEvent);

	return ret;
}

int CDLNADmsManager::SendSDD(wstring filePath, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	string buff = "";
	__int64 fileSize = 0;
	nocase::map<string, string> httpResHeader;

	fileSize = httpSend.GetContentLength(filePath);

	Format(buff, "%I64d", fileSize);
	httpResHeader.insert(pair<string, string>("Content-Length", buff));
	httpResHeader.insert(pair<string, string>("Content-Type", "text/xml"));

	httpSend.SendResponseHeader(200, &httpResHeader, clientSock, stopEvent);
	httpSend.SendFile(filePath, 0, fileSize, clientSock, stopEvent);

	return ret;
}

int CDLNADmsManager::SoapCDS(nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	char soapBody[64*1024] = "";
	int soapBodySize = 64*1024;
	nocase::map<string, string> httpResHeader;
	char nowTime[30] = "";
	char ua[128] = "";
	string resBody = "";
	string buff = "";
	CSOAPUtil soapUtil;
	SOAP_REQUEST_INFO soapReq;
	string host = "";
	nocase::map<string, string>::iterator itr;

	if( reqReader->ReadBody((BYTE*)soapBody, &soapBodySize) != 2){
		httpSend.SendResponseHeader(400, &httpResHeader, clientSock, stopEvent);
		return 400;
	}
	//OutputDebugStringA(soapBody);

	if( soapUtil.ParseSOAPRequest(soapBody, soapBodySize, &soapReq) != NO_ERR ){
		httpSend.SendResponseHeader(400, &httpResHeader, clientSock, stopEvent);
		return 400;
	}

	itr = headerList->find("HOST");
	if( itr != headerList->end() ){
		host = itr->second;
	}

	httpResHeader.insert(pair<string, string>("CONTENT-TYPE", "text/xml; charset=\"utf-8\""));
	MP_TIME_GetNowTime_RFC1123(nowTime, sizeof(nowTime));
	//httpResHeader.insert(pair<string, string>("DATE", nowTime));
	UPNP_UTIL_GetUserAgent(ua, sizeof(ua));
	httpResHeader.insert(pair<string, string>("SERVER", ua));
	httpResHeader.insert(pair<string, string>("EXT", ""));
	httpResHeader.insert(pair<string, string>("CONNECTION", "Close"));

	if( CompareNoCase(soapReq.actionName, L"GetSearchCapabilities") == 0 ){
		ret = CDS_GetSearchCapabilities(&soapReq, resBody);
	}else if( CompareNoCase(soapReq.actionName, L"GetSortCapabilities") == 0 ){
		ret = CDS_GetSortCapabilities(&soapReq, resBody);
	}else if( CompareNoCase(soapReq.actionName, L"GetSystemUpdateID") == 0 ){
		ret = CDS_GetSystemUpdateID(&soapReq, resBody);
	}else if( CompareNoCase(soapReq.actionName, L"Browse") == 0 ){
		ret = CDS_Browse(host, &soapReq, resBody);
	}else if( CompareNoCase(soapReq.actionName, L"CreateObject") == 0 ){
	}else if( CompareNoCase(soapReq.actionName, L"DestroyObject") == 0 ){
	}else{
		httpResHeader.clear();
		httpSend.SendResponseHeader(400, &httpResHeader, clientSock, stopEvent);
		return 400;
	}

	Format(buff, "%d", resBody.size());
	httpResHeader.insert(pair<string, string>("CONTENT-LENGTH", buff));

	httpSend.SendResponseHeader(ret, &httpResHeader, clientSock, stopEvent);
	httpSend.SendData((BYTE*)resBody.c_str(), (DWORD)resBody.size(), clientSock, stopEvent);

	return ret;
}

int CDLNADmsManager::SoapCMS(nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	char soapBody[64*1024] = "";
	int soapBodySize = 64*1024;
	nocase::map<string, string> httpResHeader;
	char nowTime[30] = "";
	char ua[128] = "";
	string resBody = "";
	string buff = "";
	CSOAPUtil soapUtil;
	SOAP_REQUEST_INFO soapReq;

	if( reqReader->ReadBody((BYTE*)soapBody, &soapBodySize) != 2){
		httpSend.SendResponseHeader(400, &httpResHeader, clientSock, stopEvent);
		return 400;
	}
	//OutputDebugStringA(soapBody);

	if( soapUtil.ParseSOAPRequest(soapBody, soapBodySize, &soapReq) != NO_ERR ){
		httpSend.SendResponseHeader(400, &httpResHeader, clientSock, stopEvent);
		return 400;
	}

	httpResHeader.insert(pair<string, string>("CONTENT-TYPE", "text/xml; charset=\"utf-8\""));
	MP_TIME_GetNowTime_RFC1123(nowTime, sizeof(nowTime));
	httpResHeader.insert(pair<string, string>("DATE", nowTime));
	UPNP_UTIL_GetUserAgent(ua, sizeof(ua));
	httpResHeader.insert(pair<string, string>("SERVER", nowTime));

	if( CompareNoCase(soapReq.actionName, L"GetProtocolInfo") == 0 ){
		ret = CMS_GetProtocolInfo(&soapReq, resBody);
	}else{
		httpResHeader.clear();
		httpSend.SendResponseHeader(400, &httpResHeader, clientSock, stopEvent);
		return 400;
	}

	Format(buff, "%d", resBody.size());
	httpResHeader.insert(pair<string, string>("CONTENT-LENGTH", buff));

	httpSend.SendResponseHeader(ret, &httpResHeader, clientSock, stopEvent);
	httpSend.SendData((BYTE*)resBody.c_str(), (DWORD)resBody.size(), clientSock, stopEvent);

	return ret;
}

int CDLNADmsManager::SoapAVT(nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	char soapBody[64*1024] = "";
	int soapBodySize = 64*1024;

	if( reqReader->ReadBody((BYTE*)soapBody, &soapBodySize) != 2){
		return 400;
	}
	//OutputDebugStringA(soapBody);

	CSOAPUtil soapUtil;
	SOAP_REQUEST_INFO soapReq;

	if( soapUtil.ParseSOAPRequest(soapBody, soapBodySize, &soapReq) != NO_ERR ){
		httpSend.SendResponseHeader(400, NULL, clientSock, stopEvent);
		return 400;
	}

	return ret;
}

int CDLNADmsManager::CDS_GetSearchCapabilities(SOAP_REQUEST_INFO* soapReq, string& resBody)
{
	int ret = 200;

	CSOAPUtil soapUtil;
	vector<SOAP_RESPONSE_ARG> argList;
	SOAP_RESPONSE_ARG argItem;
	char* xml = NULL;
	int xmlSize = 0;

	argItem.name = "SearchCaps";
	argItem.val = "";
	argList.push_back(argItem);

	soapUtil.CreateSOAPResponseBody((char*)UPNP_URN_CDS_1, "GetSearchCapabilitiesResponse", &argList, xml, &xmlSize);
	xml = (char*)malloc(xmlSize);
	memset(xml, 0, xmlSize);
	soapUtil.CreateSOAPResponseBody((char*)UPNP_URN_CDS_1, "GetSearchCapabilitiesResponse", &argList, xml, &xmlSize);

	resBody = xml;

	free(xml);

	return ret;
}

int CDLNADmsManager::CDS_GetSortCapabilities(SOAP_REQUEST_INFO* soapReq, string& resBody)
{
	int ret = 200;

	CSOAPUtil soapUtil;
	vector<SOAP_RESPONSE_ARG> argList;
	SOAP_RESPONSE_ARG argItem;
	char* xml = NULL;
	int xmlSize = 0;

	argItem.name = "SortCaps";
	argItem.val = "dc:title";
	argList.push_back(argItem);

	soapUtil.CreateSOAPResponseBody((char*)UPNP_URN_CDS_1, "GetSortCapabilitiesResponse", &argList, xml, &xmlSize);
	xml = (char*)malloc(xmlSize);
	memset(xml, 0, xmlSize);
	soapUtil.CreateSOAPResponseBody((char*)UPNP_URN_CDS_1, "GetSortCapabilitiesResponse", &argList, xml, &xmlSize);

	resBody = xml;

	free(xml);

	return ret;
}

int CDLNADmsManager::CDS_GetSystemUpdateID(SOAP_REQUEST_INFO* soapReq, string& resBody)
{
	int ret = 200;

	CSOAPUtil soapUtil;
	vector<SOAP_RESPONSE_ARG> argList;
	SOAP_RESPONSE_ARG argItem;
	char* xml = NULL;
	int xmlSize = 0;

	argItem.name = "Id";
	argItem.val = "1";
	argList.push_back(argItem);

	soapUtil.CreateSOAPResponseBody((char*)UPNP_URN_CDS_1, "GetSystemUpdateIDResponse", &argList, xml, &xmlSize);
	xml = (char*)malloc(xmlSize);
	memset(xml, 0, xmlSize);
	soapUtil.CreateSOAPResponseBody((char*)UPNP_URN_CDS_1, "GetSystemUpdateIDResponse", &argList, xml, &xmlSize);

	resBody = xml;

	free(xml);

	return ret;
}

int CDLNADmsManager::CDS_Browse(string host, SOAP_REQUEST_INFO* soapReq, string& resBody)
{
	int ret = 200;

	CSOAPUtil soapUtil;
	vector<SOAP_RESPONSE_ARG> argList;
	SOAP_RESPONSE_ARG argItem;
	char* xml = NULL;
	int xmlSize = 0;
	string buff = "";
	map<wstring, wstring>::iterator itr;

	wstring objectID = L"";
	wstring browseFlag = L"";
	wstring filter = L"";
	int startIndex = 0;
	int requestedCount = 0;
	wstring sortCriteria = L"";
	wstring result = L"";
	int numberReturned = 0;
	int totalMatches = 0;
	int updateID = 1;
	vector<DLNA_DMS_CONTAINER_INFO> childContainerList;
	vector<DLNA_DMS_CONTENT_INFO> childContentList;
	DLNA_DMS_CONTAINER_INFO containerInfo;
	DLNA_DMS_CONTENT_INFO contentInfo;
	int counter = 0;
	wstring metaData = L"";
	wstring hostW = L"";

	AtoW(host, hostW);

	itr = soapReq->argList.find(L"ObjectID");
	if( itr == soapReq->argList.end()){
		ret = 402;
		return ret;
	}
	objectID = itr->second;

	itr = soapReq->argList.find(L"BrowseFlag");
	if( itr == soapReq->argList.end()){
		ret = 402;
		return ret;
	}
	browseFlag = itr->second;

	itr = soapReq->argList.find(L"Filter");
	if( itr == soapReq->argList.end()){
		ret = 402;
		return ret;
	}
	filter = itr->second;

	itr = soapReq->argList.find(L"StartingIndex");
	if( itr == soapReq->argList.end()){
		ret = 402;
		return ret;
	}
	startIndex = _wtoi(itr->second.c_str());

	itr = soapReq->argList.find(L"RequestedCount");
	if( itr == soapReq->argList.end()){
		ret = 402;
		return ret;
	}
	requestedCount = _wtoi(itr->second.c_str());
	if( requestedCount == 0 ){
		requestedCount = 30;
	}

	itr = soapReq->argList.find(L"SortCriteria");
	if( itr == soapReq->argList.end()){
		ret = 402;
		return ret;
	}
	sortCriteria = itr->second;

	if( CompareNoCase(browseFlag, L"BrowseMetadata") == 0 ){
		result = DIDLStart;

		if( dmsDB.GetContainerItem(objectID, &containerInfo) != ERR_FALSE ){
			DIDL_CreateContainerMetaData(&containerInfo, metaData);
			result += metaData;
		}else if( dmsDB.GetContentItem(objectID, &contentInfo) != ERR_FALSE ){
			DIDL_CreateContentMetaData(hostW, &contentInfo, metaData);
			result += metaData;
		}else{
			ret = 402;
			return ret;
		}

		result += DIDLEnd;
		numberReturned = 1;
		totalMatches = 1;
	}else if( CompareNoCase(browseFlag, L"BrowseDirectChildren") == 0 ){
		dmsDB.GetChildItem(objectID, &childContainerList, &childContentList);
		totalMatches = (int)(childContainerList.size() + childContentList.size());
		if( requestedCount != 0 ){
			if( startIndex + requestedCount < totalMatches ){
				numberReturned = requestedCount;
			}else{
				numberReturned = totalMatches-startIndex;
			}
		}else{
			numberReturned = totalMatches-startIndex;
		}

		result = DIDLStart;

		for( size_t i=0; i<childContainerList.size(); i++ ){
			if( startIndex <= counter ){
				if( requestedCount != 0 && startIndex+requestedCount < counter+1){
					break;
				}
				metaData = L"";
				DIDL_CreateContainerMetaData(&childContainerList[i], metaData);
				result += metaData;
			}
			counter++;
		}
		for( size_t i=0; i<childContentList.size(); i++ ){
			if( startIndex <= counter ){
				if( requestedCount != 0 && startIndex+requestedCount < counter+1){
					break;
				}
				metaData = L"";
				DIDL_CreateContentMetaData(hostW, &childContentList[i], metaData);
				result += metaData;
			}
			counter++;
		}

		result += DIDLEnd;
	}else{
		ret = 402;
		return ret;
	}


	WtoA(result, buff);
	argItem.name = "Result";
	argItem.val = buff;
	argList.push_back(argItem);

	Format(buff, "%d", numberReturned);
	argItem.name = "NumberReturned";
	argItem.val = buff;
	argList.push_back(argItem);

	Format(buff, "%d", totalMatches);
	argItem.name = "TotalMatches";
	argItem.val = buff;
	argList.push_back(argItem);

	Format(buff, "%d", updateID);
	argItem.name = "UpdateID";
	argItem.val = buff;
	argList.push_back(argItem);

	soapUtil.CreateSOAPResponseBody((char*)UPNP_URN_CDS_1, "BrowseResponse", &argList, xml, &xmlSize);
	xml = (char*)malloc(xmlSize);
	memset(xml, 0, xmlSize);
	soapUtil.CreateSOAPResponseBody((char*)UPNP_URN_CDS_1, "BrowseResponse", &argList, xml, &xmlSize);

	resBody = xml;

	free(xml);

	return ret;
}

int CDLNADmsManager::DIDL_CreateContainerMetaData(DLNA_DMS_CONTAINER_INFO*item, wstring& metaData)
{
	int ret = NO_ERR;

	int restricted = 0;

	if( item->info.uploadSupportFlag == 1 ){
		restricted = 0;
	}else{
		restricted = 1;
	}

	wstring escape = item->info.title;
	EscapeXmlString(escape);

	Format(metaData, L"<container id=\"%s\" restricted=\"%d\" parentID=\"%s\"><upnp:class>object.container</upnp:class><dc:title>%s</dc:title></container>",
		item->objectID.c_str(),
		restricted,
		item->parentObjectID.c_str(),
		escape.c_str()
		);

	return ret;
}

int CDLNADmsManager::DIDL_CreateContentMetaData(wstring host, DLNA_DMS_CONTENT_INFO* item, wstring& metaData)
{
	int ret = NO_ERR;

	wstring buff;
	wstring res;

	Format(buff, L"<item id=\"%s\" restricted=\"1\" parentID=\"%s\">",
		item->objectID.c_str(), 
		item->parentObjectID.c_str()
		);
	metaData += buff;

	if( item->info.title.size() == 0 ){
		item->info.title = L"no title";
	}
	wstring escape = item->info.title;
	EscapeXmlString(escape);
	Format(buff, L"<dc:title>%s</dc:title>",
		escape.c_str()
		);
	metaData += buff;

	Format(buff, L"<upnp:class>%s</upnp:class>",
		item->info.upnpClass.c_str()
		);
	metaData += buff;


	res = L"<res ";
	if( item->info.durationSec >= 0 ){
		wstring duration;
		Format(duration, L"%02d:%02d:%02d",
			item->info.durationSec/(60*60),
			(item->info.durationSec%(60*60))/60,
			item->info.durationSec%60
			);

		Format(buff, L"duration=\"%s\" ",
			duration.c_str()
			);
		res += buff;
	}

	if( item->info.fileSize >= 0 ){
		Format(buff, L"size=\"%I64d\" ",
			item->info.fileSize
			);
		res += buff;
	}

	wstring protocolInfo;
	//Format(protocolInfo, L"protocolInfo=\"http-get:*:%s:\"",
	Format(protocolInfo, L"protocolInfo=\"http-get:*:%s:DLNA.ORG_OP=01;DLNA.ORG_FLAGS=01500000000000000000000000000000",
		item->info.mimeType.c_str()
		);
	if( item->info.protocolInfo.size() > 0 ){
		Format(buff, L";DLNA.ORG_PN=%s\" ",
			item->info.protocolInfo.c_str()
			);
	}else{
		buff = L"\" ";
	}
	protocolInfo += buff;
	
	res += protocolInfo;

	res += L">";

	wstring url;
	Format(url, L"http://%s%s",
		host.c_str(),
		item->uri.c_str()
		);
	res += url;

	res += L"</res>";

	metaData += res;

	metaData += L"</item>";

	return ret;
}

int CDLNADmsManager::CMS_GetProtocolInfo(SOAP_REQUEST_INFO* soapReq, string& resBody)
{
	int ret = 200;

	CSOAPUtil soapUtil;
	vector<SOAP_RESPONSE_ARG> argList;
	SOAP_RESPONSE_ARG argItem;
	char* xml = NULL;
	int xmlSize = 0;
	string buff = "";
	wstring buffW = L"";
	map<wstring, DLNA_PROTOCOL_INFO>::iterator itr;
	map<wstring, wstring> idList;
	map<wstring, wstring>::iterator itrID;;

	for( itr = protocolInfo.supportList.begin(); itr != protocolInfo.supportList.end(); itr++ ){
		idList.insert(pair<wstring, wstring>(itr->second.info,itr->second.info));
	}
	for( itrID = idList.begin(); itrID != idList.end(); itrID++ ){
		if( buffW.size() > 0 ){
			buffW += L",";
		}
		buffW+=itrID->second;
	}

	WtoA(buffW, buff);
	argItem.name = "Source";
	argItem.val = buff;
	argList.push_back(argItem);

	argItem.name = "Sink";
	argItem.val = "";
	argList.push_back(argItem);

	soapUtil.CreateSOAPResponseBody((char*)UPNP_URN_CMS_1, "GetProtocolInfo", &argList, xml, &xmlSize);
	xml = (char*)malloc(xmlSize);
	memset(xml, 0, xmlSize);
	soapUtil.CreateSOAPResponseBody((char*)UPNP_URN_CMS_1, "GetProtocolInfo", &argList, xml, &xmlSize);

	resBody = xml;

	free(xml);

	return ret;
}

int CDLNADmsManager::SendFile(wstring filePath, nocase::map<string, string>* headerList, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	string buff = "";
	wstring wbuff = L"";
	nocase::map<string, string>::iterator itrHead;
	nocase::map<string, string> httpResHeader;
	__int64 fileSize = 0;
	string contentType = "";
	__int64 offset = 0;
	__int64 endPos = 0;
	map<__int64, __int64> rangeList;

	if( PathFileExists(filePath.c_str()) == FALSE ){
		ret = 404;
		goto Err_End;
	}

	httpSend.GetContentType(filePath, contentType);
	fileSize = httpSend.GetContentLength(filePath);

	endPos = fileSize;

	itrHead = headerList->find("Range");
	httpResHeader.insert(pair<string, string>("Connection", "close"));
	if( itrHead == headerList->end() ){
		//RangeÇ»Çµ
		Format(buff, "%I64d", fileSize);
		httpResHeader.insert(pair<string, string>("Content-Length", buff));
		httpResHeader.insert(pair<string, string>("Content-Type", contentType));

		httpSend.SendResponseHeader(200, &httpResHeader, clientSock, stopEvent);
		httpSend.SendFile(filePath, offset, endPos, clientSock, stopEvent);
	}else{
		if( httpSend.ParseRangeHeader(fileSize, itrHead->second, &rangeList) == FALSE ){
			//RangeéwíËÇ™âΩÇ©Ç®Ç©ÇµÇ¢
			Format(buff, "%I64d", fileSize);
			httpResHeader.insert(pair<string, string>("Content-Length", buff));
			httpResHeader.insert(pair<string, string>("Content-Type", contentType));

			httpSend.SendResponseHeader(200, &httpResHeader, clientSock, stopEvent);
			httpSend.SendFile(filePath, offset, endPos, clientSock, stopEvent);
		}else{
			httpResHeader.insert(pair<string, string>("Accept-Ranges", "bytes"));
			if( rangeList.size() > 1 ){
				//ï°êîÇÃèÍèäéwíË
				Format(buff, "multipart/byteranges; boundary=%s", MULTI_PART_BOUNDARY);
				httpResHeader.insert(pair<string, string>("Content-Type", buff));

				httpSend.SendResponseHeader(206, &httpResHeader, clientSock, stopEvent);


				map<__int64, __int64>::iterator itr;
				string boundary = "";
				for( itr = rangeList.begin(); itr != rangeList.end(); itr++ ){
					boundary = "--";
					boundary += MULTI_PART_BOUNDARY;
					boundary += "\r\n";
					Format(buff, "Content-Type: %s\r\n", contentType);
					boundary += buff;
					Format(buff, "Content-Range: bytes %I64d-%I64d/%I64d\r\n", itr->first, itr->second, fileSize);
					boundary += buff;
					boundary += "\r\n";

					if( httpSend.SendData((BYTE*)boundary.c_str(), (DWORD)boundary.size(), clientSock, stopEvent) != NO_ERR ){
						goto End;
					}
					if( httpSend.SendFile(filePath, itr->first, itr->second+1, clientSock, stopEvent) != NO_ERR ){
						goto End;
					}

					boundary = "\r\n";
					httpSend.SendData((BYTE*)boundary.c_str(), (DWORD)boundary.size(), clientSock, stopEvent);
				}
				boundary = "--";
				boundary += MULTI_PART_BOUNDARY;
				boundary += "--\r\n";
				httpSend.SendData((BYTE*)boundary.c_str(), (DWORD)boundary.size(), clientSock, stopEvent);
			}else{
				//àÍÉJèäÇÃÇ›
				map<__int64, __int64>::iterator itr;
				itr = rangeList.begin();

				httpResHeader.insert(pair<string, string>("Content-Type", contentType));
				Format(buff, "bytes %I64d-%I64d/%I64d", itr->first, itr->second, fileSize);
				httpResHeader.insert(pair<string, string>("Content-Range", buff));
				Format(buff, "%I64d", itr->second-itr->first+1);
				httpResHeader.insert(pair<string, string>("Content-Length", buff));

				httpSend.SendResponseHeader(206, &httpResHeader, clientSock, stopEvent);
				httpSend.SendFile(filePath, itr->first, itr->second+1, clientSock, stopEvent);

			}
		}
	}


	return ret;

Err_End:
	httpSend.SendResponseHeader(ret, &httpResHeader, clientSock, stopEvent);
End:
	return ret;
}

int CDLNADmsManager::UploadFile(nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	char body[64*1024] = "";
	int bodySize = 64*1024;
	nocase::map<string, string> httpResHeader;

	DWORD write;
	int result = 0;
	nocase::map<string, string>::iterator itr;

	itr = headerList->find("Expect");
	if( itr != headerList->end() ){
		if( CompareNoCase(itr->second, "100-continue") == 0 ){
			httpSend.SendResponseHeader(100, &httpResHeader, clientSock, stopEvent);
		}
	}

	wstring filePath;
	GetModuleFolderPath(filePath);
	filePath += L"\\uploaddata.bin";
	HANDLE file = CreateFile(filePath.c_str(), GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	do{
		result = reqReader->ReadBody((BYTE*)body, &bodySize);
		if( result == 1 || result == 2 ){
			if( WriteFile(file, body, bodySize, &write, NULL ) == FALSE ){
				break;
			}
		}else{
			break;
		}
	}while(result == 1 );

	CloseHandle(file);

	httpSend.SendResponseHeader(ret, &httpResHeader, clientSock, stopEvent);

	return ret;
}

int CDLNADmsManager::Subscribe_CDS(nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	string buff = "";
	nocase::map<string, string> httpResHeader;
	char uuidVal[37] = "cdcdcdcd-cdcd-cdcd-cdcd-cdcdcdcdcdcd";
	string sid;
	char ua[128] = "";
	map<wstring, __int64>::iterator itrCallback;
	nocase::map<string, string>::iterator itrHeade;
	wstring callback=L"";
	wstring timeout=L"";
	__int64 timeoutSec = 0;

	UPNP_UTIL_CreateUUID(uuidVal, 37, UPNP_DEVICE_TYPE_DMS, UPNP_SERVICE_TYPE_CDS, subscribeCount);
	Format(sid, "uuid:%s", uuidVal);
	this->subscribeCount++;

	UPNP_UTIL_GetUserAgent(ua, sizeof(ua));

	httpResHeader.insert(pair<string, string>("SID", sid));
	httpResHeader.insert(pair<string, string>("SERVER", ua));
	httpResHeader.insert(pair<string, string>("TIMEOUT", "Second-1800"));
	httpResHeader.insert(pair<string, string>("Content-Length", "0"));

	httpSend.SendResponseHeader(200, &httpResHeader, clientSock, stopEvent);

	itrHeade = headerList->find("Callback");
	if( itrHeade != headerList->end()){
		AtoW(itrHeade->second, callback);

		itrHeade = headerList->find("Timeout");
		if( itrHeade != headerList->end()){
			AtoW(itrHeade->second, timeout);

			Replace(timeout, L"Second-", L"");
			timeoutSec = _wtoi(timeout.c_str()) * I64_1SEC;
		}

		Replace(callback, L"<", L"");
		Replace(callback, L">", L"");
		itrCallback = cdsSubscribeList.find(callback);
		if( itrCallback == cdsSubscribeList.end() ){
			cdsSubscribeList.insert(pair<wstring, __int64>(callback, GetNowI64Time()+timeoutSec));
		}else{
			itrCallback->second = GetNowI64Time()+timeoutSec;
		}
	}

	SendEvent();

	return ret;
}

int CDLNADmsManager::Subscribe_CMS(nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	string buff = "";
	nocase::map<string, string> httpResHeader;
	char uuidVal[37] = "cdcdcdcd-cdcd-cdcd-cdcd-cdcdcdcdcdcd";
	string sid;
	char ua[128] = "";

	UPNP_UTIL_CreateUUID(uuidVal, 37, UPNP_DEVICE_TYPE_DMS, UPNP_SERVICE_TYPE_CMS, subscribeCount);
	Format(sid, "uuid:%s", uuidVal);

	UPNP_UTIL_GetUserAgent(ua, sizeof(ua));

	httpResHeader.insert(pair<string, string>("SID", sid));
	httpResHeader.insert(pair<string, string>("SERVER", ua));
	httpResHeader.insert(pair<string, string>("TIMEOUT", "Second-1800"));
	httpResHeader.insert(pair<string, string>("Content-Length", "0"));

	httpSend.SendResponseHeader(200, &httpResHeader, clientSock, stopEvent);

	return ret;
}

int CDLNADmsManager::Subscribe_AVT(nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	string buff = "";
	nocase::map<string, string> httpResHeader;
	char uuidVal[37] = "cdcdcdcd-cdcd-cdcd-cdcd-cdcdcdcdcdcd";
	string sid;
	char ua[128] = "";

	UPNP_UTIL_CreateUUID(uuidVal, 37, UPNP_DEVICE_TYPE_DMS, UPNP_SERVICE_TYPE_ATS, subscribeCount);
	Format(sid, "uuid:%s", uuidVal);

	UPNP_UTIL_GetUserAgent(ua, sizeof(ua));

	httpResHeader.insert(pair<string, string>("SID", sid));
	httpResHeader.insert(pair<string, string>("SERVER", ua));
	httpResHeader.insert(pair<string, string>("TIMEOUT", "Second-1800"));
	httpResHeader.insert(pair<string, string>("Content-Length", "0"));

	httpSend.SendResponseHeader(200, &httpResHeader, clientSock, stopEvent);

	return ret;
}

int CDLNADmsManager::UnSubscribe_CDS(nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	nocase::map<string, string> httpResHeader;

	httpSend.SendResponseHeader(200, &httpResHeader, clientSock, stopEvent);

	return ret;
}

int CDLNADmsManager::UnSubscribe_CMS(nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	nocase::map<string, string> httpResHeader;

	httpSend.SendResponseHeader(200, &httpResHeader, clientSock, stopEvent);

	return ret;
}

int CDLNADmsManager::UnSubscribe_AVT(nocase::map<string, string>* headerList, CHttpRequestReader* reqReader, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 200;
	nocase::map<string, string> httpResHeader;

	httpSend.SendResponseHeader(200, &httpResHeader, clientSock, stopEvent);

	return ret;
}


void CDLNADmsManager::SendEvent()
{
	
}

#include "StdAfx.h"
#include "HttpFileSend.h"

#include "../../Common/StringUtil.h"
#include "../../Common/PathUtil.h"
#include "../../Common/ParseTextInstances.h"
#include "../../Common/ErrDef.h"

typedef struct _RANGE_ITEM{
	__int64 start;
	__int64 end;
}RANGE_ITEM;


CHttpFileSend::CHttpFileSend(void)
{
}


CHttpFileSend::~CHttpFileSend(void)
{
}

int CHttpFileSend::SendResponseHeader(int httpStatus, nocase::map<string, string>* paramList, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = 0;

	string sendHeader = "";

	switch(httpStatus){
	case 100:
		sendHeader = "HTTP/1.1 100 Continue\r\n";
		break;
	case 101:
		sendHeader = "HTTP/1.1 101 Switching Protocols\r\n";
		break;
	case 102:
		sendHeader = "HTTP/1.1 102 Processing\r\n";
		break;
	case 200:
		sendHeader = "HTTP/1.1 200 OK\r\n";
		break;
	case 201:
		sendHeader = "HTTP/1.1 201 Created\r\n";
		break;
	case 202:
		sendHeader = "HTTP/1.1 202 Accepted\r\n";
		break;
	case 203:
		sendHeader = "HTTP/1.1 203 Non-Authoritative Information\r\n";
		break;
	case 204:
		sendHeader = "HTTP/1.1 204 No Content\r\n";
		break;
	case 205:
		sendHeader = "HTTP/1.1 205 Reset Content\r\n";
		break;
	case 206:
		sendHeader = "HTTP/1.1 206 Partial Content\r\n";
		break;
	case 207:
		sendHeader = "HTTP/1.1 207 Multi-Status\r\n";
		break;
	case 208:
		sendHeader = "HTTP/1.1 208 Already Reported\r\n";
		break;
	case 226:
		sendHeader = "HTTP/1.1 226 IM Used\r\n";
		break;
	case 300:
		sendHeader = "HTTP/1.1 300 Multiple Choices\r\n";
		break;
	case 301:
		sendHeader = "HTTP/1.1 301 Moved Permanently\r\n";
		break;
	case 302:
		sendHeader = "HTTP/1.1 302 Found\r\n";
		break;
	case 303:
		sendHeader = "HTTP/1.1 226 303 See Other\r\n";
		break;
	case 304:
		sendHeader = "HTTP/1.1 304 Not Modified\r\n";
		break;
	case 305:
		sendHeader = "HTTP/1.1 305 Use Proxy\r\n";
		break;
	case 307:
		sendHeader = "HTTP/1.1 307 Temporary Redirect\r\n";
		break;
	case 308:
		sendHeader = "HTTP/1.1 308 Permanent Redirect\r\n";
		break;
	case 400:
		sendHeader = "HTTP/1.1 400 Bad Request\r\n";
		break;
	case 401:
		sendHeader = "HTTP/1.1 401 Unauthorized\r\n";
		break;
	case 402:
		sendHeader = "HTTP/1.1 402 Payment Required\r\n";
		break;
	case 403:
		sendHeader = "HTTP/1.1 403 Forbidden\r\n";
		break;
	case 404:
		sendHeader = "HTTP/1.1 404 Not Found\r\n";
		break;
	case 405:
		sendHeader = "HTTP/1.1 405 Method Not Allowed\r\n";
		break;
	case 406:
		sendHeader = "HTTP/1.1 406 Not Acceptable\r\n";
		break;
	case 407:
		sendHeader = "HTTP/1.1 407 Proxy Authentication Required\r\n";
		break;
	case 408:
		sendHeader = "HTTP/1.1 408 Request Timeout\r\n";
		break;
	case 409:
		sendHeader = "HTTP/1.1 409 Conflict\r\n";
		break;
	case 410:
		sendHeader = "HTTP/1.1 410 Gone\r\n";
		break;
	case 411:
		sendHeader = "HTTP/1.1 411 Length Required\r\n";
		break;
	case 412:
		sendHeader = "HTTP/1.1 412 Precondition Failed\r\n";
		break;
	case 413:
		sendHeader = "HTTP/1.1 413 Request Entity Too Large\r\n";
		break;
	case 414:
		sendHeader = "HTTP/1.1 414 Request-URI Too Long\r\n";
		break;
	case 415:
		sendHeader = "HTTP/1.1 415 Unsupported Media Type\r\n";
		break;
	case 416:
		sendHeader = "HTTP/1.1 416 Requested Range Not Satisfiable\r\n";
		break;
	case 417:
		sendHeader = "HTTP/1.1 417 Expectation Failed\r\n";
		break;
	case 422:
		sendHeader = "HTTP/1.1 422 Unprocessable Entity\r\n";
		break;
	case 423:
		sendHeader = "HTTP/1.1 423 Locked\r\n";
		break;
	case 424:
		sendHeader = "HTTP/1.1 424 Failed Dependency\r\n";
		break;
	case 426:
		sendHeader = "HTTP/1.1 426 Upgrade Required\r\n";
		break;
	case 428:
		sendHeader = "HTTP/1.1 428 Precondition Required\r\n";
		break;
	case 429:
		sendHeader = "HTTP/1.1 429 Too Many Requests\r\n";
		break;
	case 431:
		sendHeader = "HTTP/1.1 431 Request Header Fields Too Large\r\n";
		break;
	case 500:
		sendHeader = "HTTP/1.1 500 Internal Server Error\r\n";
		break;
	case 501:
		sendHeader = "HTTP/1.1 501 Not Implemented\r\n";
		break;
	case 502:
		sendHeader = "HTTP/1.1 502 Bad Gateway\r\n";
		break;
	case 503:
		sendHeader = "HTTP/1.1 503 Service Unavailable\r\n";
		break;
	case 504:
		sendHeader = "HTTP/1.1 504 Gateway Timeout\r\n";
		break;
	case 505:
		sendHeader = "HTTP/1.1 505 HTTP Version Not Supported\r\n";
		break;
	case 506:
		sendHeader = "HTTP/1.1 506 Variant Also Negotiates\r\n";
		break;
	case 507:
		sendHeader = "HTTP/1.1 507 Insufficient Storage\r\n";
		break;
	case 508:
		sendHeader = "HTTP/1.1 508 Loop Detected\r\n";
		break;
	case 510:
		sendHeader = "HTTP/1.1 510 Not Extended\r\n";
		break;
	case 511:
		sendHeader = "HTTP/1.1 511 Network Authentication Required\r\n";
		break;
	default:
		sendHeader = "HTTP/1.1 500 Internal Server Error\r\n";
		break;
	}

	if( paramList != NULL ){
		nocase::map<string, string>::iterator itr;
		for( itr = paramList->begin(); itr != paramList->end(); itr++ ){
			string buff = "";
			Format(buff, "%s: %s\r\n", itr->first.c_str(), itr->second.c_str());
			sendHeader += buff;
		}
		/*
		if( paramList->find("Content-Length") == paramList->end() ){
			itr = paramList->find("Transfer-Encoding");
			if( itr != paramList->end() ){
				if( CompareNoCase(itr->second, "chunked") != 0 ){
					sendHeader += "Content-Length: 0\r\n";
				}
			}else{
				sendHeader += "Content-Length: 0\r\n";
			}
		}*/
	}else{
		sendHeader += "Content-Length: 0\r\n";
	}

	sendHeader += "\r\n";

	ret = send(clientSock, sendHeader.c_str(), (int)sendHeader.size(), 0);
	//OutputDebugStringA(sendHeader.c_str());
	return ret;
}

int CHttpFileSend::SendFile(wstring filePath, __int64 offset, __int64 endPos, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = NO_ERR;

	HANDLE hFile = CreateFile( filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		return ERR_FALSE;
	}
	DWORD sizeH = 0;
	DWORD sizeL = GetFileSize( hFile, &sizeH );

	__int64 fileSize = 0;

	fileSize = ((__int64)sizeH)<<32 | sizeL;

	if( endPos >= 0 ){
		fileSize = endPos;
	}

	DWORD buffSize = 256*1024;
	DWORD readSize = 0;
	__int64 totalReadSize = 0;
	BYTE* readBuff = NULL;

	DWORD read;
	DWORD totalSendSize = 0;

	readBuff = new BYTE[buffSize];
	totalReadSize = offset;

	LONG setH = (LONG)(offset>>32);
	LONG setL = (LONG)(offset & 0x00000000FFFFFFFF);
	SetFilePointer(hFile, setL, &setH, FILE_BEGIN);

	fd_set ready;
	struct timeval to;
	int result;

	u_long val=1;
	int sock_buf_size = 1024*1024;

	ioctlsocket(clientSock, FIONBIO, &val);
	setsockopt( clientSock, SOL_SOCKET, SO_SNDBUF, (char *)&sock_buf_size, sizeof(sock_buf_size) );




	while(totalReadSize < fileSize){
		if( WaitForSingleObject( stopEvent, 0 ) != WAIT_TIMEOUT ){
			//’†Ž~
			break;
		}

		if(totalReadSize+buffSize < fileSize){
			readSize = buffSize;
		}else{
			readSize = (DWORD)(fileSize-totalReadSize);
		}
		ReadFile(hFile, readBuff, readSize, &read, NULL );

		totalSendSize = 0;

		while(totalSendSize<readSize){
			if( WaitForSingleObject( stopEvent, 0 ) != WAIT_TIMEOUT ){
				//’†Ž~
				ret = ERR_FALSE;
				goto Err_End;
			}

			to.tv_sec = 0;
			to.tv_usec = 500*1000;
			FD_ZERO(&ready);
			FD_SET(clientSock, &ready);

			result = select(0, NULL, &ready, NULL, &to );
			if( result == SOCKET_ERROR ){
				int a = WSAGetLastError();
				ret = ERR_FALSE;
				goto Err_End;
			}
			if ( FD_ISSET(clientSock, &ready) ){
				result = send(clientSock, (char*)readBuff+totalSendSize, readSize-totalSendSize, 0);
				if( result == SOCKET_ERROR || result == 0){
					if (WSAGetLastError() == WSAEWOULDBLOCK) {
						continue;
					}

				int a = WSAGetLastError();
					ret = ERR_FALSE;
					goto Err_End;
				}
				totalSendSize+=result;
			}
		}
		totalReadSize+=readSize;
	}

Err_End:
	CloseHandle(hFile);
	SAFE_DELETE_ARRAY(readBuff);

	return ret;
}

int CHttpFileSend::SendData(BYTE* data, DWORD dataSize, SOCKET clientSock, HANDLE stopEvent)
{
	int ret = NO_ERR;
	DWORD totalSendSize = 0;

	fd_set ready;
	struct timeval to;
	int result;

	while(totalSendSize<dataSize){
		if( WaitForSingleObject( stopEvent, 0 ) != WAIT_TIMEOUT ){
			//’†Ž~
			ret = ERR_FALSE;
			break;
		}

		to.tv_sec = 0;
		to.tv_usec = 500*1000;
		FD_ZERO(&ready);
		FD_SET(clientSock, &ready);

		result = select(0, NULL, &ready, NULL, &to );
		if( result == SOCKET_ERROR ){
			ret = ERR_FALSE;
			break;
		}
		if ( FD_ISSET(clientSock, &ready) ){
			result = send(clientSock, (char*)data+totalSendSize, dataSize-totalSendSize, 0);
			if( result == SOCKET_ERROR || result == 0){
				ret = ERR_FALSE;
				break;
			}
			totalSendSize+=result;
		}
	}

	return ret;
}

void CHttpFileSend::GetContentType(wstring filePath, string& contentType)
{
	wstring ext = L"";
	GetFileExt(filePath, ext);

	wstring textPath;
	GetModuleFolderPath(textPath);
	textPath += L"\\ContentTypeText.txt";

	CParseContentTypeText contentTypeUtil;
	contentTypeUtil.ParseText(textPath.c_str());

	wstring typeW = L"";
	contentTypeUtil.GetMimeType(ext, typeW);
	WtoA(typeW, contentType);
}

__int64 CHttpFileSend::GetContentLength(wstring filePath)
{
	HANDLE hFile = CreateFile( filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
	if( hFile == INVALID_HANDLE_VALUE ){
		return 0;
	}
	DWORD sizeH = 0;
	DWORD sizeL = GetFileSize( hFile, &sizeH );

	__int64 fileSize = 0;

	fileSize = ((__int64)sizeH)<<32 | sizeL;

	CloseHandle(hFile);

	return fileSize;
}

BOOL CHttpFileSend::ParseRangeHeader(__int64 fileSize, string val, map<__int64, __int64>* rangeList)
{
	vector<string> valList;
	vector<RANGE_ITEM> rangeItem;
	string buff1 = "";
	string buff2 = "";
	string tempBuff = "";

	if( rangeList == NULL ){
		return FALSE;
	}

	buff1 = val;
	do{
		Separate(buff1, ",", buff1, buff2);
		valList.push_back(buff1);
	}while(buff2.size() > 0);

	for( size_t i=0; i<valList.size(); i++ ){
		BOOL itemChg = FALSE;
		RANGE_ITEM item;
		Separate(valList[i], "-", buff1, buff2);
		Separate(buff1, "=", tempBuff, buff1);

		item.start = _atoi64(buff1.c_str());
		item.end = fileSize-1;

		if( buff2.size() > 0 ){
			item.end = _atoi64(buff2.c_str());
		}

		if( item.end > fileSize ){
			rangeItem.clear();
			break;
		}

		for( size_t j=0; j<rangeItem.size(); j++){
			if( rangeItem[j].start <= item.start && item.start <= rangeItem[j].end ){
				if( rangeItem[j].end < item.end ){
					rangeItem[j].end = item.end;
				}
				itemChg = TRUE;
				break;
			}
			if( rangeItem[j].start <= item.end && item.end <= rangeItem[j].end ){
				if( item.start < rangeItem[j].start ){
					rangeItem[j].start = item.start;
				}
				itemChg = TRUE;
				break;
			}
			if( item.start <= rangeItem[j].start && rangeItem[j].start <= item.end &&
				item.start <= rangeItem[j].end && rangeItem[j].end <= item.end
				){
				rangeItem[j].start = item.start;
				rangeItem[j].end = item.end;
				itemChg = TRUE;
				break;
			}
		}
		if( itemChg == FALSE ){
			rangeItem.push_back(item);
		}
	}

	for( size_t i=0 ;i<rangeItem.size(); i++ ){
		rangeList->insert(pair<__int64, __int64>(rangeItem[i].start, rangeItem[i].end));
	}

	//‘¶Ý‚µ‚È‚¢ê‡‚ÍÅ‰‚©‚çÅŒã‚Ü‚Å
	if(rangeList->size() == 0 ){
		rangeList->insert(pair<__int64, __int64>(0, fileSize-1));
		return FALSE;
	}
	return TRUE;
}

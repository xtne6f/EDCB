#include "StdAfx.h"
#include "SOAPUtil.h"

#include "../../Common/StringUtil.h"
#include "../../Common/ErrDef.h"
#include "../../UPnPCtrl/CharUtil.h"


static const char soapStart[] = "<?xml version=\"1.0\" encoding=\"utf-8\"?><s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\"><s:Body>";
static const char soapEnd[] = "</s:Body></s:Envelope>";
static const char soapActionTag[] = "<u:%s xmlns:u=\"%s\">";
static const char soapActionTagEnd[] = "</u:%s>";
static const char soapArgumentTag[] = "<%s>%s</%s>";

CSOAPUtil::CSOAPUtil(void)
{
	CoInitialize(NULL);
}


CSOAPUtil::~CSOAPUtil(void)
{
	CoUninitialize();
}

int CSOAPUtil::LoadDOMDocument(char* data, int size, MSXML2::IXMLDOMDocumentPtr xmlDoc)
{
	if( data == NULL || xmlDoc == NULL ){
		return ERR_FALSE;
	}

	IStream* pIStream = NULL;
	IPersistStreamInit* pIPStream = NULL;

	HRESULT hr = CreateStreamOnHGlobal(NULL,TRUE,&pIStream);
	if(FAILED(hr)){
		return ERR_FALSE;
	}
	hr = pIStream->Write(data,size,NULL);
	if(FAILED(hr)){
		pIStream->Release();
		pIStream = NULL;
		return ERR_FALSE;
	}
	pIStream->Commit(STGC_OVERWRITE);
	LARGE_INTEGER pos = {0, 0}; 
	pIStream->Seek(pos, STREAM_SEEK_SET, NULL);

	hr = xmlDoc->QueryInterface(IID_IPersistStreamInit, (void **)&pIPStream); 
	if(FAILED(hr)){
		pIStream->Release();
		pIStream = NULL;
		return ERR_FALSE;
	}

	hr = pIPStream->Load(pIStream);

	pIPStream->Release();
	pIPStream = NULL;
	pIStream->Release();
	pIStream = NULL;

	if(FAILED(hr)){
		return ERR_FALSE;
	}else{
		return NO_ERR;
	}
}

int CSOAPUtil::ParseSOAPRequest(char* xml, int xmlSize, SOAP_REQUEST_INFO* info)
{
	int res = NO_ERR;

	MSXML2::IXMLDOMDocumentPtr xmlDoc;
	HRESULT hr = xmlDoc.CreateInstance(MSXML2::CLSID_DOMDocument);
	if(FAILED(hr)){
		return ERR_FALSE;
	}
	xmlDoc->put_async(VARIANT_FALSE);
	xmlDoc->put_preserveWhiteSpace(VARIANT_TRUE);

	if( LoadDOMDocument(xml, xmlSize, xmlDoc) != NO_ERR ){
		xmlDoc.Release();
		xmlDoc = NULL;
		return ERR_FALSE;
	}

	//‰ðÍ
	MSXML2::IXMLDOMElementPtr pElemRoot;
	MSXML2::IXMLDOMNodePtr mediaNode = NULL;

	xmlDoc->get_documentElement(&pElemRoot);
	if( pElemRoot == NULL ){
		res = ERR_FALSE;
		goto Err_End;
	}

	for( int i=0; i<pElemRoot->childNodes->length; i++ ){
		MSXML2::IXMLDOMNodePtr bodyNode = pElemRoot->childNodes->Getitem(i);
		BSTR localName;
		bodyNode->get_baseName(&localName);
		_bstr_t bsLocalName(localName, false);
		if( (LPCWSTR)bsLocalName == NULL ){
			continue;
		}
		if(CompareNoCase(L"Body", (LPCWSTR)bsLocalName) == 0 ){
			for( int j=0; j<bodyNode->childNodes->length; j++ ){
				MSXML2::IXMLDOMNodePtr actionNode = bodyNode->childNodes->Getitem(j);
				BSTR actionName;
				actionNode->get_baseName(&actionName);
				_bstr_t bsActionName(actionName, false);
				if( (LPCWSTR)bsActionName == NULL ){
					continue;
				}

				info->actionName = (LPCWSTR)bsActionName;
				for( int k=0; k<actionNode->childNodes->length; k++){
					MSXML2::IXMLDOMNodePtr argNode = actionNode->childNodes->Getitem(k);
					BSTR argName;
					argNode->get_baseName(&argName);
					_bstr_t bsArgName(argName, false);
					if( (LPCWSTR)bsArgName == NULL ){
						continue;
					}

					BSTR argVal;
					argNode->get_text(&argVal);
					_bstr_t bsArgVal(argVal, false);

					info->argList.insert(pair<wstring,wstring>((LPCWSTR)bsArgName, (LPCWSTR)bsArgVal == NULL ? L"" : (LPCWSTR)bsArgVal));
				}
			}
			break;
		}
	}

Err_End:
	xmlDoc.Release();
	xmlDoc = NULL;

	return res;
}
/*
int CSOAPUtil::EscapeXmlString(char* src, char* dest, int* destSize)
{
	char* destTemp = NULL;
	int destTempSize = 0;
	char* srcTemp = NULL;

	srcTemp = src;
	destTempSize = strlen(srcTemp)*2+1;
	destTemp = (char*)malloc(destTempSize);
	memset(destTemp, 0, destTempSize);
	if( MP_STR_Replace(srcTemp, destTemp, &destTempSize, "&", "&amp;") == 2 ){
		free(destTemp);
		destTemp = (char*)malloc(destTempSize);
		MP_STR_Replace(srcTemp, destTemp, &destTempSize, "&", "&amp;");
	}

	srcTemp = destTemp;
	destTempSize = strlen(srcTemp)*2+1;
	destTemp = (char*)malloc(destTempSize);
	memset(destTemp, 0, destTempSize);
	if( MP_STR_Replace(srcTemp, destTemp, &destTempSize, "<", "&lt;") == 2 ){
		free(destTemp);
		destTemp = (char*)malloc(destTempSize);
		MP_STR_Replace(srcTemp, destTemp, &destTempSize, "<", "&lt;");
	}

	free(srcTemp);
	srcTemp = destTemp;
	destTempSize = strlen(srcTemp)*2+1;
	destTemp = (char*)malloc(destTempSize);
	memset(destTemp, 0, destTempSize);
	if( MP_STR_Replace(srcTemp, destTemp, &destTempSize, ">", "&gt;") == 2 ){
		free(destTemp);
		destTemp = (char*)malloc(destTempSize);
		MP_STR_Replace(srcTemp, destTemp, &destTempSize, ">", "&gt;");
	}

	free(srcTemp);
	srcTemp = destTemp;
	destTempSize = strlen(srcTemp)*2+1;
	destTemp = (char*)malloc(destTempSize);
	memset(destTemp, 0, destTempSize);
	if( MP_STR_Replace(srcTemp, destTemp, &destTempSize, "\"", "&quot;") == 2 ){
		free(destTemp);
		destTemp = (char*)malloc(destTempSize);
		MP_STR_Replace(srcTemp, destTemp, &destTempSize, "\"", "&quot;");
	}

	free(srcTemp);
	srcTemp = destTemp;
	destTempSize = strlen(srcTemp)*2+1;
	destTemp = (char*)malloc(destTempSize);
	memset(destTemp, 0, destTempSize);
	if( MP_STR_Replace(srcTemp, destTemp, &destTempSize, "'", "&apos;") == 2 ){
		free(destTemp);
		destTemp = (char*)malloc(destTempSize);
		MP_STR_Replace(srcTemp, destTemp, &destTempSize, "'", "&apos;");
	}

	if( *destSize < strlen(destTemp)+1){
		*destSize = strlen(destTemp)+1;
		return 2;
	}

	strcpy_s(dest, *destSize, destTemp);
	*destSize = strlen(destTemp)+1;
	
	return 1;
}*/
/*
int CSOAPUtil::EscapeXmlString(wstring& src)
{
	Replace(src, L"&", L"&amp;");
	Replace(src, L"<", L"&lt;");
	Replace(src, L">", L"&gt;");
	Replace(src, L"\"", L"&quot;");
	Replace(src, L"'", L"&apos;");

	return 1;
}
*/
int CSOAPUtil::CreateSOAPResponseBody(char* actionXmlns, char* actionName, vector<SOAP_RESPONSE_ARG>* argumentList, char* xml, int* xmlSize)
{
	string createXml = "";
	string buff = "";
	wstring buffW = L"";
	string buffUTF8 = "";

	createXml = soapStart;

	Format(buff, soapActionTag, actionName, actionXmlns);
	createXml+=buff;

	for( size_t i=0; i<argumentList->size(); i++ ){
		AtoW((*argumentList)[i].val, buffW);
		EscapeXmlString(buffW);
		WtoUTF8(buffW, buffUTF8);
		Format(buff, soapArgumentTag, (*argumentList)[i].name.c_str(), buffUTF8.c_str(), (*argumentList)[i].name.c_str());
		createXml+=buff;
	}

	Format(buff, soapActionTagEnd, actionName);
	createXml+=buff;

	createXml += soapEnd;

	if( *xmlSize < (int)createXml.size() ){
		*xmlSize = (int)createXml.size()+1;
		return 2;
	}

	if( xml != NULL ){
		strcpy_s(xml, *xmlSize, createXml.c_str());
	}
	*xmlSize = (int)createXml.size()+1;

	return 1;
}


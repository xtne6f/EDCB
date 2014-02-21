#pragma once

#include "../../Common/CommonDef.h"

#import <msxml3.dll> named_guids

typedef struct _SOAP_REQUEST_INFO{
	wstring actionName;
	map<wstring, wstring> argList;
}SOAP_REQUEST_INFO;

typedef struct _SOAP_RESPONSE_ARG{
	string name;
	string val;
}SOAP_RESPONSE_ARG;

class CSOAPUtil
{
public:
	CSOAPUtil(void);
	~CSOAPUtil(void);

	int ParseSOAPRequest(char* xml, int xmlSize, SOAP_REQUEST_INFO* info);

	int CreateSOAPResponseBody(char* actionXmlns, char* actionName, vector<SOAP_RESPONSE_ARG>* argumentList, char* xml, int* xmlSize);
protected:
	int LoadDOMDocument(char* data, int size, MSXML2::IXMLDOMDocumentPtr xmlDoc);
	//int EscapeXmlString(char* src, char* dest, int* destSize);
	//int EscapeXmlString(wstring& src);
};


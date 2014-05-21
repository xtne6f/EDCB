#include "StdAfx.h"
#include "RestApiManager.h"


CRestApiManager::CRestApiManager(void)
{
}


CRestApiManager::~CRestApiManager(void)
{
}

void CRestApiManager::CheckXMLChar(wstring& text)
{
	if( text.find(L"&") != string::npos ){
		Replace(text, L"&", L"&amp;");
	}
	if( text.find(L"<") != string::npos ){
		Replace(text, L"<", L"&lt;");
	}
	if( text.find(L">") != string::npos ){
		Replace(text, L">", L"&gt;");
	}
}

DWORD CRestApiManager::AnalyzeCmd(string verb, string url, string param, HTTP_STREAM* sendParam, CEpgDBManager* epgDB, vector<RESERVE_DATA*>* list, CReserveManager* resMng)
{
	DWORD ret = ERR_FALSE;
	string urlParam = "";
	if( url.find("?") != string::npos ){
		Separate(url, "?", url, urlParam);
	}
	if( urlParam.size() > 0 && param.size() > 0){
		urlParam += "&";
		urlParam += param;
	}else if( urlParam.size() == 0 && param.size() > 0){
		urlParam = param;
	}

	OutputDebugStringA(urlParam.c_str());

	if(url.find("/api/EnumService") == 0 ){
		ret = GetEnumService(urlParam, sendParam, epgDB);
	}else
	if(url.find("/api/EnumEventInfo") == 0 ){
		ret = GetEnumEventInfo(urlParam, sendParam, epgDB);
	}else
	if(url.find("/api/EnumReserveInfo") == 0 ){
		ret = GetEnumReserveInfo(urlParam, sendParam, list);
	}else
	if(url.find("/api/EnumRecPreset") == 0 ){
		ret = GetEnumRecPresetInfo(urlParam, sendParam);
	}else
	if(url.find("/api/AddReserveEPG") == 0 ){
		if(verb.find("POST") == 0 ){
			RESERVE_DATA addRes;
			ret = GetReserveDataEPG(urlParam, epgDB, &addRes);
			if( ret == TRUE ){
				vector<RESERVE_DATA> addList;
				addList.push_back(addRes);
				ret = resMng->AddReserveData(&addList);
			}
			GetReserveAddResponse(ret, sendParam);
		}
	}else
	if(url.find("/api/SearchEvent") == 0 ){
		ret = GetSearchEvent(urlParam, sendParam, epgDB);
	}

	return ret;
}

void CRestApiManager::LoadRecSetData(WORD preset, REC_SETTING_DATA* recSetData)
{
	wstring iniPath = L"";
	GetModuleIniPath(iniPath);
	WCHAR iniBuff[512]=L"";

	wstring defName = L"";
	wstring defFolderName = L"";
	wstring defFolder1SegName = L"";
	if( preset == 0 ){
		defName = L"REC_DEF";
		defFolderName = L"REC_DEF_FOLDER";
		defFolder1SegName = L"REC_DEF_FOLDER_1SEG";
	}else{
		Format(defName, L"REC_DEF%d", preset);
		Format(defFolderName, L"REC_DEF_FOLDER%d", preset);
		Format(defFolder1SegName, L"REC_DEF_FOLDER_1SEG%d", preset);
	}

	recSetData->recMode = (BYTE)GetPrivateProfileInt(defName.c_str(), L"RecMode", 1, iniPath.c_str());
    recSetData->priority = (BYTE)GetPrivateProfileInt(defName.c_str(), L"Priority", 2, iniPath.c_str());
    recSetData->tuijyuuFlag = (BYTE)GetPrivateProfileInt(defName.c_str(), L"TuijyuuFlag", 1, iniPath.c_str());
    recSetData->serviceMode = (BYTE)GetPrivateProfileInt(defName.c_str(), L"ServiceMode", 0, iniPath.c_str());
    recSetData->pittariFlag = (BYTE)GetPrivateProfileInt(defName.c_str(), L"PittariFlag", 0, iniPath.c_str());

	ZeroMemory(iniBuff, sizeof(WCHAR)*512);
    GetPrivateProfileString(defName.c_str(), L"BatFilePath", L"", iniBuff, 512, iniPath.c_str());
    recSetData->batFilePath = iniBuff;

    int count = GetPrivateProfileInt(defFolderName.c_str(), L"Count", 0, iniPath.c_str());
    for (int i = 0; i < count; i++)
    {
		wstring fn;
		wstring wn;
		wstring rn;
		Format(fn, L"%d", i);
		Format(wn, L"WritePlugIn%d", i);
		Format(rn, L"RecNamePlugIn%d", i);
        REC_FILE_SET_INFO folderInfo;
        ZeroMemory(iniBuff, sizeof(WCHAR)*512);
		GetPrivateProfileString(defFolderName.c_str(), fn.c_str(), L"", iniBuff, 512, iniPath.c_str());
        folderInfo.recFolder = iniBuff;
        ZeroMemory(iniBuff, sizeof(WCHAR)*512);
		GetPrivateProfileString(defFolderName.c_str(), wn.c_str(), L"Write_Default.dll", iniBuff, 512, iniPath.c_str());
        folderInfo.writePlugIn = iniBuff;
        ZeroMemory(iniBuff, sizeof(WCHAR)*512);
		GetPrivateProfileString(defFolderName.c_str(), rn.c_str(), L"", iniBuff, 512, iniPath.c_str());
        folderInfo.recNamePlugIn = iniBuff;

        recSetData->recFolderList.push_back(folderInfo);
    }

    count = GetPrivateProfileInt(defFolder1SegName.c_str(), L"Count", 0, iniPath.c_str());
    for (int i = 0; i < count; i++)
    {
		wstring fn;
		wstring wn;
		wstring rn;
		Format(fn, L"%d", i);
		Format(wn, L"WritePlugIn%d", i);
		Format(rn, L"RecNamePlugIn%d", i);
        REC_FILE_SET_INFO folderInfo;
        ZeroMemory(iniBuff, sizeof(WCHAR)*512);
        GetPrivateProfileString(defFolder1SegName.c_str(), fn.c_str(), L"", iniBuff, 512, iniPath.c_str());
        folderInfo.recFolder = iniBuff;
        ZeroMemory(iniBuff, sizeof(WCHAR)*512);
        GetPrivateProfileString(defFolder1SegName.c_str(), wn.c_str(), L"Write_Default.dll", iniBuff, 512, iniPath.c_str());
        folderInfo.writePlugIn = iniBuff;
        ZeroMemory(iniBuff, sizeof(WCHAR)*512);
        GetPrivateProfileString(defFolder1SegName.c_str(), rn.c_str(), L"", iniBuff, 512, iniPath.c_str());
        folderInfo.recNamePlugIn = iniBuff;

        recSetData->partialRecFolder.push_back(folderInfo);
    }

    recSetData->suspendMode = (BYTE)GetPrivateProfileInt(defName.c_str(), L"SuspendMode", 0, iniPath.c_str());
    recSetData->rebootFlag = (BYTE)GetPrivateProfileInt(defName.c_str(), L"RebootFlag", 0, iniPath.c_str());
    recSetData->useMargineFlag = (BYTE)GetPrivateProfileInt(defName.c_str(), L"UseMargineFlag", 0, iniPath.c_str());
    recSetData->startMargine = GetPrivateProfileInt(defName.c_str(), L"StartMargine", 0, iniPath.c_str());
    recSetData->endMargine = GetPrivateProfileInt(defName.c_str(), L"EndMargine", 0, iniPath.c_str());
    recSetData->continueRecFlag = (BYTE)GetPrivateProfileInt(defName.c_str(), L"ContinueRec", 0, iniPath.c_str());
    recSetData->partialRecFlag = (BYTE)GetPrivateProfileInt(defName.c_str(), L"PartialRec", 0, iniPath.c_str());
    recSetData->tunerID = (DWORD)GetPrivateProfileInt(defName.c_str(), L"TunerID", 0, iniPath.c_str());

}

DWORD CRestApiManager::GetEnumService(string param, HTTP_STREAM* sendParam, CEpgDBManager* epgDB)
{
	DWORD ret = NO_ERR;

	vector<EPGDB_SERVICE_INFO> list;
	wstring xml = L"";
	string utf8 = "";
	if( epgDB->GetServiceList(&list) == TRUE ){
		wstring buff = L"";
		xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\" ?><entry>";
		Format(buff, L"<total>%d</total><index>0</index><count>%d</count>", list.size(), list.size());
		xml += buff;
		xml += L"<items>";
		wstring serviceinfo = L"";
		for( size_t i=0; i<list.size(); i++ ){
			serviceinfo += L"<serviceinfo>";
			Format(buff, L"<ONID>%d</ONID>", list[i].ONID);
			serviceinfo += buff;
			Format(buff, L"<TSID>%d</TSID>", list[i].TSID);
			serviceinfo += buff;
			Format(buff, L"<SID>%d</SID>", list[i].SID);
			serviceinfo += buff;
			Format(buff, L"<service_type>%d</service_type>", list[i].service_type);
			serviceinfo += buff;
			Format(buff, L"<partialReceptionFlag>%d</partialReceptionFlag>", list[i].partialReceptionFlag);
			serviceinfo += buff;
			Format(buff, L"<service_provider_name>%s</service_provider_name>", list[i].service_provider_name.c_str());
			serviceinfo += buff;
			Format(buff, L"<service_name>%s</service_name>", list[i].service_name.c_str());
			serviceinfo += buff;
			Format(buff, L"<network_name>%s</network_name>", list[i].network_name.c_str());
			serviceinfo += buff;
			Format(buff, L"<ts_name>%s</ts_name>", list[i].ts_name.c_str());
			serviceinfo += buff;
			Format(buff, L"<remote_control_key_id>%d</remote_control_key_id>", list[i].remote_control_key_id);
			serviceinfo += buff;
			serviceinfo += L"</serviceinfo>";
		}
		xml += serviceinfo;
		xml += L"</items>";
		xml += L"</entry>";
	}else{
		xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\" ?><entry>";
		xml += L"<err>EPGデータを読み込み中、または存在しません</err>";
		xml += L"</entry>";
	}

	WtoUTF8(xml, utf8);

	sendParam->dataSize = (DWORD)utf8.size();
	sendParam->data = new BYTE[sendParam->dataSize];
	memcpy(sendParam->data, utf8.c_str(), sendParam->dataSize);
	if( sendParam->dataSize > 0 ){
		Format(sendParam->httpHeader, "HTTP/1.0 200 OK\r\nContent-Type: text/xml\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", sendParam->dataSize);
	}else{
		sendParam->httpHeader = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n\r\n";
	}

	return ret;
}

void CRestApiManager::EnumEventInfoCallback(vector<EPGDB_SERVICE_EVENT_INFO>* pval, void* param)
{
	void** p = (void**)param;
	p[0] = (void*)((CRestApiManager*)p[1])->GetEnumEventInfo(*(string*)p[2], (HTTP_STREAM*)p[3], (CEpgDBManager*)p[4], pval);
}

DWORD CRestApiManager::GetEnumEventInfo(string param, HTTP_STREAM* sendParam, CEpgDBManager* epgDB)
{
	void* p[] = {0, this, &param, sendParam, epgDB};
	if( epgDB->EnumEventAll(EnumEventInfoCallback, p) == FALSE ){
		return GetEnumEventInfo(param, sendParam, epgDB, NULL);
	}
	return (DWORD)p[0];
}

DWORD CRestApiManager::GetEnumEventInfo(string param, HTTP_STREAM* sendParam, CEpgDBManager* epgDB, vector<EPGDB_SERVICE_EVENT_INFO>* list)
{
	DWORD ret = NO_ERR;

	map<string,string> paramMap;
	while(param.size()>0){
		string buff;
		Separate(param, "&", buff, param);
		if(buff.size()>0){
			string key;
			string val;
			Separate(buff, "=", key, val);
			paramMap.insert(pair<string,string>(key, val));
		}
	}

	map<string,string>::iterator itr;
	WORD ONID = 0xFFFF;
	itr = paramMap.find("ONID");
	if( itr != paramMap.end() ){
		ONID = (WORD)atoi(itr->second.c_str());
	}
	WORD TSID = 0xFFFF;
	itr = paramMap.find("TSID");
	if( itr != paramMap.end() ){
		TSID = (WORD)atoi(itr->second.c_str());
	}
	WORD SID = 0xFFFF;
	itr = paramMap.find("SID");
	if( itr != paramMap.end() ){
		SID = (WORD)atoi(itr->second.c_str());
	}
	WORD basicOnly = 1;
	itr = paramMap.find("basic");
	if( itr != paramMap.end() ){
		basicOnly = (WORD)atoi(itr->second.c_str());
	}
	DWORD index = 0;
	itr = paramMap.find("index");
	if( itr != paramMap.end() ){
		index = (DWORD)atoi(itr->second.c_str());
	}
	DWORD count = 200;
	itr = paramMap.find("count");
	if( itr != paramMap.end() ){
		count = (DWORD)atoi(itr->second.c_str());
	}

	wstring xml = L"";
	string utf8 = "";
	if( list != NULL ){
		wstring buff = L"";
		DWORD total = 0;
		DWORD findCount = 0;

		xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\" ?><entry>";

		wstring serviceinfo = L"";
		serviceinfo += L"<items>";
		for( size_t i=0; i<list->size(); i++ ){
			for( size_t j=0; j<(*list)[i].eventList.size(); j++){
				EPGDB_EVENT_INFO* eventInfo = (*list)[i].eventList[j];
				if( eventInfo->original_network_id != ONID && ONID != 0xFFFF ){
					continue;
				}
				if( eventInfo->transport_stream_id != TSID && TSID != 0xFFFF ){
					continue;
				}
				if( eventInfo->service_id != SID && SID != 0xFFFF ){
					continue;
				}
				if( total < index ){
					total++;
					continue;
				}
				if( index + count <= total ){
					total++;
					continue;
				}
				total++;
				findCount++;

				serviceinfo += L"<eventinfo>";
				Format(buff, L"<ONID>%d</ONID>", eventInfo->original_network_id);
				serviceinfo += buff;
				Format(buff, L"<TSID>%d</TSID>", eventInfo->transport_stream_id);
				serviceinfo += buff;
				Format(buff, L"<SID>%d</SID>", eventInfo->service_id);
				serviceinfo += buff;
				Format(buff, L"<eventID>%d</eventID>", eventInfo->event_id);
				serviceinfo += buff;
				if( eventInfo->StartTimeFlag == 1 ){
					Format(buff, L"<startDate>%d/%d/%d</startDate>", eventInfo->start_time.wYear, eventInfo->start_time.wMonth, eventInfo->start_time.wDay);
					serviceinfo += buff;
					Format(buff, L"<startTime>%d:%d:%d</startTime>", eventInfo->start_time.wHour, eventInfo->start_time.wMinute, eventInfo->start_time.wSecond);
					serviceinfo += buff;
					Format(buff, L"<startDayOfWeek>%d</startDayOfWeek>", eventInfo->start_time.wDayOfWeek);
					serviceinfo += buff;
				}
				if( eventInfo->DurationFlag == 1 ){
					Format(buff, L"<duration>%d</duration>", eventInfo->durationSec);
					serviceinfo += buff;
				}
				if( eventInfo->shortInfo != NULL ){
					wstring chk = eventInfo->shortInfo->event_name;
					CheckXMLChar(chk);
					Format(buff, L"<event_name>%s</event_name>", chk.c_str());
					serviceinfo += buff;

					chk = eventInfo->shortInfo->text_char;
					CheckXMLChar(chk);
					Format(buff, L"<event_text>%s</event_text>", chk.c_str());
					serviceinfo += buff;
				}
				if( eventInfo->contentInfo != NULL ){
					serviceinfo += L"";
					for( size_t k=0; k<eventInfo->contentInfo->nibbleList.size(); k++){
						wstring nibble = L"";
						Format(nibble,L"<contentInfo><nibble1>%d</nibble1><nibble2>%d</nibble2></contentInfo>", 
							eventInfo->contentInfo->nibbleList[k].content_nibble_level_1,
							eventInfo->contentInfo->nibbleList[k].content_nibble_level_2);
						serviceinfo += nibble;
					}
				}
				if( eventInfo->eventGroupInfo != NULL ){
					for( size_t k=0; k<eventInfo->eventGroupInfo->eventDataList.size(); k++){
						wstring group = L"";
						Format(group,L"<groupInfo><ONID>%d</ONID><TSID>%d</TSID><SID>%d</SID><eventID>%d</eventID></groupInfo>", 
							eventInfo->eventGroupInfo->eventDataList[k].original_network_id,
							eventInfo->eventGroupInfo->eventDataList[k].transport_stream_id,
							eventInfo->eventGroupInfo->eventDataList[k].service_id,
							eventInfo->eventGroupInfo->eventDataList[k].event_id
							);
						serviceinfo += group;
					}
				}

				Format(buff, L"<freeCAFlag>%d</freeCAFlag>", eventInfo->freeCAFlag);
				serviceinfo += buff;

				if( basicOnly == 0 ){
					if( eventInfo->extInfo != NULL ){
						wstring chk = eventInfo->extInfo->text_char;
						CheckXMLChar(chk);
						Format(buff, L"<event_ext_text>%s</event_ext_text>", chk.c_str());
						serviceinfo += buff;
					}
					if( eventInfo->componentInfo != NULL ){
						Format(buff, L"<videoInfo><stream_content>%d</stream_content><component_type>%d</component_type><component_tag>%d</component_tag><text>%s</text></videoInfo>", 
							eventInfo->componentInfo->stream_content,
							eventInfo->componentInfo->component_type,
							eventInfo->componentInfo->component_tag,
							eventInfo->componentInfo->text_char.c_str()
							);
						serviceinfo += buff;
					}
					if( eventInfo->audioInfo != NULL ){
						for( size_t k=0; k<eventInfo->audioInfo->componentList.size(); k++ ){
							Format(buff, L"<audioInfo><stream_content>%d</stream_content><component_type>%d</component_type><component_tag>%d</component_tag><stream_type>%d</stream_type><simulcast_group_tag>%d</simulcast_group_tag><ES_multi_lingual_flag>%d</ES_multi_lingual_flag><main_component_flag>%d</main_component_flag><quality_indicator>%d</quality_indicator><sampling_rate>%d</sampling_rate><text>%s</text></audioInfo>", 
								eventInfo->audioInfo->componentList[k].stream_content,
								eventInfo->audioInfo->componentList[k].component_type,
								eventInfo->audioInfo->componentList[k].component_tag,
								eventInfo->audioInfo->componentList[k].stream_type,
								eventInfo->audioInfo->componentList[k].simulcast_group_tag,
								eventInfo->audioInfo->componentList[k].ES_multi_lingual_flag,
								eventInfo->audioInfo->componentList[k].main_component_flag,
								eventInfo->audioInfo->componentList[k].quality_indicator,
								eventInfo->audioInfo->componentList[k].sampling_rate,
								eventInfo->audioInfo->componentList[k].text_char.c_str()
								);
							serviceinfo += buff;
						}
					}
					if( eventInfo->eventRelayInfo != NULL ){
						for( size_t k=0; k<eventInfo->eventRelayInfo->eventDataList.size(); k++){
							wstring group = L"";
							Format(group,L"<relayInfo><ONID>%d</ONID><TSID>%d</TSID><SID>%d</SID><eventID>%d</eventID></relayInfo>", 
								eventInfo->eventRelayInfo->eventDataList[k].original_network_id,
								eventInfo->eventRelayInfo->eventDataList[k].transport_stream_id,
								eventInfo->eventRelayInfo->eventDataList[k].service_id,
								eventInfo->eventRelayInfo->eventDataList[k].event_id
								);
							serviceinfo += group;
						}
					}
				}
				serviceinfo += L"</eventinfo>";

			}
		}
		serviceinfo += L"</items>";

		Format(buff, L"<total>%d</total><index>%d</index><count>%d</count>", total, index, findCount);
		xml += buff;
		xml += serviceinfo;
		xml += L"</entry>";
	}else{
		xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\" ?><entry>";
		xml += L"<err>EPGデータを読み込み中、または存在しません</err>";
		xml += L"</entry>";
	}

	WtoUTF8(xml, utf8);

	sendParam->dataSize = (DWORD)utf8.size();
	sendParam->data = new BYTE[sendParam->dataSize];
	memcpy(sendParam->data, utf8.c_str(), sendParam->dataSize);
	if( sendParam->dataSize > 0 ){
		Format(sendParam->httpHeader, "HTTP/1.0 200 OK\r\nContent-Type: text/xml\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", sendParam->dataSize);
	}else{
		sendParam->httpHeader = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n\r\n";
	}

	return ret;
}

DWORD CRestApiManager::GetEnumReserveInfo(string param, HTTP_STREAM* sendParam, vector<RESERVE_DATA*>* list)
{
	DWORD ret = NO_ERR;

	wstring xml = L"";
	string utf8 = "";

	wstring buff = L"";
	xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\" ?><entry>";
	Format(buff, L"<total>%d</total><index>0</index><count>%d</count>", list->size(), list->size());
	xml += buff;
	xml += L"<items>";
	wstring reserveinfo = L"";
	for( size_t i=0; i<list->size(); i++ ){
		reserveinfo += L"<reserveinfo>";
		Format(buff, L"<ID>%d</ID>", (*list)[i]->reserveID);
		reserveinfo += buff;
		Format(buff, L"<title>%s</title>", (*list)[i]->title.c_str());
		reserveinfo += buff;
		Format(buff, L"<startDate>%d/%d/%d</startDate>", (*list)[i]->startTime.wYear, (*list)[i]->startTime.wMonth, (*list)[i]->startTime.wDay);
		reserveinfo += buff;
		Format(buff, L"<startTime>%d:%d:%d</startTime>", (*list)[i]->startTime.wHour, (*list)[i]->startTime.wMinute, (*list)[i]->startTime.wSecond);
		reserveinfo += buff;
		Format(buff, L"<startDayOfWeek>%d</startDayOfWeek>", (*list)[i]->startTime.wDayOfWeek);
		reserveinfo += buff;
		Format(buff, L"<duration>%d</duration>", (*list)[i]->durationSecond);
		reserveinfo += buff;
		Format(buff, L"<service_name>%s</service_name>", (*list)[i]->stationName.c_str());
		reserveinfo += buff;
		Format(buff, L"<ONID>%d</ONID>", (*list)[i]->originalNetworkID);
		reserveinfo += buff;
		Format(buff, L"<TSID>%d</TSID>", (*list)[i]->transportStreamID);
		reserveinfo += buff;
		Format(buff, L"<SID>%d</SID>", (*list)[i]->serviceID);
		reserveinfo += buff;
		Format(buff, L"<eventID>%d</eventID>", (*list)[i]->eventID);
		reserveinfo += buff;
		Format(buff, L"<comment>%s</comment>", (*list)[i]->comment.c_str());
		reserveinfo += buff;
		Format(buff, L"<overlapMode>%d</overlapMode>", (*list)[i]->overlapMode);
		reserveinfo += buff;

		reserveinfo += L"<recsetting>";
		Format(buff, L"<recMode>%d</recMode>", (*list)[i]->recSetting.recMode);
		reserveinfo += buff;
		Format(buff, L"<priority>%d</priority>", (*list)[i]->recSetting.priority);
		reserveinfo += buff;
		Format(buff, L"<tuijyuuFlag>%d</tuijyuuFlag>", (*list)[i]->recSetting.tuijyuuFlag);
		reserveinfo += buff;
		Format(buff, L"<serviceMode>%d</serviceMode>", (*list)[i]->recSetting.serviceMode);
		reserveinfo += buff;
		Format(buff, L"<pittariFlag>%d</pittariFlag>", (*list)[i]->recSetting.pittariFlag);
		reserveinfo += buff;
		Format(buff, L"<batFilePath>%s</batFilePath>", (*list)[i]->recSetting.batFilePath.c_str());
		reserveinfo += buff;
		reserveinfo += L"<recFolderList>";
		for( size_t j=0; j<(*list)[i]->recSetting.recFolderList.size(); j++ ){
			reserveinfo += L"<recFolderInfo>";
			Format(buff, L"<recFolder>%s</recFolder>", (*list)[i]->recSetting.recFolderList[j].recFolder.c_str());
			reserveinfo += buff;
			Format(buff, L"<writePlugIn>%s</writePlugIn>", (*list)[i]->recSetting.recFolderList[j].writePlugIn.c_str());
			reserveinfo += buff;
			Format(buff, L"<recNamePlugIn>%s</recNamePlugIn>", (*list)[i]->recSetting.recFolderList[j].recNamePlugIn.c_str());
			reserveinfo += buff;
			reserveinfo += L"</recFolderInfo>";
		}
		reserveinfo += L"</recFolderList>";
		Format(buff, L"<suspendMode>%d</suspendMode>", (*list)[i]->recSetting.suspendMode);
		reserveinfo += buff;
		Format(buff, L"<rebootFlag>%d</rebootFlag>", (*list)[i]->recSetting.rebootFlag);
		reserveinfo += buff;
		Format(buff, L"<useMargineFlag>%d</useMargineFlag>", (*list)[i]->recSetting.useMargineFlag);
		reserveinfo += buff;
		Format(buff, L"<startMargine>%d</startMargine>", (*list)[i]->recSetting.startMargine);
		reserveinfo += buff;
		Format(buff, L"<endMargine>%d</endMargine>", (*list)[i]->recSetting.endMargine);
		reserveinfo += buff;
		Format(buff, L"<continueRecFlag>%d</continueRecFlag>", (*list)[i]->recSetting.continueRecFlag);
		reserveinfo += buff;
		Format(buff, L"<partialRecFlag>%d</partialRecFlag>", (*list)[i]->recSetting.partialRecFlag);
		reserveinfo += buff;
		Format(buff, L"<tunerID>%d</tunerID>", (*list)[i]->recSetting.tunerID);
		reserveinfo += buff;
		reserveinfo += L"<partialRecFolder>";
		for( size_t j=0; j<(*list)[i]->recSetting.partialRecFolder.size(); j++ ){
			reserveinfo += L"<recFolderInfo>";
			Format(buff, L"<recFolder>%s</recFolder>", (*list)[i]->recSetting.partialRecFolder[j].recFolder.c_str());
			reserveinfo += buff;
			Format(buff, L"<writePlugIn>%s</writePlugIn>", (*list)[i]->recSetting.partialRecFolder[j].writePlugIn.c_str());
			reserveinfo += buff;
			Format(buff, L"<recNamePlugIn>%s</recNamePlugIn>", (*list)[i]->recSetting.partialRecFolder[j].recNamePlugIn.c_str());
			reserveinfo += buff;
			reserveinfo += L"</recFolderInfo>";
		}
		reserveinfo += L"</partialRecFolder>";
		reserveinfo += L"</recsetting>";

		reserveinfo += L"<recFileNameList>";
		for( size_t j=0; j<(*list)[i]->recFileNameList.size(); j++ ){
			Format(buff, L"<name>%s</name>", (*list)[i]->recFileNameList[j].c_str());
			reserveinfo += buff;
		}
		reserveinfo += L"</recFileNameList>";

		reserveinfo += L"</reserveinfo>";
	}
	xml += reserveinfo;
	xml += L"</items>";
	xml += L"</entry>";

	WtoUTF8(xml, utf8);

	sendParam->dataSize = (DWORD)utf8.size();
	sendParam->data = new BYTE[sendParam->dataSize];
	memcpy(sendParam->data, utf8.c_str(), sendParam->dataSize);
	if( sendParam->dataSize > 0 ){
		Format(sendParam->httpHeader, "HTTP/1.0 200 OK\r\nContent-Type: text/xml\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", sendParam->dataSize);
	}else{
		sendParam->httpHeader = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n\r\n";
	}

	return ret;
}

DWORD CRestApiManager::GetEnumRecPresetInfo(string param, HTTP_STREAM* sendParam)
{
	DWORD ret = NO_ERR;

	wstring iniPath = L"";
	GetModuleIniPath(iniPath);
	WCHAR iniBuff[512]=L"";
	GetPrivateProfileString(L"SET", L"PresetID", L"", iniBuff, 512, iniPath.c_str());
	wstring parseBuff = iniBuff;
	vector<DWORD> idList;
	idList.push_back(0);
	do{
		wstring presetID =L"";
		Separate(parseBuff, L",", presetID, parseBuff);
		idList.push_back((DWORD)_wtoi(presetID.c_str()));
	}while(parseBuff.size()>0);


	wstring xml = L"";
	string utf8 = "";

	wstring buff = L"";
	xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\" ?><entry>";
	Format(buff, L"<total>%d</total><index>0</index><count>%d</count>", idList.size(), idList.size());
	xml += buff;
	xml += L"<items>";
	wstring reserveinfo = L"";
	for( size_t i=0; i<idList.size(); i++ ){
		REC_SETTING_DATA recSetting;
		LoadRecSetData((WORD)idList[i], &recSetting);

		reserveinfo += L"<recpresetinfo>";

		if( idList[i] == 0 ){
			Format(buff, L"<id>%d</id>", idList[i]);
			reserveinfo += buff;
			reserveinfo += L"<name>デフォルト</name>";
		}else{
			wstring appName = L"";
			Format(appName, L"REC_DEF%d", idList[i]);
			ZeroMemory(iniBuff, sizeof(WCHAR)*512);
			GetPrivateProfileString(appName.c_str(), L"SetName", L"", iniBuff, 512, iniPath.c_str());

			Format(buff, L"<id>%d</id>", idList[i]);
			reserveinfo += buff;
			Format(buff, L"<name>%s</name>", iniBuff);
			reserveinfo += buff;
		}
		reserveinfo += L"<recsetting>";
		Format(buff, L"<recMode>%d</recMode>", recSetting.recMode);
		reserveinfo += buff;
		Format(buff, L"<priority>%d</priority>", recSetting.priority);
		reserveinfo += buff;
		Format(buff, L"<tuijyuuFlag>%d</tuijyuuFlag>", recSetting.tuijyuuFlag);
		reserveinfo += buff;
		Format(buff, L"<serviceMode>%d</serviceMode>", recSetting.serviceMode);
		reserveinfo += buff;
		Format(buff, L"<pittariFlag>%d</pittariFlag>", recSetting.pittariFlag);
		reserveinfo += buff;
		Format(buff, L"<batFilePath>%s</batFilePath>", recSetting.batFilePath.c_str());
		reserveinfo += buff;
		reserveinfo += L"<recFolderList>";
		for( size_t j=0; j<recSetting.recFolderList.size(); j++ ){
			reserveinfo += L"<recFolderInfo>";
			Format(buff, L"<recFolder>%s</recFolder>", recSetting.recFolderList[j].recFolder.c_str());
			reserveinfo += buff;
			Format(buff, L"<writePlugIn>%s</writePlugIn>", recSetting.recFolderList[j].writePlugIn.c_str());
			reserveinfo += buff;
			Format(buff, L"<recNamePlugIn>%s</recNamePlugIn>", recSetting.recFolderList[j].recNamePlugIn.c_str());
			reserveinfo += buff;
			reserveinfo += L"</recFolderInfo>";
		}
		reserveinfo += L"</recFolderList>";
		Format(buff, L"<suspendMode>%d</suspendMode>", recSetting.suspendMode);
		reserveinfo += buff;
		Format(buff, L"<rebootFlag>%d</rebootFlag>", recSetting.rebootFlag);
		reserveinfo += buff;
		Format(buff, L"<useMargineFlag>%d</useMargineFlag>", recSetting.useMargineFlag);
		reserveinfo += buff;
		Format(buff, L"<startMargine>%d</startMargine>", recSetting.startMargine);
		reserveinfo += buff;
		Format(buff, L"<endMargine>%d</endMargine>", recSetting.endMargine);
		reserveinfo += buff;
		Format(buff, L"<continueRecFlag>%d</continueRecFlag>", recSetting.continueRecFlag);
		reserveinfo += buff;
		Format(buff, L"<partialRecFlag>%d</partialRecFlag>", recSetting.partialRecFlag);
		reserveinfo += buff;
		Format(buff, L"<tunerID>%d</tunerID>", recSetting.tunerID);
		reserveinfo += buff;
		reserveinfo += L"<partialRecFolder>";
		for( size_t j=0; j<recSetting.partialRecFolder.size(); j++ ){
			reserveinfo += L"<recFolderInfo>";
			Format(buff, L"<recFolder>%s</recFolder>", recSetting.partialRecFolder[j].recFolder.c_str());
			reserveinfo += buff;
			Format(buff, L"<writePlugIn>%s</writePlugIn>", recSetting.partialRecFolder[j].writePlugIn.c_str());
			reserveinfo += buff;
			Format(buff, L"<recNamePlugIn>%s</recNamePlugIn>", recSetting.partialRecFolder[j].recNamePlugIn.c_str());
			reserveinfo += buff;
			reserveinfo += L"</recFolderInfo>";
		}
		reserveinfo += L"</partialRecFolder>";
		reserveinfo += L"</recsetting>";

		reserveinfo += L"</recpresetinfo>";
	}
	xml += reserveinfo;
	xml += L"</items>";
	xml += L"</entry>";

	WtoUTF8(xml, utf8);

	sendParam->dataSize = (DWORD)utf8.size();
	sendParam->data = new BYTE[sendParam->dataSize];
	memcpy(sendParam->data, utf8.c_str(), sendParam->dataSize);
	if( sendParam->dataSize > 0 ){
		Format(sendParam->httpHeader, "HTTP/1.0 200 OK\r\nContent-Type: text/xml\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", sendParam->dataSize);
	}else{
		sendParam->httpHeader = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n\r\n";
	}

	return ret;
}

DWORD CRestApiManager::GetReserveDataEPG(string param, CEpgDBManager* epgDB, RESERVE_DATA* reserveData)
{
	DWORD ret = NO_ERR;

	if( epgDB == NULL){
		return FALSE;
	}
	OutputDebugStringA(param.c_str());
	map<string,string> paramMap;
	while(param.size()>0){
		string buff;
		Separate(param, "&", buff, param);
		if(buff.size()>0){
			string key;
			string val;
			Separate(buff, "=", key, val);
			paramMap.insert(pair<string,string>(key, val));
		}
	}
	map<string,string>::iterator itr;
	WORD onid = 0;
	itr = paramMap.find("onid");
	if( itr != paramMap.end() ){
		onid = (WORD)atoi(itr->second.c_str());
	}
	WORD tsid = 0;
	itr = paramMap.find("tsid");
	if( itr != paramMap.end() ){
		tsid = (WORD)atoi(itr->second.c_str());
	}
	WORD sid = 0;
	itr = paramMap.find("sid");
	if( itr != paramMap.end() ){
		sid = (WORD)atoi(itr->second.c_str());
	}
	WORD evid = 0;
	itr = paramMap.find("evid");
	if( itr != paramMap.end() ){
		evid = (WORD)atoi(itr->second.c_str());
	}

	EPGDB_EVENT_INFO eventInfo;
	if( epgDB->SearchEpg(onid, tsid, sid, evid, &eventInfo) == FALSE ){
		return FALSE;
	}
	if( eventInfo.shortInfo != NULL ){
		reserveData->title = eventInfo.shortInfo->event_name;
	}
	reserveData->startTime = eventInfo.start_time;
	reserveData->startTimeEpg = eventInfo.start_time;
	reserveData->durationSecond = eventInfo.durationSec;
	epgDB->SearchServiceName(
		eventInfo.original_network_id,
		eventInfo.transport_stream_id,
		eventInfo.service_id,
		reserveData->stationName
		);
	reserveData->originalNetworkID = eventInfo.original_network_id;
	reserveData->transportStreamID = eventInfo.transport_stream_id;
	reserveData->serviceID = eventInfo.service_id;
	reserveData->eventID = eventInfo.event_id;

	itr = paramMap.find("presetID");
	if( itr == paramMap.end() ){
		return FALSE;
	}
	WORD presetID = (WORD)atoi(itr->second.c_str());
	reserveData->recSetting.recFolderList.clear();
	reserveData->recSetting.partialRecFolder.clear();
	LoadRecSetData(presetID, &reserveData->recSetting);

	itr = paramMap.find("recMode");
	if( itr == paramMap.end() ){
		return FALSE;
	}
	reserveData->recSetting.recMode = (BYTE)atoi(itr->second.c_str());

	if(reserveData->eventID != 0xFFFF){
		itr = paramMap.find("tuijyuuFlag");
		if( itr == paramMap.end() ){
			return FALSE;
		}
		reserveData->recSetting.tuijyuuFlag = (BYTE)atoi(itr->second.c_str());
	}else{
		reserveData->recSetting.tuijyuuFlag = 0;
	}

	itr = paramMap.find("priority");
	if( itr == paramMap.end() ){
		return FALSE;
	}
	reserveData->recSetting.priority = (BYTE)atoi(itr->second.c_str());

	if(reserveData->eventID != 0xFFFF){
		itr = paramMap.find("pittariFlag");
		if( itr == paramMap.end() ){
			return FALSE;
		}
		reserveData->recSetting.pittariFlag = (BYTE)atoi(itr->second.c_str());
	}else{
		reserveData->recSetting.pittariFlag = 0;
	}

	itr = paramMap.find("suspendMode");
	if( itr == paramMap.end() ){
		return FALSE;
	}
	reserveData->recSetting.suspendMode = (BYTE)atoi(itr->second.c_str());
	
	if( reserveData->recSetting.suspendMode == 1 || reserveData->recSetting.suspendMode == 2 ){
		itr = paramMap.find("rebootFlag");
		if( itr == paramMap.end() ){
			reserveData->recSetting.rebootFlag = 0;
		}else{
			reserveData->recSetting.rebootFlag = 1;
		}
	}else{
		reserveData->recSetting.rebootFlag = 0;
	}

	itr = paramMap.find("useDefMargineFlag");
	if( itr == paramMap.end() ){
		reserveData->recSetting.useMargineFlag = 1;
		itr = paramMap.find("startMargine");
		if( itr == paramMap.end() ){
			return FALSE;
		}
		reserveData->recSetting.startMargine = atoi(itr->second.c_str());
		itr = paramMap.find("endMargine");
		if( itr == paramMap.end() ){
			return FALSE;
		}
		reserveData->recSetting.endMargine = atoi(itr->second.c_str());
	}else{
		reserveData->recSetting.useMargineFlag = 0;
		reserveData->recSetting.startMargine = 0;
		reserveData->recSetting.endMargine = 0;
	}

	itr = paramMap.find("serviceMode");
	if( itr != paramMap.end() ){
		reserveData->recSetting.serviceMode = 0;
	}else{
		reserveData->recSetting.serviceMode = 1;
		itr = paramMap.find("serviceMode_1");
		if( itr != paramMap.end() ){
			reserveData->recSetting.serviceMode |= 0x10;
		}
		itr = paramMap.find("serviceMode_2");
		if( itr != paramMap.end() ){
			reserveData->recSetting.serviceMode |= 0x20;
		}
	}

	itr = paramMap.find("continueRecFlag");
	if( itr == paramMap.end() ){
		reserveData->recSetting.continueRecFlag = 0;
	}else{
		reserveData->recSetting.continueRecFlag = 1;
	}
	
	itr = paramMap.find("tunerID");
	if( itr == paramMap.end() ){
		return FALSE;
	}
	reserveData->recSetting.tunerID = (DWORD)atoi(itr->second.c_str());

	itr = paramMap.find("partialRecFlag");
	if( itr == paramMap.end() ){
		reserveData->recSetting.partialRecFlag = 0;
	}else{
		reserveData->recSetting.partialRecFlag = 1;
	}

	return TRUE;
}

DWORD CRestApiManager::GetReserveAddResponse(BOOL err, HTTP_STREAM* sendParam)
{
	DWORD ret = NO_ERR;

	vector<EPGDB_SERVICE_INFO> list;
	wstring xml = L"";
	string utf8 = "";
	if( err == TRUE ){
		xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\" ?><entry>";
		xml += L"<success>予約の追加に成功しました</success>";
		xml += L"</entry>";
	}else{
		xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\" ?><entry>";
		xml += L"<err>予約の追加に失敗しました</err>";
		xml += L"</entry>";
	}

	WtoUTF8(xml, utf8);

	sendParam->dataSize = (DWORD)utf8.size();
	sendParam->data = new BYTE[sendParam->dataSize];
	memcpy(sendParam->data, utf8.c_str(), sendParam->dataSize);
	if( sendParam->dataSize > 0 ){
		Format(sendParam->httpHeader, "HTTP/1.0 200 OK\r\nContent-Type: text/xml\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", sendParam->dataSize);
	}else{
		sendParam->httpHeader = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n\r\n";
	}

	return ret;
}

DWORD CRestApiManager::GetSearchEvent(string param, HTTP_STREAM* sendParam, CEpgDBManager* epgDB)
{
	DWORD ret = NO_ERR;

	map<string,string> paramMap;
	while(param.size()>0){
		string buff;
		Separate(param, "&", buff, param);
		if(buff.size()>0){
			string key;
			string val;
			Separate(buff, "=", key, val);
			paramMap.insert(pair<string,string>(key, val));
		}
	}

	map<string,string>::iterator itr;
	WORD network = 0xFFFF;
	itr = paramMap.find("network");
	if( itr != paramMap.end() ){
		network = (WORD)atoi(itr->second.c_str());
	}
	WORD days = 0;
	itr = paramMap.find("days");
	if( itr != paramMap.end() ){
		days = (WORD)atoi(itr->second.c_str());
	}

	wstring andkey;
	itr = paramMap.find("andkey");
	if( itr != paramMap.end() ){
		string utf8;
		UrlDecode(itr->second.c_str(), (DWORD)itr->second.size(), utf8);
		UTF8toW(utf8, andkey);
	}
	wstring notkey;
	itr = paramMap.find("notkey");
	if( itr != paramMap.end() ){
		string utf8;
		UrlDecode(itr->second.c_str(), (DWORD)itr->second.size(), utf8);
		UTF8toW(utf8, notkey);
	}
	vector<DWORD> genru;
	itr = paramMap.find("genru");
	if( itr != paramMap.end() ){
		string val = itr->second;
		string id;
		while(val.size() > 0 ){
			Separate(val, "-", id, val);
			genru.push_back((DWORD)atoi(id.c_str()));
		}
	}

	DWORD index = 0;
	itr = paramMap.find("index");
	if( itr != paramMap.end() ){
		index = (DWORD)atoi(itr->second.c_str());
	}
	DWORD count = 200;
	itr = paramMap.find("count");
	if( itr != paramMap.end() ){
		count = (DWORD)atoi(itr->second.c_str());
	}
	WORD basicOnly = 1;
	itr = paramMap.find("basic");
	if( itr != paramMap.end() ){
		basicOnly = (WORD)atoi(itr->second.c_str());
	}

	//検索条件
	EPGDB_SEARCH_KEY_INFO searchKey;
	searchKey.andKey = andkey;
	searchKey.notKey = notkey;
	for( size_t i=0; i<genru.size(); i++){
		EPGDB_CONTENT_DATA item;
		item.content_nibble_level_1 = (BYTE)(genru[i]>>8);
		item.content_nibble_level_2 = (BYTE)(genru[i]&0x00FF);
		searchKey.contentList.push_back(item);
	}
	//対象サービス
	vector<EPGDB_SERVICE_INFO> list;
	if( epgDB->GetServiceList(&list) == TRUE ){
		if( network != 0xFFFF ){
			for( size_t i=0; i<list.size(); i++ ){
				__int64 key = _Create64Key(list[i].ONID,list[i].TSID, list[i].SID);

				if( 0x7880 <= list[i].ONID && list[i].ONID<= 0x7FE8 ){
					//地デジ
					if( (network & 0x0001) > 0 ){
						searchKey.serviceList.push_back(key);
					}
				}else
				if( list[i].ONID == 0x0004 ){
					//BS
					if( (network & 0x0002) > 0 ){
						searchKey.serviceList.push_back(key);
					}
				}else
				if( list[i].ONID == 0x0006 || list[i].ONID == 0x0007 ){
					//CS
					if( (network & 0x0004) > 0 ){
						searchKey.serviceList.push_back(key);
					}
				}else{
					//その他
					if( (network & 0x0008) > 0 ){
						searchKey.serviceList.push_back(key);
					}
				}
			}
		}
	}

	//対象期間
	__int64 chkTime = 0;
	if( days > 0 ){
		SYSTEMTIME now;
		GetLocalTime(&now);
		now.wHour = 0;
		now.wMinute = 0;
		now.wSecond = 0;
		now.wMilliseconds = 0;

		chkTime = ConvertI64Time(now);
		chkTime += days*(29*60*60*I64_1SEC);
	}

	vector<EPGDB_SEARCH_KEY_INFO> keyList;
	keyList.push_back(searchKey);
	vector<unique_ptr<CEpgDBManager::SEARCH_RESULT_EVENT_DATA>> resultList;
	wstring xml = L"";
	string utf8 = "";
	epgDB->SearchEpg(&keyList, &resultList);
	map<__int64, __int64> addID;
	map<__int64, __int64>::iterator itrAdd;
	if ( resultList.size() > 0 ){
		xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\" ?><entry>";
		DWORD total = 0;
		DWORD findCount = 0;

		wstring serviceinfo = L"<items>";
		wstring buff;
		for( size_t i = 0; i<resultList.size(); i++){
			if( days > 0 ){
				if( chkTime < ConvertI64Time(resultList[i]->info.start_time)){
					continue;
				}
			}
			if( resultList[i]->info.eventGroupInfo != NULL ){
				BOOL find = FALSE;
				for( size_t j=0; j<resultList[i]->info.eventGroupInfo->eventDataList.size(); j++ ){
					__int64 evid = _Create64Key2(resultList[i]->info.eventGroupInfo->eventDataList[j].original_network_id,
						resultList[i]->info.eventGroupInfo->eventDataList[j].transport_stream_id,
						resultList[i]->info.eventGroupInfo->eventDataList[j].service_id,
						resultList[i]->info.eventGroupInfo->eventDataList[j].event_id);
					itrAdd = addID.find(evid);
					if( itrAdd != addID.end() ){
						find = TRUE;
					}
				}
				if( find == TRUE ){
					continue;
				}
			}
			__int64 evid = _Create64Key2(resultList[i]->info.original_network_id,
				resultList[i]->info.transport_stream_id,
				resultList[i]->info.service_id,
				resultList[i]->info.event_id);
			addID.insert(pair<__int64, __int64>(evid,evid));
			if( total < index ){
				total++;
				continue;
			}
			if( index + count <= total ){
				total++;
				continue;
			}
			total++;
			findCount++;

			EPGDB_EVENT_INFO* eventInfo = &resultList[i]->info;

			serviceinfo += L"<eventinfo>";
			Format(buff, L"<ONID>%d</ONID>", eventInfo->original_network_id);
			serviceinfo += buff;
			Format(buff, L"<TSID>%d</TSID>", eventInfo->transport_stream_id);
			serviceinfo += buff;
			Format(buff, L"<SID>%d</SID>", eventInfo->service_id);
			serviceinfo += buff;
			Format(buff, L"<eventID>%d</eventID>", eventInfo->event_id);
			serviceinfo += buff;
			if( eventInfo->StartTimeFlag == 1 ){
				Format(buff, L"<startDate>%04d/%02d/%02d</startDate>", eventInfo->start_time.wYear, eventInfo->start_time.wMonth, eventInfo->start_time.wDay);
				serviceinfo += buff;
				Format(buff, L"<startTime>%02d:%02d:%02d</startTime>", eventInfo->start_time.wHour, eventInfo->start_time.wMinute, eventInfo->start_time.wSecond);
				serviceinfo += buff;
				Format(buff, L"<startDayOfWeek>%d</startDayOfWeek>", eventInfo->start_time.wDayOfWeek);
				serviceinfo += buff;
			}
			if( eventInfo->DurationFlag == 1 ){
				Format(buff, L"<duration>%d</duration>", eventInfo->durationSec);
				serviceinfo += buff;
			}
			if( eventInfo->shortInfo != NULL ){
				wstring chk = eventInfo->shortInfo->event_name;
				CheckXMLChar(chk);
				Format(buff, L"<event_name>%s</event_name>", chk.c_str());
				serviceinfo += buff;

				chk = eventInfo->shortInfo->text_char;
				CheckXMLChar(chk);
				Format(buff, L"<event_text>%s</event_text>", chk.c_str());
				serviceinfo += buff;
			}
			if( eventInfo->contentInfo != NULL ){
				serviceinfo += L"";
				for( size_t k=0; k<eventInfo->contentInfo->nibbleList.size(); k++){
					wstring nibble = L"";
					Format(nibble,L"<contentInfo><nibble1>%d</nibble1><nibble2>%d</nibble2></contentInfo>", 
						eventInfo->contentInfo->nibbleList[k].content_nibble_level_1,
						eventInfo->contentInfo->nibbleList[k].content_nibble_level_2);
					serviceinfo += nibble;
				}
			}
			if( eventInfo->eventGroupInfo != NULL ){
				for( size_t k=0; k<eventInfo->eventGroupInfo->eventDataList.size(); k++){
					wstring group = L"";
					Format(group,L"<groupInfo><ONID>%d</ONID><TSID>%d</TSID><SID>%d</SID><eventID>%d</eventID></groupInfo>", 
						eventInfo->eventGroupInfo->eventDataList[k].original_network_id,
						eventInfo->eventGroupInfo->eventDataList[k].transport_stream_id,
						eventInfo->eventGroupInfo->eventDataList[k].service_id,
						eventInfo->eventGroupInfo->eventDataList[k].event_id
						);
					serviceinfo += group;
				}
			}

			Format(buff, L"<freeCAFlag>%d</freeCAFlag>", eventInfo->freeCAFlag);
			serviceinfo += buff;

			if( basicOnly == 0 ){
				if( eventInfo->extInfo != NULL ){
					wstring chk = eventInfo->extInfo->text_char;
					CheckXMLChar(chk);
					Format(buff, L"<event_ext_text>%s</event_ext_text>", chk.c_str());
					serviceinfo += buff;
				}
				if( eventInfo->componentInfo != NULL ){
					Format(buff, L"<videoInfo><stream_content>%d</stream_content><component_type>%d</component_type><component_tag>%d</component_tag><text>%s</text></videoInfo>", 
						eventInfo->componentInfo->stream_content,
						eventInfo->componentInfo->component_type,
						eventInfo->componentInfo->component_tag,
						eventInfo->componentInfo->text_char.c_str()
						);
					serviceinfo += buff;
				}
				if( eventInfo->audioInfo != NULL ){
					for( size_t k=0; k<eventInfo->audioInfo->componentList.size(); k++ ){
						Format(buff, L"<audioInfo><stream_content>%d</stream_content><component_type>%d</component_type><component_tag>%d</component_tag><stream_type>%d</stream_type><simulcast_group_tag>%d</simulcast_group_tag><ES_multi_lingual_flag>%d</ES_multi_lingual_flag><main_component_flag>%d</main_component_flag><quality_indicator>%d</quality_indicator><sampling_rate>%d</sampling_rate><text>%s</text></audioInfo>", 
							eventInfo->audioInfo->componentList[k].stream_content,
							eventInfo->audioInfo->componentList[k].component_type,
							eventInfo->audioInfo->componentList[k].component_tag,
							eventInfo->audioInfo->componentList[k].stream_type,
							eventInfo->audioInfo->componentList[k].simulcast_group_tag,
							eventInfo->audioInfo->componentList[k].ES_multi_lingual_flag,
							eventInfo->audioInfo->componentList[k].main_component_flag,
							eventInfo->audioInfo->componentList[k].quality_indicator,
							eventInfo->audioInfo->componentList[k].sampling_rate,
							eventInfo->audioInfo->componentList[k].text_char.c_str()
							);
						serviceinfo += buff;
					}
				}
				if( eventInfo->eventRelayInfo != NULL ){
					for( size_t k=0; k<eventInfo->eventRelayInfo->eventDataList.size(); k++){
						wstring group = L"";
						Format(group,L"<relayInfo><ONID>%d</ONID><TSID>%d</TSID><SID>%d</SID><eventID>%d</eventID></relayInfo>", 
							eventInfo->eventRelayInfo->eventDataList[k].original_network_id,
							eventInfo->eventRelayInfo->eventDataList[k].transport_stream_id,
							eventInfo->eventRelayInfo->eventDataList[k].service_id,
							eventInfo->eventRelayInfo->eventDataList[k].event_id
							);
						serviceinfo += group;
					}
				}
			}
			serviceinfo += L"</eventinfo>";
		}
		serviceinfo += L"</items>";

		Format(buff, L"<total>%d</total><index>%d</index><count>%d</count>", total, index, findCount);
		xml += buff;
		xml += serviceinfo;
		xml += L"</entry>";
	}else{
		xml += L"<?xml version=\"1.0\" encoding=\"UTF-8\" ?><entry>";
		xml += L"<err>EPGデータを読み込み中、または存在しません</err>";
		xml += L"</entry>";
	}

	WtoUTF8(xml, utf8);

	sendParam->dataSize = (DWORD)utf8.size();
	sendParam->data = new BYTE[sendParam->dataSize];
	memcpy(sendParam->data, utf8.c_str(), sendParam->dataSize);
	if( sendParam->dataSize > 0 ){
		Format(sendParam->httpHeader, "HTTP/1.0 200 OK\r\nContent-Type: text/xml\r\nContent-Length: %d\r\nConnection: close\r\n\r\n", sendParam->dataSize);
	}else{
		sendParam->httpHeader = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n\r\n";
	}

	return ret;
}

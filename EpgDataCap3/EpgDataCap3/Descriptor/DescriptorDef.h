#ifndef __DESCRIPTOR_DEF_H__
#define __DESCRIPTOR_DEF_H__

#include "CADesc.h"
#include "AudioComponentDesc.h"
#include "AVCTimingHRDDesc.h"
#include "AVCVideoDesc.h"
#include "BoardInfoDesc.h"
#include "BouquetNameDesc.h"
#include "BroadcasterNameDesc.h"
#include "CAIdentifierDesc.h"
#include "ComponentDesc.h"
#include "ComponentGroupDesc.h"
#include "ConnectedTransmissionDesc.h"
#include "ContentAvailabilityDesc.h"
#include "ContentDesc.h"
#include "CountryAvailabilityDesc.h"
#include "DataComponentDesc.h"
#include "DataContentDesc.h"
#include "DigitalCopyCtrlDesc.h"
#include "EmergencyInfoDesc.h"
#include "EventGroupDesc.h"
#include "ExtendedBroadcasterDesc.h"
#include "ExtendedEventDesc.h"
#include "HierarchicalTransmissionDesc.h"
#include "HyperlinkDesc.h"
#include "LDTLinkageDesc.h"
#include "LinkageDesc.h"
#include "LocalTimeOffsetDesc.h"
#include "LogoTransmissionDesc.h"
#include "MosaicDesc.h"
#include "NetworkNameDesc.h"
#include "NVODReferenceDesc.h"
#include "ParentalRatingDesc.h"
#include "PartialReceptionDesc.h"
#include "SatelliteDeliverySysDesc.h"
#include "SeriesDesc.h"
#include "ServiceDesc.h"
#include "ServiceGroupDesc.h"
#include "ServiceListDesc.h"
#include "ShortEventDesc.h"
#include "SIParameterDesc.h"
#include "SIPrime_tsDesc.h"
#include "StreamIdentifierDesc.h"
#include "StuffingDesc.h"
#include "SystemManagementDesc.h"
#include "TargetRegionDesc.h"
#include "TerrestrialDeliverySysDesc.h"
#include "TimeShiftedEventDesc.h"
#include "TimeShiftedServiceDesc.h"
#include "TSInfoDesc.h"
#include "VideoDecodeCtrlDesc.h"
#include "PartialTransportStreamDesc.h"
#include "NetworkIdentification.h"
#include "PartialTSTimeDesc.h"
#include "DownloadContentDesc.h"

typedef struct _DESCRIPTOR_DATA{
	CCADesc* CA;
	CAudioComponentDesc* audioComponent;
	CAVCTimingHRDDesc*	AVCTimingHRD;
	CAVCVideoDesc* AVCVideo;
	CBoardInfoDesc* boardInfo;
	CBouquetNameDesc* bouquetName;
	CBroadcasterNameDesc* broadcasterName;
	CCAIdentifierDesc* CAIdentifier;
	CComponentDesc* component;
	CComponentGroupDesc* componentGroup;
	CConnectedTransmissionDesc* connectedTransmission;
	CContentAvailabilityDesc* contentAvailability;
	CContentDesc* content;
	CCountryAvailabilityDesc* countryAvailability;
	CDataComponentDesc* dataComponent;
	CDataContentDesc* dataContent;
	CDigitalCopyCtrlDesc* digitalCopyCtrl;
	CEmergencyInfoDesc* emergencyInfo;
	CEventGroupDesc* eventGroup;
	CExtendedBroadcasterDesc* extendedBroadcaster;
	CExtendedEventDesc* extendedEvent;
	CHierarchicalTransmissionDesc* hierarchicalTransmission;
	CHyperlinkDesc* hyperlink;
	CLDTLinkageDesc* LDTLinkage;
	CLinkageDesc* linkage;
	CLocalTimeOffsetDesc* localTimeOffset;
	CLogoTransmissionDesc* logoTransmission;
	CMosaicDesc* mosaic;
	CNetworkNameDesc* networkName;
	CNVODReferenceDesc* NVODReference;
	CParentalRatingDesc* parentalRating;
	CPartialReceptionDesc* partialReception;
	CSatelliteDeliverySysDesc* satelliteDeliverySys;
	CSeriesDesc* series;
	CServiceDesc* service;
	CServiceGroupDesc* serviceGroup;
	CServiceListDesc* serviceList;
	CShortEventDesc* shortEvent;
	CSIParameterDesc* SIParameter;
	CSIPrime_tsDesc* SIPrime_ts;
	CStreamIdentifierDesc* streamIdentifier;
	CStuffingDesc* stuffing;
	CSystemManagementDesc* systemManagement;
	CTargetRegionDesc* targetRegion;
	CTerrestrialDeliverySysDesc* terrestrialDeliverySys;
	CTimeShiftedEventDesc* timeShiftedEvent;
	CTimeShiftedServiceDesc* timeShiftedService;
	CTSInfoDesc* TSInfo;
	CVideoDecodeCtrlDesc* videoDecodeCtrl;
	CPartialTransportStreamDesc* partialTransportStream;
	CNetworkIdentification* networkIdentification;
	CPartialTSTimeDesc* partialTSTime;
	CDownloadContentDesc* downloadContent;

	_DESCRIPTOR_DATA(void){
		CA = NULL;
		audioComponent = NULL;
		AVCTimingHRD = NULL;
		AVCVideo = NULL;
		boardInfo = NULL;
		bouquetName = NULL;
		broadcasterName = NULL;
		CAIdentifier = NULL;
		component = NULL;
		componentGroup = NULL;
		connectedTransmission = NULL;
		contentAvailability = NULL;
		content = NULL;
		countryAvailability = NULL;
		dataComponent = NULL;
		dataContent = NULL;
		digitalCopyCtrl = NULL;
		emergencyInfo = NULL;
		eventGroup = NULL;
		extendedBroadcaster = NULL;
		extendedEvent = NULL;
		hierarchicalTransmission = NULL;
		hyperlink = NULL;
		LDTLinkage = NULL;
		linkage = NULL;
		localTimeOffset = NULL;
		logoTransmission = NULL;
		mosaic = NULL;
		networkName = NULL;
		NVODReference = NULL;
		parentalRating = NULL;
		partialReception = NULL;
		satelliteDeliverySys = NULL;
		series = NULL;
		service = NULL;
		serviceGroup = NULL;
		serviceList = NULL;
		shortEvent = NULL;
		SIParameter = NULL;
		SIPrime_ts = NULL;
		streamIdentifier = NULL;
		stuffing = NULL;
		systemManagement = NULL;
		targetRegion = NULL;
		terrestrialDeliverySys = NULL;
		timeShiftedEvent = NULL;
		timeShiftedService = NULL;
		TSInfo = NULL;
		videoDecodeCtrl = NULL;
		partialTransportStream = NULL;
		networkIdentification = NULL;
		partialTSTime = NULL;
		downloadContent = NULL;
	};
	~_DESCRIPTOR_DATA(void){
		SAFE_DELETE(CA);
		SAFE_DELETE(audioComponent);
		SAFE_DELETE(AVCTimingHRD);
		SAFE_DELETE(AVCVideo);
		SAFE_DELETE(boardInfo);
		SAFE_DELETE(bouquetName);
		SAFE_DELETE(broadcasterName);
		SAFE_DELETE(CAIdentifier);
		SAFE_DELETE(component);
		SAFE_DELETE(componentGroup);
		SAFE_DELETE(connectedTransmission);
		SAFE_DELETE(contentAvailability);
		SAFE_DELETE(content);
		SAFE_DELETE(countryAvailability);
		SAFE_DELETE(dataComponent);
		SAFE_DELETE(dataContent);
		SAFE_DELETE(digitalCopyCtrl);
		SAFE_DELETE(emergencyInfo);
		SAFE_DELETE(eventGroup);
		SAFE_DELETE(extendedBroadcaster);
		SAFE_DELETE(extendedEvent);
		SAFE_DELETE(hierarchicalTransmission);
		SAFE_DELETE(hyperlink);
		SAFE_DELETE(LDTLinkage);
		SAFE_DELETE(linkage);
		SAFE_DELETE(localTimeOffset);
		SAFE_DELETE(logoTransmission);
		SAFE_DELETE(mosaic);
		SAFE_DELETE(networkName);
		SAFE_DELETE(NVODReference);
		SAFE_DELETE(parentalRating);
		SAFE_DELETE(partialReception);
		SAFE_DELETE(satelliteDeliverySys);
		SAFE_DELETE(series);
		SAFE_DELETE(service);
		SAFE_DELETE(serviceGroup);
		SAFE_DELETE(serviceList);
		SAFE_DELETE(shortEvent);
		SAFE_DELETE(SIParameter);
		SAFE_DELETE(SIPrime_ts);
		SAFE_DELETE(streamIdentifier);
		SAFE_DELETE(stuffing);
		SAFE_DELETE(systemManagement);
		SAFE_DELETE(targetRegion);
		SAFE_DELETE(terrestrialDeliverySys);
		SAFE_DELETE(timeShiftedEvent);
		SAFE_DELETE(timeShiftedService);
		SAFE_DELETE(TSInfo);
		SAFE_DELETE(videoDecodeCtrl);
		SAFE_DELETE(partialTransportStream);
		SAFE_DELETE(networkIdentification);
		SAFE_DELETE(partialTSTime);
		SAFE_DELETE(downloadContent);
	};
}DESCRIPTOR_DATA;

#ifdef DESCRIPTOR_DEF_H_IMPLEMENT_TABLE

//////////////////////////////////////////////////////
//限定受信方式識別の割当状況
typedef struct _CA_SYSTEM_TABLE{
	WORD CA_system_id;
	wstring name;
} CA_SYSTEM_TABLE;

static CA_SYSTEM_TABLE CASystemIDTable[]={
	{ 0x0001, L"スカパー限定受信方式" },
	{ 0x0003, L"日立方式" },
	{ 0x0004, L"Secure Navi方式" },
	{ 0x0005, L"ARIB限定受信方式" },
	{ 0x0006, L"松下CATV限定受信方式" },
	{ 0x0007, L"ケーブルラボ視聴制御方式" },
	{ 0x0008, L"u-CAS方式" },
	{ 0x0009, L"PowerKEY方式" },
	{ 0x000A, L"ARIB限定受信B方式" },
	{ 0x000B, L"パイシス限定受信方式" },
	{ 0x000C, L"MULTI2-NAGRA方式" },
	{ 0x000D, L"IPTVフォーラム・Marlin方式" }
};
//->限定受信方式識別の割当状況

//////////////////////////////////////////////////////
//コンポーネント内容とコンポーネント種別
typedef struct _COMPONENT_TYPE_TABLE{
	BYTE stream_content;
	BYTE component_type;
	wstring name;
} COMPONENT_TYPE_TABLE;

static COMPONENT_TYPE_TABLE componentTypeTable[]={
	{ 0x01, 0x01, L"映像480i(525i)、アスペクト比4:3" },
	{ 0x01, 0x02, L"映像480i(525i)、アスペクト比16:9 パンベクトルあり" },
	{ 0x01, 0x03, L"映像480i(525i)、アスペクト比16:9 パンベクトルなし" },
	{ 0x01, 0x04, L"映像480i(525i)、アスペクト比 > 16:9" },
	{ 0x01, 0x91, L"映像2160p、アスペクト比4:3" },
	{ 0x01, 0x92, L"映像2160p、アスペクト比16:9 パンベクトルあり" },
	{ 0x01, 0x93, L"映像2160p、アスペクト比16:9 パンベクトルなし" },
	{ 0x01, 0x94, L"映像2160p、アスペクト比 > 16:9" },
	{ 0x01, 0xA1, L"映像480p(525p)、アスペクト比4:3" },
	{ 0x01, 0xA2, L"映像480p(525p)、アスペクト比16:9 パンベクトルあり" },
	{ 0x01, 0xA3, L"映像480p(525p)、アスペクト比16:9 パンベクトルなし" },
	{ 0x01, 0xA4, L"映像480p(525p)、アスペクト比 > 16:9" },
	{ 0x01, 0xB1, L"映像1080i(1125i)、アスペクト比4:3" },
	{ 0x01, 0xB2, L"映像1080i(1125i)、アスペクト比16:9 パンベクトルあり" },
	{ 0x01, 0xB3, L"映像1080i(1125i)、アスペクト比16:9 パンベクトルなし" },
	{ 0x01, 0xB4, L"映像1080i(1125i)、アスペクト比 > 16:9" },
	{ 0x01, 0xC1, L"映像720p(750p)、アスペクト比4:3" },
	{ 0x01, 0xC2, L"映像720p(750p)、アスペクト比16:9 パンベクトルあり" },
	{ 0x01, 0xC3, L"映像720p(750p)、アスペクト比16:9 パンベクトルなし" },
	{ 0x01, 0xC4, L"映像720p(750p)、アスペクト比 > 16:9" },
	{ 0x01, 0xD1, L"映像240p アスペクト比4:3" },
	{ 0x01, 0xD2, L"映像240p アスペクト比16:9 パンベクトルあり" },
	{ 0x01, 0xD3, L"映像240p アスペクト比16:9 パンベクトルなし" },
	{ 0x01, 0xD4, L"映像240p アスペクト比 > 16:9" },
	{ 0x01, 0xE1, L"映像1080p(1125p)、アスペクト比4:3" },
	{ 0x01, 0xE2, L"映像1080p(1125p)、アスペクト比16:9 パンベクトルあり" },
	{ 0x01, 0xE3, L"映像1080p(1125p)、アスペクト比16:9 パンベクトルなし" },
	{ 0x01, 0xE4, L"映像1080p(1125p)、アスペクト比 > 16:9" },
	{ 0x02, 0x01, L"音声、1/0モード（シングルモノ）" },
	{ 0x02, 0x02, L"音声、1/0＋1/0モード（デュアルモノ）" },
	{ 0x02, 0x03, L"音声、2/0モード（ステレオ）" },
	{ 0x02, 0x04, L"音声、2/1モード" },
	{ 0x02, 0x05, L"音声、3/0モード" },
	{ 0x02, 0x06, L"音声、2/2モード" },
	{ 0x02, 0x07, L"音声、3/1モード" },
	{ 0x02, 0x08, L"音声、3/2モード" },
	{ 0x02, 0x09, L"音声、3/2＋LFEモード（3/2.1モード）" },
	{ 0x02, 0x0A, L"音声、3/3.1モード" },
	{ 0x02, 0x0B, L"音声、2/0/0-2/0/2-0.1モード" },
	{ 0x02, 0x0C, L"音声、5/2.1モード" },
	{ 0x02, 0x0D, L"音声、3/2/2.1モード" },
	{ 0x02, 0x0E, L"音声、2/0/0-3/0/2-0.1モード" },
	{ 0x02, 0x0F, L"音声、0/2/0-3/0/2-0.1モード" },
	{ 0x02, 0x10, L"音声、2/0/0-3/2/3-0.2モード" },
	{ 0x02, 0x11, L"音声、3/3/3-5/2/3-3/0/0.2モード" },
	{ 0x02, 0x40, L"視覚障害者用音声解説" },
	{ 0x02, 0x41, L"聴覚障害者用音声" },
	{ 0x05, 0x01, L"H.264|MPEG-4 AVC、映像480i(525i)、アスペクト比4:3" },
	{ 0x05, 0x02, L"H.264|MPEG-4 AVC、映像480i(525i)、アスペクト比16:9 パンベクトルあり" },
	{ 0x05, 0x03, L"H.264|MPEG-4 AVC、映像480i(525i)、アスペクト比16:9 パンベクトルなし" },
	{ 0x05, 0x04, L"H.264|MPEG-4 AVC、映像480i(525i)、アスペクト比 > 16:9" },
	{ 0x05, 0x91, L"H.264|MPEG-4 AVC、映像2160p、アスペクト比4:3" },
	{ 0x05, 0x92, L"H.264|MPEG-4 AVC、映像2160p、アスペクト比16:9 パンベクトルあり" },
	{ 0x05, 0x93, L"H.264|MPEG-4 AVC、映像2160p、アスペクト比16:9 パンベクトルなし" },
	{ 0x05, 0x94, L"H.264|MPEG-4 AVC、映像2160p、アスペクト比 > 16:9" },
	{ 0x05, 0xA1, L"H.264|MPEG-4 AVC、映像480p(525p)、アスペクト比4:3" },
	{ 0x05, 0xA2, L"H.264|MPEG-4 AVC、映像480p(525p)、アスペクト比16:9 パンベクトルあり" },
	{ 0x05, 0xA3, L"H.264|MPEG-4 AVC、映像480p(525p)、アスペクト比16:9 パンベクトルなし" },
	{ 0x05, 0xA4, L"H.264|MPEG-4 AVC、映像480p(525p)、アスペクト比 > 16:9" },
	{ 0x05, 0xB1, L"H.264|MPEG-4 AVC、映像1080i(1125i)、アスペクト比4:3" },
	{ 0x05, 0xB2, L"H.264|MPEG-4 AVC、映像1080i(1125i)、アスペクト比16:9 パンベクトルあり" },
	{ 0x05, 0xB3, L"H.264|MPEG-4 AVC、映像1080i(1125i)、アスペクト比16:9 パンベクトルなし" },
	{ 0x05, 0xB4, L"H.264|MPEG-4 AVC、映像1080i(1125i)、アスペクト比 > 16:9" },
	{ 0x05, 0xC1, L"H.264|MPEG-4 AVC、映像720p(750p)、アスペクト比4:3" },
	{ 0x05, 0xC2, L"H.264|MPEG-4 AVC、映像720p(750p)、アスペクト比16:9 パンベクトルあり" },
	{ 0x05, 0xC3, L"H.264|MPEG-4 AVC、映像720p(750p)、アスペクト比16:9 パンベクトルなし" },
	{ 0x05, 0xC4, L"H.264|MPEG-4 AVC、映像720p(750p)、アスペクト比 > 16:9" },
	{ 0x05, 0xD1, L"H.264|MPEG-4 AVC、映像240p アスペクト比4:3" },
	{ 0x05, 0xD2, L"H.264|MPEG-4 AVC、映像240p アスペクト比16:9 パンベクトルあり" },
	{ 0x05, 0xD3, L"H.264|MPEG-4 AVC、映像240p アスペクト比16:9 パンベクトルなし" },
	{ 0x05, 0xD4, L"H.264|MPEG-4 AVC、映像240p アスペクト比 > 16:9" },
	{ 0x05, 0xE1, L"H.264|MPEG-4 AVC、映像1080p(1125p)、アスペクト比4:3" },
	{ 0x05, 0xE2, L"H.264|MPEG-4 AVC、映像1080p(1125p)、アスペクト比16:9 パンベクトルあり" },
	{ 0x05, 0xE3, L"H.264|MPEG-4 AVC、映像1080p(1125p)、アスペクト比16:9 パンベクトルなし" },
	{ 0x05, 0xE4, L"H.264|MPEG-4 AVC、映像1080p(1125p)、アスペクト比 > 16:9" }
};
//->コンポーネント内容とコンポーネント種別

//////////////////////////////////////////////////////
//コンテント記述子におけるジャンル指定
typedef struct _NIBBLE_TABLE_1{
	BYTE content_nibble_level_1;
	wstring name;
} NIBBLE_TABLE_1;

static NIBBLE_TABLE_1 nibbleTable1[]={
	{ 0x00, L"ニュース／報道" },
	{ 0x01, L"スポーツ" },
	{ 0x02, L"情報／ワイドショー" },
	{ 0x03, L"ドラマ" },
	{ 0x04, L"音楽" },
	{ 0x05, L"バラエティ" },
	{ 0x06, L"映画" },
	{ 0x07, L"アニメ／特撮" },
	{ 0x08, L"ドキュメンタリー／教養" },
	{ 0x09, L"劇場／公演" },
	{ 0x0A, L"趣味／教育" },
	{ 0x0B, L"福祉" },
	{ 0x0E, L"拡張" },
	{ 0x0F, L"その他" },
};

typedef struct _NIBBLE_TABLE_2{
	BYTE content_nibble_level_1;
	BYTE content_nibble_level_2;
	wstring name;
} NIBBLE_TABLE_2;

static NIBBLE_TABLE_2 nibbleTable2[]={
	{ 0x00, 0x00, L"定時・総合" },
	{ 0x00, 0x01, L"天気" },
	{ 0x00, 0x02, L"特集・ドキュメント" },
	{ 0x00, 0x03, L"政治・国会" },
	{ 0x00, 0x04, L"経済・市況" },
	{ 0x00, 0x05, L"海外・国際" },
	{ 0x00, 0x06, L"解説" },
	{ 0x00, 0x07, L"討論・会談" },
	{ 0x00, 0x08, L"報道特番" },
	{ 0x00, 0x09, L"ローカル・地域" },
	{ 0x00, 0x0A, L"交通" },
	{ 0x00, 0x0F, L"その他" },
	{ 0x01, 0x00, L"スポーツニュース" },
	{ 0x01, 0x01, L"野球" },
	{ 0x01, 0x02, L"サッカー" },
	{ 0x01, 0x03, L"ゴルフ" },
	{ 0x01, 0x04, L"その他の球技" },
	{ 0x01, 0x05, L"相撲・格闘技" },
	{ 0x01, 0x06, L"オリンピック・国際大会" },
	{ 0x01, 0x07, L"マラソン・陸上・水泳" },
	{ 0x01, 0x08, L"モータースポーツ" },
	{ 0x01, 0x09, L"マリン・ウィンタースポーツ" },
	{ 0x01, 0x0A, L"競馬・公営競技" },
	{ 0x01, 0x0F, L"その他" },
	{ 0x02, 0x00, L"芸能・ワイドショー" },
	{ 0x02, 0x01, L"ファッション" },
	{ 0x02, 0x02, L"暮らし・住まい" },
	{ 0x02, 0x03, L"健康・医療" },
	{ 0x02, 0x04, L"ショッピング・通販" },
	{ 0x02, 0x05, L"グルメ・料理" },
	{ 0x02, 0x06, L"イベント" },
	{ 0x02, 0x07, L"番組紹介・お知らせ" },
	{ 0x02, 0x0F, L"その他" },
	{ 0x03, 0x00, L"国内ドラマ" },
	{ 0x03, 0x01, L"海外ドラマ" },
	{ 0x03, 0x02, L"時代劇" },
	{ 0x03, 0x0F, L"その他" },
	{ 0x04, 0x00, L"国内ロック・ポップス" },
	{ 0x04, 0x01, L"海外ロック・ポップス" },
	{ 0x04, 0x02, L"クラシック・オペラ" },
	{ 0x04, 0x03, L"ジャズ・フュージョン" },
	{ 0x04, 0x04, L"歌謡曲・演歌" },
	{ 0x04, 0x05, L"ライブ・コンサート" },
	{ 0x04, 0x06, L"ランキング・リクエスト" },
	{ 0x04, 0x07, L"カラオケ・のど自慢" },
	{ 0x04, 0x08, L"民謡・邦楽" },
	{ 0x04, 0x09, L"童謡・キッズ" },
	{ 0x04, 0x0A, L"民族音楽・ワールドミュージック" },
	{ 0x04, 0x0F, L"その他" },
	{ 0x05, 0x00, L"クイズ" },
	{ 0x05, 0x01, L"ゲーム" },
	{ 0x05, 0x02, L"トークバラエティ" },
	{ 0x05, 0x03, L"お笑い・コメディ" },
	{ 0x05, 0x04, L"音楽バラエティ" },
	{ 0x05, 0x05, L"旅バラエティ" },
	{ 0x05, 0x06, L"料理バラエティ" },
	{ 0x05, 0x0F, L"その他" },
	{ 0x06, 0x00, L"洋画" },
	{ 0x06, 0x01, L"邦画" },
	{ 0x06, 0x02, L"アニメ" },
	{ 0x06, 0x0F, L"その他" },
	{ 0x07, 0x00, L"国内アニメ" },
	{ 0x07, 0x01, L"海外アニメ" },
	{ 0x07, 0x02, L"特撮" },
	{ 0x07, 0x0F, L"その他" },
	{ 0x08, 0x00, L"社会・時事" },
	{ 0x08, 0x01, L"歴史・紀行" },
	{ 0x08, 0x02, L"自然・動物・環境" },
	{ 0x08, 0x03, L"宇宙・科学・医学" },
	{ 0x08, 0x04, L"カルチャー・伝統文化" },
	{ 0x08, 0x05, L"文学・文芸" },
	{ 0x08, 0x06, L"スポーツ" },
	{ 0x08, 0x07, L"ドキュメンタリー全般" },
	{ 0x08, 0x08, L"インタビュー・討論" },
	{ 0x08, 0x0F, L"その他" },
	{ 0x09, 0x00, L"現代劇・新劇" },
	{ 0x09, 0x01, L"ミュージカル" },
	{ 0x09, 0x02, L"ダンス・バレエ" },
	{ 0x09, 0x03, L"落語・演芸" },
	{ 0x09, 0x04, L"歌舞伎・古典" },
	{ 0x09, 0x0F, L"その他" },
	{ 0x0A, 0x00, L"旅・釣り・アウトドア" },
	{ 0x0A, 0x01, L"園芸・ペット・手芸" },
	{ 0x0A, 0x02, L"音楽・美術・工芸" },
	{ 0x0A, 0x03, L"囲碁・将棋" },
	{ 0x0A, 0x04, L"麻雀・パチンコ" },
	{ 0x0A, 0x05, L"車・オートバイ" },
	{ 0x0A, 0x06, L"コンピュータ・ＴＶゲーム" },
	{ 0x0A, 0x07, L"会話・語学" },
	{ 0x0A, 0x08, L"幼児・小学生" },
	{ 0x0A, 0x09, L"中学生・高校生" },
	{ 0x0A, 0x0A, L"大学生・受験" },
	{ 0x0A, 0x0B, L"生涯教育・資格" },
	{ 0x0A, 0x0C, L"教育問題" },
	{ 0x0A, 0x0F, L"その他" },
	{ 0x0B, 0x00, L"高齢者" },
	{ 0x0B, 0x01, L"障害者" },
	{ 0x0B, 0x02, L"社会福祉" },
	{ 0x0B, 0x03, L"ボランティア" },
	{ 0x0B, 0x04, L"手話" },
	{ 0x0B, 0x05, L"文字（字幕）" },
	{ 0x0B, 0x06, L"音声解説" },
	{ 0x0E, 0x0F, L"その他" },
	{ 0x0E, 0x00, L"BS/地上デジタル放送用番組付属情報" },
	{ 0x0E, 0x01, L"広帯域CS デジタル放送用拡張" },
	{ 0x0E, 0x02, L"衛星デジタル音声放送用拡張" },
	{ 0x0E, 0x03, L"サーバー型番組付属情報" },
	{ 0x0E, 0x04, L"IP 放送用番組付属情報" }
};
//->コンテント記述子におけるジャンル指定

//////////////////////////////////////////////////////
//サービス形式種別
typedef struct _SERVICE_TYPE_TABLE{
	BYTE service_type;
	wstring name;
} SERVICE_TYPE_TABLE;

static SERVICE_TYPE_TABLE serviceTypeTable[]={
	{ 0x01, L"デジタルＴＶサービス" },
	{ 0x02, L"デジタル音声サービス" },
	{ 0xA1, L"臨時映像サービス" },
	{ 0xA2, L"臨時音声サービス" },
	{ 0xA3, L"臨時データサービス" },
	{ 0xA4, L"エンジニアリングサービス" },
	{ 0xA5, L"プロモーション映像サービス" },
	{ 0xA6, L"プロモーション音声サービス" },
	{ 0xA7, L"プロモーションデータサービス" },
	{ 0xA8, L"事前蓄積用データサービス" },
	{ 0xA9, L"蓄積専用データサービス" },
	{ 0xAA, L"ブックマーク一覧データサービス" },
	{ 0xAB, L"サーバー型サイマルサービス" },
	{ 0xAC, L"独立ファイルサービス" },
	{ 0xC0, L"データサービス" },
	{ 0xC1, L"TLVを用いた蓄積型サービス" }
};

#endif //DESCRIPTOR_DEF_H_IMPLEMENT_TABLE

//->サービス形式種別
#endif

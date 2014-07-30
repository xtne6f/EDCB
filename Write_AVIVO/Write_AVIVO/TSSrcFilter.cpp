#include "StdAfx.h"
#include "TSSrcFilter.h"

#include "../../Common/EpgTimerUtil.h"
#include "../../BonCtrl/PESHeadInfo.h"

const AMOVIESETUP_MEDIATYPE videoSubPinTypes =
{
    &MEDIATYPE_Video,       // Major type
    &MEDIASUBTYPE_MPEG2_VIDEO      // Minor type
};

const AMOVIESETUP_MEDIATYPE audioSubPinTypes =
{
    &MEDIATYPE_Audio,       // Major type
    &MEDIASUBTYPE_MPEG_ADTS_AAC      // Minor type
};

const AMOVIESETUP_PIN sudPins[] =
{
	{
		L"Video Out",
		FALSE,
		TRUE,
		FALSE,
		FALSE,
		&CLSID_NULL,
		NULL,
		1,
		&videoSubPinTypes
	},
	{
		L"Audio Out",
		FALSE,
		TRUE,
		FALSE,
		FALSE,
		&CLSID_NULL,
		NULL,
		1,
		&audioSubPinTypes
	}
};

const AMOVIESETUP_FILTER afFilterInfo=
{
    &CLSID_TSSrc,
    L"TSSrc",
    MERIT_DO_NOT_USE,
    2,
    sudPins
};

CPushVideoPin::CPushVideoPin(HRESULT *phr, CSource *pFilter)
	: CSourceStream(NAME("Push Source Video"), phr, pFilter, L"Video Out")
{
    AM_MEDIA_TYPE amTypeVideo;
    amTypeVideo.majortype = MEDIATYPE_Video;
    amTypeVideo.subtype = MEDIASUBTYPE_MPEG2_VIDEO;
    amTypeVideo.bFixedSizeSamples = TRUE;
    amTypeVideo.bTemporalCompression = 0;
    amTypeVideo.formattype = FORMAT_MPEG2Video;
    amTypeVideo.pUnk = NULL;
    amTypeVideo.cbFormat = 0;
    amTypeVideo.pbFormat = NULL;

	m_mt = amTypeVideo;

	this->buffLockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
}

CPushVideoPin::~CPushVideoPin() {
	if( WaitForSingleObject( this->buffLockEvent, 1000 ) == WAIT_OBJECT_0 ){
		for( size_t i=0; i<this->buffData.size(); i++ ){
			SAFE_DELETE(this->buffData[i]);
		}
		this->buffData.clear();
	}
	if( this->buffLockEvent != NULL ){
		CloseHandle(this->buffLockEvent);
		this->buffLockEvent = NULL;
	}
}

HRESULT CPushVideoPin::GetMediaType(CMediaType *pMediaType)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    CheckPointer(pMediaType, E_POINTER);

	CopyMediaType( pMediaType, &m_mt );

	return S_OK;
}

HRESULT CPushVideoPin::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pRequest) {
	HRESULT hr=NOERROR;
	pRequest->cBuffers=1;
    if(188*256 > (DWORD)pRequest->cbBuffer) {
        pRequest->cbBuffer=10*1024*1024;
    }
	// アロケータプロパティを設定しなおす
    ALLOCATOR_PROPERTIES Actual;
    hr=pAlloc->SetProperties(pRequest, &Actual);
    if(FAILED(hr)) {
        return hr;
    }
    if(Actual.cbBuffer < pRequest->cbBuffer) {
        return E_FAIL;
    }
    return S_OK;
}

HRESULT CPushVideoPin::DoBufferProcessingLoop(void) {
	Command com;
	OnThreadStartPlay();
	do {
		while (!CheckRequest(&com)) {
			HRESULT hr;
			CAutoLock lock(&m_cSharedState);

			if( WaitForSingleObject( this->buffLockEvent, 500 ) == WAIT_OBJECT_0 ){
				if( this->buffData.size() != 0 ){
					if( this->buffLockEvent != NULL ){
						SetEvent(this->buffLockEvent);
					}
					IMediaSample *pSample;
					hr = GetDeliveryBuffer(&pSample,NULL,NULL,0);
					if (FAILED(hr)) {
						Sleep(1);
						continue;
					}
					hr = FillBuffer(pSample);
					if (hr == S_OK) {
						hr = Deliver(pSample);
						pSample->Release();

						if(hr != S_OK)
						{
							return S_OK;
						}
					} else if (hr == S_FALSE) {
						pSample->Release();
						DeliverEndOfStream();
						return S_OK;
					} else {
						pSample->Release();
						DeliverEndOfStream();
						m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
						return hr;
					}
				}else{
					if( this->buffLockEvent != NULL ){
						SetEvent(this->buffLockEvent);
					}
					Sleep(10);
				}
			}
		}
		// For all commands sent to us there must be a Reply call!
		if (com == CMD_RUN || com == CMD_PAUSE) {
			Reply(NOERROR);
		} else if (com != CMD_STOP) {
			Reply((DWORD) E_UNEXPECTED);
		}
	} while (com != CMD_STOP);
	return S_FALSE;
}

HRESULT CPushVideoPin::FillBuffer(IMediaSample *pSample) {
	HRESULT hr=E_FAIL;
	CheckPointer(pSample,E_POINTER);
	// フレームに書き込み
	LPBYTE pSampleData=NULL;
	const long size=pSample->GetSize();
	pSample->GetPointer(&pSampleData);

	BUFF_DATA* data = NULL;
	if( WaitForSingleObject( this->buffLockEvent, 500 ) == WAIT_OBJECT_0 ){
		if( this->buffData.size() != 0 ){
			data = this->buffData[0];
			this->buffData.erase( this->buffData.begin() );
		}
		if( this->buffLockEvent != NULL ){
			SetEvent(this->buffLockEvent);
		}
	}
	if( data != NULL ){
		REFERENCE_TIME start_time=data->timeStamp;
		REFERENCE_TIME end_time=(start_time+1);
		pSample->SetTime(&start_time, &end_time);
		pSample->SetActualDataLength(data->size);
		memcpy(pSampleData, data->data, data->size);
		delete data;
	}else{
		return S_FALSE;
	}
	return S_OK;
}

void CPushVideoPin::AddData(BYTE* data, DWORD size, LONGLONG timeStamp)
{
	BUFF_DATA* buff = new BUFF_DATA;
	buff->size = size;
	buff->data = new BYTE[buff->size];
	buff->timeStamp = timeStamp;
	memcpy(buff->data, data, size);
	while(1){
		if( WaitForSingleObject( this->buffLockEvent, 500 ) == WAIT_OBJECT_0 ){
			if( this->buffData.size() < 100 ){
				this->buffData.push_back(buff);
				if( this->buffLockEvent != NULL ){
					SetEvent(this->buffLockEvent);
				}
				break;
			}else{
				if( this->buffLockEvent != NULL ){
					SetEvent(this->buffLockEvent);
				}
				Sleep(10);
			}
		}else{
			SAFE_DELETE(buff);
			break;
		}
	}
}

void CPushVideoPin::ClearData()
{
	if( WaitForSingleObject( this->buffLockEvent, 1000 ) == WAIT_OBJECT_0 ){
		for( size_t i=0; i<this->buffData.size(); i++ ){
			SAFE_DELETE(this->buffData[i]);
		}
		this->buffData.clear();
		if( this->buffLockEvent != NULL ){
			SetEvent(this->buffLockEvent);
		}
	}
}

CPushAudioPin::CPushAudioPin(HRESULT *phr, CSource *pFilter)
	: CSourceStream(NAME("Push Source Audio"), phr, pFilter, L"Audio Out")
{
    AM_MEDIA_TYPE amTypeAudio;
    amTypeAudio.majortype = MEDIATYPE_Audio;
	amTypeAudio.subtype = MEDIASUBTYPE_MPEG_ADTS_AAC;
    amTypeAudio.bFixedSizeSamples = TRUE;
    amTypeAudio.bTemporalCompression = 0;
    amTypeAudio.formattype = FORMAT_WaveFormatEx;
    amTypeAudio.pUnk = NULL;
    amTypeAudio.cbFormat = 0;
    amTypeAudio.pbFormat = NULL;

	m_mt = amTypeAudio;

	this->buffLockEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
}

CPushAudioPin::~CPushAudioPin() {
	if( WaitForSingleObject( this->buffLockEvent, 1000 ) == WAIT_OBJECT_0 ){
		for( size_t i=0; i<this->buffData.size(); i++ ){
			SAFE_DELETE(this->buffData[i]);
		}
		this->buffData.clear();
	}
	if( this->buffLockEvent != NULL ){
		CloseHandle(this->buffLockEvent);
		this->buffLockEvent = NULL;
	}
}

HRESULT CPushAudioPin::GetMediaType(CMediaType *pMediaType)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());

    CheckPointer(pMediaType, E_POINTER);

	CopyMediaType( pMediaType, &m_mt );

	return S_OK;
}

HRESULT CPushAudioPin::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pRequest) {
	HRESULT hr=NOERROR;
	pRequest->cBuffers=1;
    if(188*256 > (DWORD)pRequest->cbBuffer) {
        pRequest->cbBuffer=10*1024*1024;
    }
	// アロケータプロパティを設定しなおす
    ALLOCATOR_PROPERTIES Actual;
    hr=pAlloc->SetProperties(pRequest, &Actual);
    if(FAILED(hr)) {
        return hr;
    }
    if(Actual.cbBuffer < pRequest->cbBuffer) {
        return E_FAIL;
    }
    return S_OK;
}

HRESULT CPushAudioPin::DoBufferProcessingLoop(void) {
	Command com;
	OnThreadStartPlay();
	do {
		while (!CheckRequest(&com)) {
			HRESULT hr;
			CAutoLock lock(&m_cSharedState);

			if( WaitForSingleObject( this->buffLockEvent, 500 ) == WAIT_OBJECT_0 ){
				if( this->buffData.size() != 0 ){
					if( this->buffLockEvent != NULL ){
						SetEvent(this->buffLockEvent);
					}
					IMediaSample *pSample;
					hr = GetDeliveryBuffer(&pSample,NULL,NULL,0);
					if (FAILED(hr)) {
						Sleep(1);
						continue;
					}
					hr = FillBuffer(pSample);
					if (hr == S_OK) {
						hr = Deliver(pSample);
						pSample->Release();

						if(hr != S_OK)
						{
							return S_OK;
						}
					} else if (hr == S_FALSE) {
						pSample->Release();
						DeliverEndOfStream();
						return S_OK;
					} else {
						pSample->Release();
						DeliverEndOfStream();
						m_pFilter->NotifyEvent(EC_ERRORABORT, hr, 0);
						return hr;
					}
				}else{
					if( this->buffLockEvent != NULL ){
						SetEvent(this->buffLockEvent);
					}
					Sleep(10);
				}
			}
		}
		// For all commands sent to us there must be a Reply call!
		if (com == CMD_RUN || com == CMD_PAUSE) {
			Reply(NOERROR);
		} else if (com != CMD_STOP) {
			Reply((DWORD) E_UNEXPECTED);
		}
	} while (com != CMD_STOP);
	return S_FALSE;
}

HRESULT CPushAudioPin::FillBuffer(IMediaSample *pSample) {
	HRESULT hr=E_FAIL;
	CheckPointer(pSample,E_POINTER);
	// フレームに書き込み
	LPBYTE pSampleData=NULL;
	const long size=pSample->GetSize();
	pSample->GetPointer(&pSampleData);

	BUFF_DATA* data = NULL;
	if( WaitForSingleObject( this->buffLockEvent, 500 ) == WAIT_OBJECT_0 ){
		if( this->buffData.size() != 0 ){
			data = this->buffData[0];
			this->buffData.erase( this->buffData.begin() );
		}
		if( this->buffLockEvent != NULL ){
			SetEvent(this->buffLockEvent);
		}
	}
	if( data != NULL ){
		REFERENCE_TIME start_time=data->timeStamp;
		REFERENCE_TIME end_time=(start_time+1);
		pSample->SetTime(&start_time, &end_time);
		pSample->SetActualDataLength(data->size);
		memcpy(pSampleData, data->data, data->size);
		delete data;
	}else{
		return S_FALSE;
	}

	return S_OK;
}

void CPushAudioPin::AddData(BYTE* data, DWORD size, LONGLONG timeStamp)
{
	BUFF_DATA* buff = new BUFF_DATA;
	buff->size = size;
	buff->data = new BYTE[buff->size];
	buff->timeStamp = timeStamp;
	memcpy(buff->data, data, size);
	while(1){
		if( WaitForSingleObject( this->buffLockEvent, 500 ) == WAIT_OBJECT_0 ){
			if( this->buffData.size() < 100 ){
				this->buffData.push_back(buff);
				if( this->buffLockEvent != NULL ){
					SetEvent(this->buffLockEvent);
				}
				break;
			}else{
				if( this->buffLockEvent != NULL ){
					SetEvent(this->buffLockEvent);
				}
				Sleep(10);
			}
		}else{
			SAFE_DELETE(buff);
			break;
		}
	}
}

void CPushAudioPin::ClearData()
{
	if( WaitForSingleObject( this->buffLockEvent, 1000 ) == WAIT_OBJECT_0 ){
		for( size_t i=0; i<this->buffData.size(); i++ ){
			SAFE_DELETE(this->buffData[i]);
		}
		this->buffData.clear();
		if( this->buffLockEvent != NULL ){
			SetEvent(this->buffLockEvent);
		}
	}
}

CTSSrcFilter::CTSSrcFilter(IUnknown *pUnk, HRESULT *phr)
	:CSource(NAME("TSSrc"), pUnk, CLSID_TSSrc)
	,videoBuff(TRUE)
	,audioBuff(TRUE)
{
	m_pVideoPin = new CPushVideoPin(phr, this);
	m_pAudioPin = new CPushAudioPin(phr, this);

	m_iPins = 2;

    if (phr)
    {
        if (m_pVideoPin == NULL || m_pAudioPin == NULL)
            *phr = E_OUTOFMEMORY;
        else
            *phr = S_OK;
    }

	this->videoPID = 0xFFFF;
	this->audioPID = 0xFFFF;
	this->PCR_PID = 0xFFFF;
	this->startTS = 0;
	this->startTSV = 0;
	this->startTSA = 0;
}


CTSSrcFilter::~CTSSrcFilter(void)
{
}

CUnknown * WINAPI CTSSrcFilter::CreateInstance(IUnknown *pUnk, HRESULT *phr)
{
    CTSSrcFilter *pNewFilter = new CTSSrcFilter(pUnk, phr );

    if (phr)
    {
        if (pNewFilter == NULL) 
            *phr = E_OUTOFMEMORY;
        else
            *phr = S_OK;
    }

    return pNewFilter;
}

DWORD CTSSrcFilter::AddTS(BYTE* data, DWORD size)
{
	DWORD addVideo = 0;
	for( DWORD i=0; i<size; i+=188 ){
		CTSPacketUtil packet;
		if( packet.Set188TS(data + i, 188) == TRUE ){
			if( packet.transport_scrambling_control == 0 ){
				//PMT
				map<WORD, CPMTUtil*>::iterator itrPmt;
				if( packet.payload_unit_start_indicator == 1 && packet.data_byteSize > 0){
					BYTE pointer = packet.data_byte[0];
					if( pointer+1 < packet.data_byteSize ){
						if( packet.data_byte[1+pointer] == 0x02 ){
							//PMT
							map<WORD, CPMTUtil*>::iterator itrPmt;
							itrPmt = this->pmtUtilMap.find(packet.PID);
							if( itrPmt == this->pmtUtilMap.end() ){
								CPMTUtil* util = new CPMTUtil;
								this->pmtUtilMap.insert(pair<WORD, CPMTUtil*>(packet.PID, util));
								if( util->AddPacket(&packet) == TRUE ){
									CheckPID();
								}
							}else{
								if( itrPmt->second->AddPacket(&packet) == TRUE ){
									CheckPID();
								}
							}
						}
					}
				}else{
					//PMTの2パケット目かチェック
					map<WORD, CPMTUtil*>::iterator itrPmt;
					itrPmt = this->pmtUtilMap.find(packet.PID);
					if( itrPmt != this->pmtUtilMap.end() ){
						if( itrPmt->second->AddPacket(&packet) == TRUE ){
							CheckPID();
						}
					}
				}
				if( packet.PID == videoPID ){
					videoBuff.Add188TS(&packet);
					BYTE* buff = NULL;
					DWORD buffSize = 0;
					BOOL ret = videoBuff.GetSectionBuff(&buff, &buffSize);
					if( ret == TRUE ){
						CPESHeadInfo info;
						info.SetData(buff, buffSize);
						LONGLONG pts = 0;
						LONGLONG time = 0;
						if( info.PTS_DTS_flags == 3 ){
							if(this->startTSV == 0){
								this->startTSV = info.DTS;
							}
							if( this->startTS == 0 ){
								this->startTS = this->startTSV;
							}
							pts = info.DTS;
						}else if( info.PTS_DTS_flags == 2 ){
							if(this->startTSV == 0){
								this->startTSV = info.PTS;
							}
							if( this->startTS == 0 ){
								this->startTS = this->startTSV;
							}
							pts = info.PTS;
						}
						if( this->startTSV > pts ){
							pts += 0x1FFFFFFFF;
						}
						time = ((pts-this->startTS)/90)*10000;
						if( info.PES_packet_data_byte_size > 0 && time >= 0){
							this->m_pVideoPin->AddData(info.PES_packet_data_byte, info.PES_packet_data_byte_size, time);
							addVideo++;
						}
					}
				}else if( packet.PID == audioPID ){
					audioBuff.Add188TS(&packet);
					BYTE* buff = NULL;
					DWORD buffSize = 0;
					BOOL ret = audioBuff.GetSectionBuff(&buff, &buffSize);
					if( ret == TRUE ){
						CPESHeadInfo info;
						info.SetData(buff, buffSize);
						LONGLONG pts = 0;
						LONGLONG time = 0;
						if( info.PTS_DTS_flags == 3 ){
							if(this->startTSA == 0){
								this->startTSA = info.DTS;
							}
							if( this->startTS == 0 ){
								this->startTS = this->startTSV;
							}
							pts = info.DTS;
						}else if( info.PTS_DTS_flags == 2 ){
							if(this->startTSA == 0){
								this->startTSA = info.PTS;
							}
							if( this->startTS == 0 ){
								this->startTS = this->startTSV;
							}
							pts = info.PTS;
						}
						if( this->startTSA > pts ){
							pts += 0x1FFFFFFFF;
						}
						time = ((pts-this->startTS)/90)*10000;
						if( info.PES_packet_data_byte_size > 0 && time >= 0){
							this->m_pAudioPin->AddData(info.PES_packet_data_byte, info.PES_packet_data_byte_size, time);
						}
					}
				}
			}
		}
	}
	return addVideo;
}

void CTSSrcFilter::ClearData()
{
	this->m_pVideoPin->ClearData();
	this->m_pAudioPin->ClearData();
	this->videoPID = 0xFFFF;
	this->audioPID = 0xFFFF;
	this->PCR_PID = 0xFFFF;
	this->startTS = 0;
	this->startTSV = 0;
	this->startTSA = 0;
}

void CTSSrcFilter::CheckPID()
{
	map<WORD, CPMTUtil*>::iterator itrPmt;
	itrPmt = pmtUtilMap.begin();

	map<WORD,WORD>::iterator itrPID;
	for( itrPID = itrPmt->second->PIDList.begin(); itrPID != itrPmt->second->PIDList.end(); itrPID++ ){
		if( itrPID->second == 0x02 || itrPID->second == 0x1B ){
			videoPID = itrPID->first;
		}else if( itrPID->second == 0x0F || itrPID->second == 0x04 ){
			audioPID = itrPID->first;
		}
	}
	this->PCR_PID = itrPmt->second->PCR_PID;
}

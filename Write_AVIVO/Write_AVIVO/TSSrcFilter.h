#pragma once

#include "../../BonCtrl/PMTUtil.h"
#include <list>

// {5DD7D1CB-29BD-420E-ABD4-21FAD67F665C}
DEFINE_GUID(CLSID_TSSrc, 
0x5dd7d1cb, 0x29bd, 0x420e, 0xab, 0xd4, 0x21, 0xfa, 0xd6, 0x7f, 0x66, 0x5c);

typedef struct _BUFF_DATA{
	vector<BYTE> data;			//TSデータ
	LONGLONG timeStamp;
} BUFF_DATA;

class CPushVideoPin : public CSourceStream
{
protected:
    CCritSec m_cSharedState;            // Protects our internal state
	HANDLE buffLockEvent;
	std::list<BUFF_DATA> buffData;
public:
	DECLARE_IUNKNOWN
    CPushVideoPin(HRESULT *phr, CSource *pFilter);
    ~CPushVideoPin();

    // Override the version that offers exactly one media type
    HRESULT GetMediaType(CMediaType *pMediaType);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
    HRESULT FillBuffer(IMediaSample *pSample);
	HRESULT DoBufferProcessingLoop(void);
    
    // Quality control
	// Not implemented because we aren't going in real time.
	// If the file-writing filter slows the graph down, we just do nothing, which means
	// wait until we're unblocked. No frames are ever dropped.
    STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
    {
        return E_FAIL;
    }

	void AddData(BYTE* data, DWORD size, LONGLONG timeStamp);
	void ClearData();
};

class CPushAudioPin : public CSourceStream
{
protected:
    CCritSec m_cSharedState;            // Protects our internal state
	HANDLE buffLockEvent;
	std::list<BUFF_DATA> buffData;
public:
	DECLARE_IUNKNOWN
    CPushAudioPin(HRESULT *phr, CSource *pFilter);
    ~CPushAudioPin();

    // Override the version that offers exactly one media type
    HRESULT GetMediaType(CMediaType *pMediaType);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pRequest);
    HRESULT FillBuffer(IMediaSample *pSample);
	HRESULT DoBufferProcessingLoop(void);
    
    // Quality control
	// Not implemented because we aren't going in real time.
	// If the file-writing filter slows the graph down, we just do nothing, which means
	// wait until we're unblocked. No frames are ever dropped.
    STDMETHODIMP Notify(IBaseFilter *pSelf, Quality q)
    {
        return E_FAIL;
    }

	void AddData(BYTE* data, DWORD size, LONGLONG timeStamp);
	void ClearData();
};

class CTSSrcFilter : public CSource
{
private:
    // Constructor is private because you have to use CreateInstance
    CTSSrcFilter(IUnknown *pUnk, HRESULT *phr);
    ~CTSSrcFilter();

    CPushVideoPin *m_pVideoPin;
	CPushAudioPin *m_pAudioPin;

	map<WORD, CPMTUtil> pmtUtilMap; //キーPMTのPID
	WORD videoPID;
	WORD audioPID;
	WORD PCR_PID;
	LONGLONG startTS;
	LONGLONG startTSV;
	LONGLONG startTSA;

	CTSBuffUtil videoBuff;
	CTSBuffUtil audioBuff;
public:
	DWORD AddTS(BYTE* data, DWORD size);
	void CheckPID();
	void ClearData();

    static CUnknown * WINAPI CreateInstance(IUnknown *pUnk, HRESULT *phr);
};


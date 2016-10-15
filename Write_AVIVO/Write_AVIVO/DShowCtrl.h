#pragma once

#include "TSSrcFilter.h"

#define SAFE_RELEASE(p) { if(p) { (p)->Release(); (p)=NULL; } }

// Microsoft DTV-DVD Video Decoder
DEFINE_GUID(IID_MSVideoDec, 
0x212690fb, 0x83e5, 0x4526, 0x8f, 0xd7, 0x74, 0x47, 0x8b, 0x79, 0x39, 0xcd);

// Microsoft DTV-DVD Audio Decoder
DEFINE_GUID(IID_MSAudioDec, 
0xe1f1a0b8, 0xbeee, 0x490d, 0xba, 0x7c, 0x06, 0x6c, 0x40, 0xb5, 0xe2, 0xb9);

// ATI Video Scaler Filter
DEFINE_GUID(IID_ATIVideoScaler, 
0x8da0a2a8, 0x30cb, 0x46f4, 0xa2, 0x8e, 0x13, 0xb6, 0xb5, 0xab, 0x92, 0x6c);

// ATI MPEG Video Encoder
DEFINE_GUID(IID_ATIVideoEnc, 
0x758c0f02, 0xdf95, 0x11d2, 0x8e, 0x75, 0x00, 0x10, 0x4b, 0x93, 0xcf, 0x06);

// ATI MPEG Audio Encoder
DEFINE_GUID(IID_ATIAudioEnc, 
0x6467dd70, 0xfbd5, 0x11d2, 0xb5, 0xb6, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00);

// ATI MPEG Multiplexer
DEFINE_GUID(IID_ATIMuxer, 
0x7a10e1e1, 0xf430, 0x11d2, 0x8e, 0x75, 0x00, 0x10, 0x4b, 0x93, 0xcf, 0x06);

// ATI MPEG File Writer
DEFINE_GUID(IID_ATIFileWriter, 
0x37a4d802, 0xe76c, 0x11d2, 0x93, 0x5c, 0x00, 0xa0, 0x24, 0xe5, 0x26, 0x61);

class CDShowCtrl
{
public:
	CDShowCtrl(void);
	~CDShowCtrl(void);

	BOOL CreateNewGraph();
	BOOL LoadGraph(LPCWSTR filePath);
	BOOL SaveGraph(LPCWSTR filePath);

	BOOL SetVideoScaleProp();
	BOOL SetVideoEncProp();
	BOOL SetAudioEncProp();
	BOOL SetMuxerProp();

	BOOL CreateRunGraph(LPCWSTR writeFilePath);
	BOOL ReConnectScale();
	BOOL DeleteGraph();

	BOOL RunGraph();
	BOOL StopGraph();
	BOOL AddTS(BYTE* data, DWORD size);
	BOOL IsRun();

protected:
	DWORD m_dwRegister;
	IGraphBuilder* graph;
	IMediaControl* mediaCtrl;
	BOOL runFlag;

	IBaseFilter* bonSrc;

	IBaseFilter* videoDec;
	IBaseFilter* audioDec;

	IBaseFilter* videoScaler;
	IBaseFilter* videoEnc;
	IBaseFilter* audioEnc;
	IBaseFilter* muxer;
	IBaseFilter* writeFile;

	CTSSrcFilter* tsSrc;

	BOOL preCreateFlag;
	DWORD preCount;
	std::list<vector<BYTE>> buffData;
protected:
	HRESULT AddGraphToRot(
		IUnknown *pUnkGraph, 
		DWORD *pdwRegister
		);

	void RemoveGraphFromRot(
		DWORD pdwRegister
		);

	HRESULT SaveGraphFile(IGraphBuilder *pGraph, const WCHAR *wszPath);
	HRESULT LoadGraphFile(IGraphBuilder *pGraph, const WCHAR* wszName);

	HRESULT ConnectFilters(
		IGraphBuilder* pGraphBuilder,
		IBaseFilter* pFilterUpstream, 
		IBaseFilter* pFilterDownstream
    );
	HRESULT DisconnectFilter(
		IGraphBuilder* pGraphBuilder,
		IBaseFilter* pFilter
		);
};

#ifndef __ATLBASE_H__
template <class T> class CComPtr
{
public:
	CComPtr() : p(NULL) {}
	CComPtr(T *lp) : p(lp) { if (p) p->AddRef(); }
	CComPtr(const CComPtr<T> &other) { p = other.p; if (p) p->AddRef(); }
	~CComPtr() { Release(); }
	T *operator=(T *lp) { Attach(lp); if (p) p->AddRef(); return p; }
	T *operator=(const CComPtr<T> &other) { return *this = other.p; }
	bool operator==(T *lp) const { return p == lp; }
	operator T*() const { return p; }
	T **operator&() { _ASSERT(!p); return &p; }
	T *operator->() const { _ASSERT(p); return p; }
	void Release() { if (p) p->Release(); p = NULL; }
	void Attach(T *lp) { Release(); p = lp; }
	HRESULT CopyTo(T **lpp) { _ASSERT(lpp); if (!lpp) return E_POINTER; *lpp = p; if (p) p->AddRef(); return S_OK; }
	T *p;
};
#endif

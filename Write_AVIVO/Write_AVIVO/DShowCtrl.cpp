#include "StdAfx.h"
#include "DShowCtrl.h"


CDShowCtrl::CDShowCtrl(void)
{
	CoInitialize(NULL);
	this->graph = NULL;
	this->mediaCtrl = NULL;
	this->tsSrc = NULL;
	this->bonSrc = NULL;
	this->videoDec = NULL;
	this->audioDec = NULL;
	this->writeFile = NULL;
	this->videoScaler = NULL;
	this->videoEnc = NULL;
	this->audioEnc = NULL;
	this->muxer = NULL;

	this->runFlag = FALSE;

	this->preCreateFlag = FALSE;
	this->preCount = 0;
}


CDShowCtrl::~CDShowCtrl(void)
{
	RemoveGraphFromRot(m_dwRegister);
	if( this->mediaCtrl != NULL ){
		this->mediaCtrl->Stop();
	}
	SAFE_RELEASE(this->videoDec);
	SAFE_RELEASE(this->audioDec);
	SAFE_RELEASE(this->writeFile);
	SAFE_RELEASE(this->videoScaler);
	SAFE_RELEASE(this->videoEnc);
	SAFE_RELEASE(this->audioEnc);
	SAFE_RELEASE(this->muxer);
	SAFE_RELEASE(this->mediaCtrl);
	SAFE_RELEASE(this->graph);
	SAFE_RELEASE(this->tsSrc);

	CoUninitialize();
}

// Adds a DirectShow filter graph to the Running Object Table,
// allowing GraphEdit to "spy" on a remote filter graph.
HRESULT CDShowCtrl::AddGraphToRot(
		IUnknown *pUnkGraph, 
		DWORD *pdwRegister
		) 
{
	CComPtr <IMoniker>              pMoniker;
	CComPtr <IRunningObjectTable>   pROT;
	WCHAR wsz[128];
	HRESULT hr;

	if (FAILED(GetRunningObjectTable(0, &pROT)))
		return E_FAIL;

	wsprintfW(wsz, L"FilterGraph %08x pid %08x\0", (DWORD_PTR) pUnkGraph, 
			GetCurrentProcessId());

	hr = CreateItemMoniker(L"!", wsz, &pMoniker);
	if (SUCCEEDED(hr)) 
		hr = pROT->Register(ROTFLAGS_REGISTRATIONKEEPSALIVE, pUnkGraph, 
							pMoniker, pdwRegister);
        
	return hr;
}

// Removes a filter graph from the Running Object Table
void CDShowCtrl::RemoveGraphFromRot(
		DWORD pdwRegister
		)
{
	CComPtr <IRunningObjectTable> pROT;

	if (SUCCEEDED(GetRunningObjectTable(0, &pROT))) 
		pROT->Revoke(pdwRegister);

}

BOOL CDShowCtrl::CreateNewGraph()
{
	if( this->graph != NULL ){
		return FALSE;
	}
	this->preCreateFlag = FALSE;
	BOOL ret = TRUE;

	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, 
		IID_IGraphBuilder, (void**)&this->graph );
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = CoCreateInstance(IID_ATIVideoScaler, NULL, CLSCTX_INPROC_SERVER, 
		IID_IBaseFilter, (void**)&this->videoScaler );
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = CoCreateInstance(IID_ATIVideoEnc, NULL, CLSCTX_INPROC_SERVER, 
		IID_IBaseFilter, (void**)&this->videoEnc );
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = CoCreateInstance(IID_ATIAudioEnc, NULL, CLSCTX_INPROC_SERVER, 
		IID_IBaseFilter, (void**)&this->audioEnc );
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}
	
	hr = CoCreateInstance(IID_ATIMuxer, NULL, CLSCTX_INPROC_SERVER, 
		IID_IBaseFilter, (void**)&this->muxer );
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	this->graph->AddFilter(this->videoScaler, L"ATI Video Scaler Filter");
	this->graph->AddFilter(this->videoEnc, L"ATI MPEG Video Encoder");
	this->graph->AddFilter(this->audioEnc, L"ATI MPEG Audio Encoder");
	this->graph->AddFilter(this->muxer, L"ATI MPEG Multiplexer");

	return TRUE;
ErrEnd:
	SAFE_RELEASE(this->videoScaler);
	SAFE_RELEASE(this->videoEnc);
	SAFE_RELEASE(this->audioEnc);
	SAFE_RELEASE(this->muxer);
	SAFE_RELEASE(this->graph);

	return ret;
}

BOOL CDShowCtrl::LoadGraph(LPCWSTR filePath)
{
	if( this->graph != NULL ){
		return FALSE;
	}
	this->preCreateFlag = FALSE;

	BOOL ret = TRUE;

	HRESULT hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, 
		IID_IGraphBuilder, (void**)&this->graph ); 
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}
	hr = LoadGraphFile(this->graph, filePath);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = this->graph->FindFilterByName(L"ATI Video Scaler Filter", &this->videoScaler);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}
	hr = this->graph->FindFilterByName(L"ATI MPEG Video Encoder", &this->videoEnc);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}
	hr = this->graph->FindFilterByName(L"ATI MPEG Audio Encoder", &this->audioEnc);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}
	hr = this->graph->FindFilterByName(L"ATI MPEG Multiplexer", &this->muxer);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	return TRUE;

ErrEnd:
	SAFE_RELEASE(this->videoScaler);
	SAFE_RELEASE(this->videoEnc);
	SAFE_RELEASE(this->audioEnc);
	SAFE_RELEASE(this->muxer);
	SAFE_RELEASE(this->graph);

	return ret;
}

BOOL CDShowCtrl::SaveGraph(LPCWSTR filePath)
{
	if( this->graph == NULL ){
		return FALSE;
	}
	BOOL ret = TRUE;

	HRESULT hr = SaveGraphFile(this->graph, filePath);
	if (FAILED(hr)){
		return FALSE;
	}

	return ret;
}

HRESULT CDShowCtrl::SaveGraphFile(IGraphBuilder *pGraph, const WCHAR *wszPath) 
{
	const WCHAR wszStreamName[] = L"ActiveMovieGraph"; 
	HRESULT hr;
    
	IStorage *pStorage = NULL;
	hr = StgCreateDocfile(
		wszPath,
		STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
		0, &pStorage);
	if(FAILED(hr)) 
	{
		return hr;
	}

	IStream *pStream;
	hr = pStorage->CreateStream(
		wszStreamName,
		STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
		0, 0, &pStream);
	if (FAILED(hr)) 
	{
		pStorage->Release();    
		return hr;
	}

	IPersistStream *pPersist = NULL;
	pGraph->QueryInterface(IID_IPersistStream, (void**)&pPersist);
	hr = pPersist->Save(pStream, TRUE);
	pStream->Release();
	pPersist->Release();
	if (SUCCEEDED(hr)) 
	{
		hr = pStorage->Commit(STGC_DEFAULT);
	}
	pStorage->Release();
	return hr;
}

HRESULT CDShowCtrl::LoadGraphFile(IGraphBuilder *pGraph, const WCHAR* wszName)
{
    IStorage *pStorage = 0;
    if (S_OK != StgIsStorageFile(wszName))
    {
        return E_FAIL;
    }
    HRESULT hr = StgOpenStorage(wszName, 0, 
        STGM_TRANSACTED | STGM_READ | STGM_SHARE_DENY_WRITE, 
        0, 0, &pStorage);
    if (FAILED(hr))
    {
        return hr;
    }
    IPersistStream *pPersistStream = 0;
    hr = pGraph->QueryInterface(IID_IPersistStream,
             reinterpret_cast<void**>(&pPersistStream));
    if (SUCCEEDED(hr))
    {
        IStream *pStream = 0;
        hr = pStorage->OpenStream(L"ActiveMovieGraph", 0, 
            STGM_READ | STGM_SHARE_EXCLUSIVE, 0, &pStream);
        if(SUCCEEDED(hr))
        {
            hr = pPersistStream->Load(pStream);
            pStream->Release();
        }
        pPersistStream->Release();
    }
    pStorage->Release();
    return hr;
}

BOOL CDShowCtrl::SetVideoScaleProp()
{
	if( this->videoScaler == NULL ){
		return FALSE;
	}
	BOOL ret = TRUE;

	ISpecifyPropertyPages *prop = NULL;
	HRESULT hr = this->videoScaler->QueryInterface(IID_ISpecifyPropertyPages, (void**)&prop);
    if (FAILED(hr)){
		return FALSE;
	}

	FILTER_INFO FilterInfo;
	this->videoScaler->QueryFilterInfo(&FilterInfo);

	CAUUID caGUID;
	prop->GetPages(&caGUID);

	OleCreatePropertyFrame(
		NULL,                   // 親ウィンドウ
		0,                      // x (予約済み)
		0,                      // y (予約済み)
		FilterInfo.achName,     // ダイアログ ボックスのキャプション
		1,                      // フィルタの数
		(IUnknown **)&this->videoScaler,  // フィルタへのポインタ
		caGUID.cElems,          // プロパティ ページの数
		caGUID.pElems,          // プロパティ ページ CLSID へのポインタ
		0,                      // ロケール識別子
		0,                      // 予約済み
		NULL                    // 予約済み
	);
	CoTaskMemFree(caGUID.pElems);

	SAFE_RELEASE(prop);

	return ret;
}

BOOL CDShowCtrl::SetVideoEncProp()
{
	if( this->videoEnc == NULL ){
		return FALSE;
	}
	BOOL ret = TRUE;

	ISpecifyPropertyPages *prop = NULL;
	HRESULT hr = this->videoEnc->QueryInterface(IID_ISpecifyPropertyPages, (void**)&prop);
    if (FAILED(hr)){
		return FALSE;
	}

	FILTER_INFO FilterInfo;
	this->videoEnc->QueryFilterInfo(&FilterInfo);

	CAUUID caGUID;
	prop->GetPages(&caGUID);

	OleCreatePropertyFrame(
		NULL,                   // 親ウィンドウ
		0,                      // x (予約済み)
		0,                      // y (予約済み)
		FilterInfo.achName,     // ダイアログ ボックスのキャプション
		1,                      // フィルタの数
		(IUnknown **)&this->videoEnc,  // フィルタへのポインタ
		caGUID.cElems,          // プロパティ ページの数
		caGUID.pElems,          // プロパティ ページ CLSID へのポインタ
		0,                      // ロケール識別子
		0,                      // 予約済み
		NULL                    // 予約済み
	);
	CoTaskMemFree(caGUID.pElems);

	SAFE_RELEASE(prop);

	return ret;
}

BOOL CDShowCtrl::SetAudioEncProp()
{
	if( this->audioEnc == NULL ){
		return FALSE;
	}
	BOOL ret = TRUE;

	ISpecifyPropertyPages *prop = NULL;
	HRESULT hr = this->audioEnc->QueryInterface(IID_ISpecifyPropertyPages, (void**)&prop);
    if (FAILED(hr)){
		return FALSE;
	}

	FILTER_INFO FilterInfo;
	this->audioEnc->QueryFilterInfo(&FilterInfo);

	CAUUID caGUID;
	prop->GetPages(&caGUID);

	OleCreatePropertyFrame(
		NULL,                   // 親ウィンドウ
		0,                      // x (予約済み)
		0,                      // y (予約済み)
		FilterInfo.achName,     // ダイアログ ボックスのキャプション
		1,                      // フィルタの数
		(IUnknown **)&this->audioEnc,  // フィルタへのポインタ
		caGUID.cElems,          // プロパティ ページの数
		caGUID.pElems,          // プロパティ ページ CLSID へのポインタ
		0,                      // ロケール識別子
		0,                      // 予約済み
		NULL                    // 予約済み
	);
	CoTaskMemFree(caGUID.pElems);

	SAFE_RELEASE(prop);

	return ret;
}

BOOL CDShowCtrl::SetMuxerProp()
{
	if( this->muxer == NULL ){
		return FALSE;
	}
	BOOL ret = TRUE;

	ISpecifyPropertyPages *prop = NULL;
	HRESULT hr = this->muxer->QueryInterface(IID_ISpecifyPropertyPages, (void**)&prop);
    if (FAILED(hr)){
		return FALSE;
	}

	FILTER_INFO FilterInfo;
	this->muxer->QueryFilterInfo(&FilterInfo);

	CAUUID caGUID;
	prop->GetPages(&caGUID);

	OleCreatePropertyFrame(
		NULL,                   // 親ウィンドウ
		0,                      // x (予約済み)
		0,                      // y (予約済み)
		FilterInfo.achName,     // ダイアログ ボックスのキャプション
		1,                      // フィルタの数
		(IUnknown **)&this->muxer,  // フィルタへのポインタ
		caGUID.cElems,          // プロパティ ページの数
		caGUID.pElems,          // プロパティ ページ CLSID へのポインタ
		0,                      // ロケール識別子
		0,                      // 予約済み
		NULL                    // 予約済み
	);
	CoTaskMemFree(caGUID.pElems);

	SAFE_RELEASE(prop);

	return ret;
}

BOOL CDShowCtrl::CreateRunGraph(LPCWSTR writeFilePath)
{
	this->runFlag = FALSE;
	if( this->graph == NULL ){
		return FALSE;
	}
	BOOL ret = TRUE;

	AddGraphToRot(this->graph, &m_dwRegister);
	HRESULT hr;

	this->buffData.clear();

	tsSrc = static_cast<CTSSrcFilter *>(CTSSrcFilter::CreateInstance(NULL, &hr));
	hr = tsSrc->QueryInterface(IID_IBaseFilter, reinterpret_cast<void**>(&this->bonSrc));
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = CoCreateInstance(IID_MSVideoDec, NULL, CLSCTX_INPROC_SERVER, 
		IID_IBaseFilter, (void**)&this->videoDec );
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = CoCreateInstance(IID_MSAudioDec, NULL, CLSCTX_INPROC_SERVER, 
		IID_IBaseFilter, (void**)&this->audioDec );
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}
	
	hr = CoCreateInstance(IID_ATIFileWriter, NULL, CLSCTX_INPROC_SERVER, 
		IID_IBaseFilter, (void**)&this->writeFile );
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	this->graph->AddFilter(this->bonSrc, L"TSSrc");
	this->graph->AddFilter(this->videoDec, L"Microsoft DTV-DVD Video Decoder");
	this->graph->AddFilter(this->audioDec, L"Microsoft DTV-DVD Audio Decoder");
	this->graph->AddFilter(this->writeFile, L"ATI MPEG File Writer");

	IFileSinkFilter2 *sink = 0;
	hr = this->writeFile->QueryInterface(IID_IFileSinkFilter2, (void**)&sink);
	hr = sink->SetFileName(writeFilePath, NULL);
	hr = sink->SetMode(AM_FILE_OVERWRITE);
	SAFE_RELEASE(sink);

	//pinのコネクト
	hr = ConnectFilters(this->graph, this->bonSrc, this->videoDec);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = ConnectFilters(this->graph, this->bonSrc, this->audioDec);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}
	
	hr = ConnectFilters(this->graph, this->videoDec, this->videoScaler);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = ConnectFilters(this->graph, this->videoScaler, this->videoEnc);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = ConnectFilters(this->graph, this->audioDec, this->audioEnc);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}
	
	hr = ConnectFilters(this->graph, this->videoEnc, this->muxer);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = ConnectFilters(this->graph, this->audioEnc, this->muxer);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = ConnectFilters(this->graph, this->muxer, this->writeFile);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}
	
	hr = this->graph->QueryInterface(IID_IMediaControl, (void **)&this->mediaCtrl);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}
	this->preCreateFlag = TRUE;
	this->preCount = 0;

	return TRUE;
ErrEnd:
	SAFE_RELEASE(this->bonSrc);
	SAFE_RELEASE(this->tsSrc);
	SAFE_RELEASE(this->videoDec);
	SAFE_RELEASE(this->audioDec);
	SAFE_RELEASE(this->writeFile);
	SAFE_RELEASE(this->videoScaler);
	SAFE_RELEASE(this->videoEnc);
	SAFE_RELEASE(this->audioEnc);
	SAFE_RELEASE(this->muxer);
	SAFE_RELEASE(this->graph);

	return ret;
}

BOOL CDShowCtrl::ReConnectScale()
{
	if( this->graph == NULL ||
		this->videoDec == NULL ||
		this->audioDec == NULL ||
		this->writeFile == NULL ||
		this->videoScaler == NULL ||
		this->videoEnc == NULL ||
		this->audioEnc == NULL ||
		this->muxer == NULL
		){
		return FALSE;
	}
	BOOL ret = TRUE;

	this->DisconnectFilter(this->graph, this->videoScaler);
	this->DisconnectFilter(this->graph, this->videoEnc);
	this->DisconnectFilter(this->graph, this->audioEnc);
	this->DisconnectFilter(this->graph, this->muxer);
	this->DisconnectFilter(this->graph, this->writeFile);

	HRESULT hr = ConnectFilters(this->graph, this->videoDec, this->videoScaler);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = ConnectFilters(this->graph, this->videoScaler, this->videoEnc);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = ConnectFilters(this->graph, this->audioDec, this->audioEnc);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}
	
	hr = ConnectFilters(this->graph, this->videoEnc, this->muxer);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = ConnectFilters(this->graph, this->audioEnc, this->muxer);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	hr = ConnectFilters(this->graph, this->muxer, this->writeFile);
	if (FAILED(hr)){
		ret = FALSE;
		goto ErrEnd;
	}

	this->preCreateFlag = FALSE;

	return TRUE;
ErrEnd:
	SAFE_RELEASE(this->bonSrc);
	SAFE_RELEASE(this->tsSrc);
	SAFE_RELEASE(this->videoDec);
	SAFE_RELEASE(this->audioDec);
	SAFE_RELEASE(this->writeFile);
	SAFE_RELEASE(this->videoScaler);
	SAFE_RELEASE(this->videoEnc);
	SAFE_RELEASE(this->audioEnc);
	SAFE_RELEASE(this->muxer);
	SAFE_RELEASE(this->graph);

	return ret;
}

BOOL CDShowCtrl::DeleteGraph()
{
	if( this->mediaCtrl != NULL ){
		this->mediaCtrl->Stop();
	}
	SAFE_RELEASE(this->bonSrc);
	SAFE_RELEASE(this->tsSrc);
	SAFE_RELEASE(this->videoDec);
	SAFE_RELEASE(this->audioDec);
	SAFE_RELEASE(this->writeFile);
	SAFE_RELEASE(this->videoScaler);
	SAFE_RELEASE(this->videoEnc);
	SAFE_RELEASE(this->audioEnc);
	SAFE_RELEASE(this->muxer);
	SAFE_RELEASE(this->mediaCtrl);
	SAFE_RELEASE(this->graph);
	this->runFlag = FALSE;
	this->preCreateFlag = FALSE;

	return TRUE;
}

BOOL CDShowCtrl::RunGraph()
{
	if( this->mediaCtrl == NULL ){
		return FALSE;
	}
	HRESULT hr = this->mediaCtrl->Run();
    if(FAILED(hr))
    {
		return FALSE;
    }
	this->runFlag = TRUE;

	return TRUE;
}

BOOL CDShowCtrl::StopGraph()
{
	if( this->mediaCtrl == NULL ){
		return FALSE;
	}
	HRESULT hr = this->mediaCtrl->Stop();
    if(FAILED(hr))
    {
		return FALSE;
    }
	this->runFlag = FALSE;

	return TRUE;
}

BOOL CDShowCtrl::IsRun()
{
	if( this->mediaCtrl == NULL ){
		return FALSE;
	}
	return this->runFlag;
}

BOOL CDShowCtrl::AddTS(BYTE* data, DWORD size)
{
	if( this->mediaCtrl == NULL ){
		return FALSE;
	}
	if( this->runFlag == FALSE ){
		return FALSE;
	}

	DWORD ret = tsSrc->AddTS(data, size);
	if( this->preCreateFlag == TRUE ){
		//Pinフォーマット決めてもらうために少しだけ流す
		if( ret > 0 ){
			this->preCount+=ret;
		}
		if( this->preCount > 15 ){
			this->preCreateFlag = FALSE;
			this->preCount = 0;
			Sleep(1000);
			StopGraph();
			tsSrc->ClearData();
			ReConnectScale();
			RunGraph();
			while( this->buffData.empty() == false ){
				if( this->buffData.front().empty() == false ){
					tsSrc->AddTS(&this->buffData.front().front(), (DWORD)this->buffData.front().size());
				}
				this->buffData.pop_front();
			}
			tsSrc->AddTS(data, size);
		}else{
			this->buffData.push_back(vector<BYTE>());
			//TODO: dataの内容をコピーしなくていいのだろうか？
			this->buffData.back().resize(size);
		}
	}

	return TRUE;
}

// ConnectFilters is called from BuildGraph
// to enumerate and connect pins
HRESULT
CDShowCtrl::ConnectFilters(
	IGraphBuilder* pGraphBuilder,
	IBaseFilter* pFilterUpstream, 
	IBaseFilter* pFilterDownstream
    )
{
    HRESULT         hr = E_FAIL;

    CComPtr <IPin>  pIPinUpstream;


    PIN_INFO        PinInfoUpstream;
    PIN_INFO        PinInfoDownstream;

    // validate passed in filters
    ASSERT (pFilterUpstream);
    ASSERT (pFilterDownstream);

    // grab upstream filter's enumerator
    CComPtr <IEnumPins> pIEnumPinsUpstream;
    hr = pFilterUpstream->EnumPins(&pIEnumPinsUpstream);

    if(FAILED(hr))
    {
        return hr;
    }

    // iterate through upstream filter's pins
    while (pIEnumPinsUpstream->Next (1, &pIPinUpstream, 0) == S_OK)
    {
        hr = pIPinUpstream->QueryPinInfo (&PinInfoUpstream);
        if(FAILED(hr))
        {
            return hr;
        }

        CComPtr <IPin>  pPinDown;
        pIPinUpstream->ConnectedTo (&pPinDown);

        // bail if pins are connected
        // otherwise check direction and connect
        if ((PINDIR_OUTPUT == PinInfoUpstream.dir) && (pPinDown == NULL))
        {
            // grab downstream filter's enumerator
            CComPtr <IEnumPins> pIEnumPinsDownstream;
            hr = pFilterDownstream->EnumPins (&pIEnumPinsDownstream);
            if(FAILED(hr))
            {
                return hr;
            }

            // iterate through downstream filter's pins
            CComPtr <IPin>  pIPinDownstream;
            while (pIEnumPinsDownstream->Next (1, &pIPinDownstream, 0) == S_OK)
            {
                // make sure it is an input pin
                hr = pIPinDownstream->QueryPinInfo(&PinInfoDownstream);
                if(SUCCEEDED(hr))
                {
                    CComPtr <IPin>  pPinUp;

                    // Determine if the pin is already connected.  Note that 
                    // VFW_E_NOT_CONNECTED is expected if the pin isn't yet connected.
                    hr = pIPinDownstream->ConnectedTo (&pPinUp);
                    if(FAILED(hr) && hr != VFW_E_NOT_CONNECTED)
                    {
                        continue;
                    }

                    if ((PINDIR_INPUT == PinInfoDownstream.dir) && (pPinUp == NULL))
                    {
						if (SUCCEEDED (pGraphBuilder->Connect(
                                        pIPinUpstream,
                                        pIPinDownstream))
                                        )
                        {
                            PinInfoDownstream.pFilter->Release();
                            PinInfoUpstream.pFilter->Release();
                            return S_OK;
                        }
                    }
                }

                PinInfoDownstream.pFilter->Release();
                pIPinDownstream = NULL;
            } // while next downstream filter pin

            //We are now back into the upstream pin loop
        } // if output pin

        pIPinUpstream = NULL;
        PinInfoUpstream.pFilter->Release();
    } // while next upstream filter pin

    return E_FAIL;
}

// ConnectFilters is called from BuildGraph
// to enumerate and connect pins
HRESULT
CDShowCtrl::DisconnectFilter(
	IGraphBuilder* pGraphBuilder,
	IBaseFilter* pFilter
    )
{
    HRESULT         hr = E_FAIL;

    // grab upstream filter's enumerator
    CComPtr <IEnumPins> pIEnumPins;
    hr = pFilter->EnumPins(&pIEnumPins);

    if(FAILED(hr))
    {
        return hr;
    }
    IPin* pIPin = NULL; 
    // iterate through upstream filter's pins
    while (pIEnumPins->Next (1, &pIPin, 0) == S_OK)
    {
		IPin* pIPinConnection = NULL; 
		hr = pIPin->ConnectedTo(&pIPinConnection);
        if(FAILED(hr))
        {
			SAFE_RELEASE(pIPin);
            continue;
        }
		hr = pGraphBuilder->Disconnect(pIPin);
		hr = pGraphBuilder->Disconnect(pIPinConnection);
		SAFE_RELEASE(pIPinConnection); 
		SAFE_RELEASE(pIPin);
	}

    return S_OK;
}

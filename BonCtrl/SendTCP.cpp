#include "stdafx.h"
#include "SendTCP.h"

CSendTCP::CSendTCP(void)
{
	m_cTcp.Initialize();
}

CSendTCP::~CSendTCP(void)
{

	m_cTcp.UnInitialize();
}


BOOL CSendTCP::StartUpload( vector<NW_SEND_INFO>* List )
{
	for( int i=0; i<(int)List->size(); i++ ){
		m_cTcp.AddSendAddr((*List)[i].ipString.c_str(), (*List)[i].port);
	}
	return m_cTcp.StartSend();
}

void CSendTCP::SendData(BYTE* pbBuff, DWORD dwSize)
{
	m_cTcp.AddSendData(pbBuff, dwSize);
}

BOOL CSendTCP::CloseUpload()
{
	m_cTcp.StopSend();
	return TRUE;
}

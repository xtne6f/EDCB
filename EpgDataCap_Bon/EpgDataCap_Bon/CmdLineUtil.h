#pragma once
#include <map>
using namespace std;

class CCmdLineUtil
{
public:
	CCmdLineUtil(void);
	~CCmdLineUtil(void);

	void ParseParam( const TCHAR* pszParam, BOOL bFlag, BOOL bLast );
	
	map<wstring, wstring> m_CmdList;

protected:
	wstring m_strOpt;
	wstring m_strOpt2;
};

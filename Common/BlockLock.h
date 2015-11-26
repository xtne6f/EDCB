#pragma once

class CBlockLock
{
public:
	CBlockLock(CRITICAL_SECTION* lock_) : lock(lock_) { EnterCriticalSection(lock); }
	~CBlockLock() { LeaveCriticalSection(lock); }
private:
	CRITICAL_SECTION* lock;
};


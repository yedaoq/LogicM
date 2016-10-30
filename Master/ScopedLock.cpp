#include "StdAfx.h"
#include "ScopedLock.h"


CScopedLock::CScopedLock(CLockable& lock)
	: lock_(lock)
{
	lock_.Lock();
}


CScopedLock::~CScopedLock(void)
{
	lock_.Unlock();
}

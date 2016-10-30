#ifndef ScopedLock_h__
#define ScopedLock_h__

#pragma once

#include <Windows.h>

class CLockable
{
public:
	virtual ~CLockable() = 0 {}
	virtual void Lock() = 0;
	virtual void Unlock() = 0;
};

class CLockableCriticalSection : public CLockable
{
public:
	virtual void Lock()
	{
		EnterCriticalSection(&cs_);
	}

	virtual void Unlock()
	{
		LeaveCriticalSection(&cs_);
	}

protected:
	CRITICAL_SECTION cs_;
};

class CLockableMutex : public CLockable
{
public:
	CLockableMutex(LPCTSTR name)
	{
		Create(name);
	}

	virtual ~CLockableMutex()
	{
		Close();
	}

	virtual void Lock()
	{
		WaitForSingleObject(handle_, INFINITE);
	}

	virtual void Unlock()
	{
		ReleaseMutex(handle_);
	}

protected:
	void Close()
	{
		if (INVALID_HANDLE_VALUE != handle_ && NULL != handle_)
		{
			CloseHandle(handle_);
			handle_ = 0;
		}
	}

	void Create(LPCTSTR name)
	{
		handle_ = CreateMutex(NULL, false, name);
	}

protected:
	HANDLE handle_;
};

class CScopedLock
{
public:
	CScopedLock(CLockable& lock);
	~CScopedLock(void);

protected:
	CLockable& lock_;
};

#endif // ScopedLock_h__
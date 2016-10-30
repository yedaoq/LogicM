#pragma once

#include <Windows.h>

class CThread
{
public:

	struct IThreadRoutine
	{
		virtual ~IThreadRoutine() = 0 {}

		virtual bool NeedSupportWindowMessage() = 0;

		virtual void ThreadRoutine(CThread* host_thread, void* params) = 0;
		virtual bool StopThreadRoutine() = 0;
	};

public:
	CThread();
	~CThread(void);

	bool	Run(IThreadRoutine* routine, void* params);
	bool	Stop(DWORD timeout_miniseconds);

	DWORD	ThreadID() { return thread_id_; }
	HANDLE	ThreadHandle() { return thread_handle_; }

	bool	IsRunning();

protected:

	static void ThreadEntry(void*);

protected:
	IThreadRoutine* routine_;
	void*			routine_params_;

	HANDLE			thread_handle_;
	DWORD			thread_id_;
	HANDLE			thread_start_sync_;	
};


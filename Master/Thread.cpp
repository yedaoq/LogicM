#include "StdAfx.h"
#include "Thread.h"
#include <process.h>

CThread::CThread()
	: routine_(0), routine_params_(0), thread_handle_(0), thread_id_(0), thread_start_sync_(0)
{}

CThread::~CThread(void)
{
	if (0 != thread_handle_)
	{
		CloseHandle(thread_handle_);
	}
}

void CThread::ThreadEntry( void* params)
{
	CThread* obj = (CThread*)params;
	obj->thread_id_ = GetCurrentThreadId();

	if (!obj->routine_)
	{
		// error 
	}

	if (obj->routine_->NeedSupportWindowMessage())
	{
		MSG msg;
		PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);
		printf("thread ui created\n");
		SetEvent(obj->thread_start_sync_);
	}

	obj->routine_->ThreadRoutine(obj, obj->routine_params_);
}

bool CThread::Run( IThreadRoutine* routine, void* params)
{
	if (!routine)
	{
		// invalid routine
		return false;
	}

	if (thread_handle_ )
	{
		// thread is running~
		return false;
	}

	routine_ = routine;
	routine_params_ = params;

	if(routine->NeedSupportWindowMessage())
	{
		thread_start_sync_ = CreateEvent(NULL, TRUE, FALSE, NULL);
	}
	
	thread_handle_ = (HANDLE)_beginthread(ThreadEntry, 0, this);

	if (routine->NeedSupportWindowMessage())
	{
		WaitForSingleObject(thread_start_sync_, INFINITE);
		printf("thread ui created,wait return\n");
		CloseHandle(thread_start_sync_);
		thread_start_sync_ = NULL;
	}

	return true;
}

bool CThread::Stop( DWORD timeout_miniseconds )
{
	if (IsRunning())
	{
		routine_->StopThreadRoutine();

		DWORD ret = WaitForSingleObject(thread_handle_, timeout_miniseconds);
		if(WAIT_TIMEOUT == ret)
		{
			TerminateThread(thread_handle_, 0);
		}
	}

	return true;
}

bool CThread::IsRunning()
{
	if (0 != thread_handle_)
	{
		DWORD exit_code = 0;
		GetExitCodeThread(thread_handle_, &exit_code);
		return STILL_ACTIVE == exit_code;
	}

	return false;
}

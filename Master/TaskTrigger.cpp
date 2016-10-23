#include "StdAfx.h"
#include "TaskTrigger.h"

unsigned int TaskTrigger::max_timer_id_ = 1;

TaskTrigger::TaskTrigger(unsigned int interval_miniseconds, unsigned int task_type)
	: interval_miniseconds_(interval_miniseconds), task_type_(task_type), task_timer_(NULL)
{
	if(interval_miniseconds_ < 5)
		interval_miniseconds_ = 5;
	else if(interval_miniseconds_ > 5000)
		interval_miniseconds_ = 5000;
}

TaskTrigger::~TaskTrigger(void)
{
}

static VOID CALLBACK TaskTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	
}

void TaskTrigger::Run()
{
	MSG msg;
	PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);

	unsigned int timer_id = InterlockedIncrement(&max_timer_id_);
	task_timer_ = SetTimer(NULL, timer_id, interval_miniseconds_, TaskTimerProc);
}

void TaskTrigger::Stop()
{
	if (NULL != task_timer_)
	{
		KillTimer(NULL, task_timer_);
	}
}

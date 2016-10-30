#include "StdAfx.h"
#include "TaskTrigger.h"
#include "TaskData.h"
#include "ITaskDispatcher.h"
#include <process.h>

CTaskTrigger::CTaskTrigger(CUIMessageLoop* message_loop, ITaskDispatcher* dispatcher)
	: message_loop_(message_loop), task_dispather_(dispatcher), max_task_id_(0)
{
	task_allocator_ = new CTaskAllocatorDefault();
}

CTaskTrigger::~CTaskTrigger(void)
{
}

bool CTaskTrigger::StartTaskTimer( int task_type, unsigned int elapse_miniseconds )
{
	if(elapse_miniseconds < 5)
		elapse_miniseconds = 5;
	else if(elapse_miniseconds > 5000)
		elapse_miniseconds = 5000;

	return 0 < message_loop_->CreateTimer(elapse_miniseconds, 0, (void*)task_type, this);
}

bool CTaskTrigger::OnTimer( UINT timer_id, UINT trigger_times, void* params )
{
	int task_type = (int)params;
	int task_id = ++max_task_id_;

	printf("master : create task id-%d, type-%d \n", task_id, task_type);

	CTaskData* task = task_allocator_->AllocTask(100);
	task->SetTaskID(task_id);
	task->SetTaskType(task_type);
	
	char buf[] = "this is a task";
	task->Write(buf, ARRAYSIZE(buf));

	task_dispather_->DispatchTask(task);
	return true;
}
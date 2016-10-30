#pragma once

#include <Windows.h>
#include "UIMessageLoop.h"

struct ITaskDispatcher;
struct ITaskAllocator;

class CTaskTrigger : public CUIMessageLoop::ITimerHandler
{
public:
	CTaskTrigger(CUIMessageLoop* message_loop, ITaskDispatcher* dispatcher);
	~CTaskTrigger(void);

	bool StartTaskTimer(int task_type, unsigned int elapse_miniseconds);

protected:
	virtual bool OnTimer( UINT timer_id, UINT trigger_times, void* params );

protected:
	CUIMessageLoop*		message_loop_;
	ITaskDispatcher*	task_dispather_;
	ITaskAllocator*		task_allocator_;
	unsigned int		max_task_id_;
};

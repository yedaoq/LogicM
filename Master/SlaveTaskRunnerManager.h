#pragma once

#include "ITaskRunnerManager.h"
#include "ScopedLock.h"
#include "UIMessageLoop.h"
#include <map>

class CSlaveTaskRunner;
class CSlaveProcessManager;
class CIODriver;
struct ITaskAllocator;

class CSlaveTaskRunnerManager : public ITaskRunnerManager, public CUIMessageLoop::ITimerHandler
{
	typedef std::map<unsigned int, CSlaveTaskRunner*> RunnerList;
public:
	CSlaveTaskRunnerManager(void);
	~CSlaveTaskRunnerManager(void);

	bool	Init(CUIMessageLoop* message_loop);

	virtual ITaskRunner* GetRunner( unsigned int task_type );

protected:
	virtual bool OnTimer( UINT timer_id, UINT trigger_times, void* params );

protected:
	RunnerList					runners_;
	CLockableCriticalSection	runners_lock_;

	CSlaveProcessManager*		process_manager_;
	CIODriver*					io_dirver_;
	ITaskAllocator*				task_allocator_;

	CUIMessageLoop*				message_loop_;
};


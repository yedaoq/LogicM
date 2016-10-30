#include "StdAfx.h"
#include "SlaveTaskRunnerManager.h"
#include "SlaveProcessManager.h"
#include "SlaveTaskRunner.h"
#include "IODriver.h"
#include "TaskData.h"

CSlaveTaskRunnerManager::CSlaveTaskRunnerManager(void)
	: process_manager_(0), io_dirver_(0), message_loop_(0), task_allocator_(0)
{}

CSlaveTaskRunnerManager::~CSlaveTaskRunnerManager(void)
{
}

ITaskRunner* CSlaveTaskRunnerManager::GetRunner( unsigned int task_type )
{
	CSlaveTaskRunner* task_runner = 0;

	CScopedLock lock(runners_lock_);
	RunnerList::iterator iter = runners_.find(task_type);
	if (runners_.end() == iter)
	{
		task_runner = new CSlaveTaskRunner(process_manager_, io_dirver_, task_allocator_, task_type);
		runners_.insert(std::make_pair(task_type, task_runner));
	}
	else
	{
		task_runner = iter->second;
	}

	return task_runner;
}

bool CSlaveTaskRunnerManager::Init( CUIMessageLoop* message_loop )
{
	if (0 == message_loop || 0 != message_loop_)
	{
		return false;
	}

	io_dirver_ = new CIODriver();
	if (!io_dirver_->Run())
	{
		delete io_dirver_;
		io_dirver_ = 0;
		return false;
	}

	task_allocator_ = new CTaskAllocatorDefault();
	process_manager_ = new CSlaveProcessManager();
	message_loop_ = message_loop;

	message_loop_->CreateTimer(200, 0, 0, this);

	return true;
}

bool CSlaveTaskRunnerManager::OnTimer( UINT timer_id, UINT trigger_times, void* params )
{
	CScopedLock lock(runners_lock_);
	for (RunnerList::iterator i = runners_.begin(); i != runners_.end(); ++i)
	{
		i->second->CheckForTaskTimeout();
	}

	return true;
}

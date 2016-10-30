#pragma once

class CTaskData;

struct ITaskRunner
{
	virtual ~ITaskRunner() = 0 {}
	virtual void AppendTask(CTaskData* task, unsigned int timeout_miniseconds) = 0;
};
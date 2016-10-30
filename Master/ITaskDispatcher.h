#pragma once

class CTaskData;

struct ITaskDispatcher
{
	virtual ~ITaskDispatcher() = 0 {}
	virtual void DispatchTask(CTaskData* task) = 0;
};


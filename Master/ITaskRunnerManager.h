#pragma once

#include "ITaskRunner.h"

struct ITaskRunnerManager
{
	virtual ~ITaskRunnerManager() = 0 {}
	virtual ITaskRunner* GetRunner(unsigned int task_type) = 0;
};
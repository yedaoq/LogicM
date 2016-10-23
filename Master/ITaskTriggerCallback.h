#ifndef ITaskTriggerCallback_h__
#define ITaskTriggerCallback_h__

struct ITaskTriggerCallback
{
	virtual ~ITaskTriggerCallback() = 0 {}
	virtual CreateTask(unsigned int task_type) = 0;
};

#endif // ITaskTriggerCallback_h__

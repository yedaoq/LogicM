#pragma once

class TaskTrigger
{
public:
	TaskTrigger(unsigned int interval_miniseconds, unsigned int task_type);
	~TaskTrigger(void);

	void Run();
	void Stop();

protected:
	unsigned int interval_miniseconds_;
	unsigned int task_type_;
	UINT_PTR	 task_timer_;

	static unsigned int max_timer_id_;
};

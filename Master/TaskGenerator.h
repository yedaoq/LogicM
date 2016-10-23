#pragma once

class TaskGenerator
{
public:
	TaskGenerator(void);
	~TaskGenerator(void);

protected:
	unsigned int interval_miniseconds_;
	unsigned int task_type_;
};

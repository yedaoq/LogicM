// Master.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "TaskData.h"
#include "TaskTrigger.h"
#include "Master.h"
#include "SlaveTaskRunnerManager.h"
#include <iostream>

#pragma comment(lib, "shlwapi.lib")

int _tmain(int argc, _TCHAR* argv[])
{
	CUIMessageLoop* message_loop = new CUIMessageLoop();
	if (!message_loop->Run())
	{
		printf("fail to init message loop !\n");
		return 0;
	}

	CMaster* master = new CMaster;
	if (!master->Init(message_loop))
	{
		printf("error : fail to init master !\n");
		return 0;
	}
	
	CTaskTrigger* task_trigger = new CTaskTrigger(message_loop, master);
	task_trigger->StartTaskTimer(1, 2000);
	task_trigger->StartTaskTimer(2, 3000);

	char read_buf[1024];
	for(std::cin.read(read_buf, ARRAYSIZE(read_buf)); std::cin.gcount(); std::cin.read(read_buf, ARRAYSIZE(read_buf)))
	{
		char* token = strtok(read_buf, " "); // C4996
		if (NULL == token)
		{
			printf("error : invalid command!\n");
			continue;
		}

		if (0 == strcmp(token, "newtask"))
		{
			token = strtok( NULL, " ");
			if (0 == token || atoi(token) <= 0)
			{
				printf("error : invalid command!\n");
				continue;
			}

			unsigned int task_type = atoi(token);
			unsigned int task_elapse_miniseconds = 3000;
			
			token = strtok( NULL, " ");
			if (0 != token && atoi(token) <= 0)
			{
				task_elapse_miniseconds = atoi(token);
			}
			
			task_trigger->StartTaskTimer(task_type, task_elapse_miniseconds);
		}
	}

	return 0;
}



CMaster::CMaster(void)
	: message_loop_(0)
{
}


CMaster::~CMaster(void)
{
}

bool CMaster::Init(CUIMessageLoop* message_loop)
{
	if (message_loop_)
	{
		return true;
	}

	bool succeed = false;

	message_loop_ = message_loop;

	do 
	{
		CSlaveTaskRunnerManager* runner_manager = new CSlaveTaskRunnerManager();
		runner_manager_ = runner_manager;
		if (!runner_manager->Init(message_loop_)) break;

		succeed = true;

	} while (false);
	
	if (!succeed)
	{
		Destroy();
	}

	return succeed;
}

void CMaster::Destroy()
{
	if (message_loop_)
	{
		message_loop_->Stop();
		delete message_loop_;
		message_loop_ = 0;
	}

	if (runner_manager_)
	{
		delete runner_manager_;
		runner_manager_ = 0;
	}
}

void CMaster::DispatchTask(CTaskData* task)
{
	if (task && runner_manager_)
	{
		ITaskRunner* runner = runner_manager_->GetRunner(task->GetTaskType());
		if (runner)
		{
			runner->AppendTask(task, 4000);
		}
		else
		{
			printf("error : can't get runner!\n");
		}
	}
}

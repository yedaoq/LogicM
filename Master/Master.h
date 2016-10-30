#pragma once
#include "UIMessageLoop.h"
#include "ITaskDispatcher.h"

struct ITaskRunnerManager;
class CUIMessageLoop;

class CMaster : public ITaskDispatcher
{
public:
	CMaster(void);
	~CMaster(void);

	bool Init(CUIMessageLoop* message_loop);
	void Destroy();

	virtual void DispatchTask(CTaskData* task);

protected:
	CUIMessageLoop*		message_loop_;
	ITaskRunnerManager* runner_manager_;
};


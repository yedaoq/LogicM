#pragma once

#include <Windows.h>

class CUIMessageLoopImpl;
class CThread;

class CUIMessageLoop
{
public:
	struct IMessageHandler
	{
		virtual ~IMessageHandler() = 0 {}
		virtual bool OnMessage(DWORD message, WPARAM wparam, LPARAM lparam) = 0;
	};

	struct ITimerHandler
	{
		virtual ~ITimerHandler() = 0 {}
		virtual bool OnTimer(UINT timer_id, UINT trigger_times, void* params) = 0;
	};

public:
	CUIMessageLoop(void);
	~CUIMessageLoop(void);

	bool Run();
	void Stop();

	inline bool Post(UINT message, WPARAM wparam, LPARAM lparam);

	void AppendMessageHandler(IMessageHandler* handler);
	void RemoveMessageHandler(IMessageHandler* handler);

	UINT CreateTimer(UINT elapse_miniseconds, UINT max_trigger_times, void* params, ITimerHandler* handler);
	void DeleteTimer(unsigned int timer_id);

protected:
	CUIMessageLoopImpl* impl_;
	CThread*			thread_;
};


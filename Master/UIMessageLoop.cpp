#include "StdAfx.h"
#include "UIMessageLoop.h"
#include "Thread.h"
#include "ScopedLock.h"

#include <map>
#include <list>

enum { WM_RESERVED_BEGIN = WM_USER + 1000, WM_TIMERCREATE, WM_TIMERDELETE, WM_RESERVED_END };

class CUIMessageLoopImpl : public CThread::IThreadRoutine
{
	enum { FLAG_STOP = 1, FLAG_RUNNING = 2, FLAG_STOPPED = 4, };

	struct TimerContext
	{
		UINT		timer_id;			// 返回给用户的定时器ID;
		UINT		timer_id_inner;		// SetTimer的返回值;
		UINT		timer_elapse_miniseconds;
		UINT		max_trigger_times;	// 最多触发多少次，超过时自动销毁定时器;
		UINT		cur_trigger_times;	// 已触发多少次;
		CUIMessageLoop::ITimerHandler*	timer_handler;
		void*		timer_params;

		TimerContext(UINT elapse_miniseconds, UINT trigger_times, void* params, CUIMessageLoop::ITimerHandler* handler)
			: timer_id(0), cur_trigger_times(0), timer_elapse_miniseconds(elapse_miniseconds), max_trigger_times(trigger_times), timer_params(params), timer_handler(handler)
		{}
	};

	typedef std::map<UINT, TimerContext*> TimerContextMap;

	typedef std::list<CUIMessageLoop::IMessageHandler*>	MessageHandlerList;

public:

	CUIMessageLoopImpl();
	
	inline bool Post(UINT message, WPARAM wparam, LPARAM lparam);

	void AppendMessageHandler(CUIMessageLoop::IMessageHandler* handler);
	void RemoveMessageHandler(CUIMessageLoop::IMessageHandler* handler);

	UINT CreateTimer(UINT elapse_miniseconds, UINT max_trigger_times, void* params, CUIMessageLoop::ITimerHandler* handler);
	void DeleteTimer(UINT timer_id);

	virtual void ThreadRoutine( CThread* host_thread, void* params);
	virtual bool NeedSupportWindowMessage() { return true; }
	virtual bool StopThreadRoutine();

protected:
	void OnTimerTrigger(UINT timer_id_inner);

	void OnTimerCreate(TimerContext* ctx);
	void OnTimerDelete(UINT timer_id, TimerContext* ctx);

	bool DispatchMessageToHandlers(const MSG& msg);

protected:
	UINT					 max_timer_id_;		
	TimerContextMap			 timer_ctxs_;
	CLockableCriticalSection timer_ctxs_lock_;

	MessageHandlerList		 message_handlers_;
	CLockableCriticalSection message_handlers_lock_;

	CThread*				host_thread_;

	bool					flag_running_;
	bool					flag_stopping_;
};

CUIMessageLoop::CUIMessageLoop(void)
	: impl_(0), thread_(0)
{}

CUIMessageLoop::~CUIMessageLoop(void)
{
	Stop();
}

void CUIMessageLoop::Stop()
{
	if (thread_)
	{
		thread_->Stop(2000);

		delete thread_;
		thread_ = 0;

		delete impl_;
		impl_ = 0;
	}
}

bool CUIMessageLoop::Run()
{
	if (thread_)
	{
		return true;
	}

	impl_ = new CUIMessageLoopImpl();
	thread_ = new CThread();

	if(thread_->Run(impl_, 0))
	{
		return true;
	}

	Stop();
	return false;
}

bool CUIMessageLoop::Post( UINT message, WPARAM wparam, LPARAM lparam )
{
	return impl_->Post(message, wparam, lparam);
}

UINT CUIMessageLoop::CreateTimer( UINT elapse_miniseconds, UINT max_trigger_times, void* params, ITimerHandler* handler )
{
	return impl_->CreateTimer(elapse_miniseconds, max_trigger_times, params, handler);
}

void CUIMessageLoop::DeleteTimer( unsigned int timer_id )
{
	impl_->DeleteTimer(timer_id);
}

void CUIMessageLoop::AppendMessageHandler( IMessageHandler* handler )
{
	impl_->AppendMessageHandler(handler);
}

void CUIMessageLoop::RemoveMessageHandler( IMessageHandler* handler )
{
	impl_->RemoveMessageHandler(handler);
}

CUIMessageLoopImpl::CUIMessageLoopImpl()
	: host_thread_(NULL), flag_running_(false), flag_stopping_(false), max_timer_id_(0)
{
	
}

bool CUIMessageLoopImpl::Post( UINT message, WPARAM wparam, LPARAM lparam )
{
	DWORD thread_id = host_thread_->ThreadID();
	return FALSE != ::PostThreadMessage(thread_id, message, wparam, lparam);
}

void CUIMessageLoopImpl::ThreadRoutine( CThread* host_thread, void* params )
{
	host_thread_ = host_thread;

	MSG msg;
	while (!flag_stopping_ && GetMessage(&msg, 0, 0, 0))
	{
		switch (msg.message)
		{
		case WM_TIMER:
			OnTimerTrigger((UINT)msg.wParam);
			break;
		case WM_TIMERCREATE:
			OnTimerCreate((TimerContext*)msg.lParam);
			break;
		case WM_TIMERDELETE:
			OnTimerDelete((UINT)msg.wParam, (TimerContext*)msg.lParam);
			break;
		default:
			if (!DispatchMessageToHandlers(msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			break;
		}
	}
}

bool CUIMessageLoopImpl::StopThreadRoutine()
{
	// ignore unhandled messages in message loop
	flag_stopping_ = 0;
	return true;
}

UINT CUIMessageLoopImpl::CreateTimer( UINT elapse_miniseconds, UINT max_trigger_times, void* params, CUIMessageLoop::ITimerHandler* timer_handler )
{
	TimerContext* ctx = new TimerContext(elapse_miniseconds, max_trigger_times, params, timer_handler);
	ctx->timer_id = InterlockedIncrement(&max_timer_id_);
	if(!Post(WM_TIMERCREATE, 0, (LPARAM)ctx))
	{
		delete ctx;
		return 0;
	}

	return ctx->timer_id;
}

void CUIMessageLoopImpl::DeleteTimer( UINT timer_id )
{
	CScopedLock scope(timer_ctxs_lock_);

	TimerContext* ctx = 0;
	for(TimerContextMap::iterator iter = timer_ctxs_.begin(); iter != timer_ctxs_.end(); ++iter)
	{
		if (iter->second->timer_id == timer_id)
		{
			ctx = iter->second;
			timer_ctxs_.erase(iter);
			break;
		}
	}

	Post(WM_TIMERDELETE, (WPARAM)timer_id, (LPARAM)ctx);
}

void CUIMessageLoopImpl::OnTimerCreate( TimerContext* ctx )
{
	CScopedLock scope(timer_ctxs_lock_);
	ctx->timer_id_inner = (UINT)SetTimer(NULL, 0, ctx->timer_elapse_miniseconds, NULL);
	if (ctx->timer_id_inner > 0)
	{
		timer_ctxs_.insert(std::make_pair(ctx->timer_id_inner, ctx));
	}
	else
	{
		printf("error : SetTimer fail\n");
	}
}

void CUIMessageLoopImpl::OnTimerDelete( UINT timer_id, TimerContext* ctx )
{
	if(0 == ctx)
	{
		CScopedLock scope(timer_ctxs_lock_);
		for(TimerContextMap::iterator iter = timer_ctxs_.begin(); iter != timer_ctxs_.end(); ++iter)
		{
			if (iter->second->timer_id == timer_id)
			{
				ctx = iter->second;
				timer_ctxs_.erase(iter);
				break;
			}
		}
	}

	if (0 != ctx)
	{
		KillTimer(0, (UINT_PTR)ctx->timer_id_inner);
		delete ctx;
	}
}

void CUIMessageLoopImpl::OnTimerTrigger( UINT timer_id )
{
	//printf("CUIMessageLoopImpl::OnTimerTrigger %d\n", timer_id);
	CScopedLock scope(timer_ctxs_lock_);
	bool continue_flag = false;

	TimerContextMap::iterator iter = timer_ctxs_.find(timer_id);
	if (iter != timer_ctxs_.end())
	{
		TimerContext* ctx = iter->second;
		++ctx->cur_trigger_times;
		continue_flag = ctx->timer_handler->OnTimer(timer_id, ctx->cur_trigger_times, ctx->timer_params);

		if(0 != ctx->max_trigger_times && ctx->cur_trigger_times >= ctx->max_trigger_times)
		{
			continue_flag = false;
		}

		if (!continue_flag)
		{
			timer_ctxs_.erase(iter);
			delete ctx;
		}
	}

	if (!continue_flag)
	{
		KillTimer(NULL, (UINT_PTR)timer_id);
	}
}

bool CUIMessageLoopImpl::DispatchMessageToHandlers( const MSG& msg )
{
	CScopedLock lock(message_handlers_lock_);
	for (MessageHandlerList::iterator i = message_handlers_.begin(); i != message_handlers_.end(); ++i)
	{
		if ((*i)->OnMessage(msg.message, msg.wParam, msg.lParam))
		{
			return true;
		}
	}

	return false;
}

void CUIMessageLoopImpl::AppendMessageHandler( CUIMessageLoop::IMessageHandler* handler )
{
	if (0 != handler)
	{
		CScopedLock lock(message_handlers_lock_);
		for(MessageHandlerList::iterator i = message_handlers_.begin(); i != message_handlers_.end(); ++i)
		{
			if (*i == handler)
			{
				return;
			}
		}

		message_handlers_.push_back(handler);
	}
}

void CUIMessageLoopImpl::RemoveMessageHandler( CUIMessageLoop::IMessageHandler* handler )
{
	if (0 != handler)
	{
		CScopedLock lock(message_handlers_lock_);
		for(MessageHandlerList::iterator i = message_handlers_.begin(); i != message_handlers_.end(); ++i)
		{
			if (*i == handler)
			{
				message_handlers_.erase(i);
				return;
			}
		}
	}
}

#pragma once

#include <list>
#include "ScopedLock.h"

struct SlaveProcessHandle
{
	HANDLE	request_write_handle;
	HANDLE	response_read_handle;
	HANDLE  process_handle;

	SlaveProcessHandle()
		: request_write_handle(0), response_read_handle(0), process_handle(0)
	{}

	SlaveProcessHandle(HANDLE request_write_handle, HANDLE response_read_handle, HANDLE process_handle);
};

struct SlaveProcessHandleV2
{
	HANDLE	message_handle;
	HANDLE  process_handle;

	SlaveProcessHandleV2()
		: message_handle(0), process_handle(0)
	{}

	SlaveProcessHandleV2(HANDLE message_handle, HANDLE process_handle);
};


class CSlaveProcessManager
{
	typedef std::list<SlaveProcessHandle>	ProcessHandleList;

public:
	struct ISlaveProcessStateHandler
	{
		virtual ~ISlaveProcessStateHandler() = 0 {}
		virtual void OnProcessCrashed(HANDLE process_handle) = 0;
	};

public:
	CSlaveProcessManager();
	~CSlaveProcessManager(void);

	void				CheckForProcessState();
	SlaveProcessHandle	CreateSlave(unsigned int task_type);
	SlaveProcessHandleV2	CreateSlaveV2(unsigned int task_type);

protected:
	bool		CreateNamedPipe(HANDLE* server_handle, HANDLE* client_handle);
	HANDLE		CreateSlaveProcess(unsigned int task_type, HANDLE stdin_handle, HANDLE stdout_handle);

protected:
	ProcessHandleList			slave_processes_;
	CLockableCriticalSection	slave_processes_lock_;
	unsigned int				max_namedpipe_id_;
};


#pragma once
#include "ITaskRunner.h"
#include <list>
#include "SlaveProcessManager.h"
#include "IODriver.h"
#include "TaskData.h"

class CTaskData;
class CSlaveProcessManager;
class CIODriver;

class CSlaveTaskRunner : public ITaskRunner, public CIODriver::IIOHandler
{
	struct TaskDataContext
	{
		CTaskData*		task_data;
		unsigned int	timeout_miniseconds;

		TaskDataContext(CTaskData* task, unsigned int timeout)
			: task_data(task), timeout_miniseconds(timeout)
		{}

		~TaskDataContext()
		{
			if (task_data)
			{
				task_data->Destroy();
				task_data = 0;
			}
		}
	};

	typedef std::list<TaskDataContext*> TaskDataContextList;
public:
	CSlaveTaskRunner(CSlaveProcessManager* process_manager, CIODriver* io_driver, ITaskAllocator* allocator, unsigned int task_type);
	~CSlaveTaskRunner(void);

	void CheckForTaskTimeout();
	void Destroy();

	virtual void AppendTask( CTaskData* task, unsigned int timeout_miniseconds );
	

protected:
	bool LaunchSlave();
	void DetachSlave();

	void BeginReadResponse();
	void BeginSendRequest();

	void OnResponseArrive();

	void OnSlaveProcessError(DWORD errorcode);

	virtual void OnIoComplete( HANDLE device_handle, LPOVERLAPPED overlapped, DWORD bytes_transfered, DWORD errorcode );

protected:
	CSlaveProcessManager*	process_manager_;
	CIODriver*				io_driver_;
	ITaskAllocator*			task_allocator_;
	unsigned int			task_type_;
	
	char*					response_read_buffer_;
	unsigned int			response_read_buffer_written_bytes_;
	static const unsigned int response_read_buffer_size_ = CTaskData::TASK_MAX_RAWBUF_SIZE;

	OVERLAPPED				response_read_overlapped_;
	OVERLAPPED				request_write_overlapped_;
	bool					slave_working_flag_;

	TaskDataContextList		queued_tasks_;
	CLockableCriticalSection queued_tasks_lock_;
	DWORD					current_task_start_tickcount_;

	//SlaveProcessHandle	slave_process_handle_;
	SlaveProcessHandleV2	slave_process_handle_;

	
};


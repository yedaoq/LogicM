#include "StdAfx.h"
#include "SlaveTaskRunner.h"
#include "SlaveProcessManager.h"
#include "IODriver.h"
#include "ScopedLock.h"
#include <stdexcept>

CSlaveTaskRunner::CSlaveTaskRunner( CSlaveProcessManager* process_manager, CIODriver* io_driver, ITaskAllocator* allocator, unsigned int task_type )
	: process_manager_(process_manager), io_driver_(io_driver), task_allocator_(allocator), task_type_(task_type), response_read_buffer_written_bytes_(0), slave_working_flag_(false)
{
	if (0 == process_manager_ || 0 == io_driver_ || 0 == task_allocator_)
	{
		throw std::invalid_argument("CSlaveTaskRunner need a valid process_manager");
	}

	response_read_buffer_ = new char[response_read_buffer_size_];
	memset(&response_read_overlapped_, 0, sizeof(response_read_overlapped_));
	memset(&request_write_overlapped_, 0, sizeof(request_write_overlapped_));
}

CSlaveTaskRunner::~CSlaveTaskRunner(void)
{
	if (0 != response_read_buffer_)
	{
		delete response_read_buffer_;
		response_read_buffer_ = 0;
	}

	process_manager_ = 0;
	io_driver_ = 0;
}

void CSlaveTaskRunner::AppendTask( CTaskData* task, unsigned int timeout_miniseconds )
{
	if (0 == task || 0 == process_manager_)
	{
		return;
	}

	CScopedLock lock(queued_tasks_lock_);
	queued_tasks_.push_back(new TaskDataContext(task, timeout_miniseconds));

	BeginSendRequest();
}

void CSlaveTaskRunner::CheckForTaskTimeout()
{
	// when use named pipe, iocp will be notified when slave crashed
// 	if (0 != slave_process_handle_.process_handle)
// 	{
// 		DWORD exit_code = 0;
// 		if(GetExitCodeProcess(slave_process_handle_.process_handle, &exit_code) && STILL_ACTIVE != exit_code)
// 		{
// 			printf("master : slave for task type %d crashed\n", task_type_);
// 			DetachSlave();
// 			BeginSendRequest();
// 			return;
// 		}
// 	}

	if (slave_working_flag_)
	{
		CScopedLock lock(queued_tasks_lock_);
		if (slave_working_flag_)
		{
			if(GetTickCount() - queued_tasks_.front()->timeout_miniseconds > current_task_start_tickcount_)
			{
				printf("master : task %d timeout, restart slave!\n", queued_tasks_.front()->task_data->GetTaskID());
				DetachSlave();
				BeginSendRequest();
				return;
			}
		}
	}
}

bool CSlaveTaskRunner::LaunchSlave()
{
	//slave_process_handle_ = process_manager_->CreateSlave(task_type_);
	slave_process_handle_ = process_manager_->CreateSlaveV2(task_type_);
	if (INVALID_HANDLE_VALUE == slave_process_handle_.process_handle || NULL == slave_process_handle_.process_handle)
	{
		return false;
	}

// 	io_driver_->Attach(slave_process_handle_.response_read_handle, this);
// 	io_driver_->Attach(slave_process_handle_.request_write_handle, this);
	io_driver_->Attach(slave_process_handle_.message_handle, this);
	
	response_read_buffer_written_bytes_ = 0;
	BeginReadResponse();
	
	return true;
}

void CSlaveTaskRunner::OnIoComplete( HANDLE device_handle, LPOVERLAPPED overlapped, DWORD bytes_transfered, DWORD errorcode )
{
	if (0 == bytes_transfered || ERROR_SUCCESS != errorcode)
	{
		OnSlaveProcessError(errorcode);
	}
	//else if (device_handle == slave_process_handle_.response_read_handle)
	else if(overlapped == &response_read_overlapped_)
	{
		response_read_buffer_written_bytes_ += bytes_transfered;
		OnResponseArrive();		
	}
	//else if ( device_handle == slave_process_handle_.request_write_handle)
	else if(overlapped == &request_write_overlapped_)
	{

	}
}

void CSlaveTaskRunner::OnSlaveProcessError(DWORD errorcode)
{
	if (ERROR_BROKEN_PIPE == errorcode)
	{
		printf("error : slave for task type %d may crashed\n", task_type_);
	}
	else
	{
		printf("error : uncxcepted io error\n", task_type_);
	}
	
	DetachSlave();
	BeginSendRequest();
}

void CSlaveTaskRunner::Destroy()
{
	DetachSlave();

	CScopedLock lock(queued_tasks_lock_);
	for (TaskDataContextList::iterator i = queued_tasks_.begin(); i != queued_tasks_.end(); ++i)
	{
		delete *i;
	}

	queued_tasks_.clear();
}

void CSlaveTaskRunner::DetachSlave()
{
	if (0 != slave_process_handle_.process_handle)
	{
		DWORD exit_code = 0;
		GetExitCodeProcess(slave_process_handle_.process_handle, &exit_code);
		if (STILL_ACTIVE == exit_code)
		{
			TerminateProcess(slave_process_handle_.process_handle, 0);
		}

		CloseHandle(slave_process_handle_.process_handle);
	}

// 	if (0 != slave_process_handle_.request_write_handle)
// 	{
// 		if (io_driver_)
// 		{
// 			io_driver_->Detach(slave_process_handle_.request_write_handle);
// 			CancelIo(slave_process_handle_.request_write_handle);
// 		}
// 
// 		CloseHandle(slave_process_handle_.request_write_handle);
// 	}
// 
// 	if (0 != slave_process_handle_.response_read_handle)
// 	{
// 		if (io_driver_)
// 		{
// 			io_driver_->Detach(slave_process_handle_.response_read_handle);
// 			CancelIo(slave_process_handle_.response_read_handle);
// 		}
// 
// 		CloseHandle(slave_process_handle_.response_read_handle);
// 	}

	if (0 != slave_process_handle_.message_handle)
	{
		if (io_driver_)
		{
			io_driver_->Detach(slave_process_handle_.message_handle);
			CancelIo(slave_process_handle_.message_handle);
		}

		CloseHandle(slave_process_handle_.message_handle);
	}

	//slave_process_handle_ = SlaveProcessHandle();
	slave_process_handle_ = SlaveProcessHandleV2();
	slave_working_flag_ = false;
}

void CSlaveTaskRunner::BeginReadResponse()
{
// 	if(!ReadFile(slave_process_handle_.response_read_handle, response_read_buffer_, response_read_buffer_size_ - response_read_buffer_written_bytes_, NULL, &response_read_overlapped_))
// 	{
// 		printf("ReadFile fail handle - %08x, error - %d\n", slave_process_handle_.response_read_handle, GetLastError());
// 	}

	//printf("BeginReadResponse\n");

	if(!ReadFile(slave_process_handle_.message_handle, response_read_buffer_, response_read_buffer_size_ - response_read_buffer_written_bytes_, NULL, &response_read_overlapped_))
	{
		if (GetLastError() != ERROR_IO_PENDING)
		{
			printf("ReadFile fail handle - %08x, error - %d\n", slave_process_handle_.message_handle, GetLastError());
			exit(0);
		}
	}
}

void CSlaveTaskRunner::BeginSendRequest()
{
	CScopedLock lock(queued_tasks_lock_);
	if (!queued_tasks_.empty() && !slave_working_flag_)
	{
		if (0 == slave_process_handle_.process_handle)
		{
			LaunchSlave();
		}

		slave_working_flag_ = true;
		CTaskData* task = queued_tasks_.front()->task_data;
		printf("master : send task %d\n", task->GetTaskID());
		
// 		if(!WriteFile(slave_process_handle_.request_write_handle, task->RawBuffer(), task->RawBufferSize(), NULL, &request_write_overlapped_))
// 		{
// 			printf("WriteFile fail handle - %08x, error - %d\n", slave_process_handle_.response_read_handle, GetLastError());
// 			exit(0);
// 		}
		if(!WriteFile(slave_process_handle_.message_handle, task->RawBuffer(), task->RawBufferSize(), NULL, &request_write_overlapped_))
		{
			if (GetLastError() != ERROR_IO_PENDING)
			{
				printf("WriteFile fail handle - %08x, error - %d\n", slave_process_handle_.message_handle, GetLastError());
				exit(0);
			}
		}
		else
		{
			current_task_start_tickcount_ = GetTickCount();
		}
	}
	else
	{
		if (slave_working_flag_)
		{
			printf("master : slave is working, do not send\n");
		}
		else
		{
			printf("master : no task to send\n");
		}		
	}
}

void CSlaveTaskRunner::OnResponseArrive()
{
	unsigned int task_size = 0;
	CTaskData* task = CTaskData::ParseTaskData(response_read_buffer_, response_read_buffer_written_bytes_, task_allocator_, &task_size);
	if (task)
	{
		if(response_read_buffer_written_bytes_ > task_size)
		{
			memmove(response_read_buffer_, response_read_buffer_ + task_size, response_read_buffer_written_bytes_ - task_size);
		}
		response_read_buffer_written_bytes_ -= task_size;

		if (0 == task->GetTaskType())
		{
			printf("slave : %s\n", (const char*)task->DataBuffer());
		}
		else
		{
			printf("master : task %d completed : %s\n", task->GetTaskID(), (const char*)task->DataBuffer());
			CScopedLock lock(queued_tasks_lock_);
			delete queued_tasks_.front()->task_data;
			queued_tasks_.pop_front();
			slave_working_flag_ = false;
			BeginSendRequest();
		}

		BeginReadResponse();
	}


}

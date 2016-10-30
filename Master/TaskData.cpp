#include "StdAfx.h"
#include "TaskData.h"
#include <malloc.h>

void CTaskData::Destroy()
{
	if (allocator_)
	{
		allocator_->FreeTask(this);
	}
}

bool CTaskData::Write( buf_t data, unsigned int data_size )
{
	if (!data || data_size <= 0 || data_size > buf_size_ - header_->data_size - TASK_HEADER_SIZE)
	{
		return false;
	}

	memcpy(buf_ + sizeof(TaskDataHeader) + header_->data_size, data, data_size);
	header_->data_size += data_size;
	return true;
}

CTaskData* CTaskData::ParseTaskData( buf_t buf, unsigned int buf_size, ITaskAllocator* allocator, unsigned int* task_size )
{
	if (0 == buf || buf_size < TASK_HEADER_SIZE || 0 == allocator)
	{
		return 0;
	}

	TaskDataHeader* header = (TaskDataHeader*)buf;
	if (header->data_size + TASK_HEADER_SIZE > buf_size)
	{
		return 0;
	}

	CTaskData* task = allocator->AllocTask(header->data_size);

	memcpy(task->buf_, buf, header->data_size + TASK_HEADER_SIZE);
	if (task_size)
	{
		*task_size = header->data_size + TASK_HEADER_SIZE;
	}

	return task;
}

CTaskData* CTaskAllocatorDefault::AllocTask( unsigned int max_data_size )
{
	if (max_data_size > CTaskData::TASK_MAX_DATA_SIZE)
	{
		return 0;
	}

	unsigned int buf_size = max_data_size + sizeof(CTaskData::TaskDataHeader);
	CTaskData::buf_t buf = (CTaskData::buf_t)malloc(buf_size);
	return new CTaskData(this, buf, buf_size);
}

void CTaskAllocatorDefault::FreeTask( CTaskData* task )
{
	if(task)
	{
		free(task->RawBuffer());
		delete task;
	}
}

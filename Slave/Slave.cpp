// Slave.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include "../Master/TaskData.h"

void SendResponse(unsigned int task_id, unsigned int task_type, ITaskAllocator* allocator, char* msg)
{
	CTaskData* task_response = allocator->AllocTask(strlen(msg) + 1);
	task_response->SetTaskID(task_id);
	task_response->SetTaskType(task_type);
	task_response->Write(msg, strlen(msg) + 1);

	HANDLE stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);

	DWORD bytes_written = 0;
	WriteFile(stdout_handle, task_response->RawBuffer(), task_response->RawBufferSize(), &bytes_written, NULL);

	task_response->Destroy();
}

void HandleTask(CTaskData* task, ITaskAllocator* task_allocator)
{
	DWORD sleep_miniseconds = GetTickCount() % 5000;
	
	char buf[1024];
	sprintf_s(buf, "task %d may cost %d to process", task->GetTaskID(), sleep_miniseconds);

	SendResponse(0, 0, task_allocator, buf);

	Sleep(sleep_miniseconds);
	if (0 == (sleep_miniseconds % 3))
	{
		SendResponse(0, 0, task_allocator, "I will be crash, HaHa~");
		Sleep(100);
		//OutputDebugStringA("exception at task handle");
		exit(0); // simulate crash
	}
	
	//OutputDebugStringA("task complete");
	SendResponse(task->GetTaskID(), task->GetTaskType(), task_allocator, "ok");
	task->Destroy();
}

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc < 1)
	{
		//OutputDebugString(GetCommandLine());
		return 0;
	}

	OutputDebugStringA("slave start");

	unsigned int task_type = _ttoi(argv[0]);
	ITaskAllocator* task_allocator = new CTaskAllocatorDefault();

	char* request_read_buffer = new char[CTaskData::TASK_MAX_RAWBUF_SIZE];
	unsigned int request_read_buffer_size = CTaskData::TASK_MAX_RAWBUF_SIZE;
	unsigned int request_read_buffer_written_bytes = 0;

	HANDLE stdin_handle = GetStdHandle(STD_INPUT_HANDLE);

	DWORD bytes_readed = 0;
	while(ReadFile(stdin_handle, request_read_buffer + request_read_buffer_written_bytes, request_read_buffer_size - request_read_buffer_written_bytes, &bytes_readed, NULL))
	{
		OutputDebugStringA("slave read data");

		request_read_buffer_written_bytes += bytes_readed;
		unsigned int task_size = 0;
		CTaskData* task = CTaskData::ParseTaskData(request_read_buffer, request_read_buffer_written_bytes, task_allocator, &task_size);
		if (!task) continue;
		
		OutputDebugStringA("slave read task");

		if (request_read_buffer_written_bytes > task_size)
		{
			memmove(request_read_buffer, request_read_buffer + task_size, request_read_buffer_written_bytes - task_size);
			request_read_buffer_written_bytes -= task_size;
		}

		request_read_buffer_written_bytes -= task_size;

		HandleTask(task, task_allocator);
	}

	OutputDebugStringA("slave exit");

	return 0;
}


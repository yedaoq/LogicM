#include "StdAfx.h"
#include "SlaveProcessManager.h"
#include <shlwapi.h>
#include <Strsafe.h>

SlaveProcessHandle::SlaveProcessHandle( HANDLE request_write_handle, HANDLE response_read_handle, HANDLE process_handle )
{
	this->request_write_handle = request_write_handle;
	this->response_read_handle = response_read_handle;
	this->process_handle = process_handle;
}

CSlaveProcessManager::CSlaveProcessManager(void)
	: max_namedpipe_id_(0)
{
}


CSlaveProcessManager::~CSlaveProcessManager(void)
{
}

void CSlaveProcessManager::CheckForProcessState()
{
	CScopedLock lock(slave_processes_lock_);

}

SlaveProcessHandle CSlaveProcessManager::CreateSlave( unsigned int task_type )
{
	printf("master : create slave %d\n", task_type);

	STARTUPINFO startup_info = {0};
	PROCESS_INFORMATION process_info = {0};
	SECURITY_ATTRIBUTES sa;

	HANDLE request_write_handle = 0, request_read_handle = 0, 
		response_write_handle = 0, response_read_handle = 0; 

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	do 
	{
		if (!CreatePipe(&request_read_handle, &request_write_handle, &sa, 0) ||
			!CreatePipe(&response_read_handle, &response_write_handle, &sa, 0)) 
		{
			printf("CreatePipe failed (%d)!\n", GetLastError());
			break;
		}

		GetStartupInfo(&startup_info);
		startup_info.cb = sizeof(startup_info);
		startup_info.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		startup_info.wShowWindow = SW_NORMAL; //SW_HIDE;
		startup_info.hStdInput = request_read_handle;
		//sInfo.hStdError = hWrite;  
		startup_info.hStdOutput = response_write_handle;

		TCHAR exe_path[MAX_PATH * 2];
		::GetModuleFileName(NULL, exe_path, ARRAYSIZE(exe_path) - 1);
		::PathRemoveFileSpec(exe_path);
		::PathAppend(exe_path, TEXT("Slave.exe"));

		TCHAR exe_cmd[64];
		_itot_s(task_type, exe_cmd, 10);

		if(!CreateProcess(exe_path, exe_cmd, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info)) 
		{
			printf("CreateProcess failed (%d)!\n", GetLastError());
			break;
		}

	} while (false);

	if (NULL == process_info.hProcess || INVALID_HANDLE_VALUE == process_info.hProcess)
	{
		if (0 != request_read_handle)
		{
			CloseHandle(request_read_handle);
		}

		if (0 != request_write_handle)
		{
			CloseHandle(request_write_handle);
		}

		if (0 != response_read_handle)
		{
			CloseHandle(response_read_handle);
		}

		if (0 != response_write_handle)
		{
			CloseHandle(response_write_handle);
		}
		printf("master : create slave %d fail!\n", task_type);

		return SlaveProcessHandle();
	}
	else
	{
		CloseHandle(request_read_handle);
		CloseHandle(response_write_handle);
		CloseHandle(process_info.hThread);
		return SlaveProcessHandle(request_write_handle, response_read_handle, process_info.hProcess);
	}
}

HANDLE CSlaveProcessManager::CreateSlaveProcess(unsigned int task_type, HANDLE stdin_handle, HANDLE stdout_handle )
{
	printf("master : create slave %d\n", task_type);

	STARTUPINFO startup_info = {0};
	PROCESS_INFORMATION process_info = {0};
	SECURITY_ATTRIBUTES sa;

	GetStartupInfo(&startup_info);
	startup_info.cb = sizeof(startup_info);
	startup_info.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	startup_info.wShowWindow = SW_NORMAL; //SW_HIDE;
	startup_info.hStdInput = stdin_handle;
	//sInfo.hStdError = hWrite;  
	startup_info.hStdOutput = stdout_handle;

	TCHAR exe_path[MAX_PATH * 2];
	::GetModuleFileName(NULL, exe_path, ARRAYSIZE(exe_path) - 1);
	::PathRemoveFileSpec(exe_path);
	::PathAppend(exe_path, TEXT("Slave.exe"));

	TCHAR exe_cmd[64];
	_itot_s(task_type, exe_cmd, 10);

	if(::CreateProcess(exe_path, exe_cmd, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info))
	{
		CloseHandle(process_info.hThread);
	}
	else
	{
		printf("CreateProcess failed (%d)!\n", GetLastError());
	}
	
	return process_info.hProcess;
}

SlaveProcessHandleV2 CSlaveProcessManager::CreateSlaveV2( unsigned int task_type )
{
	//printf("master : create slave %d\n", task_type);

	HANDLE pipe_server_handle = NULL, pipe_client_handle = NULL;
	if (!CreateNamedPipe(&pipe_server_handle, &pipe_client_handle))
	{
		return SlaveProcessHandleV2();
	}

	HANDLE process_handle = CreateSlaveProcess(task_type, pipe_client_handle, pipe_client_handle);
	
	if (INVALID_HANDLE_VALUE == process_handle || NULL == process_handle)
	{
		CloseHandle(pipe_server_handle);
		pipe_server_handle = INVALID_HANDLE_VALUE;
		process_handle = INVALID_HANDLE_VALUE;
	}

	CloseHandle(pipe_client_handle);
	return SlaveProcessHandleV2(pipe_server_handle, process_handle);
}

bool CSlaveProcessManager::CreateNamedPipe( HANDLE* server_handle, HANDLE* client_handle )
{
	int   pipe_id = InterlockedIncrement(&max_namedpipe_id_);
	TCHAR pipe_name[128] = TEXT("");
	StringCchPrintf(pipe_name, ARRAYSIZE(pipe_name) - 1, TEXT("\\\\.\\pipe\\logicm_test_%d"), pipe_id);

	SECURITY_ATTRIBUTES sa = {0};
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	DWORD open_mode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED/* |FILE_FLAG_FIRST_PIPE_INSTANCE*/;

	HANDLE local_server_handle = ::CreateNamedPipe(pipe_name, open_mode,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE, 1,	8192, 8192,	5000, &sa);

	HANDLE local_client_handle = CreateFileW(pipe_name,
		GENERIC_READ | GENERIC_WRITE,
		0,
		&sa,
		OPEN_EXISTING,
		SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION |
		FILE_FLAG_OVERLAPPED,
		NULL);

	if (INVALID_HANDLE_VALUE == local_server_handle || INVALID_HANDLE_VALUE == local_client_handle)
	{
		if (INVALID_HANDLE_VALUE != local_server_handle)
		{
			CloseHandle(local_server_handle);
		}
		if (INVALID_HANDLE_VALUE != local_client_handle)
		{
			CloseHandle(local_client_handle);
		}

		return false;
	}
	else
	{
		if (server_handle)
		{
			*server_handle = local_server_handle;
		}

		if(client_handle)
		{
			*client_handle = local_client_handle;
		}

		return true;
	}
}

SlaveProcessHandleV2::SlaveProcessHandleV2( HANDLE message_handle, HANDLE process_handle )
{
	this->message_handle = message_handle;
	this->process_handle = process_handle;
}

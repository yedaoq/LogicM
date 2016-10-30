#include "StdAfx.h"
#include "IODriver.h"
#include "Thread.h"
#include "ScopedLock.h"
#include <map>

class CIODriverImpl : public CThread::IThreadRoutine
{
	struct DeviceContext
	{
		HANDLE		device_handle;
		CIODriver::IIOHandler*	device_io_handler;
		void*		device_parms;

		DeviceContext(HANDLE device_handle, CIODriver::IIOHandler* io_handler, void* params)
		{
			this->device_handle = device_handle;
			this->device_io_handler = io_handler;
			this->device_parms = params;
		}
	};

	typedef std::map<int, DeviceContext>	DeivceMap;

	enum	{IOCP_KEY_EXIT = 0, };
public:
	CIODriverImpl()
		: iocp_handle_(0), iocp_thread_(0), max_device_key_(0), flag_eixt_(false)
	{}	

	~CIODriverImpl()
	{
		int x = 0;
	}

public:
	bool Run();

	bool Attach(HANDLE device_handle, CIODriver::IIOHandler* device_io_handler);
	void Detach(HANDLE device_handle);

	virtual bool StopThreadRoutine();
	virtual bool NeedSupportWindowMessage() { return false; }
	virtual void ThreadRoutine( CThread* host_thread, void* params );

protected:
	HANDLE						iocp_handle_;
	CThread*					iocp_thread_;

	OVERLAPPED					overlapped_exit_;
	bool						flag_eixt_;

	DeivceMap					devices_;
	CLockableCriticalSection	devices_lock_;
	int							max_device_key_;
};

void CIODriverImpl::ThreadRoutine( CThread* host_thread, void* params )
{
	DWORD		bytes_transfered = 0;
	ULONG_PTR	key = 0;
	LPOVERLAPPED overlapped = 0;

	while( !flag_eixt_ )
	{
		DWORD errorcode = ERROR_SUCCESS;
		if (!GetQueuedCompletionStatus(iocp_handle_, &bytes_transfered, (PULONG_PTR)&key, &overlapped, INFINITE))
		{
			errorcode = GetLastError();
			
			if (ERROR_OPERATION_ABORTED == errorcode || NULL == overlapped)
			{
				continue;
			}
			else
			{
				printf("GetQueuedCompletionStatus error %d\n", GetLastError());
			}
		}

		if(0 == overlapped)
		{
			printf("GetQueuedCompletionStatus unexcepted null overlapped\n");
			break;
		}

		if (IOCP_KEY_EXIT == key)
		{
			printf("iocp thread exit\n");
			break;
		}

		CScopedLock lock(devices_lock_);
		DeivceMap::iterator iter = devices_.find((int)key);
		if (devices_.end() != iter)
		{
			iter->second.device_io_handler->OnIoComplete((HANDLE)key, overlapped, bytes_transfered, errorcode);
		}
	}
}

bool CIODriverImpl::Attach( HANDLE device_handle, CIODriver::IIOHandler* device_io_handler )
{
	if (0 == iocp_thread_ || INVALID_HANDLE_VALUE == device_handle || NULL == device_handle || 0 == device_io_handler)
	{
		return false;
	}

	CScopedLock lock(devices_lock_);
	for (DeivceMap::iterator i = devices_.begin(); i != devices_.end(); ++i)
	{
		if (i->second.device_handle == device_handle)
		{
			printf("error : device %08x has been bind to IODriver\n", device_handle);
			return false;
		}
	}

	//HANDLE file_test = CreateFile(TEXT("D:\\log_network.txt"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_OVERLAPPED*/, NULL);
	//HANDLE result_handle = CreateIoCompletionPort(file_test, iocp_handle_, (ULONG_PTR)device_handle, 1);
	//CloseHandle(file_test);

	int device_id = ++max_device_key_;
	HANDLE result_handle = CreateIoCompletionPort(device_handle, iocp_handle_, (ULONG_PTR)/*device_handle*/device_id, 1);
	devices_.insert(std::make_pair(/*device_handle*/device_id, DeviceContext(device_handle, device_io_handler, 0)));
	return true;
}

void CIODriverImpl::Detach( HANDLE device_handle )
{
	CScopedLock lock(devices_lock_);
	for (DeivceMap::iterator i = devices_.begin(); i != devices_.end(); ++i)
	{
		if (i->second.device_handle == device_handle)
		{
			devices_.erase(i);
			break;
		}
	}
}

bool CIODriverImpl::Run()
{
	if (NULL != iocp_handle_)
	{
		return true;
	}

	memset(&overlapped_exit_, 0, sizeof(overlapped_exit_));

	iocp_handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);

	if(0 == iocp_handle_)
	{
		return false;
	}

	iocp_thread_ = new CThread();
	if(!iocp_thread_->Run(this, 0))
	{
		printf("error : iocp thread fial!\n");
		delete iocp_thread_;
		iocp_thread_ = 0;
		CloseHandle(iocp_handle_);
		iocp_handle_ = 0;
		return false;
	}

	return true;
}

bool CIODriverImpl::StopThreadRoutine()
{
	flag_eixt_ = 0;
	::PostQueuedCompletionStatus(iocp_handle_, 0, IOCP_KEY_EXIT, &overlapped_exit_);
	return true;
}

CIODriver::CIODriver(void)
{
	impl_ = new CIODriverImpl;
}

CIODriver::~CIODriver(void)
{
	if (impl_)
	{
		delete impl_;
		impl_ = 0;
	}
}

bool CIODriver::Run()
{
	if (impl_)
	{
		return impl_->Run();
	}
	return false;
}

bool CIODriver::Attach( HANDLE device_handle, IIOHandler* device_io_handler )
{
	if (impl_)
	{
		return impl_->Attach(device_handle, device_io_handler);
	}
	return false;
}

void CIODriver::Detach( HANDLE device_handle )
{
	if (impl_)
	{
		impl_->Detach(device_handle);
	}
}

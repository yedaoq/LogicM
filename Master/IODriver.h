#pragma once

#include <Windows.h>

class CIODriverImpl;

class CIODriver
{
public:
	struct IIOHandler 
	{
		virtual ~IIOHandler() = 0 {}
		virtual void OnIoComplete(HANDLE device_handle, LPOVERLAPPED overlapped, DWORD bytes_transfered, DWORD errorcode) = 0;
	};

public:
	CIODriver(void);
	~CIODriver(void);

	bool Run();

	bool Attach(HANDLE device_handle, IIOHandler* device_io_handler);
	void Detach(HANDLE device_handle);

protected:

	CIODriverImpl* impl_;
};


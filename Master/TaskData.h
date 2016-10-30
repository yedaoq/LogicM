#pragma once

struct ITaskAllocator;

class CTaskData
{
public:
	typedef char*			buf_t;
	typedef unsigned int	task_id_t;
	typedef unsigned int	task_type_t;
	typedef unsigned short	task_flag_t;

	struct TaskDataHeader
	{
		task_flag_t	flag;
		char		ver;
		char		reserve;
		unsigned int data_size;
		task_type_t type;
		task_id_t	id;			
	};

	static const unsigned int TASK_HEADER_SIZE = sizeof(TaskDataHeader);
	static const unsigned int TASK_MAX_DATA_SIZE = 4080;
	static const unsigned int TASK_MAX_RAWBUF_SIZE = TASK_MAX_DATA_SIZE + TASK_HEADER_SIZE + 8;

public:
	CTaskData(ITaskAllocator* allocator, buf_t buf, unsigned int buf_size)
		: buf_(buf), header_((TaskDataHeader*)buf), buf_size_(buf_size), allocator_(allocator)
	{
		header_->data_size = 0;
		header_->flag = 0xAB;
		header_->ver = 1;
	}

	static CTaskData* ParseTaskData(buf_t buf, unsigned int buf_size, ITaskAllocator* allocator, unsigned int* task_size);

	void		Destroy();

	buf_t		RawBuffer() { return buf_; }
	unsigned int RawBufferSize() { return header_->data_size + TASK_HEADER_SIZE; }

	buf_t		DataBuffer() { return buf_ + sizeof(TaskDataHeader); }
	const buf_t	DataBuffer() const { return buf_ + sizeof(TaskDataHeader); }

	unsigned int DataBufferSize() const { return buf_size_ - sizeof(TaskDataHeader); }
	unsigned int DataBufferOccupied() const { return header_->data_size; }

	bool		Write(buf_t data, unsigned int data_size);

	void		SetTaskID( task_id_t id) { ((TaskDataHeader*)buf_)->id = id; }
	task_id_t	GetTaskID() const { return ((TaskDataHeader*)buf_)->id; }

	void		SetTaskType(task_type_t type) { ((TaskDataHeader*)buf_)->type = type; }
	task_type_t	GetTaskType() const { return ((TaskDataHeader*)buf_)->type; }

	//void		Reset() { buf_ = 0; }

protected:
	buf_t			buf_;	
	TaskDataHeader*	header_;
	unsigned int	buf_size_;
	ITaskAllocator* allocator_;
};


struct ITaskAllocator
{
	virtual ~ITaskAllocator() = 0 {}
	virtual CTaskData* AllocTask(unsigned int max_data_size) = 0;
	virtual	void FreeTask(CTaskData* task) = 0;
};

class CTaskAllocatorDefault : public ITaskAllocator
{
public:
	virtual CTaskData* AllocTask(unsigned int max_data_size);
	virtual	void FreeTask(CTaskData* task);
};


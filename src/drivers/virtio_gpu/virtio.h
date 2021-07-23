#ifndef __VIRTIO_H
#define __VIRTIO_H
#include <base/stdint.h>

typedef Genode::uint8_t u8;
typedef Genode::uint16_t u16;
typedef Genode::uint32_t u32;
typedef Genode::uint64_t u64;

namespace Virtio {
	enum DeviceID {
		Invalid = 0,
		Net = 1,
		Block = 2,
		Console = 3,
		Entropy = 4,
		Memory_Legacy = 5,
		Scsi = 8,
		Gpu = 16,
		Input = 18,
		Crypto = 20,
		Socket = 19,
		
		// Virtio 1.1 Spec, officially, gives no Device ID for 9P, so this may be temporary?
		Transport_9P = 8,
	};

	class Driver;

	struct pvirtq_desc;
	struct virtq_desc;
	struct virtq_used_elem;

	template<typename T> class Virtqueue : Genode::Interface {
	protected:
		//virtual ~Virtqueue() {};
		
	public:
		virtual bool push(void* ptr, u32 size, u16 flags) = 0;
		virtual T* pop() = 0;
		virtual T* get(u16 idx) = 0;

		virtual void sync() = 0;
		virtual u16 queue_size() = 0;

		virtual void set_descriptor_area(u8* ptr) = 0;
		virtual void set_driver_area(u8* ptr) = 0;
		virtual void set_device_area(u8* ptr) = 0;
	};

	class SplitVirtqueue;

	enum desc_flags {
		Next = 1,
		Write = 2,
		Indirect = 4,
	};
		
	enum Status {
		Acknowledge = 1,
		Driver_avail = 2,
		Failed = 128,
		Features_ok = 8,
		Driver_ok = 4,
		Device_needs_reset = 64,
	};
}

// Packed Descriptor Area Entry
struct Virtio::pvirtq_desc {
	u64 addr;
	u32 len;
	u16 id;
	u16 flags;
};

// Split Descriptor Area Entry
struct Virtio::virtq_desc {
	u64 addr;
	u32 len;
	u16 flags;
	u16 next;
};

struct Virtio::virtq_used_elem {
	u32 id;
	u32 len;
};

class Virtio::SplitVirtqueue : Virtqueue<struct virtq_desc> {
protected:
	Genode::Heap& heap;
	u16 vq_id;

	virtq_desc* descriptor_area;
	u8* avail_ring;
	u8* used_ring;

	u16 qsize;
	u16 idx;
	u32 avail_flags;
	u32 used_flags;
	
public:
	SplitVirtqueue(SplitVirtqueue const&) = delete;
	SplitVirtqueue operator=(SplitVirtqueue const&) = delete;

	~SplitVirtqueue() {
		heap.free(descriptor_area, 0);
		heap.free(avail_ring, 0);
		heap.free(used_ring, 0);
	}

	SplitVirtqueue(u16 id, Genode::Heap& heap, u16 queue_size) : heap(heap), vq_id(id), descriptor_area(nullptr), avail_ring(nullptr), used_ring(nullptr), qsize(queue_size), idx(0), avail_flags(0), used_flags(0)
	{
		heap.alloc(16*queue_size, (void**)&descriptor_area);
		heap.alloc(6+2*queue_size, (void**)&avail_ring);
		heap.alloc(6+8*queue_size, (void**)&used_ring);
	}
	
	bool push(void* ptr, u32 size, u16 flags) override {
		virtq_desc* a = get(idx);
		a->addr = (u64)ptr;
		a->flags = flags;
		a->len = size;
		return false;
	}

	struct virtq_desc* get(u16 idx) override {
		return &descriptor_area[idx];
	}

	struct virtq_desc* pop() override {
		return nullptr;
	}

	struct virtq_used_elem* read_chain_head() {
		u16 idx = used_ring[2];
		return (virtq_used_elem*) &used_ring[4 + (idx*8)];
	}

	void sync() override {
	}

	u16 queue_size() override {
		return qsize;
	}
		
	virtual void set_descriptor_area(u8* ptr) override
	{
		descriptor_area = (virtq_desc*) ptr;
	}
	virtual void set_driver_area(u8* ptr) override
	{
		avail_ring = ptr;
	}
	virtual void set_device_area(u8* ptr) override
	{
		used_ring = ptr;
	}
};

class Virtio::Driver {
public:
	virtual ~Driver() {};

	virtual DeviceID get_virtio_type() = 0;

	virtual bool ok() = 0;

	virtual void set_status(u8 status) = 0;
	virtual u8 get_status() = 0;
	virtual void set_features(u64 features) = 0;	
	virtual u64 get_features() = 0;

	void reset() {
		set_status(0);
	}

	virtual u16 num_queues() = 0;
	virtual void queue_set_size(u16 vq_id, u16 size) = 0;
	virtual void queue_enable(u16 vq_id) = 0;
	virtual void queue_disable(u16 vq_id) = 0;
};

#endif //__VIRTIO_H

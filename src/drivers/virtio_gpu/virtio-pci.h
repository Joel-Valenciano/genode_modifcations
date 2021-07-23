#ifndef __VIRTIO_PCI_H
#define __VIRTIO_PCI_H
#include <base/stdint.h>
#include <base/attached_dataspace.h>

#include <util/construct_at.h>

#include <platform_session/connection.h>
#include <io_mem_session/client.h>

#include "virtio.h"

namespace Virtio {
	namespace Pci {
		struct common_config_t;
		struct notify_cap_t;
		class Driver;

		enum cfg_type {
			Invalid = 0,
			Common = 1, 
			Notify = 2,
			ISR = 3,
			Device = 4,
			PCI_Config = 5,
		};
	}
}

// Virtio over PCI common_config. accessed by MMIO.
struct Virtio::Pci::common_config_t {
	u32 device_feature_select;
	u32 device_feature;
	u32 driver_feature_select;
	u32 driver_feature;
	u16 msix_config;
	u16 num_queues;
	u8 device_status;
	u8 config_generation;

	u16 queue_select;
	u16 queue_size;
	u16 queue_msix_vector;
	u16 queue_enable;
	u16 queue_notify_off;
	u16 queue_desc;
	u64 queue_driver;
	u64 queue_device;
};

// PCI cap structure. So far unused in driver, as it is in Configuration space.
/*
struct Virtio::Pci::pci_cap_t {
	u8 cap_vendor;
	u8 cap_next;
	u8 cap_length;
	u8 cfg_type;
	u8 bar;
	u8 padding[3];
	u32 offset;
	u32 length;
};*/

class Virtio::Pci::Driver : Genode::Interface, public Virtio::Driver {
public:
	struct Bar : Genode::List<Bar>::Element {
		
	public:
		Genode::Io_mem_session_client io_mem;
		Genode::Region_map& rm;
		Genode::Attached_dataspace dataspace;
		u8 id;
		
		Bar(Genode::Region_map& rm, Genode::Io_mem_session_capability cap, u8 id) :
			io_mem(cap),
			rm(rm),
			dataspace(rm, io_mem.dataspace()),
			id(id)
		{ }

		u8* local_addr() {
			return dataspace.local_addr<u8>();
		}
	};
		
protected:	
	Genode::Env& env;
	Genode::Heap& heap;
	Platform::Device_client dev;
	common_config_t* common_cfg;
	Genode::List<Bar> bars_iomem;

public:
	Driver(Driver const&) = delete;
	Driver operator=(Driver const&) = delete;

	virtual ~Driver() {
		// Free memory used by the list of BARs.
		Bar* bar = bars_iomem.first();
		while(bar != nullptr) {
			Bar* next = bar->next();
			heap.free(bar, 0);
			bar = next;
		}
	}

	Driver(Genode::Env& env, Genode::Heap& heap, Platform::Device_capability cap) :
		env(env),
		heap(heap),
		dev(cap),
		common_cfg(nullptr),
		bars_iomem()
	{ }

	virtual bool handle_virtio_cap(const u8 cap_length,
			const u8 cfg_type,
			const u8 bar,
			const u32 off,
			const u32 len) {
		Genode::log(cap_length, cfg_type, bar, off, len);
		return false;
	}

	Bar* track_bar(u8 id) {
		Bar* bar;
		heap.alloc(sizeof(Bar), (void**)&bar);
		Genode::construct_at<Bar>(bar, env.rm(), dev.io_mem(dev.phys_bar_to_virt(id)), id);
		bars_iomem.insert(bar);
 		Genode::log("Mapping bar #", id, " to memory at: ", bar->dataspace.local_addr<void>());
		return bar;
	}

	bool ok() override {
		return common_cfg != nullptr;
	}

	u8 get_status() override {
		return common_cfg->device_status;
	}

	void set_status(u8 status) override {
		common_cfg->device_status = status;
	}
	
	u64 get_features() override {
		u32 a, b;
		u64 result;
		common_cfg->device_feature_select = 0x0;
		a = common_cfg->device_feature;
		common_cfg->device_feature_select = 0x1;
		b = common_cfg->device_feature;
		result = (u64)a << 32 | b;
		return result;
	}

	Bar* find_bar(u8 bar) {
		Bar* b = bars_iomem.first();
		while(b != nullptr) {  
			if (b->id == bar) {
				return b;
			}
			b = b->next();
		};
		return nullptr;
	}

	void set_features(u64 features) override {
		common_cfg->driver_feature_select = 0x0;
		common_cfg->driver_feature = (u32) features;
		common_cfg->driver_feature_select = 0x1;
		common_cfg->driver_feature = (u32) (features >> 32);
		return;
	}

	DeviceID get_virtio_type() override {
		return check_valid_type(dev.vendor_id(), dev.device_id());
	}

	u16 num_queues() override
	{
		return common_cfg->num_queues;
	}
	void queue_set_size(u16 vq_id, u16 size) override
	{
		common_cfg->queue_select = vq_id;
		common_cfg->queue_size = size;
	};
	void queue_enable(u16 vq_id) override
	{
		common_cfg->queue_select = vq_id;
		common_cfg->queue_enable = 1;
	};
	void queue_disable(u16 vq_id) override
	{
		common_cfg->queue_select = vq_id;
		common_cfg->queue_enable = 0;
	};

	Platform::Device_client& device() {
		return dev;
	}

	// This checks for valid Virtio IDs
	static DeviceID check_valid_type(u16 vendor_id, u16 device_id) {
		if(vendor_id == 0x1AF4) {
			if(device_id > 0x1040 || device_id < 0x107f) {
				// Check known Virtio devices
				switch(device_id - 0x1040) {
					case 1: return DeviceID::Net;
					case 2: return DeviceID::Block;
					case 3: return DeviceID::Console;
					case 4: return DeviceID::Entropy;
					case 5: return DeviceID::Memory_Legacy;
					case 8: return DeviceID::Scsi;
					case 16: return DeviceID::Gpu;
					case 18: return DeviceID::Input;
					case 20: return DeviceID::Crypto;
					case 19: return DeviceID::Socket;
					default: return DeviceID::Invalid;
				}
			} else {
				// Check for transitional devices
				switch(device_id) {
					case 0x1000: return DeviceID::Net;
					case 0x1001: return DeviceID::Block;
					case 0x1002: return DeviceID::Memory_Legacy;
					case 0x1003: return DeviceID::Console;
					case 0x1004: return DeviceID::Scsi;
					case 0x1005: return DeviceID::Entropy;
					case 0x1009: return DeviceID::Transport_9P;
					default: return DeviceID::Invalid;
				}
			}
		}
		// Device is not Virtio.
		return DeviceID::Invalid;
	}
};

#endif //__VIRTIO_PCI_H

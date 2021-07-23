#include <base/component.h>
#include <base/heap.h>
#include <base/log.h>
#include <base/attached_dataspace.h>

#include <platform_session/connection.h>

#include "virtio.h"
#include "virtio-pci.h"
#include "virtio-gpu.h"

#define VIRTIO_F_RING_INDIRECT_DESC (1ULL << 28)
#define VIRTIO_F_EVENT_IDX (1ULL << 29)

#define VIRTIO_F_VERSION_1 (1ULL << 32)
#define VIRTIO_F_ACCESS_PLATFORM (1ULL << 33)
#define VIRTIO_F_RING_PACKED (1ULL << 34)
#define VIRTIO_F_IN_ORDER (1ULL << 35)
#define VIRTIO_F_ORDER_PATFORM (1ULL << 36)
#define VIRTIO_F_SR_IOV (1ULL << 37)
#define VIRTIO_F_NOTIFICATION_DATA (1ULL << 38)

#define PCI_CAP_ENABLE (1 << 4)
#define PCI_CAP_TYPE_VENDOR 0x09
#define PCI_CMD_BIT_MEM (1 << 1)

using Genode::Heap;
using Genode::List;

using Platform::Device;
using Platform::Device_client;
using Platform::Device_capability;

using Virtio::Status;
using Virtio::SplitVirtqueue;
using Virtio::Pci::common_config_t;
using Virtio::Pci::cfg_type;
using Virtio::Pci::Driver;

struct Virtio::gpu::Driver : Virtio::Pci::Driver {
	SplitVirtqueue controlvq;
	SplitVirtqueue cursorvq;

	gpu::config_t* gpu_config;
	u16 queue_size;

	Driver(Driver const& drv) = delete;
	Driver operator=(Driver const&) = delete;
	
	Driver(Genode::Env& env, Heap& heap, Device_capability& dev, u16 queue_size) :
		Pci::Driver(env, heap, dev),
		controlvq(0, heap, queue_size),
		cursorvq(1, heap, queue_size),
		gpu_config(nullptr),
		queue_size(queue_size) { }

	bool handle_virtio_cap(const u8 cap_length,
			const u8 cfg_type,
			const u8 bar,
			const u32 data_off,
			const u32 data_len) override
	{
		Genode::log("len=", cap_length,
				", type=", cfg_type,
				", bar=", bar,
				", data=", data_off,
				"+", data_len);
		
		auto get_ptr = [this, bar, data_off] () -> u8* {
			Bar* b = find_bar(bar);
			if(b != nullptr) {
				//return b->local_addr<u8>() + (u64) data_off;				
				return b->local_addr();				
			}
			return nullptr;
		};

		switch(cfg_type) {
			case cfg_type::Common:
				{
					//common_cfg = get_ptr();
					common_cfg = (common_config_t*) get_ptr();
					Genode::log("off=", (u64) data_off,
							", common_cfg=", common_cfg);
				}
				break;
			case cfg_type::Notify:
				{
				}
				break;
			case cfg_type::ISR:
				{
				}
				break;
			case cfg_type::Device:
				{
					gpu_config = (gpu::config_t*) get_ptr();
					Genode::log("GPU_config=", gpu_config);
				}
				break;
			case cfg_type::PCI_Config:
				{
				}
				break;
			default:
				Genode::log("Unknown vendor cap.");
				break;
		}
		return false;
	}

};

/*
void overwrite_bar(u8 bar_id, Virtio::Pci::Driver& drv, u32 new_addr) {
	u8 bar_offset = 0x10 + (4 * bar_id);
	u32 bar = drv.config_read(bar_offset, Device::ACCESS_32BIT);
	u16 cmd = drv.config_read(0x04, Device::ACCESS_16BIT);
	u32 bar_masked = bar & 0xf;
	u32 cmd_masked = cmd & 0xfffc;
	
	Genode::log("BAR=", bar);
	
	drv.config_write(0x04, cmd_masked, Device::ACCESS_16BIT);	
	drv.config_write(bar_offset, new_addr | bar_masked, Device::ACCESS_32BIT);
	drv.config_write(0x04, cmd, Device::ACCESS_16BIT);
	
	bar = drv.config_read(bar_offset, Device::ACCESS_32BIT);
	Genode::log("BAR=", bar);
}
*/

void handle_virtio_cap(Virtio::Pci::Driver& driver, u8 cap) {
	Device_client& dev = driver.device();

	u8 len = dev.config_read(cap+2, Device::ACCESS_8BIT);
	u8 t = dev.config_read(cap+3, Device::ACCESS_8BIT);
	u8 bar = dev.config_read(cap+4, Device::ACCESS_8BIT);
	u32 data_off = dev.config_read(cap+8, Device::ACCESS_32BIT);
	u32 data_len = dev.config_read(cap+12, Device::ACCESS_32BIT);

	//Genode::log("cap: addr=", data_off, "+", data_len);
	auto a = dev.resource(bar);
	auto b = a.type();
	if(b == Device::Resource::Type::IO) {
		Genode::log("bar: ", bar, ", IO");
	} else {
		Genode::log("bar: ", bar, ", Mem");
	}

	driver.handle_virtio_cap(len, t, bar, data_off, data_len);
}

void map_bars(Virtio::Pci::Driver& driver) {
	for(int i = 0; i < 5; i++) {
		Device::Resource bar = driver.device().resource(i);
		
		if(bar.size() > 0) {
			Driver::Bar* b = driver.track_bar(i);
			Genode::log("Tracking Bar #", b->id);
		}
	}	
}

void init_device(Virtio::Pci::Driver& driver) {
	Platform::Device_client& dev = driver.device();
	u16 status = dev.config_read(0x06, Device::ACCESS_16BIT);
	u8 pci_caps = dev.config_read(0x34, Device::ACCESS_8BIT) & 0xfc;

	if(status & PCI_CAP_ENABLE) {	
		Genode::log("Started probing caps...");
		u8 cap = pci_caps;
		bool done = false;
		for (int i = 0; i < 12; i++) {
			//Genode::log("Checking cap #", i, "...");
			u8 id =	dev.config_read(cap, Device::ACCESS_8BIT);
			u8 next = dev.config_read(cap+1, Device::ACCESS_8BIT);
			switch (id) {
				case PCI_CAP_TYPE_VENDOR:
					Genode::log("Virtio cap.");
					handle_virtio_cap(driver, cap);
					break;
				case 0:
					done = true;
					break;
				default:
					Genode::log("Ignoring cap.");
					break;
			}

			if (done) {
				break;
			}
			cap = next;
		}
	}

}

void update_features(Virtio::Pci::Driver& driver) {
	u64 features = driver.get_features();
	u64 features_new = 0;
	if(features & VIRTIO_F_VERSION_1) {
		features_new |= VIRTIO_F_VERSION_1;
	}
	if(features & VIRTIO_F_ACCESS_PLATFORM) {
		features_new |= VIRTIO_F_ACCESS_PLATFORM;
	}
}

void probe_device(Genode::Env& env, Genode::Heap& heap, Platform::Device_capability cap) {
	Virtio::gpu::Driver driver(env, heap, cap, 2000);

	Platform::Device_client& dev = driver.device();

	u16 vendor_id = dev.vendor_id();
	u16 device_id = dev.device_id();
	Genode::log("Checking device: ", "vendor_id=", vendor_id, " device_id=", device_id, "...");

	Virtio::DeviceID type = driver.get_virtio_type();

	if(type != Virtio::DeviceID::Gpu) {
		Genode::log("Device not GPU or not Virtio.");
	} else {
		Genode::log("Device is GPU.");

		map_bars(driver);
		init_device(driver);
		
		if(driver.ok()) {
			driver.reset();
			driver.set_status(driver.get_status() | Status::Acknowledge);
			driver.set_status(driver.get_status() | Status::Driver_avail);
			update_features(driver);
			driver.set_status(driver.get_status() | Status::Features_ok);
			if(driver.get_status() & Status::Features_ok || ! (driver.get_status() & Status::Failed)) {
				driver.set_status(driver.get_status() | Status::Driver_ok);
//				SplitVirtqueue& controlvq = driver.controlvq();
//				controlvq.;
				driver.queue_enable(0);
				Genode::log("Device has ", driver.num_queues(), " virtqueues.");
				Genode::log("Initialized driver successfully.");
				return;
			}
		}
		Genode::log("Failed to initialize driver.");
	}
}

void probe_devices(Genode::Env& env) {
	Platform::Connection pci(env);
	Heap heap(env.ram(), env.rm());
        
	Platform::Device_capability prev_device_cap, device_cap;
	pci.with_upgrade([&] () { device_cap = pci.first_device(); });
	
	/*
	 * Iterate through all installed devices
	 * and print the available device information.
	 */
	while (device_cap.valid()) {
		probe_device(env, heap, device_cap);
	
		/* release last device */
		pci.release_device(prev_device_cap);
		prev_device_cap = device_cap;

		pci.with_upgrade([&] () { device_cap = pci.next_device(device_cap); });
	}
}

void Component::construct(Genode::Env& env) {
	probe_devices(env);
}


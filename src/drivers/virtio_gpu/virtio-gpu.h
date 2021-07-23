#include <base/stdint.h>
#define VIRTIO_GPU_FLAG_FENCE (1 << 0)
#define VIRTIO_GPU_MAX_SCANOUTS 20

namespace Virtio::gpu {
	class Driver;

	struct config_t;
	struct rect_t;
	struct display_info_t;
	
	struct ctrl_header_t;
	struct resp_display_info_t;	

	enum feature {
		VIRTIO_GPU_F_VIRGL = 0,
		VIRTIO_GPU_F_EDID = 1,
	};

	enum ctrl_type {
	
		/* 2d commands */ 
		VIRTIO_GPU_CMD_GET_DISPLAY_INFO = 0x0100, 
		VIRTIO_GPU_CMD_RESOURCE_CREATE_2D, 
		VIRTIO_GPU_CMD_RESOURCE_UNREF, 
		VIRTIO_GPU_CMD_SET_SCANOUT, 
		VIRTIO_GPU_CMD_RESOURCE_FLUSH, 
		VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D, 
		VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING, 
		VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING,
		VIRTIO_GPU_CMD_GET_CAPSET_INFO, 
		VIRTIO_GPU_CMD_GET_CAPSET, 
		VIRTIO_GPU_CMD_GET_EDID, 
 
		/* cursor commands */ 
		VIRTIO_GPU_CMD_UPDATE_CURSOR = 0x0300, 
		VIRTIO_GPU_CMD_MOVE_CURSOR, 
 
		/* success responses */ 
		VIRTIO_GPU_RESP_OK_NODATA = 0x1100, 
		VIRTIO_GPU_RESP_OK_DISPLAY_INFO, 
		VIRTIO_GPU_RESP_OK_CAPSET_INFO,
		VIRTIO_GPU_RESP_OK_CAPSET, 
		VIRTIO_GPU_RESP_OK_EDID, 
 
		/* error responses */ 
		VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200, 
		VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY, 
		VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID, 
		VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID, 
		VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID, 
		VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER, 
	};
}

struct Virtio::gpu::rect_t {
	u32 x;
	u32 y;
	u32 w;
	u32 h;
};

struct Virtio::gpu::config_t {
private:
	u32 events_read;
	u32 events_clear;
	u32 num_scanouts;

};

struct Virtio::gpu::ctrl_header_t {
	u32 type;
	u32 flags;
	u64 fence_id;
	u32 ctx_id;
	u32 padding;
};

struct Virtio::gpu::display_info_t {
	rect_t r;
	u32 enabled;
	u32 flags;
};

struct Virtio::gpu::resp_display_info_t {
	struct ctrl_header_t hdr;
	struct display_info_t pmodes[VIRTIO_GPU_MAX_SCANOUTS];
};




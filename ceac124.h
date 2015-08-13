#pragma once

#include "util.h"
#include "device.h"

typedef struct
{
	u8 dev_mode;
	u8 label;
	u16 padc;
	u8 file_ident;
	u16 pdac;
}
CEAC124_DeviceStatus;

typedef struct
{
	u8 device_code;
	u8 hw_version;
	u8 sw_version;
	u8 reason;
}
CEAC124_DeviceAttrib;

typedef
struct CEAC124
{
	// device instance
	CAN_Device can_device;
	
	// callbacks
	void *cb_cookie; // callback user data
	void (*cb_device_status)(void *, CEAC124_DeviceStatus);
	void (*cb_device_attrib)(void *, CEAC124_DeviceAttrib);
}
CEAC124;

int CEAC124_requestDeviceStatus(const CEAC124 *device);
int CEAC124_requestDeviceAttrib(const CEAC124 *device);

#pragma once

#include <stdio.h>

#include "util.h"
#include "device.h"

#define CEAC124_STOP_MEASURE            0x00
#define CEAC124_MULTI_CHANNEL_MEASURE   0x01
#define CEAC124_ONE_CHANNEL_MEASURE     0x02
#define CEAC124_DEVICE_STATUS           0xFE
#define CEAC124_DEVICE_ATTRIB           0xFF

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

typedef struct
{
	u8 channel_number;
	u8 gain_code;
	u8 time;
	u8 mode;
}
CEAC124_OneChannelMeasureProps;

#define CEAC124_MEASURE_GAIN_1    0x00
#define CEAC124_MEASURE_GAIN_10   0x01
#define CEAC124_MEASURE_GAIN_100  0x02
#define CEAC124_MEASURE_GAIN_1000 0x03

#define CEAC124_MEASURE_TIME_1MS   0x00
#define CEAC124_MEASURE_TIME_2MS   0x01
#define CEAC124_MEASURE_TIME_5MS   0x02
#define CEAC124_MEASURE_TIME_10MS  0x03
#define CEAC124_MEASURE_TIME_20MS  0x04
#define CEAC124_MEASURE_TIME_40MS  0x05
#define CEAC124_MEASURE_TIME_80MS  0x06
#define CEAC124_MEASURE_TIME_160MS 0x07

#define CEAC124_MEASURE_MODE_SINGLE     0x00
#define CEAC124_MEASURE_MODE_CONTINUOUS 0x10
#define CEAC124_MEASURE_MODE_STORE      0x00
#define CEAC124_MEASURE_MODE_SEND       0x20

typedef struct
{
	u8 channel_number;
	u8 gain_code;
	u32 voltage_code;
	double voltage;
}
CEAC124_OneChannelMeasureResult;

typedef
struct CEAC124
{
	// device instance
	CAN_Device can_device;
	
	// callbacks
	void *cb_cookie; // callback user data
	void (*cb_one_channel_measure)(void *, CEAC124_OneChannelMeasureResult);
	void (*cb_device_status)(void *, CEAC124_DeviceStatus);
	void (*cb_device_attrib)(void *, CEAC124_DeviceAttrib);
}
CEAC124;

int CEAC124_setup(CEAC124 *device, int id, CAN_Node *node)
{
	CAN_setupDevice(&device->can_device, id, node);
	device->cb_cookie = NULL;
	device->cb_one_channel_measure = NULL;
	device->cb_device_status = NULL;
	device->cb_device_attrib = NULL;
	return 0;
}

int CEAC124_requestStopMeasure(const CEAC124 *device)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 1;
	frame.data[0] = CEAC124_STOP_MEASURE;
	return CAN_send(device->can_device.node, &frame);
}

int CEAC124_requestOneChannelMeasure(const CEAC124 *device, CEAC124_OneChannelMeasureProps *props)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 4;
	frame.data[0] = CEAC124_ONE_CHANNEL_MEASURE;
	frame.data[1] = (props->gain_code << 6) | props->channel_number;
	frame.data[2] = props->time;
	frame.data[3] = props->mode;
	return CAN_send(device->can_device.node, &frame);
}

int CEAC124_requestDeviceStatus(const CEAC124 *device)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 1;
	frame.data[0] = CEAC124_DEVICE_STATUS;
	return CAN_send(device->can_device.node, &frame);
}

int CEAC124_requestDeviceAttrib(const CEAC124 *device)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 1;
	frame.data[0] = CEAC124_DEVICE_ATTRIB;
	return CAN_send(device->can_device.node, &frame);
}

void CEAC124_ListenerCallback(void *cookie, struct can_frame *frame)
{
	CEAC124 *dev = (CEAC124 *) cookie;
	
	int id = (frame->can_id & 0xfc) >> 2;
	printf("id: %d\n", id);
	if(dev->can_device.id != id)
		return;
	
	int cmd = frame->data[0];
	printf("cmd: %d\n", cmd);
	
	if(cmd == CEAC124_ONE_CHANNEL_MEASURE)
	{
		CEAC124_OneChannelMeasureResult res;
		res.channel_number = frame->data[1] & 0x3f;
		res.gain_code = (frame->data[1] >> 6) & 0x3;
		res.voltage_code = ((u32) frame->data[4] << 0x10) | ((u32) frame->data[3] << 0x8) | (u32) frame->data[2];
		if(res.voltage_code & 0x800000)
			res.voltage = -(double) ((~res.voltage_code) & 0xffffff);
		else
			res.voltage = res.voltage_code;
		res.voltage *= 10.0/0x3fffff;
		if(dev->cb_one_channel_measure != NULL) dev->cb_one_channel_measure(dev->cb_cookie, res);
	}
	else
	if(cmd == CEAC124_DEVICE_STATUS)
	{
		CEAC124_DeviceStatus st;
		st.dev_mode = frame->data[1];
		st.label = frame->data[2];
		st.padc = (((u16) frame->data[5]) << 0x10) & frame->data[4];
		st.file_ident = frame->data[5];
		st.pdac = (((u16) frame->data[7]) << 0x10) & frame->data[6];
		if(dev->cb_device_status != NULL) dev->cb_device_status(dev->cb_cookie, st);
	}
	else
	if(cmd == CEAC124_DEVICE_ATTRIB)
	{
		CEAC124_DeviceAttrib attr;
		attr.device_code = frame->data[1];
		attr.hw_version = frame->data[2];
		attr.sw_version = frame->data[3];
		attr.reason = frame->data[4];
		if(dev->cb_device_attrib != NULL) dev->cb_device_attrib(dev->cb_cookie, attr);
	}
}

int CEAC124_listen(CEAC124 *device, int *done)
{
	return CAN_listen(device->can_device.node, CEAC124_ListenerCallback, device, done);
}

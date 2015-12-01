#pragma once

#include <stdio.h>

#include "util.h"
#include "device.h"

#define CEAC124_ADC_COUNT 12
#define CEAC124_DAC_COUNT 4

#define CEAC124_CMD_ADC_STOP          0x00
#define CEAC124_CMD_ADC_READ_M        0x01
#define CEAC124_CMD_ADC_READ_S        0x02
#define CEAC124_CMD_DAC_WRITE         0x80
#define CEAC124_CMD_DAC_READ          0x90
#define CEAC124_CMD_DEV_STATUS        0xFE
#define CEAC124_CMD_DEV_ATTRIB        0xFF

#define CEAC124_ADC_MIN_VOLTAGE  -10.0
#define CEAC124_ADC_MAX_VOLTAGE   10.0
#define CEAC124_DAC_MIN_VOLTAGE  -10.0
#define CEAC124_DAC_MAX_VOLTAGE    9.9997
#define CEAC124_DAC_EPS 1e-4

typedef struct
{
	u8 dev_mode;
	u8 label;
	u16 padc;
	u8 file_ident;
	u16 pdac;
}
CEAC124_DevStatus;

typedef struct
{
	u8 device_code;
	u8 hw_version;
	u8 sw_version;
	u8 reason;
}
CEAC124_DevAttrib;

typedef struct
{
	u8 channel_number;
	u8 gain_code;
	u8 time;
	u8 mode;
}
CEAC124_ADCReadSProp;

typedef struct
{
	u8 channel_number;
	u8 use_code;
	u16 voltage_code;
	double voltage;
	// TODO: file attribs here
}
CEAC124_DACWriteProp;

typedef struct
{
	u8 channel_number;
}
CEAC124_DACReadProp;

#define CEAC124_ADC_READ_GAIN_1    0x00
#define CEAC124_ADC_READ_GAIN_10   0x01
#define CEAC124_ADC_READ_GAIN_100  0x02
#define CEAC124_ADC_READ_GAIN_1000 0x03

#define CEAC124_ADC_READ_TIME_1MS   0x00
#define CEAC124_ADC_READ_TIME_2MS   0x01
#define CEAC124_ADC_READ_TIME_5MS   0x02
#define CEAC124_ADC_READ_TIME_10MS  0x03
#define CEAC124_ADC_READ_TIME_20MS  0x04
#define CEAC124_ADC_READ_TIME_40MS  0x05
#define CEAC124_ADC_READ_TIME_80MS  0x06
#define CEAC124_ADC_READ_TIME_160MS 0x07

#define CEAC124_ADC_READ_MODE_SINGLE     0x00
#define CEAC124_ADC_READ_MODE_CONTINUOUS 0x10
#define CEAC124_ADC_READ_MODE_STORE      0x00
#define CEAC124_ADC_READ_MODE_SEND       0x20

typedef struct
{
	u8 channel_number;
	u8 gain_code;
	u32 voltage_code;
	double voltage;
}
CEAC124_ADCReadResult;

typedef struct
{
	u8 channel_number;
	u16 voltage_code;
	double voltage;
	// TODO: file attrib here
}
CEAC124_DACReadResult;

typedef
struct CEAC124
{
	// device instance
	CAN_Device can_device;
	
	// callbacks
	void *cb_cookie; // callback user data
	void (*cb_adc_read_s)(void *, const CEAC124_ADCReadResult *);
	void (*cb_dac_read)  (void *, const CEAC124_DACReadResult *);
	void (*cb_dev_status)(void *, const CEAC124_DevStatus *);
	void (*cb_dev_attrib)(void *, const CEAC124_DevAttrib *);
}
CEAC124;

int CEAC124_setup(CEAC124 *device, int id, CAN_Node *node)
{
	CAN_setupDevice(&device->can_device, id, node);
	device->cb_cookie = NULL;
	device->cb_adc_read_s = NULL;
	device->cb_dac_read   = NULL;
	device->cb_dev_status = NULL;
	device->cb_dev_attrib = NULL;
	return 0;
}

int CEAC124_adcStop(const CEAC124 *device)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 1;
	frame.data[0] = CEAC124_CMD_ADC_STOP;
	return CAN_send(device->can_device.node, &frame);
}

int CEAC124_adcReadS(const CEAC124 *device, CEAC124_ADCReadSProp *prop)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 4;
	frame.data[0] = CEAC124_CMD_ADC_READ_S;
	frame.data[1] = (prop->gain_code << 6) | prop->channel_number;
	frame.data[2] = prop->time;
	frame.data[3] = prop->mode;
	return CAN_send(device->can_device.node, &frame);
}

/** 
 *  Write data to one of DAC channels
 *    If prop->use_code != 0, then prop->voltage_code will be written to DAC
 *    Otherwise prop->voltage will be converted to code using following rule:
 *    If value is out of [CEAC124_DAC_MIN_VOLTAGE, CEAC124_DAC_MAX_VOLTAGE],
 *      it will be clamped to these bounds.
 */
int CEAC124_dacWrite(const CEAC124 *device, CEAC124_DACWriteProp *prop)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 3;
	frame.data[0] = CEAC124_CMD_DAC_WRITE | prop->channel_number;
	u16 voltage_code = prop->voltage_code;
	if(prop->use_code == 0)
	{
		double voltage = prop->voltage;
		if(voltage > CEAC124_DAC_MAX_VOLTAGE)
			voltage = CEAC124_DAC_MAX_VOLTAGE;
		else
		if(voltage < CEAC124_DAC_MIN_VOLTAGE)
			voltage = CEAC124_DAC_MIN_VOLTAGE;
		voltage = (voltage - CEAC124_DAC_MIN_VOLTAGE)/(CEAC124_DAC_MAX_VOLTAGE - CEAC124_DAC_MIN_VOLTAGE);
		i32 wide_code = (i32) (voltage * 0xFFFF);
		if(wide_code < 0)
			wide_code = 0;
		else
		if(wide_code > 0xFFFF)
			wide_code = 0xFFFF;
		voltage_code = wide_code & 0xFFFF;
		// printf("voltage_code: 0x%x\n", voltage_code);
	}
	frame.data[1] = voltage_code >> 8;
	frame.data[2] = voltage_code & 0xFF;
	return CAN_send(device->can_device.node, &frame);
}

int CEAC124_dacRead(const CEAC124 *device, CEAC124_DACReadProp *prop)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 1;
	frame.data[0] = CEAC124_CMD_DAC_READ | (prop->channel_number & 0x3);
	return CAN_send(device->can_device.node, &frame);
}

int CEAC124_getDevStatus(const CEAC124 *device)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 1;
	frame.data[0] = CEAC124_CMD_DEV_STATUS;
	return CAN_send(device->can_device.node, &frame);
}

int CEAC124_getDevAttrib(const CEAC124 *device)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 1;
	frame.data[0] = CEAC124_CMD_DEV_ATTRIB;
	return CAN_send(device->can_device.node, &frame);
}

void CEAC124_ListenerCallback(void *cookie, struct can_frame *frame)
{
	CEAC124 *dev = (CEAC124 *) cookie;
	
	int id = (frame->can_id & 0xfc) >> 2;
	//printf("id: 0x%x\n", id);
	if(dev->can_device.id != id)
		return;
	
	int cmd = frame->data[0];
	//printf("cmd: 0x%x\n", cmd);
	
	if(cmd == CEAC124_CMD_ADC_READ_S)
	{
		CEAC124_ADCReadResult res;
		res.channel_number = frame->data[1] & 0x3f;
		res.gain_code = (frame->data[1] >> 6) & 0x3;
		res.voltage_code = ((u32) frame->data[4] << 0x10) | ((u32) frame->data[3] << 0x8) | (u32) frame->data[2];
		if(res.voltage_code & 0x800000)
			res.voltage = -(double) ((~res.voltage_code) & 0xffffff);
		else
			res.voltage = res.voltage_code;
		res.voltage *= 10.0/0x3fffff;
		if(dev->cb_adc_read_s != NULL) dev->cb_adc_read_s(dev->cb_cookie, &res);
	}
	else
	if((cmd & 0xFC) == CEAC124_CMD_DAC_READ)
	{
		CEAC124_DACReadResult res;
		res.channel_number = cmd & 0x3;
		res.voltage_code = (((u16) frame->data[1]) << 8) | ((u16) frame->data[2]);
		res.voltage = ((((double) res.voltage_code)/0xFFFF) * (CEAC124_DAC_MAX_VOLTAGE - CEAC124_DAC_MIN_VOLTAGE)) + CEAC124_DAC_MIN_VOLTAGE;
		if(dev->cb_dac_read != NULL) dev->cb_dac_read(dev->cb_cookie, &res);
	}
	else
	if(cmd == CEAC124_CMD_DEV_STATUS)
	{
		CEAC124_DevStatus st;
		st.dev_mode = frame->data[1];
		st.label = frame->data[2];
		st.padc = (((u16) frame->data[5]) << 0x10) & frame->data[4];
		st.file_ident = frame->data[5];
		st.pdac = (((u16) frame->data[7]) << 0x10) & frame->data[6];
		if(dev->cb_dev_status != NULL) dev->cb_dev_status(dev->cb_cookie, &st);
	}
	else
	if(cmd == CEAC124_CMD_DEV_ATTRIB)
	{
		CEAC124_DevAttrib attr;
		attr.device_code = frame->data[1];
		attr.hw_version = frame->data[2];
		attr.sw_version = frame->data[3];
		attr.reason = frame->data[4];
		if(dev->cb_dev_attrib != NULL) dev->cb_dev_attrib(dev->cb_cookie, &attr);
	}
}

int CEAC124_listen(CEAC124 *device, int *done)
{
	return CAN_listen(device->can_device.node, CEAC124_ListenerCallback, device, done);
}

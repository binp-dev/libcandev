#pragma once

#include <stdio.h>

#include "util.h"
#include "device.h"

#define KOZ_CMD_ADC_STOP          0x00
#define KOZ_CMD_ADC_READ_M        0x01
#define KOZ_CMD_ADC_READ_S        0x02
#define KOZ_CMD_DAC_WRITE         0x80
#define KOZ_CMD_DAC_READ          0x90
#define KOZ_CMD_DEV_STATUS        0xFE
#define KOZ_CMD_DEV_ATTRIB        0xFF

#define KOZ_ADC_MIN_VOLTAGE  -10.0
#define KOZ_ADC_MAX_VOLTAGE   10.0
#define KOZ_DAC_MIN_VOLTAGE  -10.0
#define KOZ_DAC_MAX_VOLTAGE    9.9997
#define KOZ_DAC_EPS 1e-4

typedef struct
{
	u8 dev_mode;
	u8 label;
	u16 padc;
	u8 file_ident;
	u16 pdac;
}
KOZ_DevStatus;

typedef struct
{
	u8 device_code;
	u8 hw_version;
	u8 sw_version;
	u8 reason;
}
KOZ_DevAttrib;

typedef struct
{
	u8 channel_number;
	u8 gain_code;
	u8 time;
	u8 mode;
}
KOZ_ADCReadSProp;

typedef struct
{
	u8 channel_number;
	u8 use_code;
	u16 voltage_code;
	double voltage;
	// TODO: file attribs here
}
KOZ_DACWriteProp;

typedef struct
{
	u8 channel_number;
}
KOZ_DACReadProp;

#define KOZ_ADC_READ_GAIN_1    0x00
#define KOZ_ADC_READ_GAIN_10   0x01
#define KOZ_ADC_READ_GAIN_100  0x02
#define KOZ_ADC_READ_GAIN_1000 0x03

#define KOZ_ADC_READ_TIME_1MS   0x00
#define KOZ_ADC_READ_TIME_2MS   0x01
#define KOZ_ADC_READ_TIME_5MS   0x02
#define KOZ_ADC_READ_TIME_10MS  0x03
#define KOZ_ADC_READ_TIME_20MS  0x04
#define KOZ_ADC_READ_TIME_40MS  0x05
#define KOZ_ADC_READ_TIME_80MS  0x06
#define KOZ_ADC_READ_TIME_160MS 0x07

#define KOZ_ADC_READ_MODE_SINGLE     0x00
#define KOZ_ADC_READ_MODE_CONTINUOUS 0x10
#define KOZ_ADC_READ_MODE_STORE      0x00
#define KOZ_ADC_READ_MODE_SEND       0x20

typedef struct
{
	u8 channel_number;
	u8 gain_code;
	u32 voltage_code;
	double voltage;
}
KOZ_ADCReadResult;

typedef struct
{
	u8 channel_number;
	u16 voltage_code;
	double voltage;
	// TODO: file attrib here
}
KOZ_DACReadResult;

typedef
struct KOZ
{
	// device instance
	CAN_Device can_device;
	
	// callbacks
	void *cb_cookie; // callback user data
	void (*cb_adc_read_s)(void *, const KOZ_ADCReadResult *);
	void (*cb_dac_read)  (void *, const KOZ_DACReadResult *);
	void (*cb_dev_status)(void *, const KOZ_DevStatus *);
	void (*cb_dev_attrib)(void *, const KOZ_DevAttrib *);
}
KOZ;

int KOZ_setup(KOZ *device, int id, CAN_Node *node)
{
	CAN_setupDevice(&device->can_device, id, node);
	device->cb_cookie = NULL;
	device->cb_adc_read_s = NULL;
	device->cb_dac_read   = NULL;
	device->cb_dev_status = NULL;
	device->cb_dev_attrib = NULL;
	return 0;
}

int KOZ_adcStop(const KOZ *device)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 1;
	frame.data[0] = KOZ_CMD_ADC_STOP;
	return CAN_send(device->can_device.node, &frame);
}

int KOZ_adcReadS(const KOZ *device, KOZ_ADCReadSProp *prop)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 4;
	frame.data[0] = KOZ_CMD_ADC_READ_S;
	frame.data[1] = (prop->gain_code << 6) | prop->channel_number;
	frame.data[2] = prop->time;
	frame.data[3] = prop->mode;
	return CAN_send(device->can_device.node, &frame);
}

/** 
 *  Write data to one of DAC channels
 *    If prop->use_code != 0, then prop->voltage_code will be written to DAC
 *    Otherwise prop->voltage will be converted to code using following rule:
 *    If value is out of [KOZ_DAC_MIN_VOLTAGE, KOZ_DAC_MAX_VOLTAGE],
 *      it will be clamped to these bounds.
 */
int KOZ_dacWrite(const KOZ *device, KOZ_DACWriteProp *prop)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 3;
	frame.data[0] = KOZ_CMD_DAC_WRITE | prop->channel_number;
	u16 voltage_code = prop->voltage_code;
	if(prop->use_code == 0)
	{
		double voltage = prop->voltage;
		if(voltage > KOZ_DAC_MAX_VOLTAGE)
			voltage = KOZ_DAC_MAX_VOLTAGE;
		else
		if(voltage < KOZ_DAC_MIN_VOLTAGE)
			voltage = KOZ_DAC_MIN_VOLTAGE;
		voltage = (voltage - KOZ_DAC_MIN_VOLTAGE)/(KOZ_DAC_MAX_VOLTAGE - KOZ_DAC_MIN_VOLTAGE);
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

int KOZ_dacRead(const KOZ *device, KOZ_DACReadProp *prop)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 1;
	frame.data[0] = KOZ_CMD_DAC_READ | (prop->channel_number & 0x3);
	return CAN_send(device->can_device.node, &frame);
}

int KOZ_getDevStatus(const KOZ *device)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 1;
	frame.data[0] = KOZ_CMD_DEV_STATUS;
	return CAN_send(device->can_device.node, &frame);
}

int KOZ_getDevAttrib(const KOZ *device)
{
	struct can_frame frame;
	frame.can_id = (6 << 8) | (device->can_device.id << 2);
	frame.can_dlc = 1;
	frame.data[0] = KOZ_CMD_DEV_ATTRIB;
	return CAN_send(device->can_device.node, &frame);
}

void KOZ_ListenerCallback(void *cookie, struct can_frame *frame)
{
	KOZ *dev = (KOZ *) cookie;
	
	int id = (frame->can_id & 0xfc) >> 2;
	//printf("id: 0x%x\n", id);
	if(dev->can_device.id != id)
		return;
	
	int cmd = frame->data[0];
	//printf("cmd: 0x%x\n", cmd);
	
	if(cmd == KOZ_CMD_ADC_READ_S)
	{
		KOZ_ADCReadResult res;
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
	if((cmd & 0xFC) == KOZ_CMD_DAC_READ)
	{
		KOZ_DACReadResult res;
		res.channel_number = cmd & 0x3;
		res.voltage_code = (((u16) frame->data[1]) << 8) | ((u16) frame->data[2]);
		res.voltage = ((((double) res.voltage_code)/0xFFFF) * (KOZ_DAC_MAX_VOLTAGE - KOZ_DAC_MIN_VOLTAGE)) + KOZ_DAC_MIN_VOLTAGE;
		if(dev->cb_dac_read != NULL) dev->cb_dac_read(dev->cb_cookie, &res);
	}
	else
	if(cmd == KOZ_CMD_DEV_STATUS)
	{
		KOZ_DevStatus st;
		st.dev_mode = frame->data[1];
		st.label = frame->data[2];
		st.padc = (((u16) frame->data[5]) << 0x10) & frame->data[4];
		st.file_ident = frame->data[5];
		st.pdac = (((u16) frame->data[7]) << 0x10) & frame->data[6];
		if(dev->cb_dev_status != NULL) dev->cb_dev_status(dev->cb_cookie, &st);
	}
	else
	if(cmd == KOZ_CMD_DEV_ATTRIB)
	{
		KOZ_DevAttrib attr;
		attr.device_code = frame->data[1];
		attr.hw_version = frame->data[2];
		attr.sw_version = frame->data[3];
		attr.reason = frame->data[4];
		if(dev->cb_dev_attrib != NULL) dev->cb_dev_attrib(dev->cb_cookie, &attr);
	}
}

int KOZ_listen(KOZ *device, int *done)
{
	return CAN_listen(device->can_device.node, KOZ_ListenerCallback, device, done);
}

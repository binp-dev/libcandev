#pragma once

typedef 
struct CAN_Device
{
	int id;
}
CAN_Device;

int CAN_setupDevice(CAN_Device *device, int id)
{
	device->id = id;
	device->node = node;
}

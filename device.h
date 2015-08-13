#pragma once

#include "node.h"

typedef struct CAN_Device
{
	int id;
	CAN_Node *node;
}
CAN_Device;

int CAN_setupDevice(CAN_Device *device, int id, CAN_Node *node)
{
	device->id = id;
	device->node = node;
	return 0;
}

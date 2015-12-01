#pragma once

#include <stdlib.h>
//#include <unistd.h>
#include <string.h>
#include <errno.h>

#ifdef __XENO__
#include <rtdm/can.h>
#else
#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#endif

#define CAN_NAME_MAX_LEN 0x40

typedef
struct CAN_Node
{
	int fd;
	struct sockaddr_can addr;
	char ifname[CAN_NAME_MAX_LEN];
	struct ifreq ifr;
}
CAN_Node;

int CAN_createNode(CAN_Node *node, const char *name);
void CAN_destroyNode(CAN_Node *node);

int CAN_send(const CAN_Node *node, const struct can_frame *frame);
int CAN_receive(const CAN_Node *node, struct can_frame *frame);
int CAN_listen(const CAN_Node *node, void (*callback)(void *, struct can_frame *), void *cookie, int *done);

#pragma once

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <net/if.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>

#include <linux/can.h>
#include <linux/can/raw.h>

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

int CAN_createNode(CAN_Node *node, const char *name)
{
	int status;
	
	strncpy(node->ifname, name, CAN_NAME_MAX_LEN);
	
	node->fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
	if(node->fd < 0)
	{
		perror("Error while opening socket");
		return 1;
	}
	
	strcpy(node->ifr.ifr_name, node->ifname);
	status = ioctl(node->fd, SIOCGIFINDEX, &node->ifr);
	if(status < 0)
	{
		close(node->fd);
		perror("Error SIOCGIFINDEX ioctl to CAN socket");
		return 2;
	}
	
	node->addr.can_family  = AF_CAN;
	node->addr.can_ifindex = ifr.ifr_ifindex;
	
	printf("%s at index %d\n", node->ifname, node->ifr.ifr_ifindex);
	
	status = bind(node->fd, (struct sockaddr *) &node->addr, sizeof(node->addr));
	if(status < 0) 
	{
		close(node->fd);
		perror("Error in socket bind");
		return 3;
	}
	
	return 0;
}

void CAN_destroyNode(CAN_Node *node)
{
	close(node->fd);
}

int CAN_send(const CAN_Node *node, const struct can_frame *frame)
{
	if(frame->can_dlc < 0 || frame->can_dlc > 8)
	{
		fprintf(stderr, "data_lenght not in range 0 .. 8");
		return 1;
	}
	
	int nbytes = write(node->fd, frame, sizeof(struct can_frame));
	
	if(nbytes < 0)
	{
		perror("Error write CAN raw socket");
		return 2;
	}
	
	return 0;
}

int CAN_receive(const CAN_Node *node, struct can_frame *frame)
{
	int nbytes = read(node->fd, frame, sizeof(struct can_frame));

	if(nbytes < 0)
	{
		perror("Error read CAN raw socket");
		return 2;
	}
	
	return 0;
}

#include "node.h"

#include <unistd.h>
#include <stdio.h>

int CAN_createNode(CAN_Node *node, const char *name)
{
	int status;
	//can_mode_t mode;
	
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
	
	memset(&node->addr, 0, sizeof(node->addr));
	node->addr.can_family  = AF_CAN;
	node->addr.can_ifindex = node->ifr.ifr_ifindex;
	
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

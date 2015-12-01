#include "node.h"

#include <stdio.h>

int CAN_send(const CAN_Node *node, const struct can_frame *frame)
{
	if(frame->can_dlc < 0 || frame->can_dlc > 8)
	{
		fprintf(stderr, "data_lenght not in range 0 .. 8");
		return 3;
	}
	
	int nbytes = send(node->fd, frame, sizeof(struct can_frame), 0);
	
	if(nbytes < 0)
	{
		perror("Error send to CAN raw socket");
		return 2;
	}
	else
	if(nbytes < sizeof(struct can_frame))
	{
		return 1;
	}
	
	return 0;
}

int CAN_receive(const CAN_Node *node, struct can_frame *frame)
{
	int nbytes = recv(node->fd, frame, sizeof(struct can_frame), 0);
	if(nbytes < 0)
	{
		perror("Error receive from CAN raw socket");
		return 2;
	}
	else
	if(nbytes < sizeof(struct can_frame))
	{
		return 1;
	}
	
	return 0;
}

int CAN_listen(const CAN_Node *node, void (*callback)(void *, struct can_frame *), void *cookie, int *done)
{
	int status;
	struct can_frame frame;
	
	while(!*done)
	{
		/*
		struct timeval tv;
		fd_set set;
		
		FD_ZERO(&set);
		FD_SET(node->fd, &set);
		tv.tv_sec = 0;
		tv.tv_usec = 10*1000;
		
		int s = select(node->fd + 1, &set, NULL, NULL, &tv);
		if(s < 0)
			perror("Error select CAN raw socket");
			return 1;
		if(s == 0)
			continue;
		*/
		
		status = CAN_receive(node, &frame);
		if(status != 0)
			return 2;
		
		callback(cookie, &frame);
	}
	
	return 0;
}

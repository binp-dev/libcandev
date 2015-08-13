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

#include <exception>
#include <string>
#include <iostream>

#include "frame.hpp"
#include "exception.hpp"
#include "utility.hpp"

class CANNode
{
private:
	int fd;
	struct sockaddr_can addr;
	std::string ifname;
	struct ifreq ifr;
	
public:
	CANNode(const std::string &name) throw(CANException)
	{
		int status;
		
		ifname = name;
		
		fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
		if(fd < 0)
		{
			throw CANException("error while opening socket", errno);
		}
		
		strcpy(ifr.ifr_name, ifname.data());
		status = ioctl(fd, SIOCGIFINDEX, &ifr);
		if(status < 0)
		{
			close(fd);
			throw CANException("error SIOCGIFINDEX ioctl to CAN socket");
		}
		
		addr.can_family  = AF_CAN;
		addr.can_ifindex = ifr.ifr_ifindex;
		
		std::cout << ifname << " at index " << ifr.ifr_ifindex << std::endl;
		
		status = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
		if(status < 0) 
		{
			close(fd);
			throw CANException("error in socket bind", errno);
		}
	}
	
	virtual ~CANNode()
	{
		close(fd);
	}
	
	int get_fd() const
	{
		return fd;
	}
	
	const std::string &getName() const
	{
		return ifname;
	}
	
	void send(const CANFrame &frame)
	{
		if(frame.data_length < 0 || frame.data_length > 8)
			throw CANException("data_lenght not in range 0 .. 8");
		
		struct can_frame cframe;
		
		cframe.can_id  = frame.identifier;
		cframe.can_dlc = frame.data_length;
		memcpy(cframe.data, frame.data, frame.data_length);
		
		int nbytes = write(fd, &cframe, sizeof(struct can_frame));
		
		if(nbytes < 0)
			throw CANException("error write CAN raw socket", errno);
	}
	
	void receive(CANFrame &frame)
	{
		struct can_frame cframe;
		
		int nbytes = read(fd, &cframe, sizeof(struct can_frame));
	
		if(nbytes < 0)
			throw CANException("error read CAN raw socket", errno);
	
		memcpy(frame.data, cframe.data, cframe.can_dlc);
		frame.data_length = cframe.can_dlc;
		frame.identifier = cframe.can_id;
	}
};

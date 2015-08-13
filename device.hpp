#pragma once

#include <can/node.hpp>

class Device
{
private:
	CANNode node;
	
public:
	Device(const char *name)
	  : node(name)
	{
		
	}
	
	virtual ~Device() = default;
	
	void send(const CANFrame &frame)
	{
		node.send(frame);
	}

	void receive(CANFrame &frame)
	{
		node.receive(frame);
	}
};

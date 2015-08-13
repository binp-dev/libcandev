#pragma once

#include <cstring>

#include "can/utility.hpp"
#include "device.hpp"

class CEAC124 : public Device
{
public:
	struct Message
	{
		// identifier
		u8 priority; // 3 bits
		u8 address;  // 6 bits
		u8 reserved; // 2 bits
		// total 11 bits
		
		// data length code
		u8 data_length; // 4 bits
		// total 4 bits
		
		// data
		u8 data[8]; // data
		// total 8 bytes
	};
	
public:
	CEAC124(const char *name)
	  : Device(name)
	{
		
	}
	
	virtual ~CEAC124() = default;
	
	void send(const Message &message)
	{
		CANFrame frame;
		frame.identifier = 
		    (message.priority << 8) |
		    (message.address  << 2) |
		    (message.reserved << 0);
		frame.data_length = message.data_length;
		memcpy(frame.data, message.data, 8);
		Device::send(frame);
	}
	
	void receive(Message &message)
	{
		CANFrame frame;
		Device::receive(frame);
		message.priority = (frame.identifier >> 8) & 0x7;
		message.address  = (frame.identifier >> 2) & 0x3f;
		message.reserved = (frame.identifier >> 0) & 0x3;
		message.data_length = frame.data_length;
		memcpy(message.data, frame.data, 8);
	}
	
};

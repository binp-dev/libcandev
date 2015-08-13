#pragma once

#include "utility.hpp"

struct CANFrame
{
	u32 identifier;
	u8  data_length;
	u8  data[8];
};

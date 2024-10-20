#pragma once
#include <inttypes.h>

struct bms_common_data_t
{
	uint32_t voltage;
	uint32_t current;
	int32_t  power;
};

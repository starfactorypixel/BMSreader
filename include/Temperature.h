#pragma once
#include <inttypes.h>
#include <drivers/BMSAnt_Data.h>
#include <CUtils.h>

namespace Temp
{
	static constexpr uint8_t temp_count = BMSANT::TempsNumber + 16;
	
	int8_t temperatures[2][temp_count];


	void OnTemperature1Stream(auto coll_el, uint8_t idx);
	void OnTemperature2Stream(auto coll_el, uint8_t idx);

	CollectionStream<int8_t> Temperature1Stream(temperatures[0], temp_count, OnTemperature1Stream);
	CollectionStream<int8_t> Temperature2Stream(temperatures[1], temp_count, OnTemperature2Stream);
	
	
	void OnTemperature1Stream(auto coll_el, uint8_t idx)
	{
		CANLib::obj_temperature_1.SetValue(0, (idx + 1), CAN_TIMER_TYPE_NONE, CAN_EVENT_TYPE_NORMAL);
		CANLib::obj_temperature_1.SetValue(1, coll_el, CAN_TIMER_TYPE_NONE, CAN_EVENT_TYPE_NORMAL);
		
		return;
	}
	void OnTemperature2Stream(auto coll_el, uint8_t idx)
	{
		CANLib::obj_temperature_2.SetValue(0, (idx + 1), CAN_TIMER_TYPE_NONE, CAN_EVENT_TYPE_NORMAL);
		CANLib::obj_temperature_2.SetValue(1, coll_el, CAN_TIMER_TYPE_NONE, CAN_EVENT_TYPE_NORMAL);
		
		return;
	}
	
	void Processing()
	{
		int8_t max[2] = {INT8_MIN, INT8_MIN};
		for(uint8_t i = 0; i < temp_count; ++i)
		{
			if(temperatures[0][i] > max[0])
				max[0] = temperatures[0][i];
			
			if(temperatures[1][i] > max[1])
				max[1] = temperatures[1][i];
		}

		CANLib::obj_max_temperature_1.SetValue(0, max[0]);
		CANLib::obj_max_temperature_2.SetValue(0, max[1]);
		
		return;
	}
	
	void PutFromBms(const uint8_t bms_idx, const int16_t *temp, uint8_t count)
	{
		if(bms_idx >= 2) return;
		if(true){}
		
		for(uint8_t i = 0; i < count; ++i)
		{
			temperatures[bms_idx][i] = temp[i];
		}
		Processing();
		
		return;
	}

	void PutFrom1Wire(const uint8_t bms_idx, int16_t *temp, uint8_t count)
	{
		if(bms_idx >= 2) return;
		if(true){}

		for(uint8_t i = 6; i < count+6; ++i)
		{
			temperatures[bms_idx][i] = temp[i] / 100;
		}
		Processing();
		
		return;
	}

	void PutFrom1WireByIdx(const uint8_t bms_idx, int16_t temp, uint8_t idx)
	{
		if(bms_idx >= 2) return;
		if(true){}
		
		temperatures[bms_idx][idx+6] = temp / 100;
		Processing();
		
		return;
	}
};

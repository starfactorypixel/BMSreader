#pragma once
#include <inttypes.h>
#include <drivers/BMSAnt_Data.h>
#include <CUtils.h>

namespace Temp
{
	static constexpr uint8_t TEMP_BMS_COUNT = BMSANT::TempsNumber;
	static constexpr uint8_t TEMP_EXT_COUNT = 16;
	static constexpr uint8_t TEMP_TOTAL_COUNT = TEMP_BMS_COUNT + TEMP_EXT_COUNT;
	
	int8_t temperatures[2][TEMP_TOTAL_COUNT];


	void OnTemperature1Stream(auto coll_el, uint8_t idx);
	void OnTemperature2Stream(auto coll_el, uint8_t idx);

	CollectionStream<int8_t> Temperature1Stream(temperatures[0], TEMP_TOTAL_COUNT, OnTemperature1Stream);
	CollectionStream<int8_t> Temperature2Stream(temperatures[1], TEMP_TOTAL_COUNT, OnTemperature2Stream);
	
	
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
		for(uint8_t i = 0; i < TEMP_TOTAL_COUNT; ++i)
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
		if(count > TEMP_BMS_COUNT) return;
		
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
		if(count > TEMP_EXT_COUNT) return;

		for(uint8_t i = 0; i < count; ++i)
		{
			temperatures[bms_idx][i + TEMP_BMS_COUNT] = temp[i] / 100;
		}
		Processing();
		
		return;
	}

	void PutFrom1WireByIdx(const uint8_t bms_idx, int16_t temp, uint8_t idx)
	{
		if(bms_idx >= 2) return;
		if(idx >= TEMP_EXT_COUNT) return;
		
		temperatures[bms_idx][idx + TEMP_BMS_COUNT] = temp / 100;
		Processing();
		
		return;
	}


	inline void Setup()
	{
		CANLib::obj_temperature_1.RegisterFunctionRequest([](can_frame_t &can_frame, can_error_t &error) -> can_result_t
		{
			Temperature1Stream.Start(100);
			
			return CAN_RESULT_IGNORE;
		});
		CANLib::obj_temperature_2.RegisterFunctionRequest([](can_frame_t &can_frame, can_error_t &error) -> can_result_t
		{
			Temperature2Stream.Start(100);
			
			return CAN_RESULT_IGNORE;
		});

		return;
	}
	
	inline void Loop(uint32_t &current_time)
	{
		Temperature1Stream.Processing(current_time);
		Temperature2Stream.Processing(current_time);
		
		current_time = HAL_GetTick();
		
		return;
	}
};

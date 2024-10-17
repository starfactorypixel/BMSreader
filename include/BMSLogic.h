#pragma once

#include "BMS_low_level_abstraction.h"

#include <bms_ant_data.h>

extern UART_HandleTypeDef hBms1Uart;
extern UART_HandleTypeDef hBms2Uart;





namespace BMSLogic
{
	//*********************************************************************
	// BMS firmware settings
	//*********************************************************************

	/// @brief BMS request period
	static constexpr uint32_t CFG_BMSRequestPeriod = 250;

    /// @brief BMS request timeout
    static constexpr uint32_t CFG_BMSRequestTimeout = 250;


	//*********************************************************************
	// BMS firmware constants
	//*********************************************************************

    /// @brief BMS get info request
    















	static constexpr uint8_t PACKET_TX_REQUEST[] = {0x5A, 0x5A, 0x00, 0x00, 0x00, 0x00};

	static constexpr uint8_t PACKET_RX_SIZE = 140;
	static constexpr uint8_t PACKET_RX_HEADER[] = {0xAA, 0x55, 0xAA, 0xFF};
	
	struct data_t
	{
		UART_HandleTypeDef *hal;			// Указатель на объект HAL USART
		uint8_t hot[PACKET_RX_SIZE + 60];	// Горячий массив данных (Работа в прерывании)
		uint8_t cold[PACKET_RX_SIZE];		// Холодный массив данных (Работа в программе)
		bool ready;							// Флаг того, что массив данные приняты и готовы к анализу
		bool busy;							// Флаг того, что разбор данных не окончен и новые копировать нельзя.
	} data[2];
	
	enum bms_num_t : uint8_t { BMS_1 = 0, BMS_2 = 1 };
	






	void ReverseArray(uint8_t *array, uint8_t length)
	{
		uint8_t i = 0;
		uint8_t j = length - 1;
		uint8_t temp;
		while(i < j)
		{
			temp = array[i];
			array[i] = array[j];
			array[j] = temp;
			
			i++;
			j--;
		}
		
		return;
	}








	void RxPacket(bms_num_t idx, uint16_t data_length)
	{
		data_t *obj = &data[idx];

		if(obj->busy == true) return;
		if(data_length != BMS_BOARD_PACKET_SIZE) return;
		if(memcmp(PACKET_RX_HEADER, obj->hot, sizeof(PACKET_RX_HEADER)) != 0) return;
		
		memcpy(obj->cold, obj->hot, sizeof(obj->cold));
		ReverseArray(obj->cold, sizeof(obj->cold));
		
		obj->ready = true;
		obj->busy = true;
		
		return;
	}
	
	bool CheckCRC(bms_num_t idx)
	{
		data_t *obj = &data[idx];
		BMSANTLib::packet_raw_reverse_t *data = (BMSANTLib::packet_raw_reverse_t *)obj->cold;
		
		uint16_t crc = 0x0000;
		for(uint8_t i = 4; i < 138; ++i)
		{
			crc += obj->cold[i];
		}
		if(data->crc != crc)
		{
			//DEBUG_LOG("ERROR: BMS CRC error! Expected: 0x%04X, presented: 0x%04X", bms_raw_data_crc(bms_raw_packet_data), get_bms_raw_data_crc(bms_raw_packet_data));
			DEBUG_LOG_TOPIC("BMS", "CRC error!");
			
			return false;
		}
		
		return true;
	}
	
	
	inline void Setup()
	{
		memset(data, 0x00, sizeof(data));

		data[BMS_1].hal = &hBms1Uart;
		data[BMS_2].hal = &hBms2Uart;
		
		HAL_UARTEx_ReceiveToIdle_IT(&hBms1Uart, data[BMS_1].hot, sizeof(data[BMS_1].hot));
		//HAL_UARTEx_ReceiveToIdle_IT(&hBms2Uart, data[BMS_2].hot, sizeof(data[BMS_2].hot));
		
		return;
	}
	
	inline void Loop(uint32_t &current_time)
	{
		static uint32_t last_tick = 0;
		if(current_time - last_tick > 0)
		{
			last_tick = current_time;
			
			data_t *obj;
			for(uint8_t i = 0; i < 2; ++i)
			{
				obj = &data[i];
				
				if(obj->ready == true)
				{
					obj->ready = false;
					
					CheckCRC((bms_num_t)i);
					DEBUG_LOG_ARRAY_HEX("BMS", obj->cold, sizeof(obj->cold));
					CANLib::UpdateCANObjects_BMS(obj->cold);

					obj->busy = false;
				}
			}
		}
		
		static uint32_t iter = 0;
		if(current_time - iter > CFG_BMSRequestPeriod)
		{
			iter = current_time;
			
			HAL_UART_Transmit(data[BMS_1].hal, (uint8_t *)PACKET_TX_REQUEST, sizeof(PACKET_TX_REQUEST), 64U);
			HAL_UART_Transmit(data[BMS_2].hal, (uint8_t *)PACKET_TX_REQUEST, sizeof(PACKET_TX_REQUEST), 64U);
		}
		
		current_time = HAL_GetTick();
		
		return;
	}
}

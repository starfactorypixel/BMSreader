#pragma once

#include "BMS_low_level_abstraction.h"

#include <bms_ant_data.h>



// -----------------------------------------------------------------------------------------------------------------
#include <BMSManager.h>
#include <drivers/BMSAnt.h>
#include <drivers/BMSOther.h>






// -----------------------------------------------------------------------------------------------------------------

extern UART_HandleTypeDef hBms1Uart;
extern UART_HandleTypeDef hBms2Uart;





namespace BMSLogic
{

	void UART_TX(uint8_t idx, const uint8_t *data, const uint16_t length);


	BMSAnt ant1;
	BMSAnt ant2;
	BMSOther other;
	BMSManager bms(UART_TX, [](uint8_t idx, int8_t code)
	{
		DEBUG_LOG_TOPIC("BMS-ERR", "idx: %d, code: %d\n", idx, code);
	});







	struct data_t
	{
		UART_HandleTypeDef *hal;		// Указатель на объект HAL USART
		uint8_t hot[200];				// Горячий массив данных (Работа в прерывании)
	} uart_data[2];
	
	enum bms_num_t : uint8_t { BMS_1 = 0, BMS_2 = 1 };
	


int8_t error = 0;


	inline void UART_RX(uint8_t idx, const uint16_t length)
	{
		bms.DataRx(idx, uart_data[idx].hot, length);
		
		return;
	}
	
	
	void UART_TX(uint8_t idx, const uint8_t *data, const uint16_t length)
	{
		HAL_UART_Transmit(uart_data[idx].hal, (uint8_t *)data, length, 64U);
		
		return;
	}


	
	
	inline void Setup()
	{
		memset(uart_data, 0x00, sizeof(uart_data));

		uart_data[BMS_1].hal = &hBms1Uart;
		uart_data[BMS_2].hal = &hBms2Uart;
		
		
		
		bms.SetModel(BMS_1, ant1);
		bms.SetModel(BMS_2, ant2);
		
		
		HAL_UARTEx_ReceiveToIdle_IT(uart_data[BMS_1].hal, uart_data[BMS_1].hot, sizeof(uart_data[BMS_1].hot));
		HAL_UARTEx_ReceiveToIdle_IT(uart_data[BMS_2].hal, uart_data[BMS_2].hot, sizeof(uart_data[BMS_2].hot));

		return;
	}
	
	inline void Loop(uint32_t &current_time)
	{
		bms.Tick(current_time);


		static uint32_t tick500 = 0;
		if(current_time - tick500 > 500)
		{
			tick500 = current_time;

			DEBUG_LOG_TOPIC("BMS1", "vol: %d, cur: %d, pow: %d, err: %d\n", bms.data[0].voltage, bms.data[0].current, bms.data[0].power, error);
			DEBUG_LOG_TOPIC("BMS2", "vol: %d, cur: %d, pow: %d, err: %d\n", bms.data[1].voltage, bms.data[1].current, bms.data[1].power, error);
			DEBUG_LOG_ARRAY_HEX("BMS1-hex", (uint8_t *)ant1.data, 140);
			DEBUG_LOG_NEW_LINE();
		}
					//CANLib::UpdateCANObjects_BMS(obj->cold);


			//HAL_UART_Transmit(data[BMS_1].hal, (uint8_t *)PACKET_TX_REQUEST, sizeof(PACKET_TX_REQUEST), 64U);
			//HAL_UART_Transmit(data[BMS_2].hal, (uint8_t *)PACKET_TX_REQUEST, sizeof(PACKET_TX_REQUEST), 64U);

		current_time = HAL_GetTick();
		
		return;
	}
}

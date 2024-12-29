#pragma once
#include <inttypes.h>
#include <CUtils.h>
#include <BMSDeviceInterface.h>
#include <drivers/BMSAnt_Data.h>

class BMSAnt : public BMSDeviceInterface
{
	using callback_ready_t = void (*)(const BMSANT::packet_raw_reverse_t *data);
	
	public:
		
		BMSAnt() : BMSDeviceInterface(), _callback_ready(nullptr), _busy(false), _ready(false), _last_request_time(0)
		{
			memset(_data, 0x00, sizeof(_data));
			
			return;
		}
		
		void SetReadyCallback(callback_ready_t ready)
		{
			_callback_ready = ready;
			
			return;
		}
		
		virtual void Init() override
		{

		}
		
		virtual void Tick(uint32_t &time) override
		{
			if(_ready == true)
			{
				_ready = false;
			
				// Т.к. данные представленны в формате big-endian, обращаем массив для просторы работы
				array_reverse(_data, sizeof(_data));

				_PostProcessing();

				// Напихиваем полезные данные в общий объект
				_manager->data[_idx].voltage = data->total_voltage;
				_manager->data[_idx].current = data->total_current;
				_manager->data[_idx].power = data->total_power;
				
				if(_callback_ready != nullptr)
				{
					_callback_ready(data);
				}
				
				_busy = false;
			}
			
			if(time - _last_request_time > BMSANT::PacketRequestInerval)
			{
				_last_request_time = time;

				_manager->DataTx(_idx, BMSANT::PacketRequest, sizeof(BMSANT::PacketRequest));
			}
			
			return;
		}
		
		// Приём пакета, в прерывании. Минимум самый важный действий.
		virtual int8_t DataRx(const uint8_t *raw, const uint8_t length) override
		{
			if(_busy == true) return -1;
			if(length != BMSANT::PacketSize) return -2;
			if(memcmp(BMSANT::PacketHeader, raw, sizeof(BMSANT::PacketHeader)) != 0) return -3;
			if(_CheckCRCSum(raw) == false) return -4;
			
			memcpy(_data, raw, sizeof(_data));
			
			_ready = true;
			_busy = true;
			
			return 0;
		}
		
		// Объект готовых данных.
		const BMSANT::packet_raw_reverse_t *data = (BMSANT::packet_raw_reverse_t *)_data;
		
	private:
		
		bool _CheckCRCSum(const uint8_t *array)
		{
			uint16_t crc = 0x0000;
			
			for(uint8_t i = 4; i < 138; ++i) { crc += array[i]; }
			
			return ( ((crc >> 8) & 0xFF) == array[138] && (crc & 0xFF) == array[139] );
		}
		
		void _PostProcessing()
		{
			BMSANT::packet_raw_reverse_t *data = (BMSANT::packet_raw_reverse_t *) _data;
			
			array_reverse(data->cell_voltage, BMSANT::CellsNumber);
			array_reverse(data->temperature, BMSANT::TempsNumber);
			
			return;
		}
		
		callback_ready_t _callback_ready;
		
		bool _busy;								// Флаг того, что разбор данных не окончен и новые копировать нельзя
		bool _ready;							// Флаг того, что массив данные приняты и готовы к анализу
		uint8_t _data[BMSANT::PacketSize];		// Холодный массив данных (Работа в программе)

		uint32_t _last_request_time;
		
};

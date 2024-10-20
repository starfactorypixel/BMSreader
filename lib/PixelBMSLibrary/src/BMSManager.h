#pragma once
#include <inttypes.h>
#include <BMSDeviceInterface.h>
#include <BMSData.h>



class BMSManager
{
	static constexpr uint8_t _max_dev = 2;
	
	using callback_tx_t = void (*)(uint8_t idx, const uint8_t *data, const uint16_t length);
	using callback_error_t = void (*)(uint8_t idx, int8_t code);
	
	public:
		BMSManager(callback_tx_t tx, callback_error_t error) : _callback_tx(tx), _callback_error(error), _last_tick(0)
		{
			for(uint8_t i = 0; i < _max_dev; ++i)
			{
				_device[i] = nullptr;
			}
			
			return;
		}
		
		void SetModel(uint8_t idx, BMSDeviceInterface &device);
		void Tick(uint32_t &time);
		void DataRx(uint8_t idx, const uint8_t *data, const uint8_t length);
		void DataTx(uint8_t idx, const uint8_t *data, const uint8_t length);

		bms_common_data_t data[_max_dev] = {};
		
	private:

		callback_tx_t _callback_tx;
		callback_error_t _callback_error;
		
		BMSDeviceInterface *_device[_max_dev];

		struct error_t
		{
			int8_t code;
			bool is_error;
		} _error[_max_dev];

		uint32_t _last_tick;
		
};

#pragma once
#include <inttypes.h>
#include <string.h>
#include <BMSDeviceInterface.h>
#include <BMSData.h>

class BMSManager
{
	static constexpr uint8_t _max_dev = 2;
	
	using callback_tx_t = void (*)(uint8_t idx, const uint8_t *raw, const uint16_t length);
	using callback_error_t = void (*)(uint8_t idx, int8_t code);

	public:
		
		BMSManager(callback_tx_t tx, callback_error_t error) : _callback_tx(tx), _callback_error(error)
		{
			memset(_device, 0x00, sizeof(_device));
			memset(common_obj, 0x00, sizeof(common_obj));
			
			return;
		}
		
		void SetModel(uint8_t idx, BMSDeviceInterface &device);
		void Tick(uint32_t &time);
		void DataRx(uint8_t idx, const uint8_t *raw, const uint8_t length);
		void DataTx(uint8_t idx, const uint8_t *raw, const uint8_t length);
		void ResetCommonData(uint8_t idx);

		struct common_obj_t
		{
			BMSManagerData::common_data_t data;
			bool ready;
		} common_obj[_max_dev];
		
	private:
		
		callback_tx_t _callback_tx;
		callback_error_t _callback_error;
		
		BMSDeviceInterface *_device[_max_dev];
		
};

#pragma once
#include <inttypes.h>

class BMSManager;

class BMSDeviceInterface
{
	public:
		
		BMSDeviceInterface() : _initiated(false), _manager(nullptr), _idx(0) {}
		
		virtual void Init() = 0;
		virtual void Tick(uint32_t &time) = 0;
		virtual int8_t DataRx(const uint8_t *raw, const uint8_t length) = 0;
		
		bool IsInitiated()
		{
			return _initiated;
		}
		
		void PrepareInit(BMSManager *obj, uint8_t idx)
		{
			_initiated = true;
			_manager = obj;
			_idx = idx;
		}
	
	protected:
		
		bool _initiated;
		BMSManager *_manager;
		uint8_t _idx;
		
	private:
		
};

#pragma once
#include <BMSDeviceInterface.h>

class BMSOther : public BMSDeviceInterface
{
	public:
		
		BMSOther() : BMSDeviceInterface() {}
		
		virtual void Init() override
		{
			
		}
		
		virtual void Tick(uint32_t &time) override
		{

		}
		
		virtual void DataRx(const uint8_t *raw, const uint8_t length) override
		{
			return;
		}

	private:
		
};

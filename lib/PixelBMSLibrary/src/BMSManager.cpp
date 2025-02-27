#include <BMSManager.h>

void BMSManager::SetModel(uint8_t idx, BMSDeviceInterface &device)
{
	if(idx >= _max_dev) return;
	if(device.IsInitiated() == true) return;
	
	device.PrepareInit(this, idx);
	device.Init();
	_device[idx] = &device;
	
	return;
}

void BMSManager::Tick(uint32_t &time)
{
	BMSDeviceInterface::error_code_t error_code;
	BMSDeviceInterface *device = nullptr;
	for(uint8_t idx = 0; idx < _max_dev; ++idx)
	{
		device = _device[idx];
		if(device == nullptr) continue;
		
		device->Tick(time);
		if(device->GetNewError(error_code) == true)
		{
			_callback_error(idx, error_code);
		}
	}
	
	return;
}

void BMSManager::DataRx(uint8_t idx, const uint8_t *raw, const uint8_t length)
{
	if(idx >= _max_dev) return;
	if(_device[idx] == nullptr) return;
	
	_device[idx]->DataRx(raw, length);
	
	return;
}

void BMSManager::DataTx(uint8_t idx, const uint8_t *raw, const uint8_t length)
{
	_callback_tx(idx, raw, length);
	
	return;
}

void BMSManager::ResetCommonData(uint8_t idx)
{
	if(idx >= _max_dev) return;
	
	memset(&common_obj[idx], 0x00, sizeof(common_obj[idx]));
	
	return;
}

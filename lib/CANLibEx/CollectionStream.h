#pragma once
#include <inttypes.h>

template <typename T> 
class CollectionStream
{
	using callback_ready_t = void (*)(T coll_el, uint8_t idx);
	
	public:

		CollectionStream(const T *collection, uint8_t count, callback_ready_t callback) : _collection(collection), _count(count), _callback_ready(callback)
		{}
		
		void Start(uint16_t timer)
		{
			_timer = timer;
			_last_tick = 0;
			_send_idx = 0;
			_is_run = true;
			
			return;
		}

		void Stop()
		{
			_is_run = false;

			return;
		}
		
		void Processing(uint32_t &time)
		{
			if(_is_run == false) return;
			
			if(time - _last_tick < _timer) return;
			_last_tick = time;
			
			_callback_ready( _collection[_send_idx], _send_idx );

			if(++_send_idx == _count)
			{
				Stop();
			}
			
			return;
		}

	private:
		
		const T *_collection;
		uint8_t _count;	
		callback_ready_t _callback_ready;
		
		uint16_t _timer;
		bool _is_run;
		uint8_t _send_idx;
		
		uint32_t _last_tick;
};

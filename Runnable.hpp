#include "main.h"

#include <vector>

namespace STM32T
{
class Runnable
{
	void (*p_callable)();
	uint32_t c_interval;
	uint32_t m_lastTime;
	bool c_repeat;
	
	Runnable(void (*callable)(), uint32_t interval, uint32_t last_time, bool repeat)
		: p_callable(callable), c_interval(interval), m_lastTime(last_time), c_repeat(repeat) {}
	
	static inline std::vector<Runnable> s_list;
	
public:
	static void Init()
	{
		s_list.reserve(8);
	}
	
	static void Repeat(void (*callable)(), uint32_t interval)
	{
		s_list.push_back({callable, interval, 0, true});
	}
	
	static void DoAndRepeat(void (*callable)(), uint32_t interval, uint32_t delay = 0)
	{
		s_list.push_back({callable, interval, HAL_GetTick() - interval + delay, true});
	}
	
	static void Process()
	{
		for (auto it = s_list.begin(); it != s_list.end(); ++it)
		{
			if (const uint32_t now  = HAL_GetTick(); now - it->m_lastTime >= it->c_interval)
			{
				it->p_callable();
				
				if (!it->c_repeat)
					it = s_list.erase(it) - 1;
				else
					it->m_lastTime = now;
			}
		}
	}
};
}

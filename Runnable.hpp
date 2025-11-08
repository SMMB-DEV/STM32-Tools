#pragma once

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
	using callable_t = void (*)();
	
	static void Init()
	{
		s_list.reserve(8);
	}
	
	static void Do(callable_t callable)
	{
		s_list.push_back({callable, 0, 0, false});
	}
	
	static void DoAfter(callable_t callable, uint32_t interval)
	{
		s_list.push_back({callable, interval, HAL_GetTick(), false});
	}
	
	static void Repeat(callable_t callable, uint32_t interval)
	{
		s_list.push_back({callable, interval, 0, true});
	}
	
	static void DoAndRepeat(callable_t callable, uint32_t interval, uint32_t delay = 0)
	{
		s_list.push_back({callable, interval, HAL_GetTick() - interval + delay, true});
	}
	
	static void Remove(callable_t callable)
	{
		for (auto it = s_list.begin(); it != s_list.end(); ++it)
		{
			if (it->p_callable == callable)
			{
				s_list.erase(it);
				return;
			}
		}
	}
	
	static void Refresh(callable_t callable)
	{
		for (auto& r : s_list)
		{
			if (r.p_callable == callable)
			{
				r.m_lastTime = HAL_GetTick();
				return;
			}
		}
	}
	
	static void Process()
	{
		// Iterators are not used since the callables can modify the list.
		for (size_t i = 0; i < s_list.size(); i++)
		{
			if (const uint32_t now  = HAL_GetTick(); now - s_list[i].m_lastTime >= s_list[i].c_interval)
			{
				s_list[i].p_callable();
				
				if (!s_list[i].c_repeat)
					s_list.erase(s_list.begin() + i--);
				else
					s_list[i].m_lastTime = now;
			}
		}
	}
};
}

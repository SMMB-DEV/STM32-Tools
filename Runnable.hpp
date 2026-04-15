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
	static inline size_t s_lastIndex = 0;
	
public:
	using callable_t = void (*)();
	
	static void Init()
	{
		s_list.reserve(8);
	}
	
	static void Do(callable_t callable, uint32_t delay = 0)
	{
		s_list.push_back({callable, delay, HAL_GetTick(), false});
	}
	
	[[deprecated("Use Do().")]]
	static void DoAfter(callable_t callable, uint32_t interval)
	{
		s_list.push_back({callable, interval, HAL_GetTick(), false});
	}
	
	static void Repeat(callable_t callable, uint32_t interval)
	{
		s_list.push_back({callable, interval, HAL_GetTick(), true});
	}
	
	static void DoAndRepeat(callable_t callable, uint32_t interval, uint32_t delay = 0)
	{
		s_list.push_back({callable, interval, HAL_GetTick() - interval + delay, true});
	}
	
	static void Remove(callable_t callable, const bool all = false)
	{
		for (auto it = s_list.cbegin(); it != s_list.cend();)
		{
			if (it->p_callable == callable)
			{
				it = s_list.erase(it);
				if (!all)
					return;
			}
			else
				 ++it;
		}
	}
	
	/**
	* @brief As if the last execution of the callable was now. Refreshes the delay.
	*/
	static void Refresh(callable_t callable, const bool all = false)
	{
		for (auto& r : s_list)
		{
			if (r.p_callable == callable)
			{
				r.m_lastTime = HAL_GetTick();
				if (!all)
					return;
			}
		}
	}
	
	/**
	* @brief As if the callable needs to be executed now.
	*/
	static void Advance(callable_t callable, const bool all = false)
	{
		for (auto& r : s_list)
		{
			if (r.p_callable == callable)
			{
				r.m_lastTime = HAL_GetTick() - r.c_interval;
				if (!all)
					return;
			}
		}
	}
	
	static void ProcessSingle()
	{
		if (s_lastIndex >= s_list.size())
			s_lastIndex = 0;
		
		for (; s_lastIndex < s_list.size(); ++s_lastIndex)
		{
			if (const uint32_t now  = HAL_GetTick(); now - s_list[s_lastIndex].m_lastTime >= s_list[s_lastIndex].c_interval)
			{
				s_list[s_lastIndex].p_callable();
				
				if (!s_list[s_lastIndex].c_repeat)
					s_list.erase(s_list.begin() + s_lastIndex);
				else
					s_list[s_lastIndex++].m_lastTime = now;
				
				return;
			}
		}
	}
	
	static void Process()
	{
		do
			ProcessSingle();
		while (s_lastIndex < s_list.size());
	}
};
}

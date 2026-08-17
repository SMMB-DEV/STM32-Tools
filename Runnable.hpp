#pragma once

#include "./Log.hpp"

#include <vector>

namespace STM32T
{
	class Runnable
	{
	public:
		using callable_t = void (*)();
		
	private:
		callable_t p_callable;
		uint32_t c_interval;
		uint32_t m_lastTime;
		bool c_repeat;
		const char *c_name;
		
		Runnable(callable_t callable, uint32_t interval, uint32_t last_time, bool repeat, const char *name)
			: p_callable(callable), c_interval(interval), m_lastTime(last_time), c_repeat(repeat), c_name(name) {}
		
		#ifdef STM32T_LOG_RUNNABLE
		static constexpr Log::Logger LG = Log::g_defaultLogger.Clone(std::min(Log::Level::Debug, Log::g_defaultLogger.level), "Runnable"sv);
		#else
		static constexpr Log::Logger LG = Log::g_defaultLogger.Clone(Log::Level::None, "Runnable"sv);
		#endif
		
		static inline std::vector<Runnable> s_list;
		static inline size_t s_lastIndex = 0;
		
	public:
		static void Init()
		{
			s_list.reserve(8);
		}
		
		static void Do(callable_t callable, const char *name, uint32_t delay = 0)
		{
			s_list.push_back({callable, delay, HAL_GetTick(), false, name});
		}
		
		static void Do(callable_t callable, uint32_t delay = 0)
		{
			Do(callable, nullptr, delay);
		}
		
		[[deprecated("Use Do().")]]
		static void DoAfter(callable_t callable, uint32_t interval)
		{
			s_list.push_back({callable, interval, HAL_GetTick(), false, nullptr});
		}
		
		static void Repeat(callable_t callable, uint32_t interval, const char *name = nullptr)
		{
			s_list.push_back({callable, interval, HAL_GetTick(), true, name});
		}
		
		static void DoAndRepeat(callable_t callable, uint32_t interval, const char *name, uint32_t delay = 0)
		{
			s_list.push_back({callable, interval, HAL_GetTick() - interval + delay, true, name});
		}
		
		static void DoAndRepeat(callable_t callable, uint32_t interval, uint32_t delay = 0)
		{
			DoAndRepeat(callable, interval, nullptr, delay);
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
				const auto &item = s_list[s_lastIndex];
				if (const uint32_t now  = HAL_GetTick(); now - item.m_lastTime >= item.c_interval)
				{
					Log::LOG_D<LG>("now: %10u, last: %10u, interval: %8u, name: %s", now, item.m_lastTime, item.c_interval, item.c_name ? item.c_name : "");
					
					item.p_callable();
					
					if (!item.c_repeat)
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

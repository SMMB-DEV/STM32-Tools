#pragma once

#include "main.h"
#include <cstdio>
#include <string_view>
#include <vector>
#include <functional>
#include <type_traits>



#ifndef COM_LOG
constexpr bool _LOG = false;
#else
constexpr bool _LOG = true;
#endif




inline void LOG(void (*f)())
{
	if constexpr (_LOG)
		f();
}

template <class... Args>
inline void LOGF(const char *fmt, Args... args)
{
	if constexpr (_LOG)
		printf(fmt, args...);
}

template <class... Args>
inline void LOGFI(const size_t indent, const char *fmt, Args... args)
{
	if constexpr (_LOG)
	{
		printf("%*s", indent * 4, "");
		printf(fmt, args...);
	}
}

template <class... Args>
inline bool LOGFC(const bool c, const char *fmt, Args... args)
{
	if (c)
		LOGF(fmt, args...);
	
	return c;
}

inline void LOGSEP()
{
	if constexpr (_LOG)
		printf("--------------------------------------------------------------------------------\n");
}


//#define LOGFI(indent, fmt, ...)		{ if constexpr (_LOG) { printf("%*s" fmt, indent * 4, "", ##__VA_ARGS__); } }
#define LOGT(__msg__, __min_time__, ...) \
{ \
	if constexpr (_LOG) \
	{ \
		volatile uint32_t __start__ = HAL_GetTick(); \
		__VA_ARGS__ \
		uint32_t __elapsed__ = HAL_GetTick() - __start__; \
		if (__elapsed__ >= __min_time__) \
			printf(__msg__ ": %ums\n", __elapsed__); \
	} \
	else \
	{ \
		__VA_ARGS__ \
	} \
}

#define _countof(arr)				(sizeof(arr) / sizeof(arr[0]))



using std::operator"" sv;

namespace STM32T
{
	using strv = std::string_view;

	template <class T>
	using vec = std::vector<T>;

	constexpr size_t operator"" _Ki(unsigned long long x) noexcept
	{
		return x * 1024;
	}
	
	constexpr size_t operator"" _Ki(long double x) noexcept
	{
		return x * 1024;
	}
	
	inline void Hang(void (* const action) ())
	{
		while (true)
		{
			action();
		}
	}
	
	inline void Startup()
	{
		LOG([]()
		{
			printf("\n\n\n--------------------------------------------------------------------------------\nStart!\n");
		
			const uint32_t version = HAL_GetHalVersion();
			
			// major.minor.patch-rc
			printf("\nHAL v%hhu.%hhu.%hhu-rc%hhu\n", version >> 24, (version >> 16) & 0xFF, (version >> 8) & 0xFF, version & 0xFF);
			printf("RevID: 0x%X, DevID: 0x%X, UID: 0x%08X%08X%08X\n", HAL_GetREVID(), HAL_GetDEVID(), HAL_GetUIDw0(), HAL_GetUIDw1(), HAL_GetUIDw2());
			printf("HCLK: %.1f MHz\n", HAL_RCC_GetHCLKFreq() / 1'000'000.0f);
			printf("\n");
		});
	}
	
	template <typename T>
	inline bool WaitOnPin(GPIO_TypeDef* const port, uint16_t pin, const bool desired_state, const T timeout, T(* const get_tick)() = HAL_GetTick)
	{
		const T startTime = get_tick();
		while (HAL_GPIO_ReadPin(port, pin) != desired_state)
		{
			if (get_tick() - startTime > timeout)
				return false;
		}
		
		return true;
	}
	
	template <typename T>
	inline void WaitAfter(const T start, const T wait, T(* const get_tick)() = HAL_GetTick)
	{
		static_assert(!std::is_same_v<T, bool> && std::is_integral_v<T>, "");
		
		while (get_tick() - start < wait);
	}
	
	inline void __attribute__((deprecated)) Tokenize(vec<strv>& tokens, strv view, const strv& sep = " "sv, const bool ignoreSingleEnded = false)
	{
		//assuming view is null-terminated.
		
		const char * const start = view.data();
		
		while (view.size() > 0)
		{
			strv::size_type end = view.find(sep);
			if (end == strv::npos)
			{
				if (!ignoreSingleEnded)
					tokens.push_back(view);
			
				return;
			}
			
			if ((view.data() != start || !ignoreSingleEnded) && end > 0)	//ensures no empty tokens
			{
				((char*)view.data())[end] = '\0';	//no problems with C string functions
				tokens.push_back(view.substr(0, end));
			}
			
			view.remove_prefix(end + sep.size());
		}
	}
	
	inline void Tokenize(strv view, const strv& sep, vec<strv>& tokens, const bool ignoreSingleEnded)
	{
		//assuming view is null-terminated.
		
		const char * const start = view.data();
		
		while (view.size() > 0)
		{
			strv::size_type end = view.find(sep);
			if (end == strv::npos)
			{
				if (!ignoreSingleEnded)
					tokens.push_back(view);
			
				return;
			}
			
			if ((view.data() != start || !ignoreSingleEnded) && end > 0)	//ensures no empty tokens
			{
				((char*)view.data())[end] = '\0';	//no problems with C string functions
				tokens.push_back(view.substr(0, end));
			}
			
			view.remove_prefix(end + sep.size());
		}
	}
	
	inline void Tokenize(strv view, const strv& sep, const std::function<void (strv)>& op, const bool ignoreSingleEnded)
	{
		//note: assuming view is null-terminated.
		//todo: handle {sep} inside string literals
		
		const char * const start = view.data();
		
		while (view.size() > 0)
		{
			strv::size_type end = view.find(sep);
			if (end == strv::npos)
			{
				if (!ignoreSingleEnded)
					op(view);
			
				return;
			}
			
			if ((view.data() != start || !ignoreSingleEnded) && end > 0)	//ensures no empty tokens
			{
				((char*)view.data())[end] = '\0';	//no problems with C string functions
				op(view.substr(0, end));
			}
			
			view.remove_prefix(end + sep.size());
		}
	}
	
#ifdef HAL_RTC_MODULE_ENABLED
	inline void AdjustDateAndTime(RTC_DateTypeDef& date, RTC_TimeTypeDef& time, int32_t sec)
	{
		// 24-Hour/Binary Format
		
		if (sec == 0 || sec > 2'419'200 || sec < -2'419'200)	// max 28 days; don't want to deal with calculating number of months more than 1
			return;
		
		static constexpr uint8_t MONTH_DAYS[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };	//February can be 29
		
		//These extra days occur in each year that is an integer multiple of 4 (except for years evenly divisible by 100, but not by 400) [https://en.wikipedia.org/wiki/Leap_year]
		const bool isLeapYear = date.Year % 4 == 0 && date.Year != 0;
		const uint8_t monthDays = MONTH_DAYS[date.Month - 1] + (date.Month == 2 && isLeapYear);
		
		int16_t mins = (sec / 60) % 60;
		int16_t hours = (sec / (60 * 60)) % 24;
		int8_t days = sec / (24 * 60 * 60);
		sec %= 60;
		
		if (sec > 0)
		{
			time.Seconds += sec;
			if (time.Seconds >= 60)
			{
				time.Seconds -= 60;
				time.Minutes++;
			}
			
			time.Minutes += mins;
			if (time.Minutes >= 60)
			{
				time.Minutes -= 60;
				time.Hours++;
			}
			
			time.Hours += hours;
			if (time.Hours >= 24)
			{
				time.Hours -= 24;
				days++;
			}
			
			date.Date += days;
			if (date.Date > monthDays)
			{
				date.Date -= monthDays;
				date.Month++;
				
				if (date.Month > 12)
				{
					date.Month -= 12;
					date.Year++;
					date.Year %= 100;
				}
			}
		}
		else
		{
			time.Seconds += sec;
			if (time.Seconds > 60)
			{
				time.Seconds += 60;
				time.Minutes--;
			}
			
			time.Minutes += mins;
			if (time.Minutes > 60)	//underflow
			{
				time.Minutes += 60;
				time.Hours--;
			}
			
			time.Hours += hours;
			if (time.Hours > 24)	//underflow
			{
				time.Hours += 24;
				days--;
			}
			
			const uint8_t lastMonth = date.Month == 1 ? 12 : date.Month - 1;
			date.Date += days;
			if (date.Date == 0 || date.Date > monthDays)	//underflow
			{
				date.Date = MONTH_DAYS[lastMonth - 1] + (lastMonth == 2 && isLeapYear) - (UINT8_MAX - date.Date + 1);
				date.Month = lastMonth;
				
				if (lastMonth == 12)
				{
					date.Year--;
					if (date.Year > 100)	//underflow
						date.Year += 100;
				}
			}
		}
		
		date.WeekDay = ((date.WeekDay - 1) + 7 + days % 7) % 7 + 1;
	}
	
	inline void AdjustDateAndTime_ms(RTC_DateTypeDef& date, RTC_TimeTypeDef& time, int32_t msec)
	{
		// 24-Hour/Binary Format
		
		if (msec == 0)
			return;
		
		static constexpr uint8_t MONTH_DAYS[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };	//February can be 29
		
		//These extra days occur in each year that is an integer multiple of 4 (except for years evenly divisible by 100, but not by 400) [https://en.wikipedia.org/wiki/Leap_year]
		const bool isLeapYear = date.Year % 4 == 0 && date.Year != 0;
		const uint8_t monthDays = MONTH_DAYS[date.Month - 1] + (date.Month == 2 && isLeapYear);
		
		int8_t sec = (msec / 1000) % 60;
		int16_t mins = (msec / (60 * 1000)) % 60;
		int16_t hours = (msec / (60 * 60 * 1000)) % 24;
		int8_t days = msec / (24 * 60 * 60 * 1000);
		//msec %= 1000;
		
		if (msec > 0)
		{
			time.Seconds += sec;
			if (time.Seconds >= 60)
			{
				time.Seconds -= 60;
				time.Minutes++;
			}
			
			time.Minutes += mins;
			if (time.Minutes >= 60)
			{
				time.Minutes -= 60;
				time.Hours++;
			}
			
			time.Hours += hours;
			if (time.Hours >= 24)
			{
				time.Hours -= 24;
				days++;
			}
			
			date.Date += days;
			if (date.Date > monthDays)
			{
				date.Date -= monthDays;
				date.Month++;
				
				if (date.Month > 12)
				{
					date.Month -= 12;
					date.Year++;
					date.Year %= 100;
				}
			}
		}
		else
		{
			time.Seconds += sec;
			if (time.Seconds > 60)
			{
				time.Seconds += 60;
				time.Minutes--;
			}
			
			time.Minutes += mins;
			if (time.Minutes > 60)	//underflow
			{
				time.Minutes += 60;
				time.Hours--;
			}
			
			time.Hours += hours;
			if (time.Hours > 24)	//underflow
			{
				time.Hours += 24;
				days--;
			}
			
			const uint8_t lastMonth = date.Month == 1 ? 12 : date.Month - 1;
			date.Date += days;
			if (date.Date == 0 || date.Date > monthDays)	//underflow
			{
				date.Date = MONTH_DAYS[lastMonth - 1] + (lastMonth == 2 && isLeapYear) - (UINT8_MAX - date.Date + 1);
				date.Month = lastMonth;
				
				if (lastMonth == 12)
				{
					date.Year--;
					if (date.Year > 100)	//underflow
						date.Year += 100;
				}
			}
		}
		
		date.WeekDay = ((date.WeekDay - 1) + 7 + days % 7) % 7 + 1;
	}
#endif	// HAL_RTC_MODULE_ENABLED
}

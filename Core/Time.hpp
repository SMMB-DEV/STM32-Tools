#pragma once

#include "main.h"

#include <type_traits>
#include <limits>
#include <optional>



namespace STM32T::Time
{
	template <typename T>
	using TickFuncPtr = T (*)();
	
	template <typename T>
	using DelayFuncPtr = void (*)(T);
	
	using cycle_t = uint32_t;
	
	using ms_time_t = uint8_t;
	using us_time_t = uint16_t;
	using ns_time_t = uint16_t;
	
	using ms_cycle_t = uint32_t;
	using us_cycle_t = uint32_t;
	using ns_cycle_t = uint64_t;
	
	#ifndef STM32T_TIME_CLK
	#error "STM32T_TIME_CLK not defined!"
	#endif
	
	static_assert(STM32T_TIME_CLK % 1'000'000 == 0, "STM32T_TIME_CLK must be divisible by 1,000,000!");
	
	#ifdef DWT
	// https://community.st.com/t5/stm32-mcus-embedded-software/dwt-and-microsecond-delay/m-p/632748/highlight/true#M44839
	inline void Init()
	{
		if (!READ_BIT(CoreDebug->DEMCR, CoreDebug_DEMCR_TRCENA_Msk))
		{
			// enable core debug timers
			SET_BIT(CoreDebug->DEMCR, CoreDebug_DEMCR_TRCENA_Msk);
			
			#ifdef STM32H7
			DWT->LAR = 0xC5ACCE55;
			#endif
			
			// enable the clock counter
			SET_BIT(DWT->CTRL, DWT_CTRL_CYCCNTENA_Msk);
		}
	}
	
	[[gnu::always_inline]] inline uint32_t GetCycle() { return DWT->CYCCNT; }
	#else
	inline void Init()
	{
		if (1000 * (SysTick->LOAD + 1) != STM32T_TIME_CLK)
			Error_Handler();
	}
	
	inline uint32_t GetCycle()
	{
		static constexpr uint32_t MS = STM32T_TIME_CLK / 1000;
		
		const uint32_t
			big = HAL_GetTick(), small = SysTick->VAL,
			big2 = HAL_GetTick(), small2 = SysTick->VAL;

		if (big == big2)
			return big * MS + (MS - small);
		else
			return big2 * MS + (MS - small2);
	}
	#endif	// DWT
	
	inline constexpr cycle_t msToCycles(const ms_time_t time_ms)
	{
		static_assert(std::numeric_limits<ms_time_t>::max() * uintmax_t(STM32T_TIME_CLK / 1'000) <= std::numeric_limits<cycle_t>::max());
		return time_ms * (STM32T_TIME_CLK / 1'000);
	}
	
	inline constexpr cycle_t usToCycles(const us_time_t time_us)
	{
		static_assert(std::numeric_limits<us_time_t>::max() * uintmax_t(STM32T_TIME_CLK / 1'000'000) <= std::numeric_limits<cycle_t>::max());
		return time_us * (STM32T_TIME_CLK / 1'000'000);
	}
	
	inline constexpr cycle_t nsToCycles(const ns_time_t time_ns)
	{
		static_assert(std::numeric_limits<ns_time_t>::max() * uintmax_t(STM32T_TIME_CLK / 1'000'000) / 1000 <= std::numeric_limits<cycle_t>::max());
		return time_ns * (STM32T_TIME_CLK / 1'000'000) / 1000;
	}
	
	inline constexpr ms_cycle_t CyclesTo_ms(cycle_t cyc)
	{
		static_assert(std::numeric_limits<cycle_t>::max() / (STM32T_TIME_CLK / 1'000) <= std::numeric_limits<ms_cycle_t>::max());
		return cyc / (STM32T_TIME_CLK / 1'000);
	}
	
	inline constexpr us_cycle_t CyclesTo_us(cycle_t cyc)
	{
		static_assert(std::numeric_limits<cycle_t>::max() / (STM32T_TIME_CLK / 1'000'000) <= std::numeric_limits<us_cycle_t>::max());
		return cyc / (STM32T_TIME_CLK / 1'000'000);
	}
	
	inline constexpr ns_cycle_t CyclesTo_ns(cycle_t cyc)
	{
		static_assert(std::numeric_limits<cycle_t>::max() * uintmax_t(1000) / (STM32T_TIME_CLK / 1'000'000) <= std::numeric_limits<ns_cycle_t>::max());
		return cyc * uint64_t(1000) / (STM32T_TIME_CLK / 1'000'000);
	}
	
	inline void Delay(cycle_t cyc)
	{
		const uint32_t start = GetCycle();
		while (GetCycle() - start < cyc);
	}
	
	inline void Delay_ms(ms_time_t ms)
	{
		const uint32_t start = GetCycle(), delay = msToCycles(ms);
		while (GetCycle() - start < delay);	// < because delay is usually accurate (when STM32T_TIME_CLK is divisible by 1'000'000).
	}
	
	inline void Delay_us(us_time_t us)
	{
		const uint32_t start = GetCycle(), delay = usToCycles(us);
		while (GetCycle() - start < delay);	// < because delay is usually accurate (when STM32T_TIME_CLK is divisible by 1'000'000).
	}
	
	inline void Delay_ns(ns_time_t ns)
	{
		const uint32_t start = GetCycle(), delay = nsToCycles(ns);
		while (GetCycle() - start <= delay);	// <= because delay isn't always accurate.
	}
	
	inline bool Elapsed_Tick(const uint32_t start, const uint32_t msec)
	{
		return HAL_GetTick() - start > msec;
	}
	
	inline bool Elapsed_Tick(const uint32_t start, const uint32_t now, const uint32_t msec)
	{
		return now - start > msec;
	}
	
	inline bool Elapsed(const cycle_t startCycle, const cycle_t time_cycles)
	{
		return GetCycle() - startCycle >= time_cycles;
	}
	
	inline bool Elapsed_ms(const cycle_t startCycle, const ms_time_t time_ms)
	{
		const uint32_t delay = msToCycles(time_ms);
		return GetCycle() - startCycle >= delay;
	}
	
	inline bool Elapsed_us(const cycle_t startCycle, const us_time_t time_us)
	{
		const uint32_t delay = usToCycles(time_us);
		return GetCycle() - startCycle >= delay;
	}
	
	inline bool Elapsed_ns(const cycle_t startCycle, const ns_time_t time_ns)
	{
		const uint32_t delay = nsToCycles(time_ns);
		return GetCycle() - startCycle > delay;
	}
	
	inline uint32_t Remaining_Tick(const uint32_t start, const uint32_t msec)
	{
		const uint32_t now = HAL_GetTick();
		return Elapsed_Tick(start, now, msec) ? 0 : (msec - (now - start));
	}
	
	template <typename T>
	[[deprecated]]
	inline void WaitAfter(const T start, const T wait, TickFuncPtr<T> get_tick = HAL_GetTick)
	{
		static_assert(!std::is_same_v<T, bool> && std::is_integral_v<T>);
		
		while (get_tick() - start < wait);
	}
	
	inline void WaitAfter_Tick(const uint32_t start, const uint32_t wait)
	{
		while (HAL_GetTick() - start <= wait);
	}
	
	inline void WaitAfter(const cycle_t startCycle, const cycle_t wait_cycles)
	{
		while (GetCycle() - startCycle < wait_cycles);
	}
	
	inline void WaitAfter_ms(const cycle_t startCycle, const ms_time_t wait_ms)
	{
		const uint32_t wait = msToCycles(wait_ms);
		while (GetCycle() - startCycle < wait);
	}
	
	inline void WaitAfter_us(const cycle_t startCycle, const us_time_t wait_us)
	{
		const uint32_t wait = usToCycles(wait_us);
		while (GetCycle() - startCycle < wait);
	}
	
	inline void WaitAfter_ns(const cycle_t startCycle, const ns_time_t wait_ns)
	{
		const uint32_t wait = nsToCycles(wait_ns);
		while (GetCycle() - startCycle <= wait);
	}
	
	/**
	* @param month - [0:11]
	*/
	inline constexpr uint8_t MonthDays(int month, int year = 2001)
	{
		constexpr uint8_t MONTH_DAYS[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };	// February can be 29
		
		// These extra days occur in each year that is an integer multiple of 4 (except for years evenly divisible by 100, but not by 400) [https://en.wikipedia.org/wiki/Leap_year]
		const bool isLeapYear = year % 400 == 0 || (year % 4 == 0 && year % 100 != 0);
		return MONTH_DAYS[month] + (month == 1 && isLeapYear);
	}
	
	#ifdef HAL_RTC_MODULE_ENABLED
	inline uint32_t GetSecondFraction(RTC_HandleTypeDef *hrtc)
	{
		return hrtc->Instance->PRER & RTC_PRER_PREDIV_S;
	}
	
	[[deprecated]]
	inline void AdjustDateAndTime(RTC_DateTypeDef& date, RTC_TimeTypeDef& time, int32_t sec)
	{
		// 24-Hour/Binary Format
		
		if (sec == 0 || sec > 28 * 24 * 3600 || sec < -28 * 24 * 3600)	// Max. 28 days; don't want to deal with calculating number of months more than 1.
			return;
		
		const uint8_t monthDays = MonthDays(date.Month - 1);
		
		int8_t mins = (sec / 60) % 60;
		int8_t hours = (sec / (60 * 60)) % 24;
		int8_t days = sec / (24 * 60 * 60);
		
		if (sec > 0)
		{
			sec %= 60;
			
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
			sec %= 60;
			
			time.Seconds += sec;
			if (time.Seconds > 60)	// underflow
			{
				time.Seconds += 60;
				time.Minutes--;
			}
			
			time.Minutes += mins;
			if (time.Minutes > 60)	// underflow
			{
				time.Minutes += 60;
				time.Hours--;
			}
			
			time.Hours += hours;
			if (time.Hours > 24)	// underflow
			{
				time.Hours += 24;
				days--;
			}
			
			const uint8_t prevMonth = date.Month == 1 ? 12 : date.Month - 1;
			date.Date += days;
			if (date.Date == 0 || date.Date > monthDays)	// underflow
			{
				date.Date =MonthDays(prevMonth - 1) - (UINT8_MAX - date.Date + 1);
				date.Month = prevMonth;
				
				if (prevMonth == 12)
				{
					date.Year--;
					if (date.Year > 100)	// underflow
						date.Year += 100;
				}
			}
		}
		
		date.WeekDay = ((date.WeekDay - 1) + 4 * 7 + days) % 7 + 1;	// 4 * 7: Doesn't change mod 7; just to ensure it's a positive number.
	}
	
	/**
	* @note Expects date and time in 24-Hour, binary format. SubSeconds and SecondFraction must be set.
	*/
	inline void AdjustDateAndTime(RTC_DateTypeDef& date, RTC_TimeTypeDef& time, int64_t ms, uint16_t year_start = 2000)
	{
		std::tm tm{.tm_sec = time.Seconds, .tm_min = time.Minutes, .tm_hour = time.Hours,
			.tm_mday = date.Date, .tm_mon = date.Month - 1, .tm_year = year_start + date.Year - 1900, .tm_isdst = 0};
		
		std::time_t ts = std::mktime(&tm) + (ms / 1000);
		ms %= 1000;
		
		uint16_t msec = 1000u * (time.SecondFraction - time.SubSeconds) / (time.SecondFraction + 1u);
		
		if (ms >= 0)
			msec += ms;
		else
		{
			msec += 1000 + ms;
			--ts;
		}
		
		if (msec >= 1000)
		{
			msec -= 1000;
			++ts;
		}
		
		std::tm *tm2 = std::localtime(&ts);
		
		date.Year = tm2->tm_year % 100;
		date.Month = tm2->tm_mon + 1;
		date.Date = tm2->tm_mday;
		date.WeekDay = (tm2->tm_wday + 6) % 7 + 1;
		
		time.Minutes = tm2->tm_min;
		time.Hours = tm2->tm_hour;
		time.Seconds = tm2->tm_sec;
		time.SubSeconds = time.SecondFraction - (time.SecondFraction + 1u) * msec / 1000u;
	}
	
	inline void SetWeekDay(RTC_DateTypeDef& date, uint16_t year_start = 2000)
	{
		std::tm tm{.tm_sec = 0, .tm_min = 0, .tm_hour = 0,
			.tm_mday = date.Date, .tm_mon = date.Month - 1, .tm_year = year_start + date.Year - 1900, .tm_isdst = 0};
		
		const std::time_t ts = std::mktime(&tm);
		std::tm *tm2 = std::localtime(&ts);
		
		date.WeekDay = (tm2->tm_wday + 6) % 7 + 1;
	}
	#endif	// HAL_RTC_MODULE_ENABLED
	
	template <typename TIME_T = uint32_t>
	class Filter
	{
		TIME_T (*const cf_cyc)();
		const TIME_T c_minCyc;
		
		TIME_T m_prevCyc = 0, m_prevCyc2 = 0;
		bool m_prevState = false, m_prevState2 = false;
		
	public:
		Filter(TIME_T min_time, TIME_T (* cyc)() = Time::GetCycle) : c_minCyc(min_time), cf_cyc(cyc) {}
		
		/**
		* @retval Duration of the pulse in microseconds or 0 in case of invalid pulse.
		*/
		std::optional<std::pair<TIME_T, bool>> validate(bool state)
		{
			const TIME_T now = cf_cyc();
			
			if (state == m_prevState)
				return std::nullopt;
			
			const TIME_T duration = now - m_prevCyc;
			
			if (duration < c_minCyc)
			{
				m_prevState = m_prevState2;
				m_prevCyc = m_prevCyc2;
				
				return std::pair{duration, false};
			}
			
			m_prevState2 = m_prevState;
			m_prevState = state;
			
			m_prevCyc2 = m_prevCyc;
			m_prevCyc = now;
			
			return std::pair{duration, true};
		}
		
		void reset()
		{
			m_prevCyc = m_prevCyc2 = cf_cyc();
			m_prevState = m_prevState2;
		}
	};
}

/**
* @file
* See STM32T::Time for more information.
*/

#pragma once

#include "main.h"

#include <type_traits>
#include <limits>
#include <optional>
#include <chrono>

//__asm(".global __use_no_semihosting");

#ifndef STM32T_TIME_CLK
#error "STM32T_TIME_CLK not defined! Define it to be equal to HCLK."
#endif	// STM32T_TIME_CLK

#ifdef _DOXYGEN_
/**
* Must be a `constexpr` value equal to `HCLK`.
* @note Must be defined by the user.
*/
#define STM32T_TIME_CLK

/// Can be defined (as a `uint32_t` value) and is used by some modules to refresh the `IWDG` during time consuming tasks.
#define STM32T_IWDG_TIMEOUT

/**
* Define to include the [`time`](https://cppreference.com/cpp/chrono/c/time) function.
* @note The function always returns -1 if `HAL_RTC_MODULE_ENABLED` is not defined.
* @attention The implementation calls `HAL_RTC_GetTime` and `HAL_RTC_GetDate`. This might interfere with other uses of the RTC (e.g. in an ISR).
* @note Years [70, 99] are considered [1970, 1999] and [0, 69] are considered [2000, 2069].
*/
#define STM32T_TIME_TIME
#endif	// _DOXYGEN_

#ifdef STM32T_IWDG_TIMEOUT
extern "C" IWDG_HandleTypeDef hiwdg;
#endif	// STM32T_IWDG_TIMEOUT

#ifdef STM32T_TIME_TIME
#ifdef HAL_RTC_MODULE_ENABLED
extern "C" RTC_HandleTypeDef *hrtc;

extern "C" std::time_t time(std::time_t* timer)
{
	RTC_TimeTypeDef time;
	RTC_DateTypeDef date;
	
	HAL_RTC_GetTime(hrtc, &time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(hrtc, &date, RTC_FORMAT_BIN);
	
	std::tm tm{.tm_sec = time.Seconds, .tm_min = time.Minutes, .tm_hour = time.Hours,
		.tm_mday = date.Date, .tm_mon = date.Month - 1, .tm_year = (date.Year>= 70 ? 1970 : 2000) + date.Year - 1900, .tm_isdst = 0};
	
	std::time_t result = std::mktime(&tm);
	
	if (timer != nullptr)
		*timer = result;
	
	return result;
}
#else
#warning "HAL_RTC_MODULE_ENABLED is not defined. `time` will always return -1."

extern "C" std::time_t time(std::time_t* timer)
{
	std::time_t result = std::time_t(-1);
	
	if (timer != nullptr)
		*timer = result;
	
	return result;
}
#endif	// HAL_RTC_MODULE_ENABLED
#endif	// STM32T_TIME_TIME



/**
* Contains timing uitilities.
* 
* The @ref STM32T_TIME_CLK macro must be defined before including Time.hpp. You can define it in STM32CubeMX or Keil Options dialog.
* The module also has functions for working with the conventional tick provided by `HAL_GetTick()`.
* @see Init
* 
* This namespace introduces two clocks that can be used with `std::chrono`.
* - @ref TickClock
* - @ref HighResClock
* 
* @remark Using these clocks is generally not recommended unless you really have to use `std::chrono`.
* The conversions between different duration types can potentially be done using 64-bit integers or floating point types
* and involves multiplication and division which is not ideal for precise and time sensitive applications on a 32-bit CPU.
* Also in some cases (very large delays), floating point to integer casting could limit the maximum delay duration.
*/
namespace STM32T::Time
{
	template <typename T>
	using TickFuncPtr = T (*)();
	
	template <typename T>
	using DelayFuncPtr = void (*)(T);
	
	using tick_t = decltype(HAL_GetTick());
	
	using cycle_t = uint32_t;
	
	using ms_time_t = uint8_t;
	using us_time_t = uint16_t;
	using ns_time_t = uint16_t;
	
	using ms_cycle_t = uint32_t;
	using us_cycle_t = uint32_t;
	using ns_cycle_t = uint64_t;
	
	static_assert(STM32T_TIME_CLK % 1'000'000 == 0, "STM32T_TIME_CLK must be divisible by 1,000,000!");
	
	#if defined(DWT) or defined(_DOXYGEN_)
	/**
	* Initializes the timing module and checks @ref STM32T_TIME_CLK validity.
	* If your microcontroller has a `DWT` unit, it will be enabled and used for timing. Otherwise, the `SysTick` timer is used.
	* @copyright https://community.st.com/t5/stm32-mcus-embedded-software/dwt-and-microsecond-delay/m-p/632748/highlight/true#M44839
	*/
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
	
	/// @return The CPU cycle count.
	[[gnu::always_inline]] inline cycle_t GetCycle() { return DWT->CYCCNT; }
	#else
	inline void Init()
	{
		if (1000 * (SysTick->LOAD + 1) != STM32T_TIME_CLK)
			Error_Handler();
	}
	
	inline cycle_t GetCycle()
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
	#endif	// defined(DWT)
	
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
	
	inline void Delay_Tick(tick_t ticks)
	{
		const tick_t start = HAL_GetTick();
		
		if (ticks < std::numeric_limits<tick_t>::max() - uwTickFreq)
			ticks += uwTickFreq;	// Add a freq to guarantee minimum wait
		
		while (HAL_GetTick() - start < ticks)
		{
			#ifdef STM32T_IWDG_TIMEOUT
			HAL_IWDG_Refresh(&hiwdg);
			#endif	// STM32T_IWDG_TIMEOUT
		}
	}
	
	inline void Delay(cycle_t cyc)
	{
		const cycle_t start = GetCycle();
		while (GetCycle() - start < cyc);
	}
	
	inline void Delay_ms(ms_time_t ms)
	{
		const cycle_t start = GetCycle(), delay = msToCycles(ms);
		while (GetCycle() - start < delay);	// < because delay is usually accurate (when STM32T_TIME_CLK is divisible by 1'000'000).
	}
	
	inline void Delay_us(us_time_t us)
	{
		const cycle_t start = GetCycle(), delay = usToCycles(us);
		while (GetCycle() - start < delay);	// < because delay is usually accurate (when STM32T_TIME_CLK is divisible by 1'000'000).
	}
	
	inline void Delay_ns(ns_time_t ns)
	{
		const cycle_t start = GetCycle(), delay = nsToCycles(ns);
		while (GetCycle() - start <= delay);	// <= because delay isn't always accurate.
	}
	
	inline bool Elapsed_Tick(const tick_t start, const tick_t ticks)
	{
		return HAL_GetTick() - start > ticks;
	}
	
	inline bool Elapsed_Tick(const tick_t start, const tick_t now, const tick_t ticks)
	{
		return now - start > ticks;
	}
	
	inline bool Elapsed(const cycle_t startCycle, const cycle_t time_cycles)
	{
		return GetCycle() - startCycle >= time_cycles;
	}
	
	inline bool Elapsed_ms(const cycle_t startCycle, const ms_time_t time_ms)
	{
		const cycle_t delay = msToCycles(time_ms);
		return GetCycle() - startCycle >= delay;
	}
	
	inline bool Elapsed_us(const cycle_t startCycle, const us_time_t time_us)
	{
		const cycle_t delay = usToCycles(time_us);
		return GetCycle() - startCycle >= delay;
	}
	
	inline bool Elapsed_ns(const cycle_t startCycle, const ns_time_t time_ns)
	{
		const cycle_t delay = nsToCycles(time_ns);
		return GetCycle() - startCycle > delay;
	}
	
	inline uint32_t Remaining_Tick(const tick_t start, const tick_t ticks)
	{
		const uint32_t now = HAL_GetTick();
		return Elapsed_Tick(start, now, ticks) ? 0 : (ticks - (now - start));
	}
	
	inline void WaitAfter_Tick(const tick_t start, const tick_t wait)
	{
		while (HAL_GetTick() - start <= wait)
		{
			#ifdef STM32T_IWDG_TIMEOUT
			HAL_IWDG_Refresh(&hiwdg);
			#endif	// STM32T_IWDG_TIMEOUT
		}
	}
	
	inline void WaitAfter(const cycle_t startCycle, const cycle_t wait_cycles)
	{
		while (GetCycle() - startCycle < wait_cycles);
	}
	
	inline void WaitAfter_ms(const cycle_t startCycle, const ms_time_t wait_ms)
	{
		const cycle_t wait = msToCycles(wait_ms);
		while (GetCycle() - startCycle < wait);
	}
	
	inline void WaitAfter_us(const cycle_t startCycle, const us_time_t wait_us)
	{
		const cycle_t wait = usToCycles(wait_us);
		while (GetCycle() - startCycle < wait);
	}
	
	inline void WaitAfter_ns(const cycle_t startCycle, const ns_time_t wait_ns)
	{
		const cycle_t wait = nsToCycles(wait_ns);
		while (GetCycle() - startCycle <= wait);
	}
	
	/**
	* @param month	Must be in the range [0:11].
	* @param year
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
	
	/**
	* Uses `HAL_GetTick` for its time base.
	* @note This clock is defined as "steady" even though its tick counter could be modified manually - though that would be highly unusual.
	*/
	class TickClock
	{
	public:
		using rep = tick_t;
		using period = std::milli;
		using duration = std::chrono::duration<rep, period>;
		using time_point = std::chrono::time_point<TickClock>;
		
		static constexpr bool is_steady = true;
		
		static time_point now() { return time_point(duration(HAL_GetTick())); }
		
		/**
		* @brief Similar to `std::this_thread::sleep_for`.
		*
		* Can create very long delays with relative accuracy.
		* @note Avoid for very small delays and precise timing.
		*/
		template <class Rep, class Period>
		static void sleep_for(const std::chrono::duration<Rep, Period> sleep_duration)
		{
			const auto start = now();
			
			if (sleep_duration <= std::chrono::duration<Rep, Period>::zero())
				return;
			
			static_assert(sizeof(rep) < sizeof(uintmax_t));
			
			using max_duration_t = std::chrono::duration<std::uintmax_t, period>;
			
			max_duration_t d = sleep_duration > max_duration_t::max() ? max_duration_t::max() : std::chrono::duration_cast<max_duration_t>(sleep_duration);
			
			if (d < sleep_duration && d != max_duration_t::max())
				++d;
			
			static constexpr auto _max = uintmax_t(std::numeric_limits<rep>::max()) + 1;
			
			const duration rem(d.count() % _max);
			
			for (auto rounds = d.count() / _max; rounds; --rounds)
			{
				static constexpr auto _mid = time_point::max().time_since_epoch() / 2;
				
				while (now().time_since_epoch() < _mid);
				while (now().time_since_epoch() >= _mid);
			}
			
			while (now() < start);
			while (now() - start <= rem);
		}
	};
	
	/**
	* Uses @ref GetCycle for its time base.
	* @note This clock is defined as "steady" even though its tick counter could be modified manually - though that would be highly unusual.
	*/
	class HighResClock
	{
	public:
		using rep = cycle_t;
		using period = std::ratio<1, STM32T_TIME_CLK>;
		using duration = std::chrono::duration<rep, period>;
		using time_point = std::chrono::time_point<HighResClock>;
		
		static constexpr bool is_steady = true;
		
		static time_point now() { return time_point(duration(GetCycle())); }
		
		/// @copydoc TickClock::sleep_for
		template <class Rep, class Period>
		static void sleep_for(const std::chrono::duration<Rep, Period> sleep_duration)
		{
			const auto start = now();
			
			if (sleep_duration <= std::chrono::duration<Rep, Period>::zero())
				return;
			
			static_assert(sizeof(rep) < sizeof(uintmax_t));
			
			using max_duration_t = std::chrono::duration<std::uintmax_t, period>;
			
			max_duration_t d = sleep_duration > max_duration_t::max() ? max_duration_t::max() : std::chrono::duration_cast<max_duration_t>(sleep_duration);
			
			if (d < sleep_duration && d != max_duration_t::max())
				++d;
			
			static constexpr auto _max = uintmax_t(std::numeric_limits<rep>::max()) + 1;
			
			const duration rem(d.count() % _max);
			
			for (auto rounds = d.count() / _max; rounds; --rounds)
			{
				static constexpr auto _mid = time_point::max().time_since_epoch() / 2;
				
				while (now().time_since_epoch() < _mid);
				while (now().time_since_epoch() >= _mid);
			}
			
			while (now() < start);
			while (now() - start < rem);
		}
	};
	
		
		
		{
			
		}
		
	};
	
	template <typename TIME_T = cycle_t>
	class Filter
	{
		TIME_T (*const cf_cyc)();
		const TIME_T c_minCyc;
		
		TIME_T m_prevCyc = 0, m_prevCyc2 = 0;
		bool m_prevState = false, m_prevState2 = false;
		
	public:
		Filter(TIME_T min_time, TIME_T (* cyc)() = GetCycle) : c_minCyc(min_time), cf_cyc(cyc) {}
		
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

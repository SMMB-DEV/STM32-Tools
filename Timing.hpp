#pragma once

#include "main.h"

#include <type_traits>
#include <limits>



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
	
	#ifndef STM32T_DELAY_CLK
	#error "STM32T_DELAY_CLK not defined!"
	#endif
	
	static_assert(STM32T_DELAY_CLK % 1'000'000 == 0, "STM32T_DELAY_CLK must be divisible by 1,000,000!");
	
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
		if (1000 * (SysTick->LOAD + 1) != STM32T_DELAY_CLK)
			Error_Handler();
	}
	
	inline uint32_t GetCycle()
	{
		static constexpr uint32_t MS = STM32T_DELAY_CLK / 1000;
		
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
		static_assert(std::numeric_limits<ms_time_t>::max() * uintmax_t(STM32T_DELAY_CLK / 1'000) <= std::numeric_limits<cycle_t>::max());
		return time_ms * (STM32T_DELAY_CLK / 1'000);
	}
	
	inline constexpr cycle_t usToCycles(const us_time_t time_us)
	{
		static_assert(std::numeric_limits<us_time_t>::max() * uintmax_t(STM32T_DELAY_CLK / 1'000'000) <= std::numeric_limits<cycle_t>::max());
		return time_us * (STM32T_DELAY_CLK / 1'000'000);
	}
	
	inline constexpr cycle_t nsToCycles(const ns_time_t time_ns)
	{
		static_assert(std::numeric_limits<ns_time_t>::max() * uintmax_t(STM32T_DELAY_CLK / 1'000'000) / 1000 <= std::numeric_limits<cycle_t>::max());
		return time_ns * (STM32T_DELAY_CLK / 1'000'000) / 1000;
	}
	
	inline constexpr ms_cycle_t CyclesTo_ms(cycle_t cyc)
	{
		static_assert(std::numeric_limits<cycle_t>::max() / (STM32T_DELAY_CLK / 1'000) <= std::numeric_limits<ms_cycle_t>::max());
		return cyc / (STM32T_DELAY_CLK / 1'000);
	}
	
	inline constexpr us_cycle_t CyclesTo_us(cycle_t cyc)
	{
		static_assert(std::numeric_limits<cycle_t>::max() / (STM32T_DELAY_CLK / 1'000'000) <= std::numeric_limits<us_cycle_t>::max());
		return cyc / (STM32T_DELAY_CLK / 1'000'000);
	}
	
	inline constexpr ns_cycle_t CyclesTo_ns(cycle_t cyc)
	{
		static_assert(std::numeric_limits<cycle_t>::max() * uintmax_t(1000) / (STM32T_DELAY_CLK / 1'000'000) <= std::numeric_limits<ns_cycle_t>::max());
		return cyc * uint64_t(1000) / (STM32T_DELAY_CLK / 1'000'000);
	}
	
	inline void Delay(cycle_t cyc)
	{
		const uint32_t start = GetCycle();
		while (GetCycle() - start < cyc);
	}
	
	inline void Delay_ms(ms_time_t ms)
	{
		const uint32_t start = GetCycle(), delay = msToCycles(ms);
		while (GetCycle() - start < delay);	// < because delay is usually accurate (when STM32T_DELAY_CLK is divisible by 1'000'000).
	}
	
	inline void Delay_us(us_time_t us)
	{
		const uint32_t start = GetCycle(), delay = usToCycles(us);
		while (GetCycle() - start < delay);	// < because delay is usually accurate (when STM32T_DELAY_CLK is divisible by 1'000'000).
	}
	
	inline void Delay_ns(ns_time_t ns)
	{
		const uint32_t start = GetCycle(), delay = nsToCycles(ns);
		while (GetCycle() - start <= delay);	// <= because delay isn't always accurate.
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
	
	template <typename T>
	inline void WaitAfter(const T start, const T wait, TickFuncPtr<T> get_tick = HAL_GetTick)
	{
		static_assert(!std::is_same_v<T, bool> && std::is_integral_v<T>);
		
		while (get_tick() - start < wait);
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
}

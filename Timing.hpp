#pragma once

#include "main.h"

#include <type_traits>



namespace STM32T::Time
{
	template <typename T>
	using TickFuncPtr = T (*)();
	
	template <typename T>
	using DelayFuncPtr = void (*)(T);
	
	#ifndef STM32T_DELAY_CLK
	#warning "STM32T_DELAY_CLK not defined! Defaulting to SystemCoreClock."
	#define STM32T_DELAY_CLK	(SystemCoreClock)
	#endif
	
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
	inline void Init() {}
	
	#pragma GCC diagnostic push
	#pragma GCC diagnostic error "-Wundefined-inline"
	[[gnu::always_inline]] inline uint32_t GetCycle();
	#pragma GCC diagnostic pop
	#endif	// DWT
	
	inline uint32_t msToCycles(const uint8_t time_ms)
	{
		return time_ms * (STM32T_DELAY_CLK / 1'000);
	}
	
	inline uint32_t usToCycles(const uint16_t time_us)
	{
		return time_us * (STM32T_DELAY_CLK / 1'000'000);
	}
	
	inline uint32_t nsToCycles(const uint16_t time_ns)
	{
		return time_ns * (STM32T_DELAY_CLK / 1'000'000) / 1000;
	}
	
	inline uint32_t CyclesTo_ms(uint32_t cyc)
	{
		return cyc / (STM32T_DELAY_CLK / 1'000);
	}
	
	inline uint32_t CyclesTo_us(uint32_t cyc)
	{
		return cyc / (STM32T_DELAY_CLK / 1'000'000);
	}
	
	inline uint32_t CyclesTo_ns(uint32_t cyc)
	{
		return cyc * 1000 / (STM32T_DELAY_CLK / 1'000'000);
	}
	
	inline void Delay(uint32_t cyc)
	{
		const uint32_t start = GetCycle();
		while (GetCycle() - start < cyc);
	}
	
	inline void Delay_ms(uint8_t ms)
	{
		const uint32_t start = GetCycle(), delay = msToCycles(ms);
		while (GetCycle() - start < delay);	// < because delay is usually accurate (when STM32T_DELAY_CLK is divisible by 1'000'000).
	}
	
	inline void Delay_us(uint16_t us)
	{
		const uint32_t start = GetCycle(), delay = usToCycles(us);
		while (GetCycle() - start < delay);	// < because delay is usually accurate (when STM32T_DELAY_CLK is divisible by 1'000'000).
	}
	
	inline void Delay_ns(uint16_t ns)
	{
		const uint32_t start = GetCycle(), delay = nsToCycles(ns);
		while (GetCycle() - start <= delay);	// <= because delay isn't always accurate.
	}
	
	inline bool Elapsed(const uint32_t startCycle, const uint32_t time_cycles)
	{
		return GetCycle() - startCycle >= time_cycles;
	}
	
	inline bool Elapsed_ms(const uint32_t startCycle, const uint8_t time_ms)
	{
		const uint32_t delay = msToCycles(time_ms);
		return GetCycle() - startCycle >= delay;
	}
	
	inline bool Elapsed_us(const uint32_t startCycle, const uint16_t time_us)
	{
		const uint32_t delay = usToCycles(time_us);
		return GetCycle() - startCycle >= delay;
	}
	
	inline bool Elapsed_ns(const uint32_t startCycle, const uint16_t time_ns)
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
	
	inline void WaitAfter(const uint32_t startCycle, const uint32_t wait_cycles)
	{
		while (GetCycle() - startCycle < wait_cycles);
	}
	
	inline void WaitAfter_ms(const uint32_t startCycle, const uint8_t wait_ms)
	{
		const uint32_t wait = msToCycles(wait_ms);
		while (GetCycle() - startCycle < wait);
	}
	
	inline void WaitAfter_us(const uint32_t startCycle, const uint16_t wait_us)
	{
		const uint32_t wait = usToCycles(wait_us);
		while (GetCycle() - startCycle < wait);
	}
}

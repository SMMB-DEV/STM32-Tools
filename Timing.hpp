#pragma once

#include "./Common.hpp"



#define STM32T_GET_TICK(TIM) \
__attribute__((always_inline)) uint32_t get_tick_us() \
{ \
	return (TIM)->CNT; \
}

#define STM32T_DELAY_US_16(TIM) \
void delay_us(const uint16_t delay) \
{ \
	(TIM)->CNT = 0; \
	while ((TIM)->CNT <= delay); \
}

#define STM32T_DELAY_US_32(TIM) \
void delay_us(const uint32_t delay) \
{ \
	(TIM)->CNT = 0; \
	while ((TIM)->CNT <= delay); \
}



namespace STM32T::Time
{
	template <typename T>
	using TickFuncPtr = T (*)();
	
	template <typename T>
	using DelayFuncPtr = void (*)(T);
	
	#ifdef DWT
	// https://community.st.com/t5/stm32-mcus-embedded-software/dwt-and-microsecond-delay/m-p/632748/highlight/true#M44839
	inline void DWT_Init()
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
	
	
	#ifndef DWT_DELAY_CLK
	#warning "DWT_DELAY_CLK not defined! Defaulting to SystemCoreClock."
	#define DWT_DELAY_CLK	(SystemCoreClock)
	#endif
	
	inline uint32_t usToCycles(const uint16_t time_us)
	{
		return time_us * (DWT_DELAY_CLK / 1'000'000);
	}
	
	inline uint32_t nsToCycles(const uint16_t time_ns)
	{
		return time_ns * (DWT_DELAY_CLK / 1'000'000) / 1000;
	}
	
	inline uint32_t CyclesTo_us(uint32_t cyc)
	{
		return cyc / (DWT_DELAY_CLK / 1'000'000);
	}
	
	inline uint32_t CyclesTo_ns(uint32_t cyc)
	{
		return cyc * 1000 / (DWT_DELAY_CLK / 1'000'000);
	}
	
	/**
	* @note Doesn't overflow at UINT32_MAX. Be careful when using this function for time difference measurement.
	*/
	[[deprecated("Use DWT_GetCycle() and DWT_Elapsed() instead.")]]
	inline uint32_t DWT_GetTick()
	{
		return DWT->CYCCNT / (DWT_DELAY_CLK / 1'000'000);
	}
	
	inline uint32_t GetCycle() { return DWT->CYCCNT; }
	
	inline void Delay_us(uint16_t us)
	{
		const uint32_t start = DWT->CYCCNT, delay = usToCycles(us);
		while (DWT->CYCCNT - start < delay);	// < because delay is usually accurate (when DWT_DELAY_CLK is divisible by 1'000'000).
	}
	
	inline void Delay_ns(uint16_t ns)
	{
		const uint32_t start = DWT->CYCCNT, delay = nsToCycles(ns);
		while (DWT->CYCCNT - start <= delay);	// <= because delay isn't always accurate.
	}
	
	inline bool Elapsed_us(const uint32_t startCycle, const uint16_t time_us)
	{
		const uint32_t delay = usToCycles(time_us);
		return DWT->CYCCNT - startCycle >= delay;
	}
	
	inline bool Elapsed_ns(const uint32_t startCycle, const uint16_t time_ns)
	{
		const uint32_t delay = nsToCycles(time_ns);
		return DWT->CYCCNT - startCycle > delay;
	}
	#endif	// DWT
	
	template <typename T>
	inline void WaitAfter(const T start, const T wait, TickFuncPtr<T> get_tick = HAL_GetTick)
	{
		static_assert(is_int_v<T>);
		
		while (get_tick() - start < wait);
	}
}

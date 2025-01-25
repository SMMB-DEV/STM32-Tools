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



namespace STM32T
{
	template <typename T>
	using TickFuncPtr = T (* const)();
	
	template <typename T>
	using DelayFuncPtr = void (* const)(const T);
	
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
	
	inline void DWT_Delay(uint16_t us)
	{
		const uint32_t start = DWT->CYCCNT, delay = us * (DWT_DELAY_CLK / 1'000'000);
		while (DWT->CYCCNT - start < delay);	// < because delay is usually accurate (when DWT_DELAY_CLK is divisible by 1'000'000).
	}
	
	/**
	* @note Doesn't overflow at UINT32_MAX. Be careful when using this function for time difference measurement.
	*/
	inline uint32_t DWT_GetTick()
	{
		return DWT->CYCCNT / (DWT_DELAY_CLK / 1'000'000);
	}
	
	inline void DWT_Delay_ns(uint16_t ns)
	{
		const uint32_t start = DWT->CYCCNT, delay = ns * (DWT_DELAY_CLK / 1'000'000) / 1000;
		while (DWT->CYCCNT - start <= delay);	// <= because delay isn't always accurate.
	}
	#endif	// DWT
	
	template <typename T>
	inline void WaitAfter(const T start, const T wait, T(* const get_tick)() = HAL_GetTick)
	{
		static_assert(is_int_v<T>);
		
		while (get_tick() - start < wait);
	}
}

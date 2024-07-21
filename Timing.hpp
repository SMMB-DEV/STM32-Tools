#pragma once

#include "main.h"

#include <type_traits>



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
	
	inline void DWT_Delay(uint32_t us)
	{
		const uint32_t startTick = DWT->CYCCNT, delayTicks = us * (SystemCoreClock / 1'000'000);
		while (DWT->CYCCNT - startTick < delayTicks);
	}
	#endif
	
	template <typename T>
	inline void WaitAfter(const T start, const T wait, T(* const get_tick)() = HAL_GetTick)
	{
		static_assert(!std::is_same_v<T, bool> && std::is_integral_v<T>, "");
		
		while (get_tick() - start < wait);
	}
}

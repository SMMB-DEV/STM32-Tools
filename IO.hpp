#pragma once

#include "main.h"

#include <type_traits>



namespace STM32T
{
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
	
	class IO
	{
		static constexpr uint32_t GPIO_NUMBER = 16;
		
		GPIO_TypeDef* const Port;
		const uint16_t Pin;
		const bool active_low;
		
	public:
		IO(GPIO_TypeDef* const Port, const uint16_t Pin, const bool active_low = false) : Port(Port), Pin(Pin), active_low(active_low)
		{
			assert_param(IS_GPIO_PIN(Pin));
		}
		
		~IO() {}
		
		__attribute__((always_inline)) bool Read()
		{
			return (bool)(Port->IDR & Pin) ^ active_low;
		}
		
		__attribute__((always_inline)) bool OutGet()
		{
			return (bool)(Port->ODR & Pin) ^ active_low;
		}
		
		__attribute__((always_inline)) void Set(bool state = true)
		{
			Port->BSRR = Pin << ((active_low ? state : !state) * GPIO_NUMBER);
		}
		
		__attribute__((always_inline)) void Reset()
		{
			Set(false);
		}
		
		__attribute__((always_inline)) void Toggle()
		{
			const uint32_t odr = Port->ODR;
			Port->BSRR = ((odr & Pin) << GPIO_NUMBER) | (~odr & Pin);
		}
		
		template<typename T>
		void Timed(const T time, void (* const delay)(const T delay) = HAL_Delay)
		{
			static_assert(!std::is_same_v<T, bool> && std::is_integral_v<T>, "");
			
			Set();
			delay(time);
			Reset();
		}
		
		void Error(const uint8_t n = 3, const uint32_t time1 = 200, const uint32_t time2 = 200)
		{
			for (uint8_t i = 0; i < n ; i++)
			{
				Timed(time1);
				HAL_Delay(time2);
			}
		}
	};
}

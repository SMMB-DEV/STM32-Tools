#pragma once

#include "main.h"

#include <type_traits>



namespace STM32T
{
	template <typename T>
	[[deprecated("Use IO::Wait() instead.")]]
	inline bool __attribute__((deprecated)) WaitOnPin(GPIO_TypeDef* const port, uint16_t pin, const bool desired_state, const T timeout, T(* const get_tick)() = HAL_GetTick)
	{
		static_assert(!std::is_same_v<T, bool> && std::is_integral_v<T>, "");
		
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
		const bool active_low;
		
	public:
		template <typename T>
		using TickFuncPtr = T (* const)();
		
		template <typename T>
		using DelayFuncPtr = void (* const)(const T);
		
		const uint16_t Pin;
		
		IO(GPIO_TypeDef* const Port, const uint16_t Pin, const bool active_low = false) : Port(Port), Pin(Pin), active_low(active_low)
		{
			assert_param(IS_GPIO_PIN(Pin));
		}
		
		__attribute__((always_inline)) bool Read()
		{
			return (bool)(Port->IDR & Pin) ^ active_low;
		}
		
		__attribute__((always_inline)) bool Check()
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
		
		template<typename T = uint32_t>
		void Timed(const T time, DelayFuncPtr<T> delay = HAL_Delay, bool state = true)
		{
			static_assert(!std::is_same_v<T, bool> && std::is_integral_v<T>, "");
			
			Set(state);
			delay(time);
			Set(!state);
		}
		
		void Error(const uint8_t n = 3, const uint32_t time1 = 200, const uint32_t time2 = 200)
		{
			for (uint8_t i = 0; i < n ; i++)
			{
				Timed(time1);
				HAL_Delay(time2);
			}
		}
		
		/**
		* @param mode - Must be a value of @ref GPIO_mode_define
		* @param pull - Must be a value of @ref GPIO_pull_define
		* @param speed - Must be a value of @ref GPIO_speed_define
		*/
		void SetMode(uint32_t mode, uint32_t pull = GPIO_NOPULL, uint32_t speed = GPIO_SPEED_FREQ_LOW)
		{
			GPIO_InitTypeDef init{ .Pin = Pin, .Mode = mode, .Pull = pull, .Speed = speed };
			HAL_GPIO_Init(Port, &init);
		}
		
		template <typename T>
		bool Wait(const bool desired_state, const T timeout, TickFuncPtr<T> get_tick = HAL_GetTick)
		{
			static_assert(!std::is_same_v<T, bool> && std::is_integral_v<T>, "");
			
			const T startTime = get_tick();
			while (Read() != desired_state)
			{
				if (get_tick() - startTime > timeout)
					return false;
			}
			
			return true;
		}
		
		template <typename T>
		T CheckPulse(const bool desired_state, const T max, const T min = 0, TickFuncPtr<T> get_tick = HAL_GetTick)
		{
			static_assert(!std::is_same_v<T, bool> && std::is_integral_v<T>, "");
			
			T elapsed = 0;
			const T startTime = get_tick();
			
			while (Read() == desired_state)
			{
				if ((elapsed = get_tick() - startTime) > max)
					return 0;
			}
			
			if (elapsed < min)
				return 0;
			
			return elapsed;
		}
	};
}

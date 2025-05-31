#pragma once

#include "./Timing.hpp"



namespace STM32T
{
	template <typename T>
	inline bool WaitOnPin(GPIO_TypeDef* const port, uint16_t pin, const bool desired_state, const T timeout, T(* const get_tick)() = HAL_GetTick)
	{
		static_assert(is_int_v<T>);
		
		const T start = get_tick();
		while (HAL_GPIO_ReadPin(port, pin) != desired_state)
		{
			if (get_tick() - start > timeout)
				return false;
		}
		
		return true;
	}
	
	class IO
	{
		static constexpr uint32_t GPIO_NUMBER = 16;
		
		GPIO_TypeDef* const Port;
		
	public:
		const uint16_t Pin;
		
	protected:
		const bool active_low;
		
		static inline GPIO_TypeDef S_DUMMY_GPIO = {0};
		
	public:
		IO(GPIO_TypeDef* const Port, const uint16_t Pin, const bool active_low = false) : Port(Port), Pin(Pin), active_low(active_low)
		{
			assert_param(IS_GPIO_PIN(Pin));
		}
		
		static IO None()
		{
			return IO(&S_DUMMY_GPIO, GPIO_PIN_0);
		}
		
		static IO* NonePtr()
		{
			static IO S_IO{&S_DUMMY_GPIO, GPIO_PIN_0};
			return &S_IO;
		}
		
		__attribute__((always_inline)) bool Read() const
		{
			return (Port->IDR & Pin) ? !active_low : active_low;
		}
		
		__attribute__((always_inline)) bool Check() const
		{
			return (Port->ODR & Pin) ? !active_low : active_low;
		}
		
		__attribute__((always_inline)) void Set(bool state = true)
		{
			Port->BSRR = Pin << ((active_low ^ state) ? 0 : GPIO_NUMBER);
		}
		
		__attribute__((always_inline)) void Reset()
		{
			Set(false);
		}
		
		__attribute__((always_inline)) void Toggle()
		{
			Port->ODR ^= Pin;
		}
		
		template<typename T = uint32_t>
		void Timed(const T time, Time::DelayFuncPtr<T> delay = HAL_Delay, bool state = true)
		{
			static_assert(is_int_v<T>);
			
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
		
		bool Wait(const bool desired_state, const uint32_t timeout) const
		{
			const uint32_t start = HAL_GetTick();
			while (Read() != desired_state)
			{
				if (HAL_GetTick() - start > timeout)
					return false;
			}
			
			return true;
		}
		
		bool Wait_us(const bool desired_state, const uint16_t timeout_us) const
		{
			const uint32_t start = Time::GetCycle(), timeoutCycles = Time::usToCycles(timeout_us);
			
			while (Read() != desired_state)
			{
				if (Time::GetCycle() - start > timeoutCycles)
					return false;
			}
			
			return true;
		}
		
		uint32_t CheckPulse(const bool desired_state, const uint32_t max, const uint32_t min = 0) const
		{
			uint32_t elapsed = 0;
			const uint32_t startTime = HAL_GetTick();
			
			while (Read() == desired_state)
			{
				if ((elapsed = HAL_GetTick() - startTime) > max)
					return 0;
			}
			
			if (elapsed < min)
				return 0;
			
			return elapsed;
		}
		
		uint16_t CheckPulse_us(const bool desired_state, const uint16_t max, const uint16_t min = 0) const
		{
			uint32_t elapsed = 0;
			const uint32_t startCycle = Time::GetCycle(), minCycles = Time::usToCycles(min), maxCycles = Time::usToCycles(max);
			
			while (Read() == desired_state)
			{
				if ((elapsed = Time::GetCycle() - startCycle) >= maxCycles)
					return 0;
			}
			
			if (elapsed < minCycles)
				return 0;
			
			return Time::CyclesTo_us(elapsed);
		}
	};
	
	template <size_t COUNT>
	class IOs
	{
		static_assert(COUNT > 1 && COUNT <= 32);
		
		std::array<IO, COUNT> m_pins;
		
	public:
		static constexpr size_t PinCount() { return COUNT; }
		
		IOs(std::array<IO, COUNT>&& pins) : m_pins(std::move(pins)) {}
		
		STM32T::IO& operator[](size_t index) { return m_pins[index]; }
		
		uint32_t Read() const
		{
			uint32_t bits = 0;
			for (size_t i = 0; i < COUNT; i++)
				bits |= m_pins[i].Read() << i;
			
			return bits;
		}
		
		uint32_t Check() const
		{
			uint32_t bits = 0;
			for (size_t i = 0; i < COUNT; i++)
				bits |= m_pins[i].Check() << i;
			
			return bits;
		}
		
		void Set(const uint32_t vals = 0xFFFF'FFFF, const uint32_t mask = 0xFFFF'FFFF)
		{
			for (uint32_t i = 0; i < COUNT; i++)
			{
				if (mask & (1 << i))
					m_pins[i].Set((vals >> i) & 1);
			}
		}
		
		void Reset(const uint32_t mask = 0xFFFF'FFFF)
		{
			Set(0x0000'0000, mask);
		}
		
		void Toggle(const uint32_t mask = 0xFFFF'FFFF)
		{
			for (uint32_t i = 0; i < COUNT; i++)
			{
				if (mask & (1 << i))
					m_pins[i].Toggle();
			}
		}
	};
	
	struct ScopeIO
	{
		IO m_io;
		
		ScopeIO(IO io) : m_io(io) { m_io.Set(); }
		~ScopeIO() { m_io.Reset(); }
	};
}

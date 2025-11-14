#pragma once

#include "./Timing.hpp"
#include "./Common.hpp"



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
	
	inline bool WaitOnPin_us(GPIO_TypeDef* const port, const uint16_t pin, const bool desired_state, const uint16_t timeout)
	{
		const uint32_t start = Time::GetCycle();
		while (HAL_GPIO_ReadPin(port, pin) != desired_state)
		{
			if (Time::Elapsed_us(start, timeout))
				return false;
		}
		
		return true;
	}
	
	class IOProxy
	{
	public:
		virtual ~IOProxy() {}
		
		virtual bool Read() const = 0;
		virtual bool Check() const = 0;
		virtual bool Set(bool state = true) = 0;
		
		[[gnu::always_inline]] bool Reset()
		{
			return Set(false);
		}
		
		virtual void Toggle() = 0;
		
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
	
	class IO : public IOProxy
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
		
		[[gnu::always_inline]] bool Read() const override
		{
			return (Port->IDR & Pin) ? !active_low : active_low;
		}
		
		[[gnu::always_inline]] bool Check() const override
		{
			return (Port->ODR & Pin) ? !active_low : active_low;
		}
		
		[[gnu::always_inline]] bool Set(bool state = true) override
		{
			Port->BSRR = Pin << ((active_low ^ state) ? 0 : GPIO_NUMBER);
			
			return true;	// todo: return actual change
		}
		
		[[gnu::always_inline]] void Toggle() override
		{
			Port->ODR ^= Pin;
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
	};
	
	template <size_t COUNT>
	class _IOs
	{
		static_assert(COUNT > 1 && COUNT <= 32);
		
	public:
		class PinProxy : public IOProxy
		{
			_IOs &m_ios;
			const uint8_t c_pin;
			
		public:
			PinProxy(_IOs &ios, uint8_t pin_number) : m_ios(ios), c_pin(pin_number) {}
			
			bool Read() const override
			{
				return m_ios.ReadBit(c_pin);
			}
			
			bool Check() const override
			{
				return m_ios.CheckBit(c_pin);
			}
			
			bool Set(bool state = true) override
			{
				return m_ios.SetBit(c_pin, state);
			}
			
			void Toggle() override
			{
				m_ios.ToggleBit(c_pin);
			}
		};
		
		virtual ~_IOs() {}
		
		static constexpr size_t PinCount() { return COUNT; }
		
		PinProxy Proxy(uint8_t pin_number)
		{
			return PinProxy(*this, pin_number);
		}
		
		virtual uint32_t Read() const = 0;
		
		bool ReadBit(uint8_t bit_number)
		{
			return (Read() >> bit_number) & 1;
		}
		
		virtual uint32_t Check() const = 0;
		
		bool CheckBit(uint8_t bit_number)
		{
			return (Check() >> bit_number) & 1;
		}
		
		/**
		* @retval True if any pin's state was changed (might not be implemented).
		*/
		virtual bool Set(const uint32_t vals = 0xFFFF'FFFF, const uint32_t mask = 0xFFFF'FFFF) = 0;
		
		bool SetBit(uint8_t bit_number, bool state)
		{
			return Set(state << bit_number, 1 << bit_number);
		}
		
		/**
		* @retval True if any pin's state was changed (might not be implemented).
		*/
		virtual bool Reset(const uint32_t mask = 0xFFFF'FFFF) = 0;
		
		void ResetBit(uint8_t bit_number)
		{
			Reset(1 << bit_number);
		}
		
		virtual void Toggle(const uint32_t mask = 0xFFFF'FFFF) = 0;
		
		void ToggleBit(uint8_t bit_number)
		{
			Toggle(1 << bit_number);
		}
		
		template<typename T = uint32_t>
		void Timed(const T time, const uint32_t vals = 0xFFFF'FFFF, Time::DelayFuncPtr<T> delay = HAL_Delay, const uint32_t mask = 0xFFFF'FFFF)
		{
			static_assert(is_int_v<T>);
			
			Set(vals, mask);
			delay(time);
			Set(~vals, mask);
		}
		
		template<typename T = uint32_t>
		void TimedBit(const T time, uint8_t bit_number, STM32T::Time::DelayFuncPtr<T> delay = HAL_Delay, bool state = true)
		{
			Timed<T>(time, state << bit_number, delay, 1 << bit_number);
		}
	};
	
	template <size_t COUNT>
	class [[deprecated("Use IOArray (alias) instead.")]] IOs : public _IOs<COUNT>
	{
		std::array<IO, COUNT> m_pins;
		
	public:
		IOs(std::array<IO, COUNT>&& pins) : m_pins(std::move(pins)) {}
		
		STM32T::IO& operator[](size_t index) { return m_pins[index]; }
		
		uint32_t Read() const override
		{
			uint32_t bits = 0;
			for (size_t i = 0; i < COUNT; i++)
				bits |= m_pins[i].Read() << i;
			
			return bits;
		}
		
		uint32_t Check() const override
		{
			uint32_t bits = 0;
			for (size_t i = 0; i < COUNT; i++)
				bits |= m_pins[i].Check() << i;
			
			return bits;
		}
		
		bool Set(const uint32_t vals = 0xFFFF'FFFF, const uint32_t mask = 0xFFFF'FFFF) override
		{
			for (uint32_t i = 0; i < COUNT; i++)
			{
				if (mask & (1 << i))
					m_pins[i].Set((vals >> i) & 1);
			}
			
			return true;
		}
		
		bool Reset(const uint32_t mask = 0xFFFF'FFFF) override
		{
			return Set(0x0000'0000, mask);
		}
		
		void Toggle(const uint32_t mask = 0xFFFF'FFFF) override
		{
			for (uint32_t i = 0; i < COUNT; i++)
			{
				if (mask & (1 << i))
					m_pins[i].Toggle();
			}
		}
	};
	
	template <size_t COUNT>
	using IOArray = IOs<COUNT>;
	
	struct ScopeIO
	{
		IO m_io;
		
		ScopeIO(IO io) : m_io(io) { m_io.Set(); }
		~ScopeIO() { m_io.Reset(); }
	};
}

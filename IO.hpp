/// @file

#pragma once

#include "./Core/Time.hpp"
#include "./Core/Utils.hpp"



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
	
	/// Base class for anything representing a GPIO pin.
	class _IO
	{
	public:
		virtual ~_IO() {}
		
		/// @return The input state of the pin.
		/// @note This might not be the same as the output state of the pin (e.g. output in OD mode).
		virtual bool Read() const = 0;
		
		/// @return The output state of the pin.
		virtual bool Check() const = 0;
		
		/// Sets the state of the pin without checking previous state.
		virtual void Set(bool state = true) = 0;
		
		/// @retval true if the state was actually changed.
		virtual bool SetCheck(bool state = true) = 0;
		
		/// Has the same effect as @ref Set "Set(false)".
		[[gnu::always_inline]] void Reset() { Set(false); }
		
		/// Has the same effect as @ref SetCheck "SetCheck(false)".
		/// @copydoc SetCheck
		[[gnu::always_inline]] bool ResetCheck() { return SetCheck(false); }
		
		[[gnu::always_inline]] virtual void Toggle() { Set(!Check()); }
		
		/**
		* Pulses the pin for the specified time (set/delay/reset).
		* @tparam TIME The underlying type for representing the time.
		* @param time The duration of the pulse.
		* @param state The type of pulse (high/low)
		* @param delay The delay function to use for @p time
		*/
		template<typename TIME = Time::tick_t>
		void Pulse(const TIME time, bool state = true, Time::DelayFuncPtr<TIME> delay = HAL_Delay)
		{
			static_assert(is_int_v<TIME>);
			
			Set(state);
			delay(time);
			Set(!state);
		}
		
		template<typename TIME = uint32_t>
		[[deprecated("Use Pulse() instead. Will be removed in v0.7.0.")]]
		void Timed(const TIME time, Time::DelayFuncPtr<TIME> delay = HAL_Delay, bool state = true) { Pulse<TIME>(time, state, delay); }
		
		void Error(const uint8_t n = 3, const Time::tick_t time_high = 200, const Time::tick_t time_low = 200)
		{
			for (uint8_t i = 1; i < n ; ++i)
			{
				Pulse(time_high);
				HAL_Delay(time_low);
			}
			
			if (n)
				Pulse(time_high);
		}
		
		bool Wait(const bool desired_state, const Time::tick_t timeout) const
		{
			const auto start = HAL_GetTick();
			while (Read() != desired_state)
			{
				#ifdef STM32T_IWDG_TIMEOUT
				HAL_IWDG_Refresh(&Time::hiwdg);
				#endif
				
				if (Time::Elapsed_Tick(start, timeout))
					return false;
			}
			
			return true;
		}
		
		bool Wait_us(const bool desired_state, const Time::us_time_t timeout_us) const
		{
			const Time::cycle_t start = Time::GetCycle(), timeoutCycles = Time::usToCycles(timeout_us);
			
			while (Read() != desired_state)
			{
				if (Time::Elapsed_us(start, timeout_us))
					return false;
			}
			
			return true;
		}
		
		uint32_t CheckPulse(const bool desired_state, const Time::tick_t max, const Time::tick_t min = 0) const
		{
			Time::tick_t elapsed = 0;
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
		
		Time::us_time_t CheckPulse_us(const bool desired_state, const Time::us_time_t max, const Time::us_time_t min = 0) const
		{
			Time::cycle_t elapsed = 0;
			const Time::cycle_t startCycle = Time::GetCycle(), minCycles = Time::usToCycles(min), maxCycles = Time::usToCycles(max);
			
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
	
	class IO : public _IO
	{
	protected:
		static constexpr uint32_t GPIO_NUMBER = 16;
		
		static inline GPIO_TypeDef S_DUMMY_GPIO = {0};
		
	public:
		GPIO_TypeDef *const cp_port;
		const uint16_t c_pin;
		bool active_low;
		
		IO(const IO& other) = default;
		IO(IO&& other) = default;
		
		IO(GPIO_TypeDef* const Port, const uint16_t Pin, const bool active_low = false) : cp_port(Port), c_pin(Pin), active_low(active_low)
		{
			assert_param(IS_GPIO_PIN(c_pin));
		}
		
		static IO None(bool active_low = false)
		{
			return IO(&S_DUMMY_GPIO, GPIO_PIN_0, active_low);
		}
		
		static IO* NonePtr()
		{
			static IO S_IO{&S_DUMMY_GPIO, GPIO_PIN_0};
			return &S_IO;
		}
		
		[[gnu::always_inline]] bool Read() const override { return (cp_port->IDR & c_pin) ? !active_low : active_low; }
		
		[[gnu::always_inline]] bool Check() const override { return (cp_port->ODR & c_pin) ? !active_low : active_low; }
		
		[[gnu::always_inline]] void Set(bool state = true) override { cp_port->BSRR = c_pin << ((active_low ^ state) ? 0 : GPIO_NUMBER); }
		
		bool SetCheck(bool state = true) override
		{
			const bool old = Check();
			if (old == state)
				return false;
			
			cp_port->BSRR = c_pin << ((active_low ^ state) ? 0 : GPIO_NUMBER);
			return true;
		}
		
		[[gnu::always_inline]] void Toggle() override
		{
			const uint32_t odr = cp_port->ODR;
			cp_port->BSRR = ((odr & c_pin) << GPIO_NUMBER) | (~odr & c_pin);	// Simply XORing ODR is not atomic.
		}
		
		/**
		* @param mode - Must be a value of `GPIO_mode` (HAL)
		* @param pull - Must be a value of `GPIO_pull` (HAL)
		* @param speed - Must be a value of `GPIO_speed` (HAL)
		*/
		void SetMode(uint32_t mode, uint32_t pull = GPIO_NOPULL, uint32_t speed = GPIO_SPEED_FREQ_LOW)
		{
			GPIO_InitTypeDef init{ .Pin = c_pin, .Mode = mode, .Pull = pull, .Speed = speed };
			HAL_GPIO_Init(cp_port, &init);
		}
	};
	
	/**
	* Base class for anything representing a collection of @ref _IO "_IO"s.
	* @tparam COUNT The max. number of pins.
	* @tparam T The underlying type for bitmasks.
	*/
	template <size_t COUNT, typename T = uint32_t>
	class _IOs
	{
	public:
		using mask_t = T;
		
		static_assert(is_int_v<mask_t> && std::is_unsigned_v<mask_t>);
		static_assert(COUNT > 1 && COUNT <= sizeof(mask_t) * 8, "Invalid COUNT value!");
		
		static constexpr size_t PIN_COUNT = COUNT;
		static constexpr mask_t ALL_PINS = std::numeric_limits<mask_t>::max();
		
		/// A proxy for a single pin in the collection.
		class PinProxy : public _IO
		{
			friend class _IOs;
			
			_IOs &m_ios;
			const uint8_t c_pin;
			
			PinProxy(_IOs &ios, uint8_t pin_number) : m_ios(ios), c_pin(pin_number) {}
			
		public:
			bool Read() const override { return m_ios.ReadBit(c_pin); }
			bool Check() const override { return m_ios.CheckBit(c_pin); }
			void Set(bool state = true) override { m_ios.SetBit(c_pin, state); }
			bool SetCheck(bool state = true) override { return m_ios.SetBitCheck(c_pin, state); }
			void Toggle() override { m_ios.ToggleBit(c_pin); }
		};
		
		virtual ~_IOs() {}
		
		constexpr size_t PinCount() { return COUNT; }
		
		PinProxy Proxy(uint8_t pin_number) { return PinProxy(*this, pin_number); }
		
		virtual mask_t Read() const = 0;
		
		[[gnu::always_inline]] virtual bool ReadBit(uint8_t bit_number) const { return (Read() >> bit_number) & mask_t(1u); }
		
		virtual mask_t Check() const = 0;
		
		[[gnu::always_inline]] virtual bool CheckBit(uint8_t bit_number) const { return (Check() >> bit_number) & mask_t(1u); }
		
		/// Sets the state of pins without checking previous states.
		virtual void Set(const mask_t vals = ALL_PINS, const mask_t mask = ALL_PINS) = 0;
		
		/// @retval true if any pin's state was changed.
		virtual mask_t SetCheck(const mask_t vals = ALL_PINS, const mask_t mask = ALL_PINS) = 0;
		
		/// @copydoc _IO::Set
		[[gnu::always_inline]] virtual void SetBit(uint8_t bit_number, bool state = true) { Set(state << bit_number, mask_t(1u) << bit_number); }
		
		/// @copydoc _IO::SetCheck
		[[gnu::always_inline]] virtual bool SetBitCheck(uint8_t bit_number, bool state = true) { return SetCheck(state << bit_number, mask_t(1u) << bit_number); }
		
		/// Has the same effect as @ref Set "Set(0, mask)".
		[[gnu::always_inline]] void Reset(const mask_t mask = ALL_PINS) { Set(mask_t(0), mask); }
		
		/// Has the same effect as @ref SetCheck "SetCheck(0, mask)".
		[[gnu::always_inline]] mask_t ResetCheck(const mask_t mask = ALL_PINS) { return SetCheck(mask_t(0), mask); }
		
		/// Has the same effect as @ref SetBit "SetBit(bit_number, false)".
		[[gnu::always_inline]] virtual void ResetBit(uint8_t bit_number) { Reset(mask_t(1u) << bit_number); }
		
		/// Has the same effect as @ref SetBitCheck "SetBitcheck(bit_number, false)".
		[[gnu::always_inline]] virtual bool ResetBitCheck(uint8_t bit_number) { return ResetCheck(mask_t(1u) << bit_number); }
		
		[[gnu::always_inline]] virtual void Toggle(const mask_t mask = ALL_PINS) { Set(Check() ^ mask, mask); }
		
		[[gnu::always_inline]] virtual void ToggleBit(uint8_t bit_number) { SetBit(bit_number, !CheckBit(bit_number)); }
		
		/**
		* @param mode - Must be a value of `GPIO_mode` (HAL)
		* @param pull - Must be a value of `GPIO_pull` (HAL)
		* @param speed - Must be a value of `GPIO_speed` (HAL)
		*/
		virtual void SetMode(uint32_t mode, uint32_t pull = GPIO_NOPULL, uint32_t speed = GPIO_SPEED_FREQ_LOW, const mask_t mask = ALL_PINS) = 0;
		
		[[gnu::always_inline]] virtual void SetBitMode(uint8_t bit_number, uint32_t mode, uint32_t pull = GPIO_NOPULL, uint32_t speed = GPIO_SPEED_FREQ_LOW)
		{
			SetMode(mode, pull, speed, mask_t(1u) << bit_number);
		}
		
		/**
		* Pulses the pins for the specified time (set/delay/reset).
		* @tparam TIME The underlying type for representing the time.
		* @param time The duration of the pulse.
		* @param vals The type of pulse for each pin (high/low)
		* @param mask The bitmask for @p vals
		* @param delay The delay function to use for @p time
		*/
		template<typename TIME = uint32_t>
		void Pulse(const TIME time, const mask_t vals = ALL_PINS, const mask_t mask = ALL_PINS, Time::DelayFuncPtr<TIME> delay = HAL_Delay)
		{
			static_assert(is_int_v<TIME>);
			
			Set(vals, mask);
			delay(time);
			Set(~vals, mask);
		}
		
		/// @copydoc _IO::Pulse
		/// @param bit_number The index of the pin in the collection.
		template<typename TIME = uint32_t>
		void PulseBit(uint8_t bit_number, const TIME time, bool state = true, STM32T::Time::DelayFuncPtr<TIME> delay = HAL_Delay)
		{
			static_assert(is_int_v<TIME>);
			
			SetBit(bit_number, state);
			delay(time);
			SetBit(bit_number, !state);
		}
		
		template<typename TIME = uint32_t>
		[[deprecated("Use Pulse() instead. Will be removed in v0.7.0.")]]
		void Timed(const TIME time, const uint32_t vals = 0xFFFF'FFFF, Time::DelayFuncPtr<TIME> delay = HAL_Delay, const uint32_t mask = 0xFFFF'FFFF)
		{
			Pulse<TIME>(time, vals, mask, delay);
		}
		
		template<typename TIME = uint32_t>
		[[deprecated("Use PulseBit() instead. Will be removed in v0.7.0.")]]
		void TimedBit(const TIME time, uint8_t bit_number, STM32T::Time::DelayFuncPtr<TIME> delay = HAL_Delay, bool state = true)
		{
			PulseBit<TIME>(bit_number, time, state, delay);
		}
	};
	
	template <size_t COUNT, typename T = uint32_t>
	class IOArray : public _IOs<COUNT, T>
	{
		using base = _IOs<COUNT, T>;
		
		std::array<IO, COUNT> m_pins;
		
	public:
		using typename base::mask_t;
		using base::ALL_PINS;
		
		IOArray(std::array<IO, COUNT>&& pins) : m_pins(std::move(pins)) {}
		
		STM32T::IO& operator[](size_t index) { return m_pins[index]; }
		
		mask_t Read() const override
		{
			mask_t bits = mask_t(0);
			for (size_t i = 0; i < COUNT; i++)
				bits |= m_pins[i].Read() << i;
			
			return bits;
		}
		
		[[gnu::always_inline]] bool ReadBit(uint8_t bit_number) const override { return m_pins[bit_number].Read(); }
		
		mask_t Check() const override
		{
			mask_t bits = mask_t(0);
			for (size_t i = 0; i < COUNT; i++)
				bits |= m_pins[i].Check() << i;
			
			return bits;
		}
		
		[[gnu::always_inline]] bool CheckBit(uint8_t bit_number) const override { return m_pins[bit_number].Check(); }
		
		void Set(const mask_t vals = ALL_PINS, const mask_t mask = ALL_PINS) override
		{
			for (size_t i = 0; i < COUNT; i++)
			{
				if (mask & (mask_t(1u) << i))
					m_pins[i].Set((vals >> i) & mask_t(1u));
			}
		}
		
		mask_t SetCheck(const mask_t vals = ALL_PINS, const mask_t mask = ALL_PINS) override
		{
			mask_t changed = mask_t(0);
			
			for (size_t i = 0; i < COUNT; i++)
			{
				if (mask & (mask_t(1u) << i))
					changed |= m_pins[i].SetCheck((vals >> i) & mask_t(1u)) << i;
			}
			
			return changed;
		}
		
		[[gnu::always_inline]] void SetBit(uint8_t bit_number, bool state = true) override { m_pins[bit_number].Set(state); }
		[[gnu::always_inline]] bool SetBitCheck(uint8_t bit_number, bool state = true) override { return m_pins[bit_number].SetCheck(state); }
		
		[[gnu::always_inline]] void ResetBit(uint8_t bit_number) override { m_pins[bit_number].Reset(); }
		[[gnu::always_inline]] bool ResetBitCheck(uint8_t bit_number) override { return m_pins[bit_number].ResetCheck(); }
		
		void Toggle(const mask_t mask = ALL_PINS) override
		{
			for (size_t i = 0; i < COUNT; i++)
			{
				if (mask & (mask_t(1u) << i))
					m_pins[i].Toggle();
			}
		}
		
		[[gnu::always_inline]] void ToggleBit(uint8_t bit_number) override { m_pins[bit_number].Toggle(); }
		
		void SetMode(uint32_t mode, uint32_t pull = GPIO_NOPULL, uint32_t speed = GPIO_SPEED_FREQ_LOW, const mask_t mask = ALL_PINS) override
		{
			for (size_t i = 0; i < COUNT; i++)
			{
				if (mask & (mask_t(1u) << i))
					m_pins[i].SetMode(mode, pull, speed);
			}
		}
		
		[[gnu::always_inline]] void SetBitMode(uint8_t bit_number, uint32_t mode, uint32_t pull = GPIO_NOPULL, uint32_t speed = GPIO_SPEED_FREQ_LOW) override
		{
			m_pins[bit_number].SetMode(mode, pull, speed);
		}
	};
	
	struct ScopeIO
	{
		IO m_io;
		
		ScopeIO(IO io) : m_io(io) { m_io.Set(); }
		~ScopeIO() { m_io.Reset(); }
	};
}

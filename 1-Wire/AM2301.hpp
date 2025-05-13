#pragma once

#include "../IO.hpp"
#include "../Timing.hpp"
#include "../Common.hpp"

#include <optional>



namespace STM32T
{
	class AM2301
	{
		// All times are in microseconds unless specified.
		static constexpr uint32_t
			BEGIN_MS = 1,
			RESP_DELAY_MAX = 200, RESP_LOW_MIN = 75, RESP_LOW_MAX = 95, RESP_HIGH_MIN = 75, RESP_HIGH_MAX = 95,
			BIT_LOW_MIN = 48, BIT_LOW_MAX = 70, BIT_0_HIGH_MIN = 22, BIT_0_HIGH_MAX = 30, BIT_1_HIGH_MIN = 65, BIT_1_HIGH_MAX = 80,
			END_MIN = 45, END_MAX = 65;
		
		IO m_sda;
		
		bool ReceiveByte(uint8_t* const data)
		{
			using namespace STM32T;
			
			*data = 0;
			
			for (uint8_t mask = 0b1000'0000; mask; mask >>= 1)
			{
				if (!m_sda.CheckPulse_us(false, BIT_LOW_MAX, BIT_LOW_MIN))
					return false;
				
				uint32_t highTime = m_sda.CheckPulse_us(true, std::max(BIT_0_HIGH_MAX, BIT_1_HIGH_MAX), std::min(BIT_0_HIGH_MIN, BIT_1_HIGH_MIN));
				
				if (highTime <= BIT_0_HIGH_MAX)					// 0 signal
					;
				else if (highTime >= BIT_1_HIGH_MIN)			// 1 signal
					*data |= mask;
				else
					return false;
			}
			
			return true;
		}
		
	public:
		/**
		* @param sda - Must be open-drain and pulled up.
		*/
		AM2301(IO sda) : m_sda(sda) {}
		
		/**
		* @retval A pair of 16-bit signed { temperature, humidity } multiplied by 10.
		*/
		std::optional<std::pair<int16_t, int16_t>> GetTempHum()
		{
			// Start signal
			m_sda.Reset();
			HAL_Delay(BEGIN_MS);
			m_sda.Set();
			m_sda.Wait_us(true, 1);	// Wait for line to go high
			
			// Response signal
			if (!m_sda.Wait_us(false, RESP_DELAY_MAX) ||
				!m_sda.CheckPulse_us(false, RESP_LOW_MAX, RESP_LOW_MIN) ||
				!m_sda.CheckPulse_us(true, RESP_HIGH_MAX, RESP_HIGH_MIN))
				return std::nullopt;
			
			// Data: 16 bits humidity + 16 bits temperature + 8 bits parity
			shared_arr<int16_t> humidity, temperature;
			uint8_t parity;
			
			if (!ReceiveByte(humidity.arr + 1) || !ReceiveByte(humidity.arr + 0) || !ReceiveByte(temperature.arr + 1) || !ReceiveByte(temperature.arr + 0) || !ReceiveByte(&parity))
				return std::nullopt;
			
			// Final low pulse
			if (!m_sda.CheckPulse_us(false, END_MAX, END_MIN))
				return std::nullopt;
			
			if (((humidity.arr[0] + humidity.arr[1] + temperature.arr[0] + temperature.arr[1]) & 0xFF) != parity)
				return std::nullopt;
			
			return std::pair{ temperature.val, humidity.val };
		}
	};
}

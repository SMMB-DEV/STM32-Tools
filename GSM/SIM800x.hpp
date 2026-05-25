#pragma once

#include "./GSM.hpp"



namespace STM32T
{
	class SIM800x : public GSM<150, 20>
	{
	public:
		SIM800x(UART_HandleTypeDef* huart) : GSM(huart) {}
		
	public:
		/*
		ErrorCode SetClock(const DateTime &dt, const uint32_t timeout = 10);
		ErrorCode GetClock(DateTime &dt, const uint32_t timeout = 10);
		
		ErrorCode ReadSMS(uint32_t& number, DateTime& dt, char * const data, uint16_t& len, const uint8_t index, const CMGR_Mode mode = CMGR_Mode::Normal, const uint32_t timeout = 5000);
		ErrorCode DeleteSMS(const uint8_t index, const CMGD_DelFlag delFlag = CMGD_DelFlag::Single, const uint32_t timeout = 5000);
		
		ErrorCode AnswerCall(const uint32_t timeout = 20000);
		ErrorCode Dial(const uint32_t number, const strv& prefix = "09"sv, const strv& postfix = ";"sv, const uint32_t timeout = 20000);	//number: last 9 digits; if longer, specify prefix
		ErrorCode HangUp(const uint32_t timeout = 20000);*/
		
		void Init(const bool enable_urc, const bool initial, const uint32_t power_on = 0, const uint32_t setup_delay = 10'000)
		{
			HAL_UART_Init(p_huart);		// todo: necessary?
			EnableURC(enable_urc);
			
			if (initial)
			{
				Time::WaitAfter_Tick(power_on, setup_delay);
				STM32T::Retry(3, 1000, std::bind(&SIM800x::Setup, this, 3000), OK, Error_Handler);	// Answer delay tested on SIM800L
			}
		}
	};
}

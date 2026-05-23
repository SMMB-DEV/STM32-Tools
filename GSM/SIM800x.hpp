#pragma once

#include "./GSM.hpp"



class SIM800x : public STM32T::GSM
{
private:
	static constexpr uint8_t H2D[] =	// todo: move to Common
	{
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  15
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  31
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  47
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  63
		0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  79
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  95
		0xFF, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 111
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 127
		
	};
	
public:
	struct DateTime
	{
		static constexpr uint8_t MonthDays[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };	// February can be 29
		
		uint8_t yy, MM, dd, hh, mm, ss;
		int8_t zz;
		
		static bool Parse(DateTime& dt, const strv& view)
		{
			return 7 == sscanf(view.data(), "\"%2hhu/%2hhu/%2hhu,%2hhu:%2hhu:%2hhu%3hhd\"", &dt.yy, &dt.MM, &dt.dd, &dt.hh, &dt.mm, &dt.ss, &dt.zz) && dt.IsSet();
		}
		
		static bool IsLeapYear(uint8_t yy)
		{
			// These extra days occur in each year that is an integer multiple of 4 (except for years evenly divisible by 100, but not by 400) [https://en.wikipedia.org/wiki/Leap_year]
			return yy % 4 == 0 && yy != 0;
		}
		
		bool IsSet() const
		{
			return yy <= 99 && MM >= 1 && MM <= 12 && dd >= 1 && dd <= MonthDays[MM - 1] + (MM == 2 && IsLeapYear(yy)) && hh <= 23 && mm <= 59 && ss <= 60 && zz >= -47 && zz <= 48;
		}
		
		const char* Format() const
		{
			static char fmt[6 * 4] = { 0 };	//  123:
			
			sprintf(fmt, "%02hhu/%02hhu/%02hhu,%02hhu:%02hhu:%02hhu", yy, MM, dd, hh, mm, ss);
			return fmt;
		}
	};
	
	
	enum class CMGR_Mode : bool { Normal, NoChange };
	enum class CMGD_DelFlag : uint8_t { Single, Read, ReadSent, ReadSentUnsent, All };
	
	//todo: replace std::string_view with std::span
	
public:

	SIM800x(UART_HandleTypeDef* UART) : GSM(UART) {}
	
private:
	
public:
	ErrorCode Setup();
	
	ErrorCode GetSignalQuality(int8_t& rssi, uint8_t& ber, const uint32_t timeout = 15);
	
	ErrorCode SetClock(const DateTime &dt, const uint32_t timeout = 10);
	ErrorCode GetClock(DateTime &dt, const uint32_t timeout = 10);
	
	ErrorCode SendSMS(const uint32_t number, const char * const data, const uint16_t len, const uint32_t timeout = 60000);
	ErrorCode ReadSMS(uint32_t& number, DateTime& dt, char * const data, uint16_t& len, const uint8_t index, const CMGR_Mode mode = CMGR_Mode::Normal, const uint32_t timeout = 5000);
	ErrorCode DeleteSMS(const uint8_t index, const CMGD_DelFlag delFlag = CMGD_DelFlag::Single, const uint32_t timeout = 5000);
	
	ErrorCode AnswerCall(const uint32_t timeout = 20000);
	ErrorCode Dial(const uint32_t number, const strv& prefix = "09"sv, const strv& postfix = ";"sv, const uint32_t timeout = 20000);	//number: last 9 digits; if longer, specify prefix
	ErrorCode HangUp(const uint32_t timeout = 20000);
};

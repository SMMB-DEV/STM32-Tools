#pragma once

#ifndef   _GSM_HPP_
#define   _GSM_HPP_


//#define GSM_URC_SUPPORT


#include <string_view>
#include <vector>
#include <functional>



using std::operator""sv;

class GSM
{
public:
	enum class ErrorCode : uint8_t
	{
		INVALID_PARAM,
		UART_ERR,
		OK = 10,
		ERR,
		WRONG_FORMAT,
		UNKNOWN = 25
	};
	
protected:
	using strv = std::string_view;
	
	template <class T>
	using vect = std::vector<T>;
	
	template <class F>
	using func = std::function<F>;

	enum class CommandType : uint8_t
	{
		Test,		// AT+CMD=?
		Read,		// AT+CMD?
		Execute,	// AT+CMD
		Write,		// AT+CMD=PARAM
		Bare		// CMD
	};
	
	
	
	virtual void SendUART(strv data) = 0;
	virtual ErrorCode ReceiveUART(char* buffer, uint16_t& received, const uint32_t timeout_ms) = 0;
};

#endif	// _GSM_HPP_

#pragma once

#ifndef   _SIM800x_HPP_
#define   _SIM800x_HPP_


#include "GSM.hpp"

#include <Common/Common.hpp>

#include <cstdarg>



/* The module's echo option must be disabled before using this library. To do this:
Send "ATE0\r" to the module via the serial port and then send "AT&W" to save the configurations.*/

class SIM800x : public GSM
{
private:
	static constexpr uint8_t H2D[] =
	{
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  15
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  31
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  47
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  63
		0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  79
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  95
		0xFF, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 111
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 127
		
		//0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 143
		//0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 159
		//0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 175
		//0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 191
		//0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 207
		//0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 223
		//0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 239
		//0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	};
	
	struct ListItem
	{
		ListItem* next = nullptr;
		char* data;
		const uint32_t timestamp;
		const uint16_t size;
		const bool tokenized;
		
		ListItem(const char* data, const uint16_t size, const bool tokenized = false) : timestamp(HAL_GetTick()), size(size + 1), tokenized(tokenized)
		{
			memcpy(this->data = new char[this->size], data, size);
			this->data[size] = '\0';
		}
		
		~ListItem()
		{
			delete[] data;
		}
		
		explicit operator strv()
		{
			return strv((char*)data, size);
		}
	};
	
	volatile uint8_t Buffer[512];
	ListItem* volatile List = nullptr;
	
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

	class URC : public strv
	{
	public:
		uint32_t timestamp;
		
		URC() : strv(), timestamp(0) {}
		URC(const char* const data, const size_t size, const uint32_t timestamp) : strv(data, size), timestamp(timestamp) {}
		URC(const strv& other, const uint32_t timestamp) : strv(other), timestamp(timestamp) {}
		
		bool compare_remove(const std::string_view& remove)
		{
			if (compare(0, remove.size(), remove) == 0)
			{
				remove_prefix(remove.size());
				return true;
			}

			return false;
		}
	};
	
	
	enum class CMGR_Mode : bool { Normal, NoChange };
	enum class CMGD_DelFlag : uint8_t { Single, Read, ReadSent, ReadSentUnsent, All };
	
	//todo: replace std::string_view with std::span
	
protected:
	static constexpr uint32_t DEFAUL_RECEIVE_TIMEOUT = 100;	// > 2048 * 10 / 115200
	static constexpr size_t DEFAULT_RESPONSE_LEN = 32, DEFAULT_ARG_LEN = 32;

	ErrorCode Standard(const vect<strv>& tokens);
	
	ErrorCode Command(vect<strv>& tokens, char* buffer, uint16_t len, const uint32_t timeout, const CommandType type, const strv& cmd, const strv& args = strv(), const bool allowSingleEnded = false);
	


	template <size_t LEN = DEFAULT_RESPONSE_LEN>
	ErrorCode Tokens(const uint32_t timeout, const CommandType type, const strv& cmd,
		const strv& args = strv(), const func<ErrorCode (vect<strv>&)>& op = nullptr, const bool allowSingleEnded = false);
	
	template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
	ErrorCode Tokens(const uint32_t timeout, const CommandType type, const strv& cmd,
		const func<ErrorCode (vect<strv>&)>& op, const bool allowSingleEnded = false, const char* const fmt = "", ...);
	
	
	
	template <size_t LEN = DEFAULT_RESPONSE_LEN>
	ErrorCode FirstLastToken(const size_t expectedTokens, const uint32_t timeout, const CommandType type, const strv& cmd,
		const strv& args = strv(), const func<ErrorCode (vect<strv>&)>& op = nullptr);
	
	template <size_t ARG_LEN = DEFAULT_ARG_LEN, size_t LEN = DEFAULT_RESPONSE_LEN>
	ErrorCode FirstLastToken(const size_t expectedTokens, const uint32_t timeout, const CommandType type, const strv& cmd,
		const func<ErrorCode (vect<strv>&)>& op, const char* const fmt = "", ...);
	
public:
	UART_HandleTypeDef* const phuart;

	SIM800x(UART_HandleTypeDef* UART) : phuart(UART) {}
	
	
	void handleURCs(void (* const handler)(URC urc));
	
	HAL_StatusTypeDef receiveURC()
	{
		// FOR SOME FUCKING REASON, (SOMETIMES) IT WON'T WORK WITHOUT THIS!!!
		
		//__HAL_UART_DISABLE(phuart);
		//__HAL_UART_ENABLE(phuart);	//clears USART_ISR
		__HAL_UART_CLEAR_FLAG(phuart, UART_CLEAR_PEF | UART_CLEAR_FEF | UART_CLEAR_NEF | UART_CLEAR_OREF);
		//__HAL_UART_CLEAR_FLAG(phuart, UART_CLEAR_OREF);
		//__HAL_UART_CLEAR_FLAG(phuart, 0xFFFFFFFF);
		//HAL_UART_Init(phuart);
		
		HAL_StatusTypeDef stat = HAL_UARTEx_ReceiveToIdle_DMA(phuart, (uint8_t*)Buffer, sizeof(Buffer) - 1);	// null termination in ListItem
		__HAL_DMA_DISABLE_IT(phuart->hdmarx, DMA_IT_HT);	//Disable half transfer interrupt; It is enabled in HAL_UARTEx_ReceiveToIdle_DMA()
		
		return stat;
	}
	
	HAL_StatusTypeDef cancelURC() __attribute__((always_inline))
	{
		return HAL_UART_DMAStop(phuart);
	}
	
private:
	void addURC(const strv& token)
	{
		if (List)
		{
			ListItem* end = List;
			while (end->next)
				end = end->next;
			
			end->next = new ListItem(token.data(), token.size(), true);
		}
		else
			List = new ListItem(token.data(), token.size(), true);
	}
	
public:
	void newURC(const uint16_t Size, const bool receive = true)
	{
		if (List)
		{
			ListItem* end = List;
			while (end->next)
				end = end->next;
			
			end->next = new ListItem((char*)Buffer, Size);
		}
		else
			List = new ListItem((char*)Buffer, Size);
		
		if (receive)
			receiveURC();
	}
	
	
	ErrorCode Setup();
	
	ErrorCode AT(const uint32_t timeout = 10);
	
	ErrorCode GetSignalQuality(int8_t& rssi, uint8_t& ber, const uint32_t timeout = 15);
	
	ErrorCode SetClock(const DateTime &dt, const uint32_t timeout = 10);
	ErrorCode GetClock(DateTime &dt, const uint32_t timeout = 10);
	
	ErrorCode SendSMS(const uint32_t number, const char * const data, const uint16_t len, const uint32_t timeout = 60000);
	ErrorCode ReadSMS(uint32_t& number, DateTime& dt, char * const data, uint16_t& len, const uint8_t index, const CMGR_Mode mode = CMGR_Mode::Normal, const uint32_t timeout = 5000);
	ErrorCode DeleteSMS(const uint8_t index, const CMGD_DelFlag delFlag = CMGD_DelFlag::Single, const uint32_t timeout = 5000);
	
	ErrorCode AnswerCall(const uint32_t timeout = 20000);
	ErrorCode Dial(const uint32_t number, const strv& prefix = "09"sv, const strv& postfix = ";"sv, const uint32_t timeout = 20000);	//number: last 9 digits; if longer, specify prefix
	ErrorCode HangUp(const uint32_t timeout = 20000);
	
	/*ErrorCode CGDCONT(uint8_t cid, const char* APN, const char* ip="0.0.0.0");
	ErrorCode CGACT(uint8_t state, uint8_t cid);

	ErrorCode CIMI(char* IMSI);																//13: Format Error

	ErrorCode CIPSTART(bool mode, uint8_t* ip_d, uint16_t port);							//10:CONNECT OK    12:ALREADY CONNECT    13:CONNECT FAIL    14:OK but unclear
	ErrorCode CIPSTART(bool mode, const char* ip_d, uint16_t port);							//10:CONNECT OK    12:ALREADY CONNECT    13:CONNECT FAIL    14:OK but unclear
	
	ErrorCode CIPSEND(uint8_t* data);														//10:SEND OK    13:SEND FAIL    14:'>' then unclear
	ErrorCode CIPSEND(const char* data);													//10:SEND OK    13:SEND FAIL    14:'>' then unclear
	ErrorCode CIPSEND(uint8_t* data, uint16_t len);
	
	ErrorCode CIPCLOSE(void);

	ErrorCode CIPSHUT(void);																//10:SHUT OK
	
	ErrorCode CSMINS(void);																	//14:Unclear   20:SIM card inserted   21:SIM card not inserted
	
	ErrorCode CANT(void);																	//10-13:see SIM800C datasheet   21:ERROR(2)   14:OK but unclear   24:OK(2) but unclear
	
	ErrorCode CGREG(void);																	//5-10:see SIM800C datasheet   24:OK but unclear
	
	ErrorCode CUSD(uint8_t* code, uint8_t* response);										//20:1st OK, no 2nd OK
	ErrorCode CUSD(const char* code, uint8_t* response);									//20:1st OK, no 2nd OK
	
	ErrorCode CIPSRIP_W(bool mode);
	ErrorCode CIPSRIP_R(void);																//15:CIPSRIP=0   16:CIPSRIP=1
	
	ErrorCode CIPSTATUS(void);																//30-39:SIM800C 0-9
	
	ErrorCode CMGS(const char *da, const char * text_pdu, uint8_t toda=129);				//14:Unclear   21:ERROR(2)
	
	ErrorCode CSMP(uint8_t dcs, uint8_t fo=49, uint8_t vp=167, uint8_t pid=0);*/
};

#endif // _SIM800x_HPP_


#include "./SIM800x.hpp"



/*using STM32T::SIM800x;



SIM800x::ErrorCode SIM800x::SetClock(const DateTime &dt, const uint32_t timeout)
{
	if (!dt.IsSet())
		return ErrorCode::INVALID_PARAM;
	
	return Tokens(timeout, CommandType::Write, "+CCLK"sv, nullptr, false, "\"%02hhu/%02hhu/%02hhu,%02hhu:%02hhu:%02hhu%+02hhd\"", dt.yy, dt.MM, dt.dd, dt.hh, dt.mm, dt.ss, dt.zz);
}

SIM800x::ErrorCode SIM800x::GetClock(DateTime &dt, const uint32_t timeout)
{
	//40: \r\n+CCLK: "00/00/00,00:00:00:+00"\r\n\r\nOK\r\n
	return FirstLastToken<40>(2, timeout, CommandType::Read, "+CCLK"sv, strv(), [&dt](vect<strv>& tokens) -> ErrorCode
	{
		return DateTime::Parse(dt, tokens[0]) ? ErrorCode::OK : ErrorCode::WRONG_FORMAT;
	});
}



SIM800x::ErrorCode SIM800x::ReadSMS(uint32_t& number, DateTime& dt, char * const data, uint16_t& len, const uint8_t index, const CMGR_Mode mode, const uint32_t timeout)
{
	if (!data || !len)
		return ErrorCode::INVALID_PARAM;
	
	//438: \r\n+CMGR: "REC UNREAD","{13 * 2 bytes}",["{12 bytes}"],"00/00/00,00:00:00+00",[000,00,0,0,"",000,000]\r\n{160 * 2 bytes}\r\nOK\r\n
	return FirstLastToken<DEFAULT_ARG_LEN, 438>(3, timeout, CommandType::Write, "+CMGR"sv, [&number, &dt, &data, &len](vect<strv>& tokens) -> ErrorCode
	{
		if (tokens[1].size() % 2 != 0)
			return ErrorCode::UNKNOWN;
		
		//assuming +CSCS="HEX"
		char stat[11], oa[27]; 	//REC UNREAD, 13 * 2 + 1
		int oaLen, readLen;
		if (2 != sscanf(tokens[0].data(), "\"%10[ABCDEFGHIJKLMNOPQRSTUVWXYZ ]\",\"%n%26[0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ]%n\",", stat, &oaLen, oa, &readLen))
			return ErrorCode::WRONG_FORMAT;
		
		tokens[0].remove_prefix(readLen + 2);	// ",
		oaLen = readLen - oaLen;
		
		if (oaLen % 2 != 0)
			return ErrorCode::WRONG_FORMAT;
		
		for (uint8_t i = 0; i < oaLen; i += 2)
			oa[i / 2] = (H2D[oa[i]] << 4) | H2D[oa[i + 1]];
		
		oa[oaLen /= 2] = '\0';
		
		//todo: implement more number formats
		//+989xxxxxxxxx
		if (strncmp(oa, "+989", 4) == 0 && oaLen == 13)
		{
			number = 0;
			uint32_t exp = 1;
			for (uint8_t i = 12; i >= 4; i--)
			{
				if (isdigit(oa[i]))
				{
					number += (oa[i] - '0') * exp;
					exp *= 10;
				}
				else
					return ErrorCode::WRONG_FORMAT;
			}
		}
		else
			return ErrorCode::UNKNOWN;
		
		
		
		//extract date and time
		CompareAndRemove(tokens[0], "\"\","sv);	//todo: write more comprehensive format
		
		if (!DateTime::Parse(dt, tokens[0]))
			return ErrorCode::WRONG_FORMAT;
		
		
		
		//extract message body/text
		const char* body = tokens[1].data();
		uint16_t i = 0;
		for (; i < tokens[1].size() && i / 2 < len; i += 2)
		{
			if (isxdigit(body[i]) && isxdigit(body[i + 1]) && !islower(body[i]) && !islower(body[i + 1]))
				data[i / 2] = (H2D[body[i]] << 4) | H2D[body[i + 1]];
			else
				return ErrorCode::WRONG_FORMAT;
		}
		
		len = i / 2;
		
		return ErrorCode::OK;
	},
	"%hhu,%hhu", index, mode);
}

SIM800x::ErrorCode SIM800x::DeleteSMS(const uint8_t index, const CMGD_DelFlag delFlag, const uint32_t timeout)
{
	if (delFlag > CMGD_DelFlag::All)
		return ErrorCode::INVALID_PARAM;
	
	return Tokens(timeout, CommandType::Write, "+CMGD"sv, nullptr, false, "%hhu,%hhu", index, delFlag);
}



SIM800x::ErrorCode SIM800x::AnswerCall(const uint32_t timeout)
{
	return Tokens(timeout, CommandType::Execute, "A"sv);
}

SIM800x::ErrorCode SIM800x::Dial(const uint32_t number, const strv& prefix, const strv& postfix, const uint32_t timeout)
{
	return Tokens(timeout, CommandType::Execute, "D"sv, nullptr, false, "%*s%u%*s", prefix.size(), prefix.data(), number, postfix.size(), postfix.data());
}

SIM800x::ErrorCode SIM800x::HangUp(const uint32_t timeout)
{
	return Tokens(timeout, CommandType::Execute, "H"sv);
}*/

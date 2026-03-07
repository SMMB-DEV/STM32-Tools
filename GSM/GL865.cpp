#include "./GL865.hpp"

using STM32T::vec;
using STM32T::operator"" _Ki;



GL865::ErrorCode GL865::NetworkCheck(uint8_t& stat)
{
	return ResponseToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Read, "+CGREG", strv(), [&](strv token) -> ErrorCode
	{
		uint8_t n;
		if (sscanf(token.data(), "%hhu,%hhu", &n, &stat) == 2)
			return OK;
		
		return ERR;
	});
}

bool GL865::NetworkWait(const uint32_t timeout)
{
	for (const uint32_t start = HAL_GetTick(); HAL_GetTick() - start < timeout;)
	{
		uint8_t stat;
		if (NetworkCheck(stat) == ErrorCode::OK && stat == 1 || stat == 5)
			return true;
		
		HAL_Delay(1000);
	}
	
	return false;
}

GL865::ErrorCode GL865::FTPOpen(strv address, strv user, strv pass)
{
	return SingleToken<256>(100'000, CommandType::Write, "#FTPOPEN"sv, "OK"sv, false, "%.*s,%.*s,%.*s,1", address.length(), address.data(), user.length(), user.data(), pass.length(), pass.data());
}

GL865::ErrorCode GL865::FTPTimeout(uint32_t timeout)
{
	return SingleToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Write, "#FTPTO"sv, "OK"sv, false, "%u", std::clamp(timeout / 100, 100u, 5000u));
}

GL865::ErrorCode GL865::FTPCWD(strv dir)
{
	return SingleToken(15'000, CommandType::Write, "#FTPCWD"sv, dir);
}

GL865::ErrorCode GL865::FTPType(bool ascii)
{
	return SingleToken(15'000, CommandType::Write, "#FTPTYPE"sv, "OK"sv, false, "%u", ascii);
}

GL865::ErrorCode GL865::FTPFileSize(strv file, size_t& size)
{
	return ResponseToken(15'000, CommandType::Write, "#FTPFSIZE"sv, file, [&](strv token) -> ErrorCode
	{
		return sscanf(token.data(), "%zu", &size) == 1 ? OK : WRONG_FORMAT;
	});
}

GL865::ErrorCode GL865::FTPList(strv name)
{
	return SingleToken(15'000, CommandType::Write, "#FTPLIST"sv, name);
}

GL865::ErrorCode GL865::FTPPut(strv file)
{
	return SingleToken(15'000, CommandType::Write, "#FTPPUT"sv, "OK"sv, false, "\"%.*s\",1", file.length(), file.data());
}

GL865::ErrorCode GL865::FTPAppend(strv data, bool final)
{
	ErrorCode code = SingleToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Write, "#FTPAPPEXT"sv, "> "sv, true, "%hu,%hhu", std::min(data.size(), 1500u), final);
	
	if (code != OK)
	{
		SendUART(data);		// Send both in case that it works and so that it doesn't get stuck.
		return code;
	}
	
	// \r\n#FTPAPPEXT: 1500\r\n\r\nOK\r\n
	return ResponseToken(15'000, CommandType::Bare, "#FTPAPPEXT"sv, data, [](strv token) -> ErrorCode
	{
		return sscanf(token.data(), "%*hu") == 0 ? OK : WRONG_FORMAT;
	});
}

GL865::ErrorCode GL865::FTPGet(strv file)
{
	return SingleToken(15'000, CommandType::Write, "#FTPGETPKT"sv, file);
}

GL865::ErrorCode GL865::FTPRecv(char* buf, uint16_t& len)
{
	return NoToken<DEFAULT_ARG_LEN, 3000 + DEFAULT_RESPONSE_LEN>(15'000, CommandType::Write, "#FTPRECV"sv, [&](strv data) -> ErrorCode
	{
		if (!data.remove_prefix("\r\n#FTPRECV: "sv))
			return WRONG_FORMAT;
		
		uint16_t data_len;
		uint32_t n;
		if (sscanf(data.data(), "%hu%n", &data_len, &n) != 1)
			return WRONG_FORMAT;
		
		if (data.compare(n, 2, "\r\n"sv) != 0)
			return WRONG_FORMAT;
		
		data.remove_prefix(n + 2);
		
		static constexpr strv end = "\r\n\r\nOK\r\n"sv;
		if (data.compare(data.size() - end.size(), end.size(), end) != 0)
			return WRONG_FORMAT;
		
		data.remove_suffix(end.size());
		
		if (data.size() != data_len)
			return WRONG_FORMAT;
		
		len = data.copy(buf, data_len);
		
		return ErrorCode::OK;
	}, "%hu", std::min<uint16_t>(len , 3000));
}

GL865::ErrorCode GL865::FTPClose()
{
	return SingleToken(15'000, CommandType::Execute, "#FTPCLOSE"sv);
}

GL865::ErrorCode GL865::SIMCheck(uint8_t& status)
{
	return ResponseToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Read, "#QSS"sv, strv(), [&](strv token) -> ErrorCode
	{
		uint8_t mode;
		if (sscanf(token.data(), "%hhu,%hhu", &mode, &status) == 2)
			return OK;
		
		return ERR;
	});
}

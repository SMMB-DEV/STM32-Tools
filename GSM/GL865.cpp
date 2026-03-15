#include "./GL865.hpp"

GL865::ErrorCode GL865::FTPFileSize(strv file, size_t& size)
{
	return ResponseToken(15'000, CommandType::Write, "#FTPFSIZE"sv, file, [&](strv token) -> ErrorCode
	{
		return sscanf(token.data(), "%zu", &size) == 1 ? OK : WRONG_FORMAT;
	});
}

GL865::ErrorCode GL865::FTPPut(strv file)
{
	return ReceiveOK(15'000, CommandType::Write, "#FTPPUT"sv, "\"%.*s\",1", file.length(), file.data());
}

GL865::ErrorCode GL865::FTPAppend(strv data, bool final)
{
	ErrorCode code = WaitForReady(DEFAUL_RECEIVE_TIMEOUT, CommandType::Write, "#FTPAPPEXT"sv, "%hu,%hhu", std::min(data.size(), 1500u), final);
	
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

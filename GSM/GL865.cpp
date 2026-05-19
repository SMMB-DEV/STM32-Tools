#include "./GL865.hpp"

GL865::ErrorCode GL865::FTPFileSize(strv file, size_t& size)
{
	return ResponseToken(15'000, CommandType::Write, "#FTPFSIZE"sv, [&](const std::vector<strv>& tokens) -> ErrorCode
	{
		return sscanf(tokens[0].data(), "%zu", &size) == 1 ? OK : WRONG_FORMAT;
	}, 1, 2, file);
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
	return ResponseToken(15'000, CommandType::Bare, "#FTPAPPEXT"sv, [](const std::vector<strv>& tokens) -> ErrorCode
	{
		return sscanf(tokens[0].data(), "%*hu") == 0 ? OK : WRONG_FORMAT;
	}, 1, 2, data);
}

GL865::ErrorCode GL865::SIMCheck(uint8_t& status)
{
	return ResponseToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Read, "#QSS"sv, [&](const std::vector<strv>& tokens) -> ErrorCode
	{
		uint8_t mode;
		if (sscanf(tokens[0].data(), "%hhu,%hhu", &mode, &status) == 2)
			return OK;
		
		return ERR;
	});
}

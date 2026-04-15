#pragma once

#include "./GSM.hpp"
#include "../IO.hpp"
#include "../Core/span.hpp"



using STM32T::strv;
using STM32T::wstrv;
using std::operator"" sv;

#ifdef STM32T_IWDG_TIMEOUT
extern "C" IWDG_HandleTypeDef hiwdg;
#endif	// STM32T_IWDG_TIMEOUT



class GL865 : public STM32T::GSM<130, 55>
{
	static constexpr uint32_t MAX_DNS_TIME = 20'000, DEFAULT_FTP_TIMEOUT = 500'000;
	
	STM32T::IO m_pwr;
	const STM32T::IO c_pwrMon;
	uint32_t m_lastPowerOff = 0, m_ftpTimeout = DEFAULT_FTP_TIMEOUT;
	
	static bool IsIPAddress(strv host)
	{
		strv tokens[4];
		if (4 != host.tokenize("."sv, tokens, std::size(tokens), false, false))
			return false;
		
		for (const auto& octet : tokens)
		{
			uint8_t x;
			if (octet.to_num(x) == 0)
				return false;
		}
		
		return true;
	}
	
	int32_t ReceiveUART(char *buffer, uint16_t len, const uint32_t timeout, const uint32_t idle_timeout) override
	{
		const uint32_t start = HAL_GetTick();
		
		__HAL_UART_CLEAR_OREFLAG(p_huart);
		HAL_StatusTypeDef stat = HAL_TIMEOUT;
		
		#ifdef STM32T_IWDG_TIMEOUT
		while (1)
		{
			HAL_IWDG_Refresh(&hiwdg);
			
			if (stat == HAL_OK)
				goto ok;
			else if (stat != HAL_TIMEOUT)
				break;
			
			const uint32_t t = std::min(STM32T::Time::Remaining_Tick(start, timeout), (STM32T_IWDG_TIMEOUT));
			if (!t)
				break;
			
			stat = HAL_UART_Receive(p_huart, (uint8_t *)buffer, 1, t);
		}
		#else
		stat = HAL_UART_Receive(p_huart, (uint8_t *)buffer, 1, timeout);
		if (stat == HAL_OK)
			goto ok;
		#endif	// STM32T_IWDG_TIMEOUT
		
		return stat == HAL_TIMEOUT ? TIMEOUT : UART_ERR;
		
	ok:
		const uint16_t orig_len = len;
		
		for (uint16_t i = 1; i < len; i++)
		{
			if (HAL_GetTick() - start > timeout)
				return i;
			
			stat = HAL_UART_Receive(p_huart, (uint8_t *)&buffer[i], 1, idle_timeout);
			if (stat != HAL_OK)
				return i;
		}
		
		return orig_len;
	}
	
	ErrorCode Setup(const uint32_t timeout_ms = 1000)
	{
		static constexpr strv CMD = "E0;&K0;&P;+IPR=115200;#SLED=2;"
			"+CGDCONT=1,\"IP\",\"mcinet\";"			// ",\"0.0.0.0\";"
			"+CGDCONT=2,\"IP\",\"mtnirancell\";"	// ",\"0.0.0.0\";"
			"+CGDCONT=3,\"IP\",\"rightel\";"		// ",\"0.0.0.0\";"
			"#SCFGEXT=1,0,1,0,0,1;"					// <conneId>,<srMode>,<recvDataMode>,<keepalive>[,<ListenAutoRsp>[,<sendDataMode>]]
			"#SCFGEXT=2,0,1,0,0,1;"
			"+CMEE=1;+CMGF=1;+CSCS=\"UCS2\";+CSMP=49,167,0,8;#DIALMODE=1;"
			"+CSAS;#SLEDSAV;&W"sv;
		
		// The first time might fail due to echo still being enabled, but the next tries should succeed.
		return SingleToken<CMD.size() + 9>(timeout_ms, CommandType::Execute, CMD);
	}
	
	ErrorCode ConfigSocket(const uint8_t conn_id, const uint8_t cid, const uint32_t conn_to)
	{
		if (conn_id < 1 || conn_id > 6 || cid > MAX_CID || conn_to < 1000 || conn_to > 120000)
			return INVALID;
		
		// Old: x,x,1500,600,50,0
		return ReceiveOK(DEFAUL_RECEIVE_TIMEOUT, CommandType::Write, "#SCFG"sv, "%hhu,%hhu,128,600,%hu,1", conn_id, cid, STM32T::ceil(conn_to, 100u));
	}
	
public:
	static constexpr uint8_t MAX_CID = 5, MIN_CONN_ID = 1 , MAX_CONN_ID = 6;
	
	/**
	* @param pwr_mon - Must be high when the PWRMON pin is high.
	*/
	GL865(UART_HandleTypeDef* huart, STM32T::IO pwr, STM32T::IO pwr_mon) : GSM(huart), m_pwr(pwr), c_pwrMon(pwr_mon) {}
	
	int32_t NetworkCheck()
	{
		return ResponseToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Read, "+CGREG", strv(), [&](strv token) -> ErrorCode
		{
			uint8_t n, stat;
			if (sscanf(token.data(), "%hhu,%hhu", &n, &stat) == 2)
				return ErrorCode(stat);
			
			return ERR;
		});
	}
	
	ErrorCode ClockRead(DateTime& dt)
	{
		return ResponseToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Read, "#CCLK"sv, strv(), [&](const strv token)
		{
			if (auto opt = DateTime::Parse(token); opt)
			{
				dt = opt.value();
				return OK;
			}
			
			return WRONG_FORMAT;
		});
	}
	
	ErrorCode NTP(const strv host, const uint16_t port, DateTime& dt, const bool update_module_clock = false, const uint16_t timeout_s = 10)
	{
		return ResponseToken(timeout_s * 1000 + DEFAUL_RECEIVE_TIMEOUT, CommandType::Write, "#NTP"sv, [&](const strv token)
		{
			if (auto opt = DateTime::ParseNTP(token); opt)
			{
				dt = opt.value();
				return OK;
			}
			
			addURC(token);
			
			return WRONG_FORMAT;
		}, true, "\"%.*s\",%hu,%hhu,%hu", host.length(), host.data(), port, update_module_clock, timeout_s);
	}
	
	/**
	* @param buf - Will be null-terminated.
	* @param max_len - Must be at most 256
	*/
	int32_t USSD(strv ussd, wchar_t *buf, uint16_t max_len, const uint32_t resp_timeout)
	{
		static_assert(sizeof(wchar_t) == 2);
		
		if (!buf || max_len > 256 || ussd.size() > DEFAULT_ARG_LEN - 4)
			return INVALID;
		
		// \r\n+CUSD: x,"str",yyy\r\n
		return DelayedResponseToken<DEFAULT_ARG_LEN, 256 * 4 + 20>(resp_timeout, CommandType::Write, "+CUSD"sv, [buf, max_len](strv token)
		{
			size_t start, end;
			if (0 != sscanf(token.data(), "%*1hhu,\"%zn%*[1234567890ABCDEF]%zn\",%*3hhu", &start, &end))
				return WRONG_FORMAT;
			
			strv data = token.substr(start, end - start);
			
			if (data.size() % 4)
				return WRONG_FORMAT;
			
			uint16_t j = 0;
			for (size_t i = 0; i < data.size() && j < max_len - 1; i += 4, j++)
			{
				const auto opt = STM32T::C2H<wchar_t>(data.data() + i);
				if (!opt)
					return WRONG_FORMAT;
				
				buf[j] = *opt;
			}
			
			buf[j] = 0;
			
			return ErrorCode(j);
		}, "1,\"%.*s\"", ussd.size(), ussd.data());
	}
	
	ErrorCode SMSend(strv number, STM32T::span<const wstrv> msgs, const uint32_t timeout = 60'000)
	{
		static_assert(sizeof(wstrv::value_type) == sizeof(uint16_t));
		
		using STM32T::Log::LOG_D;
		using STM32T::Log::LOG_W;
		
		LOG_D<LG>("Sending SM to %.*s...", number.size(), number.data());
		
		auto number2 = std::make_unique<char[]>(number.size() * 4);
		for (size_t i = 0; i < number.size(); i++)
			STM32T::H2C<uint16_t>(number[i], number2.get() + i * 4);
		
		const uint32_t start = HAL_GetTick();
		ErrorCode code = WaitForReady(1000, CommandType::Write, "+CMGS"sv, {number2.get(), number.size() * 4});
		if (code != OK)
		{
			SendUART(ESC);
			return code;
		}
		
		for (auto msg : msgs)
		{
			for (auto ch : msg)
			{
				SendUART(STM32T::H2C(ch >> 12));
				SendUART(STM32T::H2C(ch >> 8));
				SendUART(STM32T::H2C(ch >> 4));
				SendUART(STM32T::H2C(ch));
			}
		}
		
		m_noSendWait = true;
		
		// \r\n+CMGS: 255\r\n\r\nOK\r\n
		uint8_t n;
		ErrorCode res = ResponseToken(STM32T::Time::Remaining_Tick(start, timeout), CommandType::Bare, "+CMGS"sv, CTRL_Z, [&n](strv token) -> ErrorCode
		{
			return sscanf(token.data(), "%3hhu", &n) == 1 ? OK : WRONG_FORMAT;
		});
		
		if (res == OK)
			LOG_D<LG>("SM sent successfully (%hhu).", n);
		else
			LOG_W<LG>("SM could not be sent (%hhu)!", res);
		
		return res;
	}
	
	ErrorCode Call(strv number, const uint32_t timeout = 30'000)
	{
		STM32T::Log::LOG_D<LG>("Calling %.*s...", number.size(), number.data());
		return SingleToken(timeout, CommandType::Execute, "D"sv, {{"OK"sv, OK}, {"NO CARRIER"sv, FAIL}}, false, "%.*s;", number.size(), number.data());
	}
	
	ErrorCode HangUp(const uint32_t timeout = 30'000)
	{
		return SingleToken(timeout, CommandType::Execute, "H"sv);
	}
	
	/**
	* @param rssi - Signal strength in dbm. Not available if positive or zero.
	* @param ber - Bit error rate (best case) in thousandths. Not available if negative.
	*/
	ErrorCode SignalQuality(int8_t& rssi, int16_t& ber)
	{
		// \r\n+CSQ: 99,99\r\n + \r\nOK\r\n
		return ResponseToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Execute, "+CSQ"sv, {}, [&rssi, &ber](strv token) -> ErrorCode
		{
			uint8_t t1, t2;
			if (2 != sscanf(token.data(), "%2hhu,%2hhu", &t1, &t2) || (t1 > 31 && t1 != 99) || (t2 > 7 && t2 != 99))
				return WRONG_FORMAT;
			
			if (t1 <= 31)
				rssi = -113 + t1 * 2;
			else
				rssi = 0;
			
			if (t2 <= 7)
			{
				ber = 1;
				for (uint8_t i = 0; i < t2; i++)
					ber *= 2;
				
				if (t2 == 0)
					ber = 0;
			}
			else
				ber = -1;
			
			return OK;
		});
	}
	
	ErrorCode ContextActivate(const uint8_t cid, const bool enable = true, const uint32_t timeout_ms = 150'000)
	{
		if (cid > MAX_CID)
			return INVALID;
		
		// \r\n#SGACT: xxx.xxx.xxx.xxx\r\n + \r\nOK\r\n
		return ResponseToken(timeout_ms, CommandType::Write, "#SGACT"sv, [](strv token) { return IsIPAddress(token) ? OK : WRONG_FORMAT; }, true, "%hhu,%hhu",
			cid, enable);
	}
	
	int32_t ContextStatus(const uint8_t cid)
	{
		if (cid < 1 || cid > MAX_CID)
			return INVALID;
		
		// \r\n + #SGACT: x,y\r\n (x5) + \r\nOK\r\n
		return Tokens3<2 + 13 * 5 + 6>(DEFAUL_RECEIVE_TIMEOUT, CommandType::Read, "#SGACT"sv, {}, [cid](std::vector<strv>& tokens) -> ErrorCode
		{
			for (auto& line : tokens)
			{
				uint8_t cid_r, stat_r;
				if (2 != std::sscanf(line.data(), "#SGACT: %1hhu,%1hhu", &cid_r, &stat_r))
					continue;
				
				if (cid_r == cid)
					return (ErrorCode)stat_r;
			}
			
			return UNKNOWN;
		});
	}
	
	ErrorCode SocketDial(const uint8_t conn_id, const uint8_t cid, strv host, uint16_t port, uint16_t udp_port = 0, const uint16_t timeout_ms = 10000)
	{
		if (host.size() > 256 - 14)
			return BIG_PARAM;
		
		ErrorCode code = ConfigSocket(conn_id, cid, timeout_ms);
		if (code != OK)
			return code;
		
		m_noSendWait = true;
		
		return ReceiveOK<256>((IsIPAddress(host) ? 0 : MAX_DNS_TIME) + timeout_ms, CommandType::Write, "#SD"sv, "%hhu,%hhu,%hu,\"%.*s\",0,%hu,1",
			conn_id, bool(udp_port), port, host.length(), host.data(), udp_port);
	}
	
	ErrorCode SocketDial(const uint8_t conn_id, const uint8_t cid, const uint8_t (&host)[4], uint16_t port, uint16_t udp_port = 0, const uint16_t timeout_ms = 10000)
	{
		ErrorCode code = ConfigSocket(conn_id, cid, timeout_ms);
		if (code != OK)
			return code;
		
		m_noSendWait = true;
		
		return ReceiveOK(timeout_ms, CommandType::Write, "#SD"sv, "%hhu,%hhu,%hu,\"%hhu.%hhu.%hhu.%hhu\",0,%hu,1",
			conn_id, bool(udp_port), port, host[0], host[1], host[2], host[3], udp_port);
	}
	
	ErrorCode SocketClose(const uint8_t conn_id)
	{
		if (conn_id < 1 || conn_id > 6)
			return INVALID;
		
		return ReceiveOK(3000, CommandType::Write, "#SH"sv, "%hhu", conn_id);
	}
	
	int32_t SocketStatus(const uint8_t conn_id)
	{
		if (conn_id < 1 || conn_id > 6)
			return INVALID;
		
		// \r\n#SS: 0,0,xxx.xxx.xxx.xxx,ppppp,yyy.yyy.yyy.yyy,ppppp\r\n + \r\nOK\r\n (62)
		return ResponseToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Write, "#SS"sv, [conn_id](strv token) -> ErrorCode
		{
			uint8_t conn_id_r, conn_stat;
			if (sscanf(token.data(), "%hhu,%hhu", &conn_id_r, &conn_stat) == 2 && conn_id_r == conn_id)
				return ErrorCode(conn_stat);
			
			return WRONG_FORMAT;
		}, true, "%hhu", conn_id);
	}
	
	ErrorCode SocketSend(const uint8_t conn_id, STM32T::span<const strv> data)
	{
		size_t total_len = 0;
		for (auto chunk : data)
			total_len += chunk.size();
		
		if (total_len > 1500 || conn_id < MIN_CONN_ID || conn_id > MAX_CONN_ID)
			return INVALID;
		
		ErrorCode code = WaitForReady(DEFAUL_RECEIVE_TIMEOUT, CommandType::Write, "#SSEND"sv, "%hhu", conn_id);
		if (code != OK)
		{
			SendUART(ESC);
			return code;
		}
		
		for (auto chunk : data)
		{
			for (char ch : chunk)
			{
				SendUART(STM32T::H2C(ch >> 4));
				SendUART(STM32T::H2C(ch));
			}
		}
		
		m_noSendWait = true;
		
		return SingleToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Bare, {}, CTRL_Z);
	}
	
	int32_t SocketRead(const uint8_t conn_id, char *const data, const uint16_t len, uint32_t timeout_ms = DEFAUL_RECEIVE_TIMEOUT)
	{
		if (timeout_ms < DEFAUL_RECEIVE_TIMEOUT)
			return INVALID;
		
		if (conn_id < MIN_CONN_ID || conn_id > MAX_CONN_ID || len > 1500)
			return INVALID;
		
		// #SRECV: x,yyyy\r\n + \r\ndata\r\n + \r\nOK\r\n
		return ResponseToken<DEFAULT_ARG_LEN, 16 + 4 + 1500 * 2 + 6>(3, timeout_ms, CommandType::Write, "#SRECV"sv, 2,
		[this, conn_id, len, data](std::vector<strv>& tokens) -> ErrorCode
		{
			uint8_t recv_conn_id;
			uint16_t recv_len;
			
			if (2 != sscanf(tokens[0].data(), "%1hhu,%4hu", &recv_conn_id, &recv_len)
				|| recv_len > len || tokens[1].size() != recv_len * 2 || recv_conn_id != conn_id)
				return WRONG_FORMAT;
			
			for (uint16_t i = 0, j = 0; i < recv_len; i++, j += 2)
			{
				const auto opt = STM32T::C2H<uint8_t>(tokens[1].data() + j);
				if (!opt)
					return WRONG_FORMAT;
				
				if (data)
					data[i] = *opt;
			}
			
			return ErrorCode(recv_len);
		}, "%hhu,%hu", conn_id, len);
	}
	
	ErrorCode FTPTimeout(uint32_t ftp_to)
	{
		if (ftp_to < 10'000 || ftp_to > 500'000)
			return INVALID;
		
		ftp_to = STM32T::next_multiple(ftp_to, 100u);
		
		ErrorCode code = ReceiveOK(DEFAUL_RECEIVE_TIMEOUT, CommandType::Write, "#FTPTO"sv, "%hu", ftp_to / 100u);
		
		if (code == OK)
			m_ftpTimeout = ftp_to;
		
		return code;
	}
	
	/**
	* @note Requires PDP context #1 or GSM context.
	*/
	ErrorCode FTPOpen(strv host, uint16_t port, strv user, strv pass, const bool passive = true)
	{
		if (host.size() + user.size() + pass.size() > 256 - 6)
			return BIG_PARAM;
		
		return ReceiveOK<256>(100'000, CommandType::Write, "#FTPOPEN"sv, "%.*s:%hu,%.*s,%.*s,%hhu",
			host.length(), host.data(), port, user.length(), user.data(), pass.length(), pass.data(), passive);
	}
	
	ErrorCode FTPClose()
	{
		return SingleToken(m_ftpTimeout, CommandType::Execute, "#FTPCLOSE"sv);
	}
	
	int32_t FTPGetO(strv file, const std::function<void (strv chunk, size_t handled_before)>& chunk_handler, const uint32_t dl_to)
	{
		return ReceiveOnline(m_ftpTimeout, dl_to, CommandType::Write, "#FTPGET"sv, [&](strv chunk, size_t handled) -> ErrorCode
		{
			if (chunk_handler)
				chunk_handler(chunk, handled);
			
			return OK;
		}, "\"%.*s\"", file.size(), file.data());
		
		ErrorCode code = SingleToken(m_ftpTimeout, CommandType::Write, "#FTPGET"sv, {{"CONNECT"sv, OK}, {"NO CARRIER"sv, FAIL}}, false, "\"%.*s\"",
			file.size(), file.data());
		
		return code;
	}
	
	ErrorCode FTPType(bool ascii)
	{
		return ReceiveOK(m_ftpTimeout, CommandType::Write, "#FTPTYPE"sv, "%hhu", ascii);
	}
	
	ErrorCode FTPCWD(strv dir)
	{
		return SingleToken(m_ftpTimeout, CommandType::Write, "#FTPCWD"sv, dir);
	}
	
	int32_t FTPListO(char *buf, size_t len, strv name = strv(), const uint32_t list_to = 15'000)
	{
		return ReceiveOnline(m_ftpTimeout, list_to, name.size() ? CommandType::Write : CommandType::Execute, "#FTPLIST"sv,
			[&buf, &len](strv chunk, size_t handled) -> ErrorCode
			{
				if (!len)
					return BUF_FULL;
				
				const size_t copy_size = std::min(len, chunk.size());
				memcpy(buf, chunk.data(), copy_size);
				buf += copy_size;
				len -= copy_size;
				
				if (copy_size < chunk.size())
					return BUF_FULL;
				
				return OK;
			}, name);
	}
	
	ErrorCode FTPFileSize(strv file, size_t& size);
	ErrorCode FTPPut(strv file);
	ErrorCode FTPAppend(strv data, bool final = false);
	ErrorCode FTPGet(strv file);
	ErrorCode FTPRecv(char* buf, uint16_t& len);
	
	ErrorCode SIMCheck(uint8_t& status);
	
	void Init(const bool enable_urc, const bool initial = false)
	{
		STM32T::Time::WaitAfter_Tick(m_lastPowerOff, 1500);
		m_pwr.Set();
		
		HAL_UART_Init(p_huart);		// fixme: necessary?
		EnableURC(enable_urc);
		
		if (initial)
		{
			HAL_Delay(5000 + 1000);
			
			STM32T::Retry(3, 1000, std::bind(&GL865::Setup, this, 1000), OK, Error_Handler);
		}
		
		HAL_Delay(1000);
		if (c_pwrMon.Read())
		{
			HAL_Delay(300);
			if (AT(1000) == OK)
				return;
		}
		
		if (!c_pwrMon.Wait(1, 5000))
		{
			// todo: reset module
			PowerOff(true);
			Error_Handler();
		}
		
		HAL_Delay(1000 + 300);
		if (AT(1000) != ErrorCode::OK)
		{
			PowerOff(true);
			Error_Handler();
		}
	}
	
	void PowerOff(const bool fast = false)
	{
		EnableURC(false);
		
		if (!fast && c_pwrMon.Read())
		{
			Command(0, CommandType::Execute, "#SYSHALT"sv, {}, nullptr, 0);
			c_pwrMon.Wait(0, 15000);
		}
		
		m_pwr.Reset();
		m_lastPowerOff = HAL_GetTick();
		m_ftpTimeout = DEFAULT_FTP_TIMEOUT;
	}
};

#pragma once

#include "./GSM.hpp"
#include "../IO.hpp"
#include "../Log.hpp"



using STM32T::strv;
using STM32T::wstrv;
using std::operator"" sv;



class GL865 : public STM32T::GSM<100, 16, 50, 4>
{
	static constexpr STM32T::Log::Logger lg = STM32T::Log::g_defaultLogger.Clone(STM32T::Log::Level::Debug, "GSM"sv);
	
	static constexpr uint32_t MAX_DNS_TIME = 20'000;
	
	STM32T::IO m_pwr;
	const STM32T::IO c_pwrMon;
	uint32_t m_lastPowerOff = 0;
	
	static bool IsIPAddress(strv host)
	{
		strv tokens[4];
		if (4 != host.tokenize("."sv, tokens, std::size(tokens), false, false))
			return false;
		
		for (const auto& octet : tokens)
		{
			uint8_t x;
			if (octet.ExtractInteger(x) == 0)
				return false;
		}
		
		return true;
	}
	
	void addURC(const strv token) override {}
	
	int16_t ReceiveUART(char *buffer, uint16_t len, const uint32_t timeout, const uint32_t idle_timeout) override
	{
		const uint32_t start = HAL_GetTick();
		const uint16_t orig_len = len;
		
		__HAL_UART_CLEAR_OREFLAG(p_huart);
		HAL_StatusTypeDef stat = HAL_UART_Receive(p_huart, (uint8_t *)buffer, 1, timeout);
		if (stat != HAL_OK)
			return stat == HAL_TIMEOUT ? TIMEOUT : UART_ERR;
		
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
		m_noSendWait = true;
		
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
		return SingleToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Write, "#SCFG"sv, "OK"sv, false, "%hhu,%hhu,128,600,%hu,1", conn_id, cid, STM32T::ceil(conn_to, 100u));
	}
	
public:
	static constexpr uint8_t MAX_CID = 5, MIN_CONN_ID = 1 , MAX_CONN_ID = 6;
	
	/**
	* @param pwr_mon - Must be high when the PWRMON pin is high.
	*/
	GL865(UART_HandleTypeDef* huart, STM32T::IO pwr, STM32T::IO pwr_mon) : GSM(huart), m_pwr(pwr), c_pwrMon(pwr_mon) {}
	
	ErrorCode NetworkCheck(uint8_t& stat);
	bool NetworkWait(const uint32_t timeout);
	
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
	int16_t USSD(strv ussd, wchar_t *buf, uint16_t max_len, const uint32_t resp_timeout)
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
				const uint8_t _3 = STM32T::C2H(data[i]), _2 = STM32T::C2H(data[i + 1]), _1 = STM32T::C2H(data[i + 2]), _0 = STM32T::C2H(data[i + 3]);
				if (_3 == 0xFF || _2 == 0xFF || _1 == 0xFF || _0 == 0xFF)
					return WRONG_FORMAT;
				
				buf[j] = (_3 << 12) | (_2 << 8) | (_1 << 4) | _0;
			}
			
			buf[j] = 0;
			
			return ErrorCode(j);
		}, "1,\"%.*s\"", ussd.size(), ussd.data());
	}
	
	ErrorCode SMSend(strv number, const wstrv *msgs, const size_t msg_count, const uint32_t timeout = 60'000)
	{
		static_assert(sizeof(wstrv::value_type) == sizeof(uint16_t));
		
		LOG_D<lg>("Sending SM to %.*s...\n", number.size(), number.data());
		
		auto number2 = std::make_unique<char[]>(number.size() * 4);
		for (size_t i = 0; i < number.size(); i++)
		{
			char *buf = number2.get() + i * 4;
			
			using namespace STM32T;
			buf[0] = H2C(number[i] >> 12);
			buf[1] = H2C(number[i] >> 8);
			buf[2] = H2C(number[i] >> 4);
			buf[3] = H2C(number[i]);
		}
		
		const uint32_t start = HAL_GetTick();
		ErrorCode code = WaitForReady(1000, CommandType::Write, "+CMGS"sv, {number2.get(), number.size() * 4});
		if (code != OK)
		{
			SendUART(ESC);
			return code;
		}
		
		for (size_t i = 0; i < msg_count; i++)
		{
			for (auto& ch : msgs[i])
			{
				using namespace STM32T;
				
				char ucs2[sizeof(ch) * 2] = {H2C(ch >> 12), H2C(ch >> 8), H2C(ch >> 4), H2C(ch)};
				SendUART(strv(ucs2, std::size(ucs2)));
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
			LOG_D<lg>("SM sent successfully (%hhu).\n", n);
		else
			LOG_W<lg>("SM could not be sent (%hhu)!\n", res);
		
		return res;
	}
	
	ErrorCode SMSend(strv number, std::initializer_list<wstrv> msgs, const uint32_t timeout = 60'000)
	{
		return SMSend(number, msgs.begin(), msgs.size(), timeout);
	}
	
	ErrorCode SMSend(strv number, wstrv msg, const uint32_t timeout = 60'000)
	{
		return SMSend(number, &msg, 1, timeout);
	}
	
	ErrorCode Call(strv number, const uint32_t timeout = 30'000)
	{
		LOG_D<lg>("Calling %.*s...\n", number.size(), number.data());
		return SingleToken(timeout, CommandType::Execute, "D"sv, "OK"sv, false, "%.*s;", number.size(), number.data());
	}
	
	ErrorCode HangUp(const uint32_t timeout = 30'000)
	{
		return SingleToken(timeout, CommandType::Execute, "H"sv);
	}
	
	/**
	* @param rssi - Signal strength in dbm. Not vailable if positive or zero.
	* @param ber - Bit error rate (best case) in thousandths. Not vailable if negative.
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
		return ResponseToken<27 + 6>(timeout_ms, CommandType::Write, "#SGACT"sv, [](strv token) { return OK; }, true, "%hhu,%hhu", cid, enable);
	}
	
	int16_t ContextStatus(const uint8_t cid)
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
		if (host.size() > 256 - 22)
			return INVALID;
		
		ErrorCode code = ConfigSocket(conn_id, cid, timeout_ms);
		if (code != OK)
			return code;
		
		m_noSendWait = true;
		
		// todo: separate OpenSocket() for IP address
		return SingleToken<256>((IsIPAddress(host) ? 0 : MAX_DNS_TIME) + timeout_ms,
			CommandType::Write, "#SD"sv, "OK"sv, false, "%hhu,%hhu,%hu,\"%.*s\",0,%hu,1", conn_id, bool(udp_port), port, host.length(), host.data(), udp_port);
	}
	
	ErrorCode SocketClose(const uint8_t conn_id)
	{
		if (conn_id < 1 || conn_id > 6)
			return INVALID;
		
		return SingleToken(3000, CommandType::Write, "#SH"sv, "OK"sv, false, "%hhu", conn_id);
	}
	
	int16_t SocketStatus(const uint8_t conn_id)
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
	
	ErrorCode SocketSend(const uint8_t conn_id, strv data)
	{
		if (data.size() > 1500 || conn_id < MIN_CONN_ID || conn_id > MAX_CONN_ID)
			return INVALID;
		
		ErrorCode code = WaitForReady(DEFAUL_RECEIVE_TIMEOUT, CommandType::Write, "#SSEND"sv, "%hhu", conn_id);
		if (code != OK)
		{
			SendUART(ESC);
			return code;
		}
		
		for (char ch : data)
		{
			const char hi = STM32T::H2C(ch >> 4), lo = STM32T::H2C(ch & 0x0F);
			
			SendUART({&hi, 1});
			SendUART({&lo, 1});
		}
		
		m_noSendWait = true;
		
		return SingleToken(DEFAUL_RECEIVE_TIMEOUT, CommandType::Bare, {}, CTRL_Z);
	}
	
	int16_t SocketRead(const uint8_t conn_id, char *const data, const uint16_t len, uint32_t timeout_ms = DEFAUL_RECEIVE_TIMEOUT)
	{
		if (timeout_ms < DEFAUL_RECEIVE_TIMEOUT)
			return INVALID;
		
		if (conn_id < MIN_CONN_ID || conn_id > MAX_CONN_ID || len > 1500)
			return INVALID;
		
		timeout_ms += SEND_GUARD_TIME;
		const uint32_t start = HAL_GetTick();
		
		ErrorCode code = NoToken<2>(DEFAUL_RECEIVE_TIMEOUT, CommandType::Write, "#SRECV"sv, [](strv data) -> ErrorCode
		{
			return data == "\r\n"sv ? OK : ERR;
		}, "%hhu,%hu", conn_id, len);
		
		if (code != OK)
			return code;
		
		m_noSendWait = true;
		
		// #SRECV: x,yyyy\r\n + \r\ndata\r\n + \r\nOK\r\n
		return ResponseToken<16 + 4 + 1500 * 2 + 6>(3, STM32T::Time::Remaining_Tick(start, timeout_ms), CommandType::Bare, "#SRECV"sv, {}, 2,
		[this, conn_id, len, data](std::vector<strv>& tokens) -> ErrorCode
		{
			uint8_t recv_conn_id;
			uint16_t recv_len;
			
			if (2 != sscanf(tokens[0].data(), "%1hhu,%4hu", &recv_conn_id, &recv_len) || recv_len > len || tokens[1].size() != recv_len * 2 || recv_conn_id != conn_id)
				return WRONG_FORMAT;
			
			for (uint16_t i = 0, j = 0; i < tokens[1].size(); i++, j += 2)
			{
				const uint8_t hi = STM32T::C2H(tokens[1][j]), lo = STM32T::C2H(tokens[1][j + 1]);
				if (hi == 0xFF || lo == 0xFF)
					return WRONG_FORMAT;
				
				if (data)
					data[i] = (hi << 4) | lo;
			}
			
			return ErrorCode(recv_len);
		});
	}
	
	ErrorCode FTPOpen(strv address, strv user, strv pass);
	ErrorCode FTPTimeout(uint32_t timeout);
	ErrorCode FTPCWD(strv dir);
	ErrorCode FTPType(bool ascii);
	ErrorCode FTPFileSize(strv file, size_t& size);
	ErrorCode FTPList(strv file);
	ErrorCode FTPPut(strv file);
	ErrorCode FTPAppend(strv data, bool final = false);
	ErrorCode FTPGet(strv file);
	ErrorCode FTPRecv(char* buf, uint16_t& len);
	ErrorCode FTPClose();
	
	ErrorCode SIMCheck(uint8_t& status);
	
	void Init(const bool initial = false)
	{
		STM32T::Time::WaitAfter_Tick(m_lastPowerOff, 1500);
		m_pwr.Set();
		
		HAL_UART_Init(p_huart);		// fixme: necessary?
		
		if (initial)
		{
			HAL_Delay(5000 + 1000);
			
			using namespace std::placeholders;
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
		if (!fast && c_pwrMon.Read())
		{
			uint16_t x;
			Command(0, CommandType::Execute, "#SYSHALT"sv, {}, nullptr, x);
			c_pwrMon.Wait(0, 15000);
		}
		
		m_pwr.Reset();
		m_lastPowerOff = HAL_GetTick();
	}
};

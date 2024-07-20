#include "SIM800x.hpp"



#define Mode1_Timer_On      64    //default: 64
#define Mode1_Timer_Off     800   //default: 800

#define Mode2_Timer_On      64    //default: 64
#define Mode2_Timer_Off     3000   //default: 3000

#define Mode3_Timer_On      64    //default: 64
#define Mode3_Timer_Off     300   //default: 300




bool CompareAndRemove(std::string_view& token, const std::string_view& remove)
{
	if (token.compare(0, remove.size(), remove) == 0)
	{
		token.remove_prefix(remove.size());
		return true;
	}

	return false;
}



GSM::ErrorCode SIM800x::Standard(const vect<strv>& tokens)
{
	for (size_t i = 0; i < tokens.size(); i++)
	{
		if (tokens[i] == "OK"sv)
		{
			for (i = i + 1; i < tokens.size(); i++)
				addURC(tokens[i]);
			
			return ErrorCode::OK;
		}
		
		if (tokens[i] == "ERROR"sv)
		{
			for (i = i + 1; i < tokens.size(); i++)
				addURC(tokens[i]);
			
			return ErrorCode::ERR;
		}
		
		addURC(tokens[i]);
	}
	
	return ErrorCode::UNKNOWN;
}

GSM::ErrorCode SIM800x::Command(vect<strv>& tokens, char* buffer, uint16_t len, const uint32_t timeout, const CommandType type, const strv& cmd, const strv& args, const bool allowSingleEnded)
{
	//Send(type, cmd, args);
	if (type != CommandType::Bare)
	{
		HAL_UART_Transmit(phuart, (uint8_t*)"AT", 2, HAL_UART_TIMEOUT_VALUE);
		HAL_UART_Transmit(phuart, (uint8_t*)cmd.data(), cmd.length(), HAL_UART_TIMEOUT_VALUE);
	}
	
	if (type == CommandType::Write || type == CommandType::Test)
		HAL_UART_Transmit(phuart, (uint8_t*)"=", 1, HAL_UART_TIMEOUT_VALUE);
	
	if (type == CommandType::Write || type == CommandType::Execute || type == CommandType::Bare)
		HAL_UART_Transmit(phuart, (uint8_t*)args.data(), args.length(), HAL_UART_TIMEOUT_VALUE);
	else
		HAL_UART_Transmit(phuart, (uint8_t*)"?", 1, HAL_UART_TIMEOUT_VALUE);
	
	HAL_UART_Transmit(phuart, (uint8_t*)"\r", 1, HAL_UART_TIMEOUT_VALUE);
	
	
	
	ErrorCode stat = ReceiveUART(buffer, len, timeout);
	if (stat != ErrorCode::OK)
		return stat;
	
	COM::Tokenize(strv(buffer, len), "\r\n"sv, tokens, !allowSingleEnded);
	
	return ErrorCode::OK;
}



template <size_t LEN>
GSM::ErrorCode SIM800x::Tokens(const uint32_t timeout, const CommandType type, const strv& cmd, const strv& args, const func<ErrorCode (vect<strv>&)>& op, const bool allowSingleEnded)
{
	char buffer[LEN];
	vect<strv> tokens;
	ErrorCode code = Command(tokens, buffer, sizeof(buffer), timeout, type, cmd, args, allowSingleEnded);
	if (code != ErrorCode::OK)
		return code;
	
	return op ? op(tokens) : Standard(tokens);
}

template <size_t ARG_LEN, size_t LEN>
GSM::ErrorCode SIM800x::Tokens(const uint32_t timeout, const CommandType type, const strv& cmd, const func<ErrorCode (vect<strv>&)>& op, const bool allowSingleEnded, const char* const fmt, ...)
{
	if (!fmt)
		return ErrorCode::INVALID_PARAM;
	
	char args[ARG_LEN];
	
	va_list print_args;
	va_start(print_args, fmt);
	
	int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
	if (argsLen < 0)
		return ErrorCode::UNKNOWN;
	
	va_end(print_args);
	
	if (argsLen > sizeof(args))	//did not fit inside {args}
		Error_Handler();
	
	return Tokens<LEN>(timeout, type, cmd, strv(args, argsLen), op, allowSingleEnded);
}



template <size_t LEN>
GSM::ErrorCode SIM800x::FirstLastToken(const size_t expectedTokens, const uint32_t timeout, const CommandType type, const strv& cmd, const strv& args, const func<ErrorCode (vect<strv>&)>& op)
{
	return Tokens<LEN>(timeout, type, cmd, args, [&](vect<strv>& tokens) -> ErrorCode
	{
		if (tokens.size() != expectedTokens)
			return Standard(tokens);
		
		char* cmdr = new char[cmd.size() + 2];
		memcpy(cmdr, cmd.data(), cmd.size());
		cmdr[cmd.size()] = ':';
		cmdr[cmd.size() + 1] = ' ';
		
		bool cmp = CompareAndRemove(tokens[0], strv(cmdr, cmd.size() + 2));
		
		delete[] cmdr;
		
		if (!cmp || tokens.back() != "OK"sv)
		{
			for (const auto& token : tokens)
				addURC(token);
			
			return ErrorCode::UNKNOWN;
		}
		
		tokens.pop_back();
		
		return op ? op(tokens) : Standard(tokens);
	});
}

template <size_t ARG_LEN, size_t LEN>
GSM::ErrorCode SIM800x::FirstLastToken(const size_t expectedTokens, const uint32_t timeout, const CommandType type, const strv& cmd, const func<ErrorCode (vect<strv>&)>& op, const char* const fmt, ...)
{
	if (!fmt)
		return ErrorCode::INVALID_PARAM;
	
	char args[ARG_LEN];
	
	va_list print_args;
	va_start(print_args, fmt);
	
	int argsLen = vsnprintf(args, sizeof(args), fmt, print_args);
	if (argsLen < 0)
		return ErrorCode::UNKNOWN;
	
	va_end(print_args);
	
	if (argsLen > sizeof(args))	//did not fit inside {args}
		Error_Handler();
	
	return FirstLastToken<LEN>(expectedTokens, timeout, type, cmd, strv(args, argsLen), op);
}



/*void SIM800x::Power(bool p)
{
	if ((S_Port && Status() == p) || !P_Port)
		return;
	
	HAL_GPIO_WritePin(P_Port, P_Pin, GPIO_PIN_SET);
	HAL_Delay(2000);
	HAL_GPIO_WritePin(P_Port, P_Pin, GPIO_PIN_RESET);
}

bool SIM800x::Status(void)
{
	if (S_Port==0)
		return 0;
	
	return HAL_GPIO_ReadPin(S_Port, S_Pin);
}

uint8_t SIM800x::Net(void)
{
	return 0;
}

uint8_t STATUS(const char *a)
{
	if (strncmp(a, "CONNECT OK", 10) == 0)
		return 36;
	
	if (strncmp(a, "TCP CLOSED", 10) == 0 || strncmp(a, "UDP CLOSED", 10) == 0)
		return 38;
	
	if (strncmp(a, "IP INITIAL", 10) == 0)
		return 30;
	
	if (strncmp(a, "TCP CONNECTING", 14) == 0 || strncmp(a, "UDP CONNECTING", 14) == 0 || strncmp(a, "SERVER LISTENING", 16) == 0 || strncmp(a, "IP PROCESSING", 13) == 0)
		return 35;
	
	if (strncmp(a, "IP STATUS", 10) == 0)
		return 34;
	
	if (strncmp(a, "IP START", 8) == 0)
		return 31;
	
	if (strncmp(a, "IP CONFIG", 9) == 0)
		return 32;
	
	if (strncmp(a, "IP GPRSACT", 10) == 0)
		return 33;
	
	if (strncmp(a, "TCP CLOSING", 11) == 0 || strncmp(a,"UDP CLOSING", 11) == 0)
		return 37;
	
	if (strncmp(a, "PDP DEACT", 9) == 0)
		return 39;
	
	return 25;
}*/






//void SIM800x::handleURCs(const func<void(uint32_t timestamp, strv urc)>& handler)
void SIM800x::handleURCs(void (* const handler)(URC urc))
{
	while (List)
	{
		if (!List->tokenized)
			COM::Tokenize(strv(*List), "\r\n"sv, [&](strv urc) -> void { handler(URC(urc, List->timestamp)); }, true);
		else
			handler(URC(List->data, List->size - 1, List->timestamp));
		
		ListItem* current = List;
		List = List->next;
		delete current;
	}
}



GSM::ErrorCode SIM800x::Setup()
{
	return Tokens(10000, CommandType::Execute, "E0;+IPR=115200;+CLTS=1;+CMGF=1;+CSCS=\"HEX\";+SLEDS=1,50,450;+SLEDS=2,400,3100;+SLEDS=3,100,100;+CREG=2;&W"sv);
}

GSM::ErrorCode SIM800x::AT(const uint32_t timeout)
{
	return Tokens(timeout, CommandType::Execute, strv());
}



GSM::ErrorCode SIM800x::GetSignalQuality(int8_t& rssi, uint8_t& ber, const uint32_t timeout)
{
	return FirstLastToken(2, timeout, CommandType::Execute, "+CSQ"sv, strv(), [&rssi, &ber](vect<strv>& tokens) -> ErrorCode
	{
		if (2 != sscanf(tokens[0].data(), "%2hhd,%1hhu", &rssi, &ber))
			return ErrorCode::WRONG_FORMAT;
		
		switch (rssi)
		{
			case 0:
				rssi = -115;
				break;
			
			case 1:
				rssi = -111;
				break;
			
			default:
				if (rssi >= 2 && rssi <= 31)
					rssi = (rssi - 2) * 2 - 110;
				else if (rssi != 99)
					return ErrorCode::WRONG_FORMAT;
		}
		
		return ErrorCode::OK;
	});
}



GSM::ErrorCode SIM800x::SetClock(const DateTime &dt, const uint32_t timeout)
{
	if (!dt.IsSet())
		return ErrorCode::INVALID_PARAM;
	
	return Tokens(timeout, CommandType::Write, "+CCLK"sv, nullptr, false, "\"%02hhu/%02hhu/%02hhu,%02hhu:%02hhu:%02hhu%+02hhd\"", dt.yy, dt.MM, dt.dd, dt.hh, dt.mm, dt.ss, dt.zz);
}

GSM::ErrorCode SIM800x::GetClock(DateTime &dt, const uint32_t timeout)
{
	//40: \r\n+CCLK: "00/00/00,00:00:00:+00"\r\n\r\nOK\r\n
	return FirstLastToken<40>(2, timeout, CommandType::Read, "+CCLK"sv, strv(), [&dt](vect<strv>& tokens) -> ErrorCode
	{
		return DateTime::Parse(dt, tokens[0]) ? ErrorCode::OK : ErrorCode::WRONG_FORMAT;
	});
}



GSM::ErrorCode SIM800x::SendSMS(const uint32_t number, const char * const data, const uint16_t len, const uint32_t timeout)
{
	static constexpr char ctrl_z = 26, esc = 27;
	
	if (number > 999'999'999 || !data || len > 160)	//empty SMS is okay
		return ErrorCode::INVALID_PARAM;
	
	
	ErrorCode code = Tokens(DEFAUL_RECEIVE_TIMEOUT, CommandType::Write, "+CMGS"sv,
	[&](vect<strv>& tokens) -> ErrorCode { return tokens.size() == 1 && tokens[0] == "> "sv ? ErrorCode::OK : Standard(tokens); }, true, "\"09%09u\"", number);
	
	if (code != ErrorCode::OK)
	{
		HAL_UART_Transmit(phuart, (uint8_t*)&esc, sizeof(esc), HAL_UART_TIMEOUT_VALUE);
		
		return code;
	}
	
	
	
	char args[160 * 2 + 1];
	
	for (uint16_t i = 0; i < len; i++)
	{
		if (2 != snprintf(args + i * 2, sizeof(args) - i * 2, "%02X", data[i]))
			return ErrorCode::UNKNOWN;
	}
	
	args[len * 2] = ctrl_z;
	
	// \r\n+CMGS: 255\r\n\r\nOK\r\n
	return FirstLastToken(2, timeout, CommandType::Bare, "+CMGS"sv, strv(args, len * 2 + 1), [](vect<strv>& tokens) -> ErrorCode
	{
		if (0 != sscanf(tokens[0].data(), "%*3hhu"))
			return ErrorCode::WRONG_FORMAT;
		
		return ErrorCode::OK;
	});
}

GSM::ErrorCode SIM800x::ReadSMS(uint32_t& number, DateTime& dt, char * const data, uint16_t& len, const uint8_t index, const CMGR_Mode mode, const uint32_t timeout)
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

GSM::ErrorCode SIM800x::DeleteSMS(const uint8_t index, const CMGD_DelFlag delFlag, const uint32_t timeout)
{
	if (delFlag > CMGD_DelFlag::All)
		return ErrorCode::INVALID_PARAM;
	
	return Tokens(timeout, CommandType::Write, "+CMGD"sv, nullptr, false, "%hhu,%hhu", index, delFlag);
}



GSM::ErrorCode SIM800x::AnswerCall(const uint32_t timeout)
{
	return Tokens(timeout, CommandType::Execute, "A"sv);
}

GSM::ErrorCode SIM800x::Dial(const uint32_t number, const strv& prefix, const strv& postfix, const uint32_t timeout)
{
	return Tokens(timeout, CommandType::Execute, "D"sv, nullptr, false, "%*s%u%*s", prefix.size(), prefix.data(), number, postfix.size(), postfix.data());
}

GSM::ErrorCode SIM800x::HangUp(const uint32_t timeout)
{
	return Tokens(timeout, CommandType::Execute, "H"sv);
}



/*SIM800x::ErrorCode SIM800x::CGDCONT(uint8_t cid, const char* APN, const char* ip)
{
	uint8_t size = strlen(APN), size2 = strlen(ip);
	if (cid < 1 || cid > 3 || size > 11 || size2 > 15)
		return ErrorCode::INVALID_PARAM;
	
	uint8_t send[20]="AT+CGDCONT=1,\"IP\",\"";
	send[11]=cid+'0';
	HAL_UART_Init(phuart);
	HAL_UART_Transmit(phuart, send, 19, HAL_MAX_DELAY);
	HAL_UART_Transmit(phuart, (uint8_t*)APN, size, HAL_MAX_DELAY);
	HAL_UART_Transmit(phuart, (uint8_t*)"\",\"", 3, HAL_MAX_DELAY);
	HAL_UART_Transmit(phuart, (uint8_t*)ip, size2, HAL_MAX_DELAY);
	HAL_UART_Transmit(phuart, (uint8_t*)"\",0,0\r\n", 7, HAL_MAX_DELAY);
	
	send[0]=re(send+1, 9, 1000);
	if (send[0]!=0)
		return (ErrorCode)send[0];
	
	return standard((char*)send + 3);
}

SIM800x::ErrorCode SIM800x::CGACT(uint8_t state, uint8_t cid)
{
	if (state>1 || cid<1 || cid>3)
		return ErrorCode::INVALID_PARAM;
	
	uint8_t send[15]="AT+CGACT=1,1\r\n";
	send[9]=state+'0';
	send[11]=cid+'0';
	HAL_UART_Init(phuart);
	HAL_UART_Transmit(phuart, send, 14, HAL_MAX_DELAY);
	send[0]=re(send+1, 9, 15000);
	if (send[0]!=0)
		return (ErrorCode)send[0];
	
	return standard((char*)send + 3);
}

SIM800x::ErrorCode SIM800x::CIMI(char* IMSI)
{
	uint8_t rec[29], res;
	HAL_UART_Init(phuart);
	HAL_UART_Transmit(phuart, (uint8_t*)"AT+CIMI\r\n", 9, HAL_MAX_DELAY);
	res=re(rec, 28, 100);
	if (res!=0)
		return (ErrorCode)res;
	
	if (strlen((char*)rec)<25)
		return (ErrorCode)13;
	
	strncpy(IMSI, (char*)rec + 2, 15);
	IMSI[15]=0;
	return standard((char*)rec + 21);
}

SIM800x::ErrorCode SIM800x::CIPSTART(bool mode, uint8_t* ipd, uint16_t port)    //0:TCP   1:UDP    ip_d:IP address or domain name
{
	static const uint8_t tcp[20]="AT+CIPSTART=\"TCP\",\"", udp[20]="AT+CIPSTART=\"UDP\",\"";
	uint8_t p[10]="\",", res, rec[40]={0};
	itoa(p+2, port);
	strcat((char*)p, "\r\n");
	
	HAL_UART_Transmit(phuart, mode ? (uint8_t*)udp : (uint8_t*)tcp, 19, HAL_MAX_DELAY);
	uint16_t len=strlen((char*)ipd);
	HAL_UART_Transmit(phuart, ipd, len, HAL_MAX_DELAY);
	len=strlen((char*)p);
	HAL_UART_Transmit(phuart, p, len, HAL_MAX_DELAY);
	res=re(rec,32,100);
	if (res!=0)
		return (ErrorCode)res;
	
	if (strncmp((char*)rec+2,"OK",2)==0)
	{
		res=re(rec,39,10000);
		if (res!=0)
			return (ErrorCode)(res+3);
		
		if (strncmp((char*)rec+2,"CONNECT OK",10)==0 || strstr((char*)rec,"CONNECT OK"))
			return ErrorCode::OK;
		
		if (strstr((char*)rec+21,"CONNECT FAIL"))
			return (ErrorCode)13;
		
		return (ErrorCode)14;
	}
	if (strncmp((char*)rec+2,"ERROR",5)==0)
	{
		if (strncmp((char*)rec+11,"ALREADY CONNECT",10)==0)
			return (ErrorCode)12;
		
		return ErrorCode::ERR;
	}
	return ErrorCode::UNKNOWN;
}
SIM800x::ErrorCode SIM800x::CIPSTART(bool mode, const char* ipd, uint16_t port)
{
	return CIPSTART(mode,(uint8_t*) ipd,port);
}

SIM800x::ErrorCode SIM800x::CIPSEND(uint8_t* data)
{
	uint8_t c[13]="AT+CIPSEND\r\n",r;
	HAL_UART_Transmit(phuart, c, 12, HAL_MAX_DELAY);
	r=re(c,10,10);
	if (r!=0)
		return (ErrorCode)r;
	
	if (c[2]=='>')
	{
		uint8_t z=26;
		uint16_t len=strlen((char*)data);
		HAL_UART_Transmit(phuart, data, len, HAL_MAX_DELAY);
		HAL_UART_Transmit(phuart, &z, 1, HAL_MAX_DELAY);
		r=re(c,11,5000);
		if (r!=0)
			return r+3;
		
		if (strncmp((char*)c+2,"SEND OK",7)==0)
			return 10;
		
		if (strncmp((char*)c+2,"SEND FAIL",9)==0)
			return 13;
		
		return 14;
	}
	if (strncmp((char*)c+2,"ERROR",5)==0)
		return 11;
	
	return 25;
}

SIM800x::ErrorCode SIM800x::CIPSEND(const char* data)
{
	return CIPSEND((uint8_t*)data);
}

SIM800x::ErrorCode SIM800x::CIPSEND(uint8_t* data, uint16_t len)
{
	static const uint8_t send[12]="AT+CIPSEND=";
	uint8_t res, p[8], rec[12];
	itoa(p, len);
	strcat((char*)p, "\r\n");
	HAL_UART_Transmit(phuart, (uint8_t*)send, 11, HAL_MAX_DELAY);
	uint16_t LEN=strlen((char*)p);
	HAL_UART_Transmit(phuart, p, LEN, HAL_MAX_DELAY);
	
	res=re(rec,10,10);
	if (res!=0)
		return res;
	
	if (rec[2]=='>')
	{
		HAL_UART_Transmit(phuart, data, len, HAL_MAX_DELAY);
		res=re(rec,11,5000);
		if (res!=0)
			return res+3;
		
		if (strncmp((char*)rec+2,"SEND OK",7)==0)
			return 10;
		
		if (strncmp((char*)rec+2,"SEND FAIL",9)==0)
			return 13;
		
		return 14;
	}
	if (strncmp((char*)rec+2,"ERROR",5)==0)
		return 11;
	
	return 25;
}

SIM800x::ErrorCode SIM800x::CIPCLOSE(void)
{
	uint8_t c[14]="AT+CIPCLOSE\r\n", r;
	HAL_UART_Transmit(phuart, c, 13, HAL_MAX_DELAY);
	r=re(c,12,1000);
	if (r!=0)
		return r;
	
	if (strncmp((char*)c+2,"CLOSE OK",8)==0)
		return 10;
	
	if (strncmp((char*)c+2,"ERROR",5)==0)
		return 11;
		
	return 25;
}

SIM800x::ErrorCode SIM800x::CIPSHUT(void)
{
	uint8_t c[13]="AT+CIPSHUT\r\n", r;
	HAL_UART_Transmit(phuart, c, 12, HAL_MAX_DELAY);
	r=re(c,11,5000);
	if (r!=0)
		return r;
	
	if (strncmp((char*)c+2,"SHUT OK",7)==0)
		return 10;
	
	if (strncmp((char*)c+2,"ERROR",5)==0)
		return 11;
		
	return 25;
}

SIM800x::ErrorCode SIM800x::CSMINS(void)
{
	uint8_t c[23]="AT+CSMINS?\r",r;
	HAL_UART_Transmit(phuart, c, 11, HAL_MAX_DELAY);
	r=re(c,22,10);
	if (r!=0)
		return r;
	
	if (strncmp((char*)&c[2],"ERROR",5)==0)
		return 11;
	
	if (strncmp((char*)&c[18],"OK",2)!=0)
		return 14;
	
	if (c[13]=='1')
		return 20;
	
	if (c[13]=='0')
		return 21;
	
	return 25;
}

SIM800x::ErrorCode SIM800x::CANT(void)
{
	uint8_t c[15]="AT+CANT=1,1,1\r",r;
	HAL_UART_Transmit(phuart, c, 14, HAL_MAX_DELAY);
	r=re(c,9,10);
	if (r!=0)
		return r;
	
	if (strncmp((char*)&c[2],"OK",2)==0)
	{
		r=re(c,12,1500);
		if (r!=0)
		{
			strcpy((char*)c,"AT+CANT=1,0,1\r");
			HAL_UART_Transmit(phuart, c, 14, HAL_MAX_DELAY);
			return r+3;
		}
		int8_t j=c[9]-'0';
		HAL_Delay(100);
		strcpy((char*)c,"AT+CANT=1,0,1\r");
		HAL_UART_Transmit(phuart, c, 14, HAL_MAX_DELAY);
		r=re(c,10,10);
		if (strncmp((char*)&c[2],"OK",2)==0)
		{
			if (j>=0 && j<=3)
				return j+10;
			
			return 24;
		}
		if (r!=0)
			return r+6;
		
		if (strncmp((char*)&c[2],"ERROR",5)==0)
			return 21;
		
		return 14;
	}
	if (strncmp((char*)&c[2],"ERROR",5)==0)
		return 11;
	
	return 25;
}

SIM800x::ErrorCode SIM800x::CGREG(void)
{
	uint8_t c[32]="AT+CGREG?\r",r;
	HAL_UART_Transmit(phuart, c, 10, HAL_MAX_DELAY);
	r=re(c,31,10);
	if (r!=0)
		return r;
	
	if (strncmp((char*)&c[2],"ERROR",5)==0)
		return 11;
	
	if (strstr((char*)&c[17],"OK"))
	{
		int8_t s=c[12]-'0';
		if (s>=0 && s<=5)
			return s+5;
		
		return 24;
	}
	return 25;
}

SIM800x::ErrorCode SIM800x::CUSD(uint8_t* code, uint8_t* response)
{
	uint8_t c[300]="AT+CUSD=1,",r,shut[]="AT+CUSD=2\r";
	c[10]='"';
	strcpy((char*)c+11,(char*)code);
	uint16_t s=strlen((char*)c);
	c[s++]='"';   c[s++]='\r';   c[s]=0;
	HAL_UART_Transmit(phuart, c, s, HAL_MAX_DELAY);
	r=re(c,9,10);
	if (r!=0)
	{
		HAL_UART_Transmit(phuart, shut, 10, HAL_MAX_DELAY);
		return r;
	}
	if (strncmp((char*)&c[2],"ERROR",5)==0)
		return 11;
	
	if (strncmp((char*)&c[2],"OK",2)==0)
	{
		r=re(c,299,10000);
		if (r!=0)
		{
			HAL_UART_Transmit(phuart, shut, 10, HAL_MAX_DELAY);
			return r+3;
		}
		if (strncmp((char*)c+2,"+CUSD:",6)==0)
		{
			uint16_t i=13;
			while(c[i]!='"')
			{
				response[i-13]=c[i];
				i++;
			}
			HAL_UART_Transmit(phuart, shut, 10, HAL_MAX_DELAY);
			r=re(c,9,10);
			if (r!=0)
				return r+6;
			
			if (strncmp((char*)&c[2],"OK",2)==0)
				return 10;
			
			return 20;
		}
		HAL_UART_Transmit(phuart, shut, 10, HAL_MAX_DELAY);
		return 13;
	}
	HAL_UART_Transmit(phuart, shut, 10, HAL_MAX_DELAY);
	return 25;
}

SIM800x::ErrorCode SIM800x::CUSD(const char* code, uint8_t* response)
{
	return CUSD((uint8_t*)code,response);
}

SIM800x::ErrorCode SIM800x::CIPSRIP_W(bool mode)
{
	uint8_t c[15]="AT+CIPSRIP=",r;   c[11]=mode+'0';   c[12]='\r';   c[13]='\n';   c[14]=0;
	r=send(c,14,10,HAL_MAX_DELAY,50);
	return r!=HAL_OK ? r : standard((char*)c);
}

SIM800x::ErrorCode SIM800x::CIPSRIP_R(void)
{
	uint8_t c[15]="AT+CIPSRIP?\r\n",r=send(c,13,14,HAL_MAX_DELAY,50);
	if (r!=HAL_OK)
		return r;
	
	if (strstr((char*)c,"+CIPSRIP:"))
		return c[12]-33;
	
	return standard((char*)c);
}

SIM800x::ErrorCode SIM800x::CIPSTATUS(void)
{
	uint8_t c[35]="AT+CIPSTATUS\r\n",r=send(c,14,34,HAL_MAX_DELAY,50);
	if (r!=HAL_OK)
		return r;
	
	//int16_t f=STRFINDa(c,"STATE:");
	//if (f!=-1)
	//	return STATUS((char*)c+f+1);
	
	size_t t = strv((char*)c).find("STATE:");
	if (t != strv::npos)
		return STATUS((char*)c+t+1);
	
	return standard((char*)c);
}

SIM800x::ErrorCode SIM800x::CMGS(const char *da, const char * text_pdu, uint8_t toda)
{
	if (text_pdu==0)
		return 0;
	
	uint8_t c[25]="AT+CMGS=\",", r, z=26;
	
	if (cmgf==2)
	{
		uint8_t mode=CMGF_R((bool*)&cmgf);
		if (mode!=10)
			return mode;
	}
	
	uint16_t len=strlen(text_pdu);
	if (cmgf)
	{
		if (da==0)
			return 0;
		
		HAL_UART_Transmit(phuart, c, 9, HAL_MAX_DELAY);
		HAL_UART_Transmit(phuart, (uint8_t*)da, strlen(da), HAL_MAX_DELAY);
		itoa(c+10, toda);
		strcat((char*)c+11, "\r\n");
		HAL_UART_Transmit(phuart, c+8, 5+strlen((char*)c+13), HAL_MAX_DELAY);
	}
	else
	{
		itoa(c+8, len);
		strcat((char*)c+9, "\r\n");
		HAL_UART_Transmit(phuart, c, 11+strlen((char*)c+11), HAL_MAX_DELAY);
	}
	r=re(c, 10, 50);
	
	if (r!=HAL_OK)
		return r;
	
	if (c[2]=='>')
	{
		HAL_UART_Transmit(phuart, (uint8_t*)text_pdu, len ,HAL_MAX_DELAY);
		HAL_UART_Transmit(phuart, &z, 1, HAL_MAX_DELAY);
		r=re(c, 24, 20000);
		if (r!=HAL_OK)
			return r+3;
		
		if (strstr((char*)c+12, "OK"))
			return 10;
		
		if (strncmp((char*)c+2, "ERROR", 5)==0)
			return 21;
		
		return 14;
	}
	
	if (strncmp((char*)c+2, "ERROR", 5)==0)
		return 11;
	
	return 25;
}











SIM800x::ErrorCode SIM800x::CSMP(uint8_t dcs, uint8_t fo, uint8_t vp, uint8_t pid)
{
	uint8_t c[26]="AT+CSMP=", n[5]=",";
	itoa(n+1, fo);
	strcpy((char*)c+8, (char*)n+1);
	itoa(n+1, vp);
	strcat((char*)c+9, (char*)n);
	itoa(n+1, pid);
	strcat((char*)c+11, (char*)n);
	itoa(n+1, dcs);
	strcat((char*)c+13, (char*)n);
	strcat((char*)c+15, "\r\n");
	
	return RETURN(send(c, 15+strlen((char*)c+15), 11, HAL_MAX_DELAY, 50), (char*)c);
}*/



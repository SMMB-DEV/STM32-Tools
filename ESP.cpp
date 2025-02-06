/**
  ******************************************************************************
  * @file           : ESP.cpp
  * @brief          : ESP8266 functions source code
  ******************************************************************************
  * @attention
  *
  * This file was made by Seyed Mohammad Mahdi Bagheri.
	* Please inform me of any problems and bugs:
	*
	* Telegram : @SMMB98
	* LinkedIn : www.linkedin.com/in/seyed-mohamad-mahdi-bagheri-2592b0194
  *
  ******************************************************************************
	*
	* @version        : 0.1.5
	*
	******************************************************************************
  */

#include "ESP.hpp"
#include "STR.hpp"

#define rec_wait   1

ESP::ESP(UART_HandleTypeDef* UART, GPIO_TypeDef* CE_Port, uint32_t CE_Pin, GPIO_TypeDef* RST_PORT, uint32_t RST_PIN)
{
	uart=UART;
	ch_pd_port=CE_Port;
	ch_pd_pin=CE_Pin;
	rst_pin=RST_PIN;
	rst_port=RST_PORT;
	HAL_GPIO_WritePin(rst_port,rst_pin,GPIO_PIN_SET);
}

HAL_StatusTypeDef ESP::send(uint8_t *datasend, uint16_t size, uint16_t size2, uint32_t timeout, uint32_t timeout2)
{
	//HAL_UART_DMAStop(uart);
	//__HAL_UART_CLEAR_FLAG(uart, UART_FLAG_TC);
	//HAL_UART_Init(uart);
	HAL_UART_Transmit(uart,datasend,size,timeout);
	HAL_StatusTypeDef r=re(datasend,size2,timeout2);
	//HAL_UART_DMAResume(uart);
	return r;
}

HAL_StatusTypeDef ESP::re(uint8_t* datare,uint16_t size,uint32_t timeout)
{
	HAL_UART_Init(uart);
	//__HAL_UART_CLEAR_FLAG(uart, UART_FLAG_TC);
	uint16_t i=1;
	HAL_StatusTypeDef a=HAL_UART_Receive(uart,datare,1,timeout);
	if (a!=HAL_OK)
	{
		return a;
	}
	while(i<size)
	{
		if (HAL_UART_Receive(uart,&datare[i],1,rec_wait)!=HAL_OK)
		{
			break;
		}
		i++;
	}
	datare[i]=0;
	return HAL_OK;
}

uint8_t ESP::standard(uint8_t *d)
{
	if (STRFIND(d,"OK"))
	{
		return 10;
	}
	if (STRFIND(d,"ERROR"))
	{
		return 11;
	}
	if (STRFIND(d,"busy"))
	{
		return 15;
	}
	if (STRFIND(d,"FAIL"))
	{
		return 13;
	}
	return 25;
}


uint8_t ESP::Enable(void)
{
	HAL_GPIO_WritePin(ch_pd_port,ch_pd_pin,GPIO_PIN_SET);
	HAL_Delay(2000);
	return ATE(0);
}

void ESP::Disable(void)
{
	HAL_GPIO_WritePin(ch_pd_port,ch_pd_pin,GPIO_PIN_RESET);
}

uint8_t ESP::AT(void)
{
	uint8_t c[16]="AT\r\n",r=send(c,4,15,10,10);
	return r!=HAL_OK ? r : standard(c);
}

uint8_t ESP::ATE(bool s)
{
	uint8_t c[16]="ATE",r;
	c[3]=s+'0';
	STRCPY(c+4,"\r\n");
	c[15]=0;
	r=send(c,6,15,10,10);
	return r!=HAL_OK ? r : standard(c);
}

uint8_t ESP::CWMODE_R(bool def)
{
	uint8_t c[23]="AT+CWMODE_";
	if (def)
	{
		STRCPY(c+10,"DEF?\r\n");
	}
	else
	{
		STRCPY(c+10,"CUR?\r\n");
	}
	uint8_t r=send(c,16,22,10,10);
	if (r!=HAL_OK)
	{
		return r;
	}
	r=standard(c);
	if (r==10)
	{
		if (STRCMP(c,"+CWMODE_CUR:",12))
		{
			return 14+c[12]-'0';
		}
		return 14;
	}
	return r;
}

uint8_t ESP::CWMODE_W(bool save, uint8_t mode)
{
	if (mode<1 || mode>3)
	{
		return 0;
	}
	uint8_t c[23]="AT+CWMODE_";
	if (save)
	{
		STRCPY(c+10,"DEF=");
	}
	else
	{
		STRCPY(c+10,"CUR=");
	}
	c[14]=mode+'0';
	STRCPY(c+15,"\r\n");
	uint8_t r=send(c,17,22,10,100);
	return r!=HAL_OK ? r : standard(c);
}

uint8_t ESP::CWJAP_R(bool def, uint8_t *ssid, uint8_t *bssid, uint8_t *channel, int8_t *signal)
{
	uint8_t c[115]="AT+CWJAP_";
	if (def)
	{
		STRCPY(c+9,"DEF?\r\n");
	}
	else
	{
		STRCPY(c+9,"CUR?\r\n");
	}
	uint8_t r=send(c,15,114,10,100);
	if (r!=HAL_OK)
	{
		return r;
	}
	if (STRFIND(c,"No AP"))
	{
		return 15;
	}
	r=standard(c);
	if (STRCMP(c,"+CWJAP_",7))
	{
		uint16_t i=12,o=0;
		if (ssid!=0)
		{
			while (c[i]!='"')
			{
				ssid[i-12]=c[i];
				i++;
			}
			ssid[i-12]=0;
			i+=3;
		}
		else
		{
			while (c[i]!=',') {i++;}
			i+=2;
		}
		if (bssid!=0)
		{
			while (c[i]!='"')
			{
				bssid[o]=c[i];
				i++;
				o++;
			}
			bssid[o]=0;
			o=0;
		}
		else
		{
			while (c[i]!=',') {i++;}
			i+=2;
		}
		if (channel!=0)
		{
			*channel=c[i]-'0';
		}
		if (signal!=0)
		{
			o=i+2;
			while (c[i]!='\r')
			{
				i++;
			}
			c[i]=0;
			*signal=atoi(c+o);
		}
	}
	return r;
}

uint8_t ESP::CWJAP_W(bool save, uint8_t *ssid, uint8_t *pwd, uint8_t *bssid)
{
	uint8_t c[150]="AT+CWJAP_";
	if (save)
	{
		STRCPY(c+9,"DEF=\"");
	}
	else
	{
		STRCPY(c+9,"CUR=\"");
	}
	STRCPY(c+14,ssid);
	STRCAT(c,"\",\"");
	STRCAT(c,pwd);
	if (bssid!=0)
	{
		STRCAT(c,"\",\"");
		STRCAT(c,bssid);
	}
	STRCAT(c,"\"\r\n");
	uint8_t r=send(c,STRLEN(c),24,10,3500);
	if (STRFIND(c,"WIFI DISCONNECT"))
	{
		r=re(c,24,3500);
		if (r!=HAL_OK)
		{
			return 17;
		}
	}
	if (STRFIND(c,"WIFI CONNECTED"))
	{
		r=re(c,24,15000);
		if (STRFIND(c,"WIFI GOT IP"))
		{
			return 15;
		}
		return 16;
	}
	return standard(c);
}

uint8_t ESP::CWJAP_W(bool def, uint8_t *ssid, uint8_t *pwd)
{
	return CWJAP_W(def,ssid,pwd,0);
}

uint8_t ESP::CWQAP(void)
{
	uint8_t c[15]="AT+CWQAP\r\n", r=send(c,10,14,10,100);
	return standard(c);
}

uint8_t ESP::CIPSTA_R(bool def, uint8_t * IP)
{
	return CIPSTA_R(def,IP,0,0,1);
}

uint8_t ESP::CIPSTA_R(bool def, uint8_t *IP, uint8_t *Gateway, uint8_t *Netmask)
{
	return CIPSTA_R(def,IP,Gateway,Netmask,0);
}

uint8_t ESP::CIPSTA_R(bool def, uint8_t *IP, uint8_t *Gateway, uint8_t *Netmask, bool ip)
{
	uint8_t c[115]="AT+CIPSTA_",r;
	if (def)
	{
		STRCPY(c+10,"DEF?\r\n");
	}
	else
	{
		STRCPY(c+10,"CUR?\r\n");
	}
	r=send(c,16,114,10,100);
	if (r!=HAL_OK)
	{
		return r;
	}
	if (STRCMP(c+12,"ip:\"",4))
	{
		uint8_t i=16,i2;
		while (c[i]!='"')
		{
			IP[i-16]=c[i];
			i++;
		}
		if (ip)
		{
			return standard(c+90);
		}
		i+=24;
		i2=i;
		while (c[i]!='"')
		{
			Gateway[i-i2]=c[i];
			i++;
		}
		i+=24;
		i2=i;
		while (c[i]!='"')
		{
			Netmask[i-i2]=c[i];
			i++;
		}
		return standard(c+90);
	}
	return standard(c);
}

uint8_t ESP::CIPMUX_R(void)
{
	uint8_t c[20]="AT+CIPMUX?\r\n",r=send(c,12,19,10,10);
	if (r!=HAL_OK)
	{
		return r;
	}
	if (STRCMP(c,"+CIPMUX:",8))
	{
		return c[8]-33;
	}
	return standard(c);
}

uint8_t ESP::CIPMUX_W(bool mux)
{
	uint8_t c[15]="AT+CIPMUX=",r;
	c[10]=mux+'0';
	STRCPY(c+11,"\r\n");
	r=send(c,13,14,10,100);
	return r!=HAL_OK ? r : standard(c);
}

uint8_t ESP::CIPSTART_TCP(uint8_t *IP, uint16_t Port,uint16_t keepalive, uint8_t ID)
{
	if (keepalive>7200)
	{
		return 0;
	}
	uint8_t c[60]="AT+CIPSTART=",port[6],ka[5],r;
	itoa(port,Port);
	itoa(ka,keepalive);
	if (ID<=4)
	{
		c[12]=ID+'0';
		c[13]=',';
	}
	STRCAT(c,"\"TCP\",\"");
	STRCAT(c,IP);
	STRCAT(c,"\",");
	STRCAT(c,port);
	STRCAT(c,",");
	STRCAT(c,ka);
	STRCAT(c,"\r\n");
	r=send(c,STRLEN(c),30,10,1000);
	if (r!=HAL_OK)
	{
		return r;
	}
	if (STRFIND(c,"ALREADY CONNECTED"))
	{
		return 16;
	}
	if (STRFIND(c,"CONNECT"))
	{
		return 15;
	}
	return standard(c);
}

uint8_t ESP::CIPSEND(uint8_t ID, uint8_t *Data)
{
	bool _1=1;
	uint8_t c[60]="AT+CIPSEND=",len[6],r;
	if (ID<=4)
	{
		c[11]=ID+'0';
		c[12]=',';
	}
	itoa(len,STRLEN(Data));
	STRCAT(c,len);
	STRCAT(c,"\r\n");
	r=send(c,STRLEN(c),10,10,2000);
	if (r!=HAL_OK)
	{
		return r;
	}
	if (STRFIND(c,">"))
	{
		//HAL_Delay(100);
		HAL_UART_Transmit(uart,Data,STRLEN(Data),10);
		r=re(c,35,5000);
		if (r!=HAL_OK)
		{
			return r+3;
		}
		if (STRFIND(c,"Recv"))
		{
			check:
			if (STRFIND(c+12,"SEND OK"))
			{
				return 15;
			}
			if (STRFIND(c+12,"SEND FAIL"))
			{
				return 16;
			}
			if (_1)
			{
				r=re(c,15,5000);
				if (r!=HAL_OK)
				{
					return r+6;
				}
				_1=0;
				goto check;
			}
		}
	}
	return standard(c);
}

uint8_t ESP::CIPCLOSE(void)
{
	uint8_t c[20]="AT+CIPCLOSE\r\n",r=send(c,13,19,10,10);
	return r!=HAL_OK ? r : standard(c);
}

uint8_t ESP::CIPCLOSEMODE(uint8_t ID, bool mode)
{
	uint8_t c[25]="AT+CIPCLOSEMODE=",r,i=16;
	if (ID<=4)
	{
		c[16]=ID+'0';
		c[17]=',';
		i+=2;
	}
	c[i++]=mode+'0';
	c[i++]='\r';
	c[i++]='\n';
	r=send(c,i,14,10,100);
	return r!=HAL_OK ? r : standard(c);
}

uint8_t ESP::CIPSTATUS(void)
{
	uint8_t c[80]="AT+CIPSTATUS\r\n",r=send(c,14,79,10,10);
	if (r!=HAL_OK)
	{
		return r;
	}
	if (STRCMP(c,"STATUS:",7))
	{
		return c[7]-35;
	}
	return standard(c);
}















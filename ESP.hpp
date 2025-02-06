/**
  ******************************************************************************
  * @file           : ESP.hpp
  * @brief          : ESP8266 functions header file
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

#include "stm32f0xx_hal.h"


//0:wrong param   1-3:uart1   4-6:uart2   7-9:uart3   10:ok   11:error   12:busy   13:FAIL   14:ok but unclear   20:ok, no 2nd ok   21:error2   24:ok2 but unclear   25:unknown

/******Genaral******/
#define ESP_WP            0
#define ESP_OK            10
#define ESP_ERROR         11
#define ESP_Busy          12
#define ESP_FAIL          13
#define ESP_UNKNOWN       25

class ESP
{
	UART_HandleTypeDef *uart;
	uint32_t ch_pd_pin,rst_pin;
	GPIO_TypeDef *ch_pd_port,*rst_port;
	
	
	HAL_StatusTypeDef send(uint8_t *datasend, uint16_t size, uint16_t size2, uint32_t timeout, uint32_t timeout2);
	uint8_t standard(uint8_t *d);
	
	uint8_t CIPSTA_R(bool def, uint8_t *IP, uint8_t *Gateway, uint8_t *Netmask, bool ip);
	
	
	
public:
ESP(UART_HandleTypeDef* UART, GPIO_TypeDef *CE_Port, uint32_t CE_Pin, GPIO_TypeDef *RST_PORT, uint32_t RST_PIN);

HAL_StatusTypeDef re(uint8_t* datare, uint16_t size, uint32_t timeout);

uint8_t Enable(void);
void Disable(void);

uint8_t AT(void);

uint8_t ATE(bool s);

uint8_t CWMODE_R(bool def);                                                                    //15=Station   16=AP   17=Station+AP

uint8_t CWMODE_W(bool save, uint8_t mode);                                                     //mode: 1=Station   2=AP   3=Station+AP

uint8_t CWJAP_R(bool def, uint8_t *ssid, uint8_t *bssid, uint8_t *channel, int8_t *signal);    //15:No AP

uint8_t CWJAP_W(bool save, uint8_t *ssid, uint8_t *pwd);
uint8_t CWJAP_W(bool save, uint8_t *ssid, uint8_t *pwd, uint8_t *bssid);                       //15:Connected and got IP   16:Connected no IP   17:Just disconnected

uint8_t CWQAP(void);

uint8_t CIPSTA_R(bool def, uint8_t *IP);                                                       //Copies ip address to IP
uint8_t CIPSTA_R(bool def, uint8_t *IP, uint8_t *Gateway, uint8_t *Netmask);

uint8_t CIPMUX_R(void);                                                                        //15:CIPMUX=0   16:CIPMUX=1
uint8_t CIPMUX_W(bool mux);

uint8_t CIPSTART_TCP(uint8_t *IP, uint16_t Port,uint16_t keepalive, uint8_t ID);               //ID: 0-4:ID   >4:CIPMUX=0   keepalive: 0:disable   1-7200:time    return value: 15:CONNECT   16:Already Connected

uint8_t CIPSEND(uint8_t ID, uint8_t *Data);                                                    //ID: 0-4:ID   >4:CIPMUX=0   return value: 15:SEND OK   16:SEND FAIL

uint8_t CIPCLOSE(void);

uint8_t CIPCLOSEMODE(uint8_t ID, bool mode);                                                   //ID: 0-4:ID   >4:CIPMUX=0

uint8_t CIPSTATUS(void);                                                                       //15-18: ESP 2-5








};





#pragma once

#include "../Timing.hpp"
#include "./ILCD.hpp"



namespace STM32T
{
	class HD44780 : public ILCD
	{
		void (* const f_write)(bool rs, uint8_t data);
		uint8_t (* const f_read)(bool rs);
		uint8_t m_addressCounter = 0;
		bool m_twoLines, m_4Bit;
		
		void Write(const bool rs, const uint8_t data, const uint16_t timeout_us = 50)
		{
			f_write(rs, data);
			
			if (m_4Bit)
				f_write(rs, data << 4);
			
			BusyWait(timeout_us);
			
			if (rs)
				DWT_Delay(4);	// tADD
		}
		
		uint8_t Read(const bool rs)
		{
			uint8_t data = f_read(rs);
			
			if (m_4Bit)
				data |= (f_read(rs) >> 4);
			
			if (rs)
			{
				BusyWait(50);
				DWT_Delay(4);	// tADD
			}
			else
				m_addressCounter = data & 0b0111'1111;
			
			return data;
		}
		
		bool BusyWait(uint16_t timeout_us)
		{
			const uint32_t start = DWT_GetTick();
			while (Read(0) & 0b1000'0000)		// Busy Flag
			{
				if (DWT_GetTick() - start >= timeout_us)
					return false;
			}
			
			return true;
		}
		
		void SetAddress(uint8_t addr)
		{
			Write(0, 0b1000'0000 | addr);
		}
		
	public:
		/**
		* @param write - Function to set DB7:0 (or DB7:4) and RS pins according to input arguments.
		*				It must always set RW pin to 0 and the data pins must be configurd as output. The E pin must also be cycled according to the datasheet.
		* @param read - Function to read DB7:0 (or DB7:4) and set RS pin according to input argument.
		*				It must always set RW pin to 1 and the data pins must be configurd as input. The E pin must also be cycled according to the datasheet.
		*				If the LCD is configured in 4-bit mode, DB3:0 must be returned as 0.
		* @param line_count - Determines the number of lines on the display (usually 2). Valid values: 1, 2, 4
		* @param _4_bit - Determines wether the display should work in 4-bit mode or 8-bit mode (usually 4-bit mode).
		*/
		HD44780(void (* const write)(bool rs, uint8_t data), uint8_t (* const read)(bool rs), uint8_t line_count = 2, bool _4_bit = true) :
			f_write(write), f_read(read), m_twoLines(line_count != 1), m_4Bit(_4_bit) {}
		~HD44780() {}
		
		void Init()
		{
			DWT_Init();
			HAL_Delay(15);		// For VCC; probably not necessary.
			f_write(0, 0b0011'0000);
			DWT_Delay(4100);
			f_write(0, 0b0011'0000);
			DWT_Delay(100);
			f_write(0, 0b0011'0000);
			
			DWT_Delay(100);		// Instead of BusyWait() because interface is stil in 8-bit mode.
			
			f_write(0, 0b0010'0000 | (!m_4Bit << 4) | (m_twoLines << 3));	// 5x8 font by default
			DWT_Delay(100);
			
			if (m_4Bit)
				Write(0, 0b0010'0000 | (m_twoLines << 3));
			
			Write(0, 0b0000'1100);			// Display on, Cursor off, Blink off
			ClearScreen();
			Write(0, 0b0000'0110);			// Entry mode set: Increment, No display shift
		}
		
		void ClearScreen()
		{
			Write(0, 0b0000'0001, 1600);	// Display clear
		}
		
		void NextLine()
		{
			if (m_twoLines)
				SetAddress((m_addressCounter & 0x40) + 0x40);	// 0x00 or 0x40
		}
		
		void Gotoxl(uint8_t x, uint8_t l)
		{
			if (m_twoLines)
				SetAddress(l * 0x40 + x % 0x40);
			else
				SetAddress(x);
		}
		
		__attribute__((always_inline)) HD44780& xl(uint8_t x, uint8_t l) { Gotoxl(x, l); return *this; }
		__attribute__((always_inline)) HD44780& clear() { ClearScreen(); return xl(0, 0); }
		
		void PutChar(const uint8_t ch, bool interpret_specials = true) override
		{
			if (interpret_specials)
			{
				switch (ch)
				{
					case '\n':
					case '\r':
					{
						NextLine();
						return;
					}
					
					/*case '\b':
					{
						const uint16_t s = m_line * m_screenLen + m_page * MAX_CURSOR + m_cursor - (FONT_WIDTH + 1);
						const uint8_t cursor = s % m_screenLen, line = s / m_screenLen;
						
						Goto(cursor, line);
						PutChar(' ');
						Goto(cursor, line);
						
						return;
					}*/
					
					case '\t':
					{
						PutChar(' ');
						PutChar(' ');
						
						return;
					}
					
					/*case 127:	//DEL
					{
						uint8_t cursor = m_page * MAX_CURSOR + m_cursor, line = m_line;
						PutChar(' ');
						Goto(cursor, line);
						
						return;
					}*/
				}
			}
			
			Write(1, ch);
		}
	};
}

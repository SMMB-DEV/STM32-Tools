#pragma once

#include "../Timing.hpp"
#include "./ILCD.hpp"



namespace STM32T
{
	class HD44780 : public ILCD
	{
		uint8_t (* const f_rw)(bool rw, bool rs, uint8_t data);
		uint8_t m_addressCounter = 0;
		const uint8_t m_colCount;
		bool m_twoLines, m_4Bit;	// todo: Store the number of lines as well as m_twoLines.
		
		void Write(const bool rs, const uint8_t data, const uint16_t timeout_us = 50)
		{
			f_rw(0, rs, data);
			
			if (m_4Bit)
				f_rw(0, rs, data << 4);
			
			BusyWait(timeout_us);
			
			if (rs)
				Time::Delay_us(4);	// tADD
		}
		
		uint8_t Read(const bool rs)
		{
			uint8_t data = f_rw(1, rs, 0);
			
			if (m_4Bit)
				data |= (f_rw(1, rs, 0) >> 4);
			
			if (rs)
			{
				BusyWait(50);
				Time::Delay_us(4);	// tADD
			}
			else
				m_addressCounter = data & 0b0111'1111;
			
			return data;
		}
		
		bool BusyWait(uint16_t timeout_us)
		{
			const uint32_t start = Time::GetCycle();
			while (Read(0) & 0b1000'0000)		// Busy Flag
			{
				if (Time::Elapsed_us(start, timeout_us))
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
	* @param rw - Function to set DB7:0 (or DB7:4), RW and RS pins according to input arguments and return the data on the bus (DB7:0 or DB7:4) if necessary.
		*				The E pin must also be cycled according to the datasheet.
		* @param line_count - Determines the number of lines on the display (usually 2). Valid values: 1, 2, 4
		* @param _4_bit - Determines wether the display should work in 4-bit mode or 8-bit mode (usually 4-bit mode).
		*/
		HD44780(uint8_t (* const rw)(bool rw, bool rs, uint8_t data), const uint8_t line_count, const uint8_t col_count, bool _4_bit = true)
			: f_rw(rw), m_twoLines(line_count != 1), m_colCount(col_count), m_4Bit(_4_bit) {}
		
		~HD44780() {}
		
		HD44780& Init()
		{
			Time::DWT_Init();
			HAL_Delay(15);		// For VCC; probably not necessary.
			f_rw(0, 0, 0b0011'0000);
			Time::Delay_us(4100);
			f_rw(0, 0, 0b0011'0000);
			Time::Delay_us(100);
			f_rw(0, 0, 0b0011'0000);
			
			Time::Delay_us(100);		// Instead of BusyWait() because interface is stil in 8-bit mode.
			
			f_rw(0, 0, 0b0010'0000 | (!m_4Bit << 4) | (m_twoLines << 3));	// 5x8 font by default
			Time::Delay_us(100);
			
			if (m_4Bit)
				Write(0, 0b0010'0000 | (m_twoLines << 3));
			
			Display(false, false, false);
			Clear();
			Write(0, 0b0000'0110);			// Entry mode set: Increment, No display shift
			Display(true, false, false);
			
			return *this;
		}
		
		HD44780& Clear()
		{
			Write(0, 0b0000'0001, 1600);	// Display clear
			return *this;
		}
		
		void Display(const bool on, const bool cursor = false, const bool blink = false)
		{
			Write(0, 0b0000'1000 | (on << 2) | (cursor << 1) | blink);
		}
		
		void NextLine()
		{
			if (m_twoLines)
				SetAddress((m_addressCounter & 0x40) + 0x40);	// 0x00 or 0x40
		}
		
		HD44780& XL(uint8_t x, uint8_t l)
		{
			if (m_twoLines)
				SetAddress(l * 0x40 + x % 0x40);
			else
				SetAddress(x);
			
			return *this;
		}
		
		void PutChar(const char ch, bool interpret_specials = true, bool auto_next_line = true) override
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
			
			if (auto_next_line && m_twoLines && (m_addressCounter & 0x3F) >= m_colCount - 1)
				NextLine();
			
			Write(1, ch);
		}
	};
}

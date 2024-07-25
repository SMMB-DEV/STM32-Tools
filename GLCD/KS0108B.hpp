// *** TODO: ADD LICENSE ***

#pragma once
	
#include "main.h"

namespace STM32T
{
	enum State: uint8_t {initial=0, final, medial, isolated};

	class Char
	{
	private:
		uint8_t statecount, *width;
		const uint8_t **datah, **datal;
		bool scset, *chset;
	public:
		Char();
		~Char();
		void Set(const uint8_t *d1, const uint8_t *d2, State s, uint8_t wid);
		void SetStateCount(uint8_t s);
		uint8_t getwidth(State s);
		const uint8_t* getdh(State s);
		const uint8_t* getdl(State s);
	};



	/**
	* @note The RST pin of the LCD must be set high at least 1us after powering on the LCD.
	*/
	class KS0108B
	{
		// todo: make cursor 16 bits in functions.
		
	public:
		static constexpr uint8_t MAX_LINES = 8, MAX_CURSOR = 64, PIXELS_PER_LINE = 8, SCREEN_WIDTH = MAX_LINES * PIXELS_PER_LINE;
		
	private:
		//Char *FontFa = nullptr;          //[36];
		
		inline uint16_t MapIndex() const
		{
			return m_line * m_screenLen + m_page * MAX_CURSOR + m_cursor;
		}
		
		__attribute__((always_inline)) inline uint16_t MapIndex(uint8_t cursor) const
		{
			return m_line * m_screenLen + cursor;
		}
		
		__attribute__((always_inline)) inline uint16_t MapIndex(uint8_t line, uint8_t cursor) const
		{
			return line * m_screenLen + cursor;
		}
		
		bool faset = false, Fa = false;
		
		void Command(const bool rs, const bool rw, uint8_t d7_0) const;
		bool SetPage(const uint8_t page, const bool force = false);
		void SetLine(const uint8_t line, const bool force = true);		//Lines:0-7
		void SetCursor(const uint8_t cursor, const bool force = true);	//0-63
		void Display(const bool display) const;							//0:OFF   1:ON
		void Goto(const uint8_t cursor, const uint8_t line, const bool force = false);			//x:0-191   y:0-7
		void CheckCursor(const uint8_t lines = 1);
		void Write(const uint8_t byte, const bool check = true, const uint8_t lines = 1);
		void Write_H(const uint8_t byte, const uint8_t shift, const bool check = true, const uint8_t lines = 1);
		void Write_L(const uint8_t byte, const uint8_t shift, const bool check = true, const uint8_t lines = 1);
		
		void Line_y(const uint8_t x, const uint8_t y, const uint8_t len, const bool draw);
		void Line_x(const uint8_t x, const uint8_t y, const uint8_t len, const bool draw);
		
		
		
		
		void (* const f_command)(uint8_t data, bool rw, bool rs);
		void (* const f_setCS)(uint8_t);
		
		const uint8_t m_pageCount;
		const uint16_t m_screenLen;
		
		uint8_t * const m_screenMap;
		
		uint8_t m_cursor = 0, m_row = 0, m_line = 0, m_page = 0;
		
	public:
		
		/**
		* @param command - Function to set D0:7, RW, RS and EN pins according to the timing specifications on page 8 of the KS0108B datasheet.
		*			A simplified version could set all the pins and then pulse the EN pin high and then low for a few microseconds.
		* @param set_cs - Function to enable/disable LCD pages (controls CSx pins).
		* @param page_count - Max. 8 pages.
		*/
		KS0108B(void (*command)(uint8_t data, bool rw, bool rs), void (*set_cs)(uint8_t), const uint8_t page_count = 2);
		~KS0108B();
		
		//void SetFa();
		
		void Test();
		
		void Init();
		
		/**
		* @brief Fills or clears the Screen and goes to (0, 0)
		*/
		void ClearScreen(const bool fill = false);
		void Gotoxy(const uint8_t x, const uint8_t y);
		void Movexy(const int16_t x, const int8_t y);						//Moves cursor x pixels horizontally and y pixels vertically. (can be negative or positive)
		void Gotoxl(const uint8_t x, const uint8_t line);
		
		void WriteByte(const uint8_t byte);
		//uint8_t Read(void);
		void NextLine(const uint8_t lines = 1);								//Goes to the next line and sets the cursor to 0.
		
		void Pixel(const uint8_t x, const uint8_t y, bool fill = true);
		
		void PutChar(const uint8_t ch, bool big = false, bool interpret_specials = true);
		void PutStr(const char* str, bool big = false);
		void PutStrn(const char* str, uint16_t n, bool big = false);
		//void PutStrf(const char* fmt, ...);
		void PutStrfxl(const uint8_t x, const uint8_t line, const char* fmt, ...);
		void PutStrfxy(const uint8_t x, const uint8_t y, const char* fmt, ...);
		
		void PutStrxl(const uint8_t x, const uint8_t line, const char* str)
		{
			Gotoxl(x, line);
			PutStr(str);
		}
		
		void PutStrxy(const uint8_t x, const uint8_t y, const char* str)
		{
			Gotoxy(x, y);
			PutStr(str);
		}
		
		void PutStrnxl(const uint8_t x, const uint8_t line, const char* str, uint16_t n)
		{
			Gotoxl(x, line);
			PutStrn(str, n);
		}
		
		//void PutNum(int32_t num);
		//void PutNum(float num);
		
		//void PutCharFa(uint8_t ch, State s);								//Prints a character in Farsi. (ch:0-36   s:initial-medial-final-isolated)
		//void PutStrFa(uint8_t *str);										//Prints a string in Farsi. (UTF-8 without signature)
		//void PutStrFa(const char *str);
		
		/**
		* @param bmp - The first four bytes contain two little endian shorts (length and width).
		*/
		void Bitmap(const uint8_t * bmp, uint16_t x, uint8_t y);
		
		void Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool draw = true);
		
		void Rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool draw = true);
		void FillRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool fill = true);
		void NegateRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
		
		void Circle(uint8_t x, uint8_t y, uint8_t r, bool draw = true);
		void FillCircle(uint8_t x, uint8_t y, uint8_t r, bool fill = true);
		void FillCircleNew(uint8_t x, uint8_t y, uint8_t r, bool fill = true);
	};
}


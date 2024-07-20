// *** TODO: ADD LICENSE ***
	
#include <stm32f0xx_hal.h>
#include <cstring>

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



class GLCD
{
public:
	struct Config
	{
		GPIO_TypeDef * const Port_RS, * const Port_RW, * const Port_E, * const Port_RST, * const Port_CS1, * const Port_CS2, * const Port_CS3;
		GPIO_TypeDef* Port_D[8];
		
		const uint16_t Pin_RS, Pin_RW, Pin_E, Pin_RST, Pin_CS1, Pin_CS2, Pin_CS3;
		uint16_t Pin_D[8];
		
		Config(GPIO_TypeDef *Port_RS, GPIO_TypeDef *Port_RW, GPIO_TypeDef *Port_E, GPIO_TypeDef *Port_RST,
			GPIO_TypeDef *Port_CS1, GPIO_TypeDef *Port_CS2, GPIO_TypeDef *Port_CS3, GPIO_TypeDef* Port_D[8],
			uint16_t Pin_RS, uint16_t Pin_RW, uint16_t Pin_E, uint16_t Pin_RST, uint16_t Pin_CS1, uint16_t Pin_CS2, uint16_t Pin_CS3, uint16_t Pin_D[8]) :
		
			Port_RS(Port_RS), Port_RW(Port_RW), Port_E(Port_E), Port_RST(Port_RST), Port_CS1(Port_CS1), Port_CS2(Port_CS2), Port_CS3(Port_CS3),
			Pin_RS(Pin_RS), Pin_RW(Pin_RW), Pin_E(Pin_E), Pin_RST(Pin_RST), Pin_CS1(Pin_CS1), Pin_CS2(Pin_CS2), Pin_CS3(Pin_CS3)
		{
			memcpy(this->Port_D, Port_D, sizeof(this->Port_D));
			memcpy(this->Pin_D, Pin_D, sizeof(this->Pin_D));
			
			if (!Port_CS3)
			{
				Port_CS3 = Port_CS2;
				Pin_CS3 = 0;
			}
		}
	};

private:
	static constexpr uint8_t MAX_LINES = 8, MAX_CURSOR = 64, PIXELS_PER_LINE = 8, SCREEN_WIDTH = MAX_LINES * PIXELS_PER_LINE;

	//Char *FontFa = nullptr;          //[36];
	
	const Config m_config;
	const uint8_t m_pageCount;
	const uint16_t m_screenlen;
	uint8_t * const m_map;
	
	uint8_t m_cursor, m_row, m_line, m_page;

	inline uint16_t MapIndex() const
	{
		return m_line * m_screenlen + m_page * MAX_CURSOR + m_cursor;
	}
	
	__attribute__((always_inline)) inline uint16_t MapIndex(uint8_t cursor) const
	{
		return m_line * m_screenlen + cursor;
	}
	
	__attribute__((always_inline)) inline uint16_t MapIndex(uint8_t line, uint8_t cursor) const
	{
		return line * m_screenlen + cursor;
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
	void Write_L(const uint8_t byte, const uint8_t bits, const bool check = true, const uint8_t lines = 1);

	void Line_y(const uint8_t x, const uint8_t y, const uint8_t len, const bool draw);
	void Line_x(const uint8_t x, const uint8_t y, const uint8_t len, const bool draw);
	
public:
	GLCD(const Config& config, const uint8_t pageCount = 2, const bool init = false);	//pageCount: 2-3
	~GLCD();
	//void SetFa();
	
	void Init();
	void FillScreen(const bool fill = true);							//Fills or clears the Screen and goes to (0,0)
	void Gotoxy(const uint8_t x, const uint8_t y);
	void Movexy(const int16_t x, const int8_t y);						//Moves cursor x pixels horizontally and y pixels vertically. (can be negative or positive)
	
	void WriteByte(const uint8_t byte);
	//uint8_t Read(void);
	void NextLine(const uint8_t lines = 1);								//Goes to the next line and sets the cursor to 0.
	
	void Pixel(const uint8_t x, const uint8_t y, bool fill = true);
	
	void PutChar(const uint8_t ch, bool big = false);
	void PutStr(const uint8_t* str, bool big = false);
	void PutStr(const char* str, bool big = false);
	void PutStr(const uint8_t* str, uint16_t n, bool big = false);
	//void PutNum(int32_t num);
	//void PutNum(float num);
	
	//void PutCharFa(uint8_t ch, State s);								//Prints a character in Farsi. (ch:0-36   s:initial-medial-final-isolated)
	//void PutStrFa(uint8_t *str);										//Prints a string in Farsi. (UTF-8 without signature)
	//void PutStrFa(const char *str);
	
	void Line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool draw = true);
	
	void Rectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool draw = true);
	void FillRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool fill = true);
	void NegateRectangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
	
	void Circle(uint8_t x, uint8_t y, uint8_t r, bool draw = true);
	void FillCircle(uint8_t x, uint8_t y, uint8_t r, bool fill = true);
	void FillCircleNew(uint8_t x, uint8_t y, uint8_t r, bool fill = true);
};



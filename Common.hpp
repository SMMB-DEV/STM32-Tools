#pragma once

extern "C"
{
#include "main.h"
}

#include <string_view>
#include <vector>
#include <functional>
#include <type_traits>



#define _countof(arr)				(sizeof(arr) / sizeof(arr[0]))



using std::operator"" sv;
using strv = std::string_view;	// todo: Make strv a subclass with a compare_remove() method

namespace STM32T
{
	inline constexpr strv WHITESPACE = " \r\n\t\f\v"sv;
	
	template <class T>
	using vec = std::vector<T>;
	
	template <class F>
	using func = std::function<F>;
	
	template <class T>
	constexpr bool is_int_v = !std::is_same_v<T, bool> && std::is_integral_v<T>;

	inline constexpr size_t operator"" _Ki(unsigned long long x) noexcept
	{
		return x * 1024;
	}
	
	inline constexpr size_t operator"" _Ki(long double x) noexcept
	{
		return x * 1024;
	}
	
	inline constexpr uint8_t operator"" _u8(unsigned long long x) noexcept
	{
		return x;
	}

	inline constexpr uint16_t operator"" _u16(unsigned long long x) noexcept
	{
		return x;
	}
	
	/**
	* @brief https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
	*/
	inline bool is_power_2(uint32_t val)
	{
		return val && !(val & (val - 1));
	}
	
	/**
	* @brief https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
	*/
	inline uint8_t bit_count(uint32_t val)
	{
		uint8_t c;
		for (c = 0; val; c++)
			val &= val - 1;		//clear the least significant bit set
		
		return c;
	}
	
	/**
	* @brief https://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
	* @retval True if val has odd parity.
	*/
	template <typename T>
	inline bool parity(T val)
	{
		static_assert(std::is_integral_v<T>);
		
		if constexpr (sizeof(T) > sizeof(uint64_t))
			val ^= val >> 64;
		
		if constexpr (sizeof(T) > sizeof(uint32_t))
			val ^= val >> 32;
		
		if constexpr (sizeof(T) > sizeof(uint16_t))
			val ^= val >> 16;
		
		if constexpr (sizeof(T) > sizeof(uint8_t))
			val ^= val >> 8;
		
		val ^= val >> 4;
		val &= 0x0F;
		return (0b0110'1001'1001'0110 >> val) & 1;
	}
	
	template <typename T>
	inline constexpr T ceil(T x, T y)
	{
		static_assert(is_int_v<T>);
		
		return x / y + (x % y != 0);
	}
	
	strv trim(strv view, strv trimChars = WHITESPACE)
	{
		view.remove_prefix(std::min(view.find_first_not_of(trimChars), view.size()));
		view.remove_suffix(view. size() - std::min(view.find_last_not_of(trimChars) + 1, view.size()));
		
		return view;
	}
	
	template<class T>
	union shared_arr
	{
		static_assert(sizeof(T) > sizeof(uint8_t));
		
		uint8_t arr[sizeof(T)];
		T val;
		
		shared_arr<T>& operator=(const shared_arr<T>& other)
		{
			val = other.val;
			return *this;
		}
	};
	
	struct ScopeAction
	{
		void (* const m_end)();
		
		ScopeAction(void (* end)()) : m_end(end) {}
		~ScopeAction() { m_end(); }
	};
	
	struct ScopeActionF
	{
		using funt_t = func<void()>;
		
		const funt_t m_end;
		
		ScopeActionF(funt_t &&end) : m_end(end) {}
		~ScopeActionF() { m_end(); }
	};
	
	template <typename T, T MIN, T MAX>
	class ClampedInt
	{
		static_assert(is_int_v<T> && MAX > MIN);
		
		static constexpr bool is_unlimited = MIN == std::numeric_limits<T>::min() && MAX == std::numeric_limits<T>::max();
		
		
		T m_val;
		
	public:
		ClampedInt() : m_val(std::clamp(T(0), MIN, MAX)) {}
		ClampedInt(const T val) : m_val(std::clamp(val, MIN, MAX)) {}
		
		operator T() const volatile { return m_val; }
		
		ClampedInt& operator=(const T val)
		{
			m_val = std::clamp(val, MIN, MAX);
			return *this;
		}
		
		volatile ClampedInt& operator=(const T val) volatile
		{
			m_val = std::clamp(val, MIN, MAX);
			return *this;
		}
		
		bool operator==(const T val) { return m_val == val; }
		
		ClampedInt& operator++()	// prefix
		{
			if constexpr (is_unlimited)
				m_val++;
			else
			{
				if (m_val >= MAX)
					m_val = MIN;
				else
					m_val++;
			}
			
			return *this;
		}
		
		ClampedInt& operator--()	// prefix
		{
			if constexpr (is_unlimited)
				m_val--;
			else
			{
				if (m_val <= MIN)
					m_val = MAX;
				else
					m_val--;
			}
			
			return *this;
		}
		
		ClampedInt operator++(int)	// postfix
		{
			ClampedInt val = *this;
			
			if constexpr (is_unlimited)
				m_val++;
			else
			{
				if (m_val >= MAX)
					m_val = MIN;
				else
					m_val++;
			}
			
			return val;
		}
		
		ClampedInt operator--(int)	// postfix
		{
			ClampedInt val = *this;
			
			if constexpr (is_unlimited)
				m_val--;
			else
			{
				if (m_val <= MIN)
					m_val = MAX;
				else
					m_val--;
			}
			
			return val;
		}
		
		volatile ClampedInt& operator++() volatile	// prefix
		{
			if constexpr (is_unlimited)
				m_val++;
			else
			{
				if (m_val >= MAX)
					m_val = MIN;
				else
					m_val++;
			}
			
			return *this;
		}
		
		volatile ClampedInt& operator--() volatile	// prefix
		{
			if constexpr (is_unlimited)
				m_val--;
			else
			{
				if (m_val <= MIN)
					m_val = MAX;
				else
					m_val--;
			}
			
			return *this;
		}
		
		ClampedInt operator++(int) volatile	// postfix
		{
			ClampedInt val = *this;
			
			if constexpr (is_unlimited)
				m_val++;
			else
			{
				if (m_val >= MAX)
					m_val = MIN;
				else
					m_val++;
			}
			
			return val;
		}
		
		ClampedInt operator--(int) volatile	// postfix
		{
			ClampedInt val = *this;
			
			if constexpr (is_unlimited)
				m_val--;
			else
			{
				if (m_val <= MIN)
					m_val = MAX;
				else
					m_val--;
			}
			
			return val;
		}
	};
	
	/**
	* @brief Useful for queuing work inside an ISR to be handled in the main loop. Has fast insertion and slow access and removal.
	*		Allocates memory on the heap. If the list gets full, new elements cannot be added.
	* @note In very rare cases, it might lose some data. Do not use for critical information.
	*/
	template <class T, size_t MAX_SIZE = SIZE_MAX>
	class linked_list
	{
		static_assert(MAX_SIZE > 0);
		
		class container
		{
			friend class linked_list<T, MAX_SIZE>;
			
			container * volatile p_next = nullptr;
			T m_val;
			
			container(const T& val) : m_val(val) {}
			container(T&& val) : m_val(val) {}
		};
		
		container * volatile p_list = nullptr;
		volatile size_t m_size = 0;
		volatile bool popping = false;
		
	public:
		linked_list() {}
		~linked_list()
		{
			while (pop_front());
		}
		
		/**
		* @note Must not be called on an empty linked_list.
		*/
		T& front() const { return p_list->m_val; }
		
		bool empty() const { return !m_size; }
		
		bool full() const { return m_size >= MAX_SIZE; }
		
		/**
		* @brief Meant to be called from the main loop.
		*/
		bool pop_front()
		{
			if (empty())
				return false;
			
			container *current = p_list;
			
			popping = true;
			p_list = p_list->p_next;
			popping = false;
			
			m_size--;
			delete current;
			
			return true;
		}
		
		/**
		* @brief Meant to be called from an ISR.
		* @note Don't call this from multiple ISRs with different priorities.
		*/
		template <class... Args>
		void emplace_back(Args&&... args)
		{
			if (full())
				return;
			
			if (m_size == 1 && popping && p_list != nullptr)	// fixme
				return;
			
			container * volatile *end = &p_list;
			
			while (*end)	// Not relying on m_size because it might not be in sync with p_list.
				end = &((*end)->p_next);
			
			*end = new container(T{args...});
		}
	};
	
	template <class T, size_t MAX_SIZE = 256>
	class PriorityQueue
	{
		using index_t = uint8_t;
		static_assert(MAX_SIZE <= (1 << (sizeof(index_t) * 8)) && MAX_SIZE > 0);
		
		T m_items[MAX_SIZE];
		uint8_t m_prio[MAX_SIZE] = { 0 };	// 0 means non-existent. Higher number means higher priority.
		
		// m_front always modified in pop_front() and m_back always modified in push_back().
		//volatile index_t m_front = 0, m_back = 0;
		volatile ClampedInt<index_t, 0, MAX_SIZE - 1> m_front = 0, m_back = 0;
		
	public:
		PriorityQueue() {}
		~PriorityQueue() {}
		
		bool empty() const volatile
		{
			return m_front == m_back;
		}
		
		bool full() const volatile
		{
			return m_front - 1 == m_back;
		}
		
		/**
		* @param prio - The priority of the item; the greater the number, the higher the priority. Priority 0 means the item won't be added.
		*/
		void emplace_back(T&& t, const uint8_t prio = 1)
		{
			if (full() || !prio)
				return;
			
			m_items[m_back] = std::move(t);
			m_prio[m_back] = prio;
			m_back++;
		}
		
		/**
		* @param prio - The priority of the item; the greater the number, the higher the priority. Priority 0 means the item won't be added.
		*/
		void push_back(const T& t, const uint8_t prio)
		{
			if (full() || !prio)
				return;
			
			m_items[m_back] = t;
			m_prio[m_back] = prio;
			m_back++;
		}
		
		std::optional<T> pop_front()
		{
			const index_t back = m_back, front = m_front;	// For fewer volatile accesses
			
			// Find the first index of the highest priority
			index_t prio_index;
			uint8_t max_prio = 0;
			for (index_t i = front; i != back; i++)
			{
				if (m_prio[i])	// Ignore previously handled items
				{
					if (m_prio[i] > max_prio)
					{
						max_prio = m_prio[i];
						prio_index = i;
					}
				}
			}
			
			if (!max_prio)
			{
				m_front = back;
				return std::nullopt;
			}
			
			
			m_prio[prio_index] = 0;
			
			// Remove handled items at the beginning of the list
			for (index_t i = front; i != back; i++)
			{
				if (m_prio[i])
					break;
				
				m_front++;
			}
			
			return std::move(m_items[prio_index]);
		}
	};
	
	template<bool condition>
	struct warn_if {};

	template<> struct __attribute__((deprecated)) warn_if<false> { constexpr warn_if() = default; };
	
	#define static_warn(x, ...) ((void) STM32T::warn_if<x>())
	//#define static_warn(x, ...) (__attribute__((deprecated)) (void))
	
	template<typename T, typename BUF_TYPE = char*>
	inline T pack_be(BUF_TYPE buf)
	{
		{
			using namespace std;
			
			static_assert(is_same_v<make_unsigned_t<remove_const_t<remove_pointer_t<BUF_TYPE>>>, unsigned char>);
			static_assert(!is_same_v<T, bool> && is_integral_v<T>);
		}
		
		T val = 0;
		for (size_t i = 0; i < sizeof(T); i++)
		{
			val <<= 8;
			val |= *buf++;
		}
		
		return val;
	}
	
	template<typename T, typename BUF_TYPE = char*>
	inline T pack_le(BUF_TYPE buf)
	{
		{
			using namespace std;
			
			static_assert(is_same_v<BUF_TYPE, char*> || is_same_v<BUF_TYPE, uint8_t*>);
			static_assert(!is_same_v<T, bool> && is_integral_v<T>, "");
		}
		
		if (reinterpret_cast<uintptr_t>(buf) % alignof(T))
		{
			T val = 0;
			buf += sizeof(T);
			for (size_t i = 0; i < sizeof(T); i++)
			{
				val <<= 8;
				val |= *--buf;
			}
			
			return val;
		}
		else
			return *(T*)buf;
	}
	
	template<typename T, typename BUF_TYPE = char*>
	inline void unpack_be(BUF_TYPE buf, T val)
	{
		static_assert(std::is_same_v<BUF_TYPE, char*> || std::is_same_v<BUF_TYPE, uint8_t*>);
		static_assert(!std::is_same_v<T, bool> && std::is_integral_v<T>, "");
		
		for (int i = sizeof(T) - 1; i >= 0; i--)
			*buf++ = val >> (i * 8);
	}
	
	template<typename T, typename BUF_TYPE = char*>
	inline void unpack_le(BUF_TYPE buf, T val)
	{
		static_assert(std::is_same_v<BUF_TYPE, char*> || std::is_same_v<BUF_TYPE, uint8_t*>);
		static_assert(!std::is_same_v<T, bool> && std::is_integral_v<T>, "");
		
		if (reinterpret_cast<uintptr_t>(buf) % alignof(T))
		{
			for (size_t i = 0; i < sizeof(T); i++)
				*buf++ = val >> (i * 8);
		}
		else
			*(T*)buf = val;
	}
	
	template<typename T>
	inline T le2be(const T t)
	{
		static_assert(!std::is_same_v<T, bool> && std::is_integral_v<T> && sizeof(T) > 1, "");
		
		shared_arr<T> _t {.val = t};
		
		for (uint8_t i = 0; i < sizeof(T) / 2; i++)
			std::swap(_t.arr[i], _t.arr[sizeof(T) - 1 - i]);
		
		return _t.val;
	}
	
	inline void Tokenize(strv view, const strv sep, vec<strv>& tokens, const bool ignoreSingleEnded, size_t (strv::*f_find)(strv, size_t) const = &strv::find)
	{
		// Assuming view is null-terminated.
		
		const char * const start = view.data();
		
		while (view.size() > 0)
		{
			size_t end = (view.*f_find)(sep, 0);
			if (end == strv::npos)
			{
				if (!ignoreSingleEnded)
					tokens.push_back(view);		// This is why the view must be null-terminated.
			
				return;
			}
			
			if ((view.data() != start || !ignoreSingleEnded) && end > 0)	// No empty tokens
			{
				((char*)view.data())[end] = '\0';	// No problems with C string functions. todo: use std::span
				tokens.push_back(view.substr(0, end));
			}
			
			view.remove_prefix(end + sep.size());
		}
	}
	
	inline void Tokenize(strv view, const strv sep, const func<void (strv)>& op, const bool ignoreSingleEnded, size_t (strv::*f_find)(strv, size_t) const = &strv::find)
	{
		// note: Assuming view is null-terminated.
		
		const char * const start = view.data();
		
		while (view.size() > 0)
		{
			size_t end = (view.*f_find)(sep, 0);
			if (end == strv::npos)
			{
				if (!ignoreSingleEnded)
					op(view);
			
				return;
			}
			
			if ((view.data() != start || !ignoreSingleEnded) && end > 0)	// No empty tokens
			{
				((char*)view.data())[end] = '\0';	// No problems with C string functions
				op(view.substr(0, end));
			}
			
			view.remove_prefix(end + sep.size());
		}
	}
	
	/**
	* @param op - Handles each token. It op() returns true, it means that no more tokens should be processed.
	*/
	inline void Tokenize(strv view, const strv sep, const func<bool (strv)>& op, const bool ignoreSingleEnded, size_t (strv::*f_find)(strv, size_t) const = &strv::find)
	{
		// note: Assuming view is null-terminated.
		// todo: Handle {sep} inside string literals
		
		const char * const start = view.data();
		
		while (view.size() > 0)
		{
			size_t end = (view.*f_find)(sep, 0);
			if (end == strv::npos)
			{
				if (!ignoreSingleEnded)
					op(view);
			
				return;
			}
			
			if ((view.data() != start || !ignoreSingleEnded) && end > 0)	// No empty tokens
			{
				((char*)view.data())[end] = '\0';	// No problems with C string functions
				if (op(view.substr(0, end)))
					return;
			}
			
			view.remove_prefix(end + sep.size());
		}
	}
	
	
	
#ifdef HAL_RTC_MODULE_ENABLED
	inline void AdjustDateAndTime(RTC_DateTypeDef& date, RTC_TimeTypeDef& time, int32_t sec)
	{
		// 24-Hour/Binary Format
		
		if (sec == 0 || sec > 28 * 24 * 3600 || sec < -28 * 24 * 3600)	// Max. 28 days; don't want to deal with calculating number of months more than 1.
			return;
		
		static constexpr uint8_t MONTH_DAYS[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };	// February can be 29
		
		// These extra days occur in each year that is an integer multiple of 4 (except for years evenly divisible by 100, but not by 400) [https://en.wikipedia.org/wiki/Leap_year]
		const bool isLeapYear = date.Year % 4 == 0 && date.Year != 0;
		const uint8_t monthDays = MONTH_DAYS[date.Month - 1] + (date.Month == 2 && isLeapYear);
		
		int16_t mins = (sec / 60) % 60;
		int16_t hours = (sec / (60 * 60)) % 24;
		int8_t days = sec / (24 * 60 * 60);
		sec %= 60;
		
		if (sec > 0)
		{
			time.Seconds += sec;
			if (time.Seconds >= 60)
			{
				time.Seconds -= 60;
				time.Minutes++;
			}
			
			time.Minutes += mins;
			if (time.Minutes >= 60)
			{
				time.Minutes -= 60;
				time.Hours++;
			}
			
			time.Hours += hours;
			if (time.Hours >= 24)
			{
				time.Hours -= 24;
				days++;
			}
			
			date.Date += days;
			if (date.Date > monthDays)
			{
				date.Date -= monthDays;
				date.Month++;
				
				if (date.Month > 12)
				{
					date.Month -= 12;
					date.Year++;
					date.Year %= 100;
				}
			}
		}
		else
		{
			time.Seconds += sec;
			if (time.Seconds > 60)
			{
				time.Seconds += 60;
				time.Minutes--;
			}
			
			time.Minutes += mins;
			if (time.Minutes > 60)	//underflow
			{
				time.Minutes += 60;
				time.Hours--;
			}
			
			time.Hours += hours;
			if (time.Hours > 24)	//underflow
			{
				time.Hours += 24;
				days--;
			}
			
			const uint8_t prevMonth = date.Month == 1 ? 12 : date.Month - 1;
			date.Date += days;
			if (date.Date == 0 || date.Date > monthDays)	//underflow
			{
				date.Date = MONTH_DAYS[prevMonth - 1] + (prevMonth == 2 && isLeapYear) - (UINT8_MAX - date.Date + 1);
				date.Month = prevMonth;
				
				if (prevMonth == 12)
				{
					date.Year--;
					if (date.Year > 100)	//underflow
						date.Year += 100;
				}
			}
		}
		
		date.WeekDay = ((date.WeekDay - 1) + 4 * 7 + days) % 7 + 1;	// 4 * 7: Doesn't change mod 7; just to ensure it's a positive number.
	}
#endif	// HAL_RTC_MODULE_ENABLED
}

#pragma once

extern "C"
{
	#include "main.h"
}

#include <vector>
#include <functional>
#include <type_traits>



#define _countof(arr)				(sizeof(arr) / sizeof(arr[0]))



namespace STM32T
{
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
	
	inline constexpr uint32_t pow10(uint8_t pow)
	{
		uint32_t val = 1;
		for (uint8_t i = 0; i < pow; i++)
			val *= 10;
		
		return val;
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
	
	class ScopeAction
	{
		void (*m_end)();
		
	public:
		ScopeAction(void (* end)()) : m_end(end) {}
		~ScopeAction()
		{
			if (m_end)
				m_end();
		}
		
		void EarlyExit()
		{
			if (m_end)
				m_end();
			
			m_end = nullptr;
		}
	};
	
	class ScopeActionF
	{
	public:
		using funt_t = func<void()>;
		
		ScopeActionF(funt_t &&end) : m_end(std::move(end)) {}
		~ScopeActionF()
		{
			if (m_end)
				m_end();
		}
		
		void EarlyExit()
		{
			if (m_end)
				m_end();
			
			m_end = nullptr;
		}
		
	private:
		funt_t m_end;
	};
	
	template <typename T, T MIN, T MAX>
	class ClampedInt
	{
		T m_val;
		
	public:
		static constexpr T val_min = MIN, val_max = MAX;
		static constexpr uintmax_t RANGE = MAX - MIN;
		static_assert(is_int_v<T> && MAX > MIN);
		
		static constexpr bool is_unlimited = MIN == std::numeric_limits<T>::min() && MAX == std::numeric_limits<T>::max();
		
		
		ClampedInt() : m_val(std::clamp(T(0), MIN, MAX)) {}
		ClampedInt(const T val) : m_val(std::clamp(val, MIN, MAX)) {}
		
		operator T() const volatile { return m_val; }
		
		T val() { return m_val; }
		
		ClampedInt& operator=(const T val)
		{
			if constexpr (is_unlimited)
				m_val = val;
			else
				m_val = std::clamp(val, MIN, MAX);
			
			return *this;
		}
		
		volatile ClampedInt& operator=(const T val) volatile
		{
			m_val = std::clamp(val, MIN, MAX);
			return *this;
		}
		
		template <typename E>
		bool operator==(const E val)
		{
			static_assert(is_int_v<E>);
			
			return m_val == val;
		}
		
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
		
		template <typename A>
		ClampedInt& operator+=(A add)
		{
			static_assert(is_int_v<A>);
			
			if constexpr (is_unlimited)
				m_val += add;
			else
			{
				if (add >= 0)
				{
					add %= RANGE + 1;
					
					const uintmax_t rem = MAX - m_val;	// always <= RANGE
					if (add > rem)
					{
						add -= rem + 1;
						m_val = MIN;
					}
					
					m_val += add;
				}
				else
				{
					uintmax_t add_pos = static_cast<uintmax_t>((add + 1) * -1) + 1;
					add_pos %= RANGE + 1;
					
					const uintmax_t rem = m_val - MIN;
					if (add_pos > rem)
					{
						add_pos -= rem + 1;
						m_val = MAX;
					}
					
					m_val -= add_pos;
				}
			}
			
			return *this;
		}
	};
	
	template <typename T>
	class DynClampedInt
	{
		static_assert(is_int_v<T>);
		
	public:
		const T min, max;
		const uintmax_t range;
		
	private:
		T m_val;
		
	public:
		bool IsUnlimited()
		{
			return min == std::numeric_limits<T>::min() && max == std::numeric_limits<T>::max();
		}
		
		
		DynClampedInt(const T min, const T max) : min(min), max(max), range(max - min), m_val(std::clamp(T(0), min, max)) {}
		DynClampedInt(const T val, const T min, const T max) : min(min), max(max), range(max - min), m_val(std::clamp(val, min, max)) {}
		
		operator T() const volatile { return m_val; }
		
		T val() { return m_val; }
		
		DynClampedInt& operator=(const T val)
		{
			m_val = std::clamp(val, min, max);
			return *this;
		}
		
		volatile DynClampedInt& operator=(const T val) volatile
		{
			m_val = std::clamp(val, min, max);
			return *this;
		}
		
		template <typename E>
		bool operator==(const E val)
		{
			static_assert(is_int_v<E>);
			
			return m_val == val;
		}
		
		DynClampedInt& operator++()	// prefix
		{
			if (m_val >= max)
				m_val = min;
			else
				m_val++;
			
			return *this;
		}
		
		DynClampedInt& operator--()	// prefix
		{
			if (m_val <= min)
				m_val = max;
			else
				m_val--;
			
			return *this;
		}
		
		DynClampedInt operator++(int)	// postfix
		{
			DynClampedInt val = *this;
			
			if (m_val >= max)
				m_val = min;
			else
				m_val++;
			
			return val;
		}
		
		DynClampedInt operator--(int)	// postfix
		{
			DynClampedInt val = *this;
			
			if (m_val <= min)
				m_val = max;
			else
				m_val--;
			
			return val;
		}
		
		volatile DynClampedInt& operator++() volatile	// prefix
		{
			if (m_val >= max)
				m_val = min;
			else
				m_val++;
			
			return *this;
		}
		
		volatile DynClampedInt& operator--() volatile	// prefix
		{
			if (m_val <= min)
				m_val = max;
			else
				m_val--;
			
			return *this;
		}
		
		DynClampedInt operator++(int) volatile	// postfix
		{
			DynClampedInt val = *this;
			
			if (m_val >= max)
				m_val = min;
			else
				m_val++;
			
			return val;
		}
		
		DynClampedInt operator--(int) volatile	// postfix
		{
			DynClampedInt val = *this;
			
			if (m_val <= min)
				m_val = max;
			else
				m_val--;
			
			return val;
		}
		
		template <typename A>
		DynClampedInt& operator+=(A add)
		{
			static_assert(is_int_v<A>);
			
			if (IsUnlimited())
				m_val += add;
			else
			{
				if (add >= 0)
				{
					add %= range + 1;
					
					const uintmax_t rem = max - m_val;	// always <= RANGE
					if (add > rem)
					{
						add -= rem + 1;
						m_val = min;
					}
					
					m_val += add;
				}
				else
				{
					uintmax_t add_pos = static_cast<uintmax_t>((add + 1) * -1) + 1;
					add_pos %= range + 1;
					
					const uintmax_t rem = m_val - min;
					if (add_pos > rem)
					{
						add_pos -= rem + 1;
						m_val = max;
					}
					
					m_val -= add_pos;
				}
			}
			
			return *this;
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
	
	template<typename T>
	inline T pack_be(const void *buf)
	{
		static_assert(is_int_v<T>);
		
		const uint8_t *buf2 = reinterpret_cast<const uint8_t *>(buf);
		
		T val = 0;
		for (size_t i = 0; i < sizeof(T); i++)
		{
			val <<= 8;
			val |= *buf2++;
		}
		
		return val;
	}
	
	template<typename T>
	inline T pack_le(const void *buf)
	{
		static_assert(is_int_v<T>);
		
		if (reinterpret_cast<uintptr_t>(buf) % alignof(T))
		{
			const uint8_t *buf2 = reinterpret_cast<const uint8_t *>(buf);
			
			T val = 0;
			buf2 += sizeof(T);
			for (size_t i = 0; i < sizeof(T); i++)
			{
				val <<= 8;
				val |= *--buf2;
			}
			
			return val;
		}
		else
			return *reinterpret_cast<const T*>(buf);
	}
	
	template<typename T>
	inline void unpack_be(void *buf, T val)
	{
		static_assert(is_int_v<T>);
		
		uint8_t *buf2 = reinterpret_cast<uint8_t *>(buf);
		for (int i = sizeof(T) - 1; i >= 0; i--)
			*buf2++ = val >> (i * 8);
	}
	
	template<typename T>
	inline void unpack_le(void *buf, T val)
	{
		static_assert(is_int_v<T>);
		
		if (reinterpret_cast<uintptr_t>(buf) % alignof(T))
		{
			uint8_t *buf2 = reinterpret_cast<uint8_t *>(buf);
			for (size_t i = 0; i < sizeof(T); i++)
				*buf2++ = val >> (i * 8);
		}
		else
			*reinterpret_cast<T*>(buf) = val;
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

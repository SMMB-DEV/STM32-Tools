#pragma once

#include "main.h"

#include "./Timing.hpp"

#include <vector>
#include <functional>
#include <type_traits>



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

	inline constexpr uint32_t operator"" _u32(unsigned long long x) noexcept
	{
		return x;
	}

	inline constexpr uint64_t operator"" _u64(unsigned long long x) noexcept
	{
		return x;
	}
	
	template<class T, size_t N>
	[[deprecated("Use std::size() instead.")]] inline constexpr size_t _countof(const T (&arr)[N]) noexcept
	{
		return N;
	}
	
	inline constexpr char H2C(uint8_t x)
	{
		constexpr char CHARS[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
		
		return CHARS[x & 0x0F];
	}
	
	inline constexpr char C2H(uint8_t ch)
	{
		constexpr uint8_t HEX[256] =
		{
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  15
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  31
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  47
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  63
			0xFF, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  79
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	//  95
			0xFF, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 111
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,	// 127
			
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		};
		
		return HEX[ch];
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
	inline constexpr T ceil(T p, T q)
	{
		static_assert(is_int_v<T>);
		
		return p / q + (p % q != 0);
	}
	
	template <typename T>
	[[deprecated("Use next_multiple() instead.")]]
	inline constexpr T round_up(T n, T multiple)
	{
		static_assert(is_int_v<T>);
		
		return ceil(n, multiple) * multiple;
	}
	
	template <typename T>
	inline constexpr T next_multiple(T n, T multiple)
	{
		static_assert(is_int_v<T>);
		
		return ceil(n, multiple) * multiple;
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
		
		void Cancel()
		{
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
		
		void Cancel()
		{
			m_end = nullptr;
		}
		
	private:
		funt_t m_end;
	};
	
	template <typename T, T MIN, T MAX>
	class ClampedInt
	{
		static_assert(is_int_v<T> && MAX > MIN);
		
		T m_val;
		
	public:
		[[deprecated("Use VAL_MIN and VAL_MAX instead.")]]
		static constexpr T val_min = MIN, val_max = MAX;
		
		[[deprecated("Use IS_UNLIMITED instead.")]]
		static constexpr bool is_unlimited = MIN == std::numeric_limits<T>::min() && MAX == std::numeric_limits<T>::max();
		
		static constexpr T VAL_MIN = MIN, VAL_MAX = MAX;
		static constexpr uintmax_t RANGE = MAX - MIN;
		static constexpr bool IS_UNLIMITED = MIN == std::numeric_limits<T>::min() && MAX == std::numeric_limits<T>::max();
		
		
		ClampedInt() : m_val(std::clamp(T(0), MIN, MAX)) {}
		ClampedInt(const T val) : m_val(std::clamp(val, MIN, MAX)) {}
		
		T val() const { return m_val; }
		
		operator T() const { return m_val; }
		operator T() const volatile { return m_val; }
		
		ClampedInt& operator=(const T val)
		{
			if constexpr (IS_UNLIMITED)
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
			if constexpr (IS_UNLIMITED)
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
			if constexpr (IS_UNLIMITED)
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
			
			if constexpr (IS_UNLIMITED)
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
			
			if constexpr (IS_UNLIMITED)
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
			if constexpr (IS_UNLIMITED)
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
			if constexpr (IS_UNLIMITED)
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
			
			if constexpr (IS_UNLIMITED)
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
			
			if constexpr (IS_UNLIMITED)
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
			
			if constexpr (IS_UNLIMITED)
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
		
		template <typename A>
		ClampedInt& operator-=(A sub)
		{
			static_assert(is_int_v<A>);
			
			if constexpr (IS_UNLIMITED)
				m_val -= sub;
			else
			{
				if (sub >= 0)
				{
					sub %= RANGE + 1;
					
					const uintmax_t rem = m_val - MIN;	// always <= RANGE
					if (sub > rem)
					{
						sub -= rem + 1;
						m_val = MAX;
					}
					
					m_val -= sub;
				}
				else
				{
					uintmax_t sub_pos = static_cast<uintmax_t>((sub + 1) * -1) + 1;
					sub_pos %= RANGE + 1;
					
					const uintmax_t rem = MAX - m_val;
					if (sub_pos > rem)
					{
						sub_pos -= rem + 1;
						m_val = MIN;
					}
					
					m_val += sub_pos;
				}
			}
			
			return *this;
		}
		
		template <typename A>
		volatile ClampedInt& operator+=(A add) volatile
		{
			static_assert(is_int_v<A>);
			
			if constexpr (IS_UNLIMITED)
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
		
		template <typename A>
		volatile ClampedInt& operator-=(A sub) volatile
		{
			static_assert(is_int_v<A>);
			
			if constexpr (IS_UNLIMITED)
				m_val -= sub;
			else
			{
				if (sub >= 0)
				{
					sub %= RANGE + 1;
					
					const uintmax_t rem = m_val - MIN;	// always <= RANGE
					if (sub > rem)
					{
						sub -= rem + 1;
						m_val = MAX;
					}
					
					m_val -= sub;
				}
				else
				{
					uintmax_t sub_pos = static_cast<uintmax_t>((sub + 1) * -1) + 1;
					sub_pos %= RANGE + 1;
					
					const uintmax_t rem = MAX - m_val;
					if (sub_pos > rem)
					{
						sub_pos -= rem + 1;
						m_val = MIN;
					}
					
					m_val += sub_pos;
				}
			}
			
			return *this;
		}
	};
	
	template <typename T>
	class DynClampedInt
	{
		static_assert(is_int_v<T>);
		
		uintmax_t m_range;
		T m_min, m_max, m_val;
		
	public:
		bool IsUnlimited()
		{
			return m_min == std::numeric_limits<T>::min() && m_max == std::numeric_limits<T>::max();
		}
		
		
		DynClampedInt(const T min, const T max) : DynClampedInt(T(0), min, max) {}
		DynClampedInt(const T val, const T min, const T max) : m_range(max - min), m_min(min), m_max(max), m_val(std::clamp(val, min, max)) {}
		
		T val() const { return m_val; }
		
		operator T() const { return m_val; }
		operator T() const volatile { return m_val; }
		
		DynClampedInt& operator=(const T val)
		{
			m_val = std::clamp(val, m_min, m_max);
			return *this;
		}
		
		volatile DynClampedInt& operator=(const T val) volatile
		{
			m_val = std::clamp(val, m_min, m_max);
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
			if (m_val >= m_max)
				m_val = m_min;
			else
				m_val++;
			
			return *this;
		}
		
		DynClampedInt& operator--()	// prefix
		{
			if (m_val <= m_min)
				m_val = m_max;
			else
				m_val--;
			
			return *this;
		}
		
		DynClampedInt operator++(int)	// postfix
		{
			DynClampedInt val = *this;
			
			if (m_val >= m_max)
				m_val = m_min;
			else
				m_val++;
			
			return val;
		}
		
		DynClampedInt operator--(int)	// postfix
		{
			DynClampedInt val = *this;
			
			if (m_val <= m_min)
				m_val = m_max;
			else
				m_val--;
			
			return val;
		}
		
		volatile DynClampedInt& operator++() volatile	// prefix
		{
			if (m_val >= m_max)
				m_val = m_min;
			else
				m_val++;
			
			return *this;
		}
		
		volatile DynClampedInt& operator--() volatile	// prefix
		{
			if (m_val <= m_min)
				m_val = m_max;
			else
				m_val--;
			
			return *this;
		}
		
		DynClampedInt operator++(int) volatile	// postfix
		{
			DynClampedInt val = *this;
			
			if (m_val >= m_max)
				m_val = m_min;
			else
				m_val++;
			
			return val;
		}
		
		DynClampedInt operator--(int) volatile	// postfix
		{
			DynClampedInt val = *this;
			
			if (m_val <= m_min)
				m_val = m_max;
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
					add %= m_range + 1;
					
					const uintmax_t rem = m_max - m_val;	// always <= RANGE
					if (add > rem)
					{
						add -= rem + 1;
						m_val = m_min;
					}
					
					m_val += add;
				}
				else
				{
					uintmax_t add_pos = static_cast<uintmax_t>((add + 1) * -1) + 1;
					add_pos %= m_range + 1;
					
					const uintmax_t rem = m_val - m_min;
					if (add_pos > rem)
					{
						add_pos -= rem + 1;
						m_val = m_max;
					}
					
					m_val -= add_pos;
				}
			}
			
			return *this;
		}
		
		template <typename A>
		DynClampedInt& operator-=(A sub)
		{
			static_assert(is_int_v<A>);
			
			if constexpr (IsUnlimited())
				m_val -= sub;
			else
			{
				if (sub >= 0)
				{
					sub %= m_range + 1;
					
					const uintmax_t rem = m_val - m_min;	// always <= RANGE
					if (sub > rem)
					{
						sub -= rem + 1;
						m_val = m_max;
					}
					
					m_val -= sub;
				}
				else
				{
					uintmax_t sub_pos = static_cast<uintmax_t>((sub + 1) * -1) + 1;
					sub_pos %= m_range + 1;
					
					const uintmax_t rem = m_max - m_val;
					if (sub_pos > rem)
					{
						sub_pos -= rem + 1;
						m_val = m_min;
					}
					
					m_val += sub_pos;
				}
			}
			
			return *this;
		}
		
		template <typename A>
		volatile DynClampedInt& operator+=(A add) volatile
		{
			static_assert(is_int_v<A>);
			
			if (IsUnlimited())
				m_val += add;
			else
			{
				if (add >= 0)
				{
					add %= m_range + 1;
					
					const uintmax_t rem = m_max - m_val;	// always <= RANGE
					if (add > rem)
					{
						add -= rem + 1;
						m_val = m_min;
					}
					
					m_val += add;
				}
				else
				{
					uintmax_t add_pos = static_cast<uintmax_t>((add + 1) * -1) + 1;
					add_pos %= m_range + 1;
					
					const uintmax_t rem = m_val - m_min;
					if (add_pos > rem)
					{
						add_pos -= rem + 1;
						m_val = m_max;
					}
					
					m_val -= add_pos;
				}
			}
			
			return *this;
		}
		
		template <typename A>
		volatile DynClampedInt& operator-=(A sub) volatile
		{
			static_assert(is_int_v<A>);
			
			if constexpr (IsUnlimited())
				m_val -= sub;
			else
			{
				if (sub >= 0)
				{
					sub %= m_range + 1;
					
					const uintmax_t rem = m_val - m_min;	// always <= RANGE
					if (sub > rem)
					{
						sub -= rem + 1;
						m_val = m_max;
					}
					
					m_val -= sub;
				}
				else
				{
					uintmax_t sub_pos = static_cast<uintmax_t>((sub + 1) * -1) + 1;
					sub_pos %= m_range + 1;
					
					const uintmax_t rem = m_max - m_val;
					if (sub_pos > rem)
					{
						sub_pos -= rem + 1;
						m_val = m_min;
					}
					
					m_val += sub_pos;
				}
			}
			
			return *this;
		}
		
		void SetMax(T max)
		{
			m_max = max;
			m_range = m_max - m_min;
			
			if (m_val > max)
				m_val = max;
		}
		
		void SetMin(T min)
		{
			m_min = min;
			m_range = m_max - m_min;
			
			if (m_val > min)
				m_val = min;
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
	
	template <class T, size_t MAX_SIZE, typename INDEX_T = size_t>
	class StaticQueue
	{
	protected:
		static_assert(std::is_unsigned_v<INDEX_T>);
		using index_t = ClampedInt<INDEX_T, 0, MAX_SIZE - 1>;
		
		T m_items[MAX_SIZE];
		
		// m_front always modified in pop_front() and m_back always modified in push_back().
		volatile index_t m_front = 0, m_back = 0;
		
	public:
		StaticQueue() {}
		~StaticQueue() {}
		
		bool empty() const
		{
			return m_front == m_back;
		}
		
		bool full() const
		{
			index_t end = m_back;
			return (++end).val() == m_front;
		}
		
		void clear()
		{
			m_front = m_back;
		}
		
		size_t size() const
		{
			index_t begin = m_front, end = m_back;
			
			if (end >= begin)
				return end - begin;
			
			return MAX_SIZE - (begin - end);
		}
		
		bool push_back(T&& t)
		{
			if (full())
				return false;
			
			m_items[m_back++] = std::move(t);
			return true;
		}
		
		bool push_back(const T& t)
		{
			if (full())
				return false;
			
			m_items[m_back++] = t;
			return true;
		}
		
		std::optional<T> pop_back()
		{
			if (empty())
				return std::nullopt;
			
			return std::move(m_items[--m_back]);
		}
		
		std::optional<T> pop_front()
		{
			if (empty())
				return std::nullopt;
			
			return std::move(m_items[m_front++]);
		}
		
		size_t erase(size_t count)
		{
			size_t _size = size();
			if (count > _size)
				count = _size;
			
			m_front += count;
			return count;
		}
		
		T& operator[](size_t i)
		{
			index_t index = m_front;
			return m_items[index += i];
		}
	};
	
	template <class T, size_t MAX_SIZE = 256>
	class PriorityQueue : protected StaticQueue<T, MAX_SIZE, uint8_t>
	{
		using parent = StaticQueue<T, MAX_SIZE, uint8_t>;
		
	public:
		using prio_t = uint8_t;
		
	private:
		using typename parent::index_t;
		using parent::m_back;
		using parent::m_front;
		using parent::m_items;
		
		prio_t m_prio[MAX_SIZE] = { 0 };	// 0 means non-existent. Higher number means higher priority.
		
	public:
		PriorityQueue() {}
		~PriorityQueue() {}
		
		using parent::empty;
		using parent::full;
		using parent::clear;
		
		/**
		* @param prio - The priority of the item; the greater the number, the higher the priority. Priority 0 means the item won't be added.
		*/
		void push_back(T&& t, const prio_t prio = 1)
		{
			if (full() || !prio)
				return;
			
			m_prio[m_back] = prio;
			parent::push_back(std::move(t));
		}
		
		/**
		* @param prio - The priority of the item; the greater the number, the higher the priority. Priority 0 means the item won't be added.
		*/
		void push_back(const T& t, const prio_t prio)
		{
			if (full() || !prio)
				return;
			
			m_prio[m_back] = prio;
			parent::push_back(t);
		}
		
		/**
		* @param filter - Should return an optional pair of bools indicating which of the two items should be removed.
		*					If it returns nothing, the search is canceled and the first item is returned, otherwise the search continues.
		*/
		std::optional<T> pop_front(const func<std::optional<std::pair<bool, bool>> (const T&, const T&)>& filter = nullptr)
		{
			const index_t back = m_back, front = m_front;	// For fewer volatile accesses
			
			// Find the first index of the highest priority
			index_t prio_index;
			prio_t max_prio = 0;
			for (index_t i = front; i != back; ++i)
			{
				if (m_prio[i] && m_prio[i] > max_prio)	// Ignore previously handled items
				{
					max_prio = m_prio[i];
					prio_index = i;
				}
			}
			
			if (!max_prio)
			{
				m_front = back;		// Can't call clear(). m_back might have changed.
				return std::nullopt;
			}
			
			if (filter && prio_index != back)
			{
				index_t i = prio_index;
				++i;
				
				for (; i != back; ++i)
				{
					if (m_prio[i] == max_prio)
					{
						const auto opt = filter(m_items[prio_index], m_items[i]);
						if (!opt)
							break;
						
						const auto [rem_first, rem_second] = *opt;
						
						if (rem_first && rem_second)
						{
							m_prio[prio_index] = m_prio[i] = 0;
							return pop_front(filter);
						}
						else if (rem_first)
						{
							m_prio[prio_index] = 0;
							prio_index = i;
						}
						else if (rem_second)
							m_prio[i] = 0;
					}
				}
			}
			
			
			m_prio[prio_index] = 0;
			
			// Remove handled items at the beginning of the list
			for (index_t i = front; i != back; ++i)
			{
				if (m_prio[i])
					break;
				
				++m_front;
			}
			
			return std::move(m_items[prio_index]);
		}
	};
	
	template <typename TIME_T = uint32_t>
	class Filter
	{
		TIME_T (*const cf_cyc)();
		const TIME_T c_minCyc;
		
		TIME_T m_prevCyc = 0, m_prevCyc2 = 0;
		bool m_prevState = false, m_prevState2 = false;
		
	public:
		Filter(TIME_T min_time, TIME_T (* cyc)() = Time::GetCycle) : c_minCyc(min_time), cf_cyc(cyc) {}
		
		/**
		* @retval Duration of the pulse in microseconds or 0 in case of invalid pulse.
		*/
		std::optional<std::pair<TIME_T, bool>> validate(bool state)
		{
			const TIME_T now = cf_cyc();
			
			if (state == m_prevState)
				return std::nullopt;
			
			const TIME_T duration = now - m_prevCyc;
			
			if (duration < c_minCyc)
			{
				m_prevState = m_prevState2;
				m_prevCyc = m_prevCyc2;
				
				return std::pair{duration, false};
			}
			
			m_prevState2 = m_prevState;
			m_prevState = state;
			
			m_prevCyc2 = m_prevCyc;
			m_prevCyc = now;
			
			return std::pair{duration, true};
		}
		
		void reset()
		{
			m_prevCyc = m_prevCyc2 = cf_cyc();
			m_prevState = m_prevState2;
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
	inline size_t unpack_be(void *buf, T val)
	{
		static_assert(is_int_v<T>);
		
		uint8_t *buf2 = reinterpret_cast<uint8_t *>(buf);
		for (int i = sizeof(T) - 1; i >= 0; i--)
			*buf2++ = val >> (i * 8);
		
		return sizeof(T);
	}
	
	template<typename T>
	inline size_t unpack_le(void *buf, T val)
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
		
		return sizeof(T);
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
	
	template <class F, class R, class... Args>
	inline bool Retry(uint8_t retry, const uint32_t delay_ms, F&& _try, R ok, void (* const fail)(), Args&&... args)
	{
		if (_try(std::forward<Args>(args)...) == ok)
			return true;
		
		while (retry--)
		{
			HAL_Delay(delay_ms);
			if (_try(std::forward<Args>(args)...) == ok)
				return true;
		}
		
		if (fail)
			fail();
		
		return false;
	}

	#define RETRY(_retry_count, _delay_ms, _try, _ok, _fail) \
	{ \
		uint8_t _retry = _retry_count; \
		if (_try != _ok) \
		{ \
			bool _success = false; \
			while (_retry--) \
			{ \
				HAL_Delay(_delay_ms); \
				if (_try == _ok) \
				{ \
					_success = true; \
					break; \
				} \
			} \
			\
			if (!_success) \
				_fail; \
		} \
	}
}

using STM32T::_countof;

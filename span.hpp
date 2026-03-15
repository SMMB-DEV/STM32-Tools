#pragma once

#include <cstddef>
#include <limits>
#include <array>
#include <memory>		// pointer_traits



namespace STM32T
{
	template <class Ptr>
	constexpr auto to_address(const Ptr& p) noexcept { return std::pointer_traits<Ptr>::to_address(p); }
	template <class T>
	constexpr T* to_address(T* p) noexcept { return p; }
	
	
	template<class T>
	struct type_identity { using type = T; };
	template< class T >
	using type_identity_t = typename type_identity<T>::type;
	
	
	
	inline constexpr std::size_t dynamic_extent = std::numeric_limits<std::size_t>::max();
	
	template <typename T, std::size_t Extent = dynamic_extent>
	class span
	{
		T *p_data;
		size_t m_size = extent;
		
	public:
		static constexpr std::size_t extent = Extent;
		
		template <typename U>
		class _riterator
		{
			friend class span<T>;
			
			U *p_elem;
			
			_riterator(U *elem) : p_elem(elem) {}
			
		public:
			using value_type = U;
			using difference_type = std::ptrdiff_t;
			using reference = U&;
			using pointer = U*;
			using iterator_category = std::random_access_iterator_tag;
			
			reference operator*() const { return *p_elem; }
			pointer operator->() const { return p_elem; }
			reference operator[](difference_type d) { return *(p_elem + d); }
			
			_riterator& operator++()
			{
				--p_elem;
				return *this;
			}
			
			_riterator operator++(int)
			{
				auto tmp = *this;
				++*this;
				return tmp;
			}
			
			_riterator& operator--()
			{
				++p_elem;
				return *this;
			}
			
			_riterator operator--(int)
			{
				auto tmp = *this;
				--*this;
				return tmp;
			}
			
			_riterator& operator+=(difference_type d)
			{
				p_elem -= d;
				return *this;
			}
			
			_riterator& operator-=(difference_type d)
			{
				p_elem += d;
				return *this;
			}
			
			_riterator operator+(difference_type d)
			{
				_riterator temp = *this;
				return temp += d;
			}
			
			_riterator operator-(difference_type d)
			{
				_riterator temp = *this;
				return temp -= d;
			}
			
			difference_type operator-(const _riterator& other)
			{
				return other.p_elem - p_elem;
			}
			
			bool operator<(const _riterator& other) { return p_elem >= other.p_elem; }
			bool operator<=(const _riterator& other) { return p_elem > other.p_elem; }
			bool operator>(const _riterator& other) { return p_elem <= other.p_elem; }
			bool operator>=(const _riterator& other) { return p_elem < other.p_elem; }
			
			bool operator==(const _riterator& other) const { return p_elem == other.p_elem; }
			bool operator!=(const _riterator& other) const { return p_elem != other.p_elem; }
		};
		
		
		using element_type = T;
		using value_type = std::remove_cv_t<T>;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = const T&;
		
		using iterator = pointer;
		using const_iterator = const_pointer;
		using reverse_iterator = _riterator<T>;
		using const_reverse_iterator = _riterator<const T>;
		
		
		
		
		
		constexpr span() noexcept : p_data(nullptr), m_size(0) { static_assert(extent == 0 || extent == dynamic_extent); }
		
		template <class It>
		explicit (extent != dynamic_extent)
		constexpr span(It first, size_type count) : p_data(to_address(first)), m_size(count) {}
		
		template <class It, class End>
		explicit (extent != dynamic_extent)
		constexpr span(It first, End last) : p_data(to_address(first)), m_size(last - first) {}
		
		template <std::size_t N>
		constexpr span(type_identity_t<element_type> (&arr)[N]) noexcept : p_data(std::data(arr)), m_size(N) {}
		
		template <class U, std::size_t N>
		constexpr span(std::array<U, N>& arr) noexcept : p_data(std::data(arr)), m_size(N) {}
		
		template <class U, std::size_t N>
		constexpr span(const std::array<U, N>& arr) noexcept : p_data(std::data(arr)), m_size(N) {}
		
		explicit (extent != dynamic_extent)
		constexpr span(std::initializer_list<value_type> il) noexcept : p_data(il.begin()), m_size(il.size()) {}
		
		template <class U, std::size_t N>
		explicit (extent != dynamic_extent && N == dynamic_extent)
		constexpr span(const span<U, N>& source) noexcept : p_data(source.data()), m_size(source.size()) {}
		
		constexpr span(const span& other) noexcept = default;
		
		constexpr span& operator=(const span& other) noexcept = default;
		
		
		
		constexpr iterator begin() const noexcept { return iterator(p_data); }
		constexpr const_iterator cbegin() const noexcept { return const_iterator(p_data); }
		constexpr iterator end() const noexcept { return iterator(p_data + m_size); }
		constexpr const_iterator cend() const noexcept { return const_iterator(p_data + m_size); }
		constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(p_data + m_size - 1); }
		constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(p_data + m_size - 1); }
		constexpr reverse_iterator rend() const noexcept { return reverse_iterator(p_data - 1); }
		constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(p_data - 1); }
		
		
		
		constexpr reference front() const { return p_data[0]; }
		constexpr reference back() const { return p_data[m_size - 1]; }
		constexpr reference operator[](size_type idx) const { return p_data[idx]; }
		constexpr pointer data() const noexcept { return p_data; }
		
		
		
		constexpr size_type size() const noexcept { return m_size; }
		constexpr size_type size_bytes() const noexcept { return m_size * sizeof(element_type); }
		constexpr bool empty() const noexcept { return m_size == 0; }
		
		
		
		template <std::size_t Count>
		constexpr span<element_type, Count> first() const { return span(p_data, Count); }
		constexpr span<element_type, dynamic_extent> first(size_type Count) const { return span(p_data, Count); }
		
		template <std::size_t Count>
		constexpr span<element_type, Count> last() const { return span(p_data + m_size - Count, Count); }
		constexpr span<element_type, dynamic_extent> last(size_type Count) const { return span(p_data + m_size - Count, Count); }
		
		constexpr span<element_type, dynamic_extent> subspan(size_type Offset, size_type Count = dynamic_extent) const
		{
			return span(p_data, Count == dynamic_extent ? m_size - Offset : Count);
		}
	};
}

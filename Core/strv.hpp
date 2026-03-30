#pragma once

#include <string_view>
#include <functional>
#include <charconv>



namespace STM32T
{
	using std::operator"" sv;
	
	template <class CharT, class Traits = std::char_traits<CharT>>
	class bstrv : public std::basic_string_view<CharT, Traits>
	{
		using base = std::basic_string_view<CharT, Traits>;
		
		static constexpr base whitespace()
		{
			if constexpr (std::is_same_v<CharT, char>)
				return " \r\n\t\f\v"sv;
			else if constexpr (std::is_same_v<CharT, char16_t>)
				return u" \r\n\t\f\v"sv;
			else if constexpr (std::is_same_v<CharT, char32_t>)
				return U" \r\n\t\f\v"sv;
			else if constexpr (std::is_same_v<CharT, wchar_t>)
				return L" \r\n\t\f\v"sv;
			#if __cplusplus >= 202002L
			else if constexpr (std::is_same_v<CharT, char8_t>)
				return u8" \r\n\t\f\v"sv;
			#endif
			else
				return bstrv();
		}
		
		static constexpr base numbers()
		{
			if constexpr (std::is_same_v<CharT, char>)
				return "0123456789"sv;
			else if constexpr (std::is_same_v<CharT, char16_t>)
				return u"0123456789"sv;
			else if constexpr (std::is_same_v<CharT, char32_t>)
				return U"0123456789"sv;
			else if constexpr (std::is_same_v<CharT, wchar_t>)
				return L"0123456789"sv;
			#if __cplusplus >= 202002L
			else if constexpr (std::is_same_v<CharT, char8_t>)
				return u8"0123456789"sv;
			#endif
			else
				return bstrv();
		}
		
	public:
		static constexpr base WHITESPACE = whitespace(), NUMBERS = numbers();
		
		using base::npos;
		using typename base::const_pointer;
		using typename base::pointer;
		using typename base::size_type;
		using typename base::value_type;
		
		using base::back;
		using base::data;
		
		using base::size;
		using base::empty;
		using base::remove_prefix;
		using base::remove_suffix;
		using base::find;
		using base::rfind;
		using base::find_first_not_of;
		using base::find_last_not_of;
		
		using base::base;
		
		constexpr bstrv(base&& other) : base(std::move(other)) {}
			
		constexpr bstrv substr(size_type pos, size_type count = npos) const { return base::substr(pos, count); }
		
		constexpr bool starts_with(bstrv sv) const noexcept { return bstrv(data(), std::min(size(), sv.size())) == sv; }
		constexpr bool starts_with(CharT ch) const noexcept { return !empty() && Traits::eq(back(), ch); }
		constexpr bool starts_with(const CharT *s) const { return starts_with(bstrv(s)); }
		
		constexpr bool ends_with(bstrv sv) const noexcept { return size() >= sv.size() && compare(size() - sv.size(), npos, sv) == 0; }
		constexpr bool ends_with(CharT ch) const noexcept { return !empty() && Traits::eq(back(), ch); }
		constexpr bool ends_with(const CharT *s) const { return ends_with(bstrv(s)); }
		
		constexpr bool contains(bstrv sv) const noexcept { return find(sv) != npos; }
		constexpr bool contains(CharT c) const noexcept { return find(c) != npos; }
		constexpr bool contains(const CharT *s) const { return find(s) != npos; }
		
		constexpr bool rcontains(bstrv sv) const noexcept { return rfind(sv) != npos; }
		constexpr bool rcontains(CharT c) const noexcept { return rfind(c) != npos; }
		constexpr bool rcontains(const CharT *s) const { return rfind(s) != npos; }
		
		[[deprecated("Use a tokenizing method that doesn't null-terminate the tokens.")]]
		void tokenize(const bstrv sep, std::vector<bstrv>& tokens, const bool ignoreSingleEnded, size_t (bstrv::*f_find)(base, size_t) const = &base::find) const
		{
			// Assuming view is null-terminated.
			
			bstrv view = *this;
			const const_pointer start = view.data();
			
			while (!view.empty())
			{
				size_t end = (view.*f_find)(sep, 0);
				if (end == npos)
				{
					if (!ignoreSingleEnded)
						tokens.push_back(view);		// This is why the view must be null-terminated.
				
					return;
				}
				
				if ((view.data() != start || !ignoreSingleEnded) && end > 0)	// No empty tokens
				{
					((pointer)view.data())[end] = '\0';	// No problems with C string functions. todo: use std::span
					tokens.push_back(view.substr(0, end));
				}
				
				view.remove_prefix(end + sep.size());
			}
		}
		
		void tokenize2(const bstrv sep, std::vector<bstrv>& tokens, const bool ignoreSingleEnded) const
		{
			bstrv view = *this;
			const const_pointer start = view.data();
			
			while (!view.empty())
			{
				size_t end = view.find(sep, 0);
				if (end == npos)
				{
					if (!ignoreSingleEnded)
						tokens.push_back(view);
				
					return;
				}
				
				if ((view.data() != start || !ignoreSingleEnded) && end > 0)	// No empty tokens
					tokens.push_back(view.substr(0, end));
				
				view.remove_prefix(end + sep.size());
			}
		}
		
		[[deprecated("Use a tokenizing method that doesn't null-terminate the tokens.")]]
		void tokenize(const bstrv sep, const std::function<void (bstrv)>& op, const bool ignoreSingleEnded, size_t (bstrv::*f_find)(base, size_t) const = &base::find) const
		{
			// note: Assuming view is null-terminated.
			
			bstrv view = *this;
			const const_pointer start = view.data();
			
			while (!view.empty())
			{
				size_t end = (view.*f_find)(sep, 0);
				if (end == npos)
				{
					if (!ignoreSingleEnded)
						op(view);
				
					return;
				}
				
				if ((view.data() != start || !ignoreSingleEnded) && end > 0)	// No empty tokens
				{
					((pointer)view.data())[end] = '\0';	// No problems with C string functions
					op(view.substr(0, end));
				}
				
				view.remove_prefix(end + sep.size());
			}
		}
		
		void tokenize2(const bstrv sep, const std::function<void (bstrv)>& op, const bool ignoreSingleEnded) const
		{
			bstrv view = *this;
			const const_pointer start = view.data();
			
			while (!view.empty())
			{
				size_t end = view.find(sep, 0);
				if (end == npos)
				{
					if (!ignoreSingleEnded)
						op(view);
				
					return;
				}
				
				if ((view.data() != start || !ignoreSingleEnded) && end > 0)	// No empty tokens
					op(view.substr(0, end));
				
				view.remove_prefix(end + sep.size());
			}
		}
		
		/**
		* @note If op returns true, this function returns.
		*/
		[[deprecated("Use a tokenizing method that doesn't null-terminate the tokens.")]]
		void tokenize(const bstrv sep, const std::function<bool (bstrv)>& op, const bool ignoreSingleEnded, size_t (bstrv::*f_find)(base, size_t) const = &base::find) const
		{
			// note: Assuming view is null-terminated.
			
			bstrv view = *this;
			const const_pointer start = view.data();
			
			while (!view.empty())
			{
				size_t end = (view.*f_find)(sep, 0);
				if (end == npos)
				{
					if (!ignoreSingleEnded)
						op(view);
				
					return;
				}
				
				if ((view.data() != start || !ignoreSingleEnded) && end > 0)	// No empty tokens
				{
					((pointer)view.data())[end] = '\0';	// No problems with C string functions
					if (op(view.substr(0, end)))
						return;
				}
				
				view.remove_prefix(end + sep.size());
			}
		}
		
		void tokenize2(const bstrv sep, const std::function<bool (bstrv)>& op, const bool ignoreSingleEnded) const
		{
			bstrv view = *this;
			const const_pointer start = view.data();
			
			while (!view.empty())
			{
				size_t end = view.find(sep, 0);
				if (end == npos)
				{
					if (!ignoreSingleEnded)
						op(view);
				
					return;
				}
				
				if ((view.data() != start || !ignoreSingleEnded) && end > 0)	// No empty tokens
					if (op(view.substr(0, end)))
						return;
				
				view.remove_prefix(end + sep.size());
			}
		}
		
		size_t tokenize(const bstrv delim, bstrv *tokens, size_t max_tokens, const bool end_req, const bool add_empty = true) const
		{
			size_t idx = 0, start = 0, end = find(delim, start);
			while (end != npos && idx < max_tokens)
			{
				const size_t new_len = end - start;
				if (new_len || add_empty)
					tokens[idx++] = {data() + start, new_len};
				
				start = end + delim.size();
				end = find(delim, start);
			}
			
			if (!end_req && (start < size() || add_empty) && idx < max_tokens)
				tokens[idx++] = {data() + start, size() - start};
			
			return idx;
		}
		
		bstrv& ltrim(base trimChars = WHITESPACE)
		{
			remove_prefix(std::min(find_first_not_of(trimChars), size()));
			return *this;
		}
		
		bstrv& rtrim(base trimChars = WHITESPACE)
		{
			remove_suffix(size() - std::min(find_last_not_of(trimChars) + 1, size()));
			return *this;
		}
		
		bstrv& trim(base trimChars = WHITESPACE)
		{
			return ltrim(trimChars).rtrim(trimChars);
		}
		
		bool remove_prefix(base prefix)
		{
			if (starts_with(prefix))
			{
				remove_prefix(prefix.size());
				return true;
			}

			return false;
		}
		
		bool remove_prefix(CharT ch)
		{
			if (starts_with(ch))
			{
				remove_prefix(1);
				return true;
			}

			return false;
		}
		bool remove_prefix(const CharT *s) { return remove_prefix(bstrv(s)); }
		
		bool remove_suffix(base suffix)
		{
			if (ends_with(suffix))
			{
				remove_suffix(suffix.size());
				return true;
			}

			return false;
		}
		
		bool remove_suffix(CharT ch)
		{
			if (ends_with(ch))
			{
				remove_suffix(1);
				return true;
			}

			return false;
		}
		
		bool remove_suffix(const CharT *s) { return remove_suffix(bstrv(s)); }
		
		[[deprecated("Use remove_prefix(strv) instead.")]]
		bool compare_remove_prefix(base remove)
		{
			return remove_prefix(remove);
		}
		
		[[deprecated("Use remove_suffix(strv) instead.")]]
		bool compare_remove_suffix(base remove)
		{
			return remove_suffix(remove);
		}
		
		[[deprecated("Use remove_prefix(strv) instead.")]]
		bool compare_remove(base remove)
		{
			return compare_remove_prefix(remove);
		}
		
		
		
		template <typename N>
		size_t to_num(N& num, size_t from = 0, size_t count = npos) const
		{
			static_assert(std::is_arithmetic_v<N>);
			
			bstrv sub = substr(from, count).trim();
			sub.remove_prefix(CharT('+'));
			
			if (!sub.empty())
			{
				const std::from_chars_result result = std::from_chars(sub.data(), sub.data() + sub.size(), num);
				if (result.ec == std::errc())
					return result.ptr - sub.data();
			}
			
			return 0;
		}
		
		template <typename I>
		[[deprecated("Use to_num() instead.")]]
		size_t ExtractInteger(I& integer, size_t from = 0, size_t count = npos) const { return to_num<I>(integer, from, count); }
	};
	
	using strv		= bstrv<std::string_view::value_type>;
	using u16strv	= bstrv<std::u16string_view::value_type>;
	using u32strv	= bstrv<std::u32string_view::value_type>;
	using wstrv		= bstrv<std::wstring_view::value_type>;
	
	constexpr strv		operator ""_sv(const char* str, std::size_t len) noexcept		{ return strv(str, len); }
	constexpr u16strv	operator ""_sv(const char16_t* str, std::size_t len) noexcept	{ return u16strv(str, len); }
	constexpr u32strv	operator ""_sv(const char32_t* str, std::size_t len) noexcept	{ return u32strv(str, len); }
	constexpr wstrv		operator ""_sv(const wchar_t* str, std::size_t len) noexcept	{ return wstrv(str, len); }
	
	#if __cplusplus >= 202002L
	using u8strv	= bstrv<std::u8string_view::value_type>;
	constexpr u8strv	operator ""_sv(const char8_t* str, std::size_t len) noexcept 	{ return u8strv(str, len); }
	#endif
}

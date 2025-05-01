#pragma once

#include <string_view>
#include <functional>



namespace STM32T
{
	using std::operator"" sv;
	
	template <class CharT>
	class bstrv : public std::basic_string_view<CharT>
	{
		using base = std::basic_string_view<CharT>;
		
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
		
		using base::size;
		using base::remove_prefix;
		using base::remove_suffix;
		using base::find_first_not_of;
		using base::find_last_not_of;
		using base::substr;
		
		using base::base;
		
		constexpr bstrv(base&& other) : base(std::move(other)) {}
		
		void tokenize(const bstrv sep, std::vector<bstrv>& tokens, const bool ignoreSingleEnded, size_t (bstrv::*f_find)(base, size_t) const = &base::find) const
		{
			// Assuming view is null-terminated.
			
			bstrv view = *this;
			const const_pointer start = view.data();
			
			while (view.size() > 0)
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
		
		void tokenize(const bstrv sep, const std::function<bool (bstrv)>& op, const bool ignoreSingleEnded, size_t (bstrv::*f_find)(base, size_t) const = &base::find) const
		{
			// note: Assuming view is null-terminated.
			// todo: Handle {sep} inside string literals
			
			bstrv view = *this;
			const const_pointer start = view.data();
			
			while (view.size() > 0)
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
		
		bool compare_remove_prefix(base remove)
		{
			if (base::compare(0, remove.size(), remove) == 0)
			{
				remove_prefix(remove.size());
				return true;
			}

			return false;
		}
		
		bool compare_remove_suffix(base remove)
		{
			if (size() >= remove.size() && base::compare(size() - remove.size(), remove.size(), remove) == 0)
			{
				remove_suffix(remove.size());
				return true;
			}

			return false;
		}
		
		[[deprecated("Use compare_remove_prefix() instead.")]]
		bool compare_remove(base remove)
		{
			return compare_remove_prefix(remove);
		}
	};
	
	using strv = bstrv<std::string_view::value_type>;
	using u16strv = bstrv<std::u16string_view::value_type>;
	using u32strv = bstrv<std::u32string_view::value_type>;
	using wstrv = bstrv<std::wstring_view::value_type>;
	
	#if __cplusplus >= 202002L
	using u8strv = bstrv<std::u8string_view::value_type>;
	#endif
}

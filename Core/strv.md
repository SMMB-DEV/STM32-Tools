# strv.hpp

STM32T::bstrv is a subclass of std::basic_string_view with the following typedefs:

- `strv = bstrv<char>` -> `std::string_view`
- `wstrv = bstrv<wchar_t>` -> `std::wstring_view`
- `u8strv = bstrv<char8_t>` -> `std::u8string_view` (available only if C++20 or later is used)
- `u16strv = bstrv<char16_t>` -> `std::u16string_view`
- `u32strv = bstrv<char32_t>` -> `std::u32string_view`

`std::basic_string_view` is implicitly convertible to `STM32T::bstrv` but you can use `STM32T::operator"" _sv` to explicitly create a `bstrv`.

### Extra functionality (compared to `std::basic_string_view`):

- Tokenizing
- Trimming
- Conditionally removing prefixes and suffixes if they match an exact view
- Converting view to an integer

---

##### [Go Back](./README.md)

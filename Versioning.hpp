#pragma once

#include "./Common.hpp"

#include <charconv>



namespace STM32T
{
	class Version
	{
		shared_arr<uint32_t> m_data;
		
		Version(shared_arr<uint32_t> data) : m_data(data) {}
		
	public:
		enum PreRelease : uint8_t { Unspecified = 0, Alpha = 16, Beta = 64, RC = 128, Normal = 255 };
		
		/**
		* @brief Constructs an invald Version.
		*/
		Version() : m_data{ 0 } {}
		Version(const uint32_t val) : m_data{.val = val} {}
		constexpr Version(const uint8_t major, const uint8_t minor, const uint8_t patch, const PreRelease pr = Unspecified) : m_data{pr, patch, minor, major} {}
		
		Version(const Version& other) = default;
		Version(const Version& other, PreRelease new_pr) : m_data(other.m_data)
		{
			m_data.arr[0] = new_pr;
		}
		
		Version& operator=(const Version& other)
		{
			m_data = other.m_data;
			return *this;
		}
		
		explicit operator uint32_t() const
		{
			return m_data.val;
		}
		
		uint8_t Major() const
		{
			return m_data.arr[3];
		}
		
		uint8_t Minor() const
		{
			return m_data.arr[2];
		}
		
		uint8_t Patch() const
		{
			return m_data.arr[1];
		}
		
		constexpr uint8_t PreRelease() const
		{
			return m_data.arr[0];
		}
		
		constexpr bool Valid() const
		{
			const auto pr = PreRelease();
			return pr == Alpha || pr == Beta || pr == RC || pr == Normal;
		}
		
		bool Complete() const
		{
			return PreRelease();
		}
		
		bool operator==(const Version& other) const
		{
			constexpr uint32_t NO_PR_MASK = 0xFFFF'FF00;
			
			if (Complete() && other.Complete())
				return m_data.val == other.m_data.val;
			
			return (m_data.val & NO_PR_MASK) == (other.m_data.val & NO_PR_MASK);
		}
		
		bool operator<(const Version& other) const
		{
			if (Major() != other.Major())
				return Major() < other.Major();
			
			if (Minor() != other.Minor())
				return Minor() < other.Minor();
			
			if (Patch() != other.Patch())
				return Patch() < other.Patch();
			
			if (Complete() && other.Complete())
				return PreRelease() < other.PreRelease();
			
			return false;
		}
		
		bool operator<=(const Version& other) const
		{
			if (Major() != other.Major())
				return Major() < other.Major();
			
			if (Minor() != other.Minor())
				return Minor() < other.Minor();
			
			if (Patch() != other.Patch())
				return Patch() < other.Patch();
			
			if (Complete() && other.Complete())
				return PreRelease() <= other.PreRelease();
			
			return true;
		}
		
		bool operator>(const Version& other) const { return !operator<=(other); }
		
		bool operator>=(const Version& other) const { return !operator<(other); }
		
		bool operator!=(const Version& other) const { return !operator==(other); }
		
		operator strv() const
		{
			static char buf[20];
			
			int len = sprintf(buf, "%hhu.%hhu.%hhu", m_data.arr[3], m_data.arr[2], m_data.arr[1]);
			switch (PreRelease())
			{
				case Unspecified:
				{
					strcpy(buf + len, "-x");
					len += 2;
					break;
				}
				
				case Alpha:
				{
					strcpy(buf + len, "-alpha");
					len += 6;
					break;
				}
				
				case Beta:
				{
					strcpy(buf + len, "-beta");
					len += 5;
					break;
				}
				
				case RC:
				{
					strcpy(buf + len, "-rc");
					len += 3;
					break;
				}
				
				case Normal:
					break;
				
				default:
					strcpy(buf + len, "-inv");
					len += 4;
			}
			
			return strv(buf, len);
		}
		
		static Version from_strv(strv ver)
		{
			shared_arr<uint32_t> data;
			
			constexpr char SEP[] = { '.', '.', '-' };
			
			for (uint8_t i = 0; i < 3; i++)
			{
				size_t sep_idx = ver.find(SEP[i]);
				if (sep_idx == strv::npos)
				{
					// Don't return if it's the patch number
					if (i < 2)
						return Version();
					else
						sep_idx = ver.size();
				}
				
				uint8_t val;
				auto [_, ec] = std::from_chars(ver.data(), ver.data() + sep_idx, val, 10);
				
				if (ec == std::errc::invalid_argument || ec == std::errc::result_out_of_range)
					return Version();
				
				data.arr[sizeof(data) - 1 - i] = val;
				ver.remove_prefix(std::min(sep_idx + 1, ver.size()));
			}
			
			if (ver == "alpha"sv)
				data.arr[0] = Alpha;
			else if (ver == "beta"sv)
				data.arr[0] = Beta;
			else if (ver == "rc"sv)
				data.arr[0] = RC;
			else if (ver == "x"sv)
				data.arr[0] = Unspecified;
			else if (ver.empty())
				data.arr[0] = Normal;
			else
				return Version();
			
			return Version(data);
		}
		
		void Next()
		{
			switch (PreRelease())
			{
				case Alpha:
					m_data.arr[0] = Beta;
					break;
				
				case Beta:
					m_data.arr[0] = RC;
					break;
				
				case RC:
					m_data.arr[0] = Normal;
					break;
				
				case Normal:
				{
					m_data.arr[0] = Alpha;
					
					for (uint8_t i = 1; i < 4; i++)
					{
						m_data.arr[i]++;
						if (m_data.arr[i] != 0)
							break;
					}
					
					break;
				}
				
				case Unspecified:
				default:
					m_data.arr[0] = Alpha;
					break;
			}
		}
	};
}

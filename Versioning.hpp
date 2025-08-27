#pragma once

#include "./Common.hpp"
#include "./strv.hpp"

#include <charconv>


#ifdef MAX
#undef MAX
#endif


namespace STM32T
{
	class Version
	{
		static inline char s_buf_strv[20];
		
		shared_arr<uint32_t> m_data;
		
		Version(shared_arr<uint32_t> data) : m_data(data) {}
		
	public:
		enum PreRelease : uint8_t
		{
			Unspecified = 0,
			
			Alpha = 32,
			Alpha0 = Alpha, Alpha1, Alpha2, Alpha3, Alpha4, Alpha5, Alpha6, Alpha7,
			Alpha8, Alpha9, Alpha10, Alpha11, Alpha12, Alpha13, Alpha14, Alpha15,
			Alpha16, Alpha17, Alpha18, Alpha19, Alpha20, Alpha21, Alpha22, Alpha23,
			Alpha24, Alpha25, Alpha26, Alpha27, Alpha28, Alpha29, Alpha30, Alpha31,
			
			Beta = 64,
			Beta0 = Beta, Beta1, Beta2, Beta3, Beta4, Beta5, Beta6, Beta7,
			Beta8, Beta9, Beta10, Beta11, Beta12, Beta13, Beta14, Beta15,
			Beta16, Beta17, Beta18, Beta19, Beta20, Beta21, Beta22, Beta23,
			Beta24, Beta25, Beta26, Beta27, Beta28, Beta29, Beta30, Beta31,
			
			RC = 96,
			RC0 = RC, RC1, RC2, RC3, RC4, RC5, RC6, RC7,
			RC8, RC9, RC10, RC11, RC12, RC13, RC14, RC15,
			RC16, RC17, RC18, RC19, RC20, RC21, RC22, RC23,
			RC24, RC25, RC26, RC27, RC28, RC29, RC30, RC31,
			
			Normal = 255
		};
		
		static constexpr Version MAX()
		{
			return Version(UINT32_MAX);
		}
		
		/**
		* @brief Constructs an invald Version.
		*/
		Version() : m_data{ 0 } {}
		constexpr Version(const uint32_t val) : m_data{.val = val} {}
		constexpr Version(const uint8_t major, const uint8_t minor, const uint8_t patch, const PreRelease pr = Unspecified) : m_data{pr, patch, minor, major} {}
		
		constexpr Version(const Version& other) = default;
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
		
		constexpr uint8_t Major() const
		{
			return m_data.arr[3];
		}
		
		constexpr uint8_t Minor() const
		{
			return m_data.arr[2];
		}
		
		constexpr uint8_t Patch() const
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
			return (pr >= Alpha0 && pr <= RC31) || pr == Normal;
		}
		
		constexpr bool Complete() const
		{
			return PreRelease();
		}
		
		constexpr bool IsAlpha() const
		{
			return PreRelease() >= Alpha0 && PreRelease() <= Alpha31;
		}
		
		constexpr bool IsBeta() const
		{
			return PreRelease() >= Beta0 && PreRelease() <= Beta31;
		}
		
		constexpr bool IsRC() const
		{
			return PreRelease() >= RC0 && PreRelease() <= RC31;
		}
		
		constexpr bool IsNormal() const
		{
			return PreRelease() == Normal;
		}
		
		constexpr bool operator==(const Version& other) const
		{
			constexpr uint32_t NO_PR_MASK = 0xFFFF'FF00;
			
			if (Complete() && other.Complete())
				return m_data.val == other.m_data.val;
			
			return (m_data.val & NO_PR_MASK) == (other.m_data.val & NO_PR_MASK);
		}
		
		constexpr bool operator<(const Version& other) const
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
		
		constexpr bool operator<=(const Version& other) const
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
		
		constexpr bool operator>(const Version& other) const { return !operator<=(other); }
		
		constexpr bool operator>=(const Version& other) const { return !operator<(other); }
		
		constexpr bool operator!=(const Version& other) const { return !operator==(other); }
		
		operator strv() const
		{
			int len = sprintf(s_buf_strv, "%hhu.%hhu.%hhu", Major(), Minor(), Patch()), len2;
			
			const uint8_t pr = PreRelease();
			
			if ((pr > 0 && pr < Alpha0) || (pr > RC31 && pr < Normal))
				len2 = sprintf(s_buf_strv + len, "-inv");
			else if (pr == Normal)
				len2 = 0;
			else if (pr >= RC0)
				len2 = sprintf(s_buf_strv + len, "-rc%hhu", pr - RC0);
			else if (pr >= Beta0)
				len2 = sprintf(s_buf_strv + len, "-beta%hhu", pr - Beta0);
			else if (pr >= Alpha0)
				len2 = sprintf(s_buf_strv + len, "-alpha%hhu", pr - Alpha0);
			else
				len2 = sprintf(s_buf_strv + len, "-x");
			
			len += len2;
			
			return strv(s_buf_strv, len);
		}
		
		
		operator wstrv() const
		{
			static wchar_t s_buf[20];
			
			int len = swprintf(s_buf, _countof(s_buf), L"%hhu.%hhu.%hhu", Major(), Minor(), Patch()), len2;
			
			const uint8_t pr = PreRelease();
			
			if ((pr > 0 && pr < Alpha0) || (pr > RC31 && pr < Normal))
				len2 = swprintf(s_buf + len, _countof(s_buf) - len, L"-inv");
			else if (pr == Normal)
				len2 = 0;
			else if (pr >= RC0)
				len2 = swprintf(s_buf + len, _countof(s_buf) - len, L"-rc%hhu", pr - RC0);
			else if (pr >= Beta0)
				len2 = swprintf(s_buf + len, _countof(s_buf) - len, L"-beta%hhu", pr - Beta0);
			else if (pr >= Alpha0)
				len2 = swprintf(s_buf + len, _countof(s_buf) - len, L"-alpha%hhu", pr - Alpha0);
			else
				len2 = swprintf(s_buf + len, _countof(s_buf) - len, L"-x");
			
			len += len2;
			
			return wstrv(s_buf, len);
		}
		
		strv NormalName() const
		{
			const int len = sprintf(s_buf_strv, "%hhu.%hhu.%hhu", Major(), Minor(), Patch());
			
			return strv(s_buf_strv, len);
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
				const std::from_chars_result result = std::from_chars(ver.data(), ver.data() + sep_idx, val);
				
				if (result.ec != std::errc())
					return Version();
				
				data.arr[sizeof(data) - 1 - i] = val;
				ver.remove_prefix(std::min(sep_idx + 1, ver.size()));
			}
			
			if (ver.remove_prefix("alpha"sv))
				data.arr[0] = Alpha0;
			else if (ver.remove_prefix("beta"sv))
				data.arr[0] = Beta0;
			else if (ver.remove_prefix("rc"sv))
				data.arr[0] = RC0;
			else if (ver == "x"sv)
				data.arr[0] = Unspecified;
			else if (ver.empty())
				data.arr[0] = Normal;
			else
				return Version();
			
			if (data.arr[0] != Unspecified && data.arr[0] != Normal && !ver.empty())
			{
				uint8_t val;
				const std::from_chars_result result = std::from_chars(ver.data(), ver.data() + ver.size(), val);
				
				if (result.ec != std::errc() || val > 31)
					return Version();
				
				data.arr[0] += val;
			}
			
			return Version(data);
		}
		
		void Next()
		{
			const uint8_t pr = PreRelease();
			
			if (pr == Normal)
			{
				m_data.arr[0] = Alpha0;
				
				for (uint8_t i = 1; i < 4; i++)
				{
					m_data.arr[i]++;
					if (m_data.arr[i] != 0)
						break;
				}
			}
			else if (pr >= Alpha0 && pr < RC31)
				m_data.arr[0]++;
			else if (pr == RC31)
				m_data.arr[0] = Normal;
			else
				m_data.arr[0] = Alpha;
		}
		
		void NextNormal()
		{
			m_data.arr[0] = Normal;
			
			for (uint8_t i = 1; i < 4; i++)
			{
				m_data.arr[i]++;
				if (m_data.arr[i] != 0)
					break;
			}
		}
	};
}

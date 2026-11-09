/**
* @file
* @brief A simplified implementation of [Semantic Versioning](https://semver.org)
* # Simplified Semantic Versioning 1.0.0
*
* This versioning scheme is mostly identical to [Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html) with limited major, minor, and patch versions (each max. 255) and fewer and more strict [pre-release identifiers](https://semver.org/spec/v2.0.0.html#spec-item-9). [Build metadata](https://semver.org/spec/v2.0.0.html#spec-item-10) cannot be displayed using this format.
*
* If a patch version reaches 255, the next version will increment the minor version and reset the patch to 0 even if it's not actually a minor release. Similarly, after a minor version of 255, the major version is incremented and the minor and patch versions are reset to 0. If the major version reaches 255, maybe contemplate why you need 256 major versions.
*
* ## Storage
*
* The version can be stored (and transferred) as a 32-bit number divided into four 8-bit sections. Each section holds part of the version information.
* 
* |	8 (MSB)	|	8		|	8		|	8 (LSB)					|
* |-----------|-----------|-----------|---------------------------|
* |	Major	|	Minor	|	Patch	|	Pre-release identifier	|
*
* This makes comparing two versions as easy as comparing two 32-bit numbers while taking into account their precedence.
*
* ## Pre-release identifiers
*
* These are the **only** valid pre-release identifiers and their associated numeric value:
* 
* |	Value	|	Text Form		|	Description							|
* |-----------|-------------------|---------------------------------------|
* |	255		|					|	Normal version (not pre-release)	|
* |	97-127	|	-rc<b>x</b>		|	Release candidate - x from 1 to 31	|
* |	96		|	-rc, -rc0		|	Release candidate					|
* |	65-95	|	-beta<b>x</b>	|	Beta version - x from 1 to 31		|
* |	64		|	-beta, -beta0	|	Beta version						|
* |	33-63	|	-alpha<b>x</b>	|	Alpha version - x from 1 to 31		|
* |	32		|	-alpha, -alpha0	|	Alpha version						|
* |	0		|	-x				|	Unspecified (not a valid pre-release but when comparing versions, it means that pre-release is not considered)	|
*
* The pre-release usually goes from _-alpha_ to _-beta_ and then to _-rc_ and finally to a normal version but **it can skip any of these states**.
*/

#pragma once

#include "./Core/strv.hpp"

#include <charconv>



namespace STM32T
{
	class Version
	{
		static inline char s_buf_strv[20];
		
		uint8_t m_data[4];
		
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
		
		static constexpr Version Max()
		{
			return Version(UINT32_MAX);
		}
		
		/**
		* @brief Constructs an invald Version.
		*/
		constexpr Version() : m_data{} {}
		constexpr Version(const uint32_t val) : m_data{ uint8_t(val), uint8_t(val >> 8), uint8_t(val >> 16), uint8_t(val >> 24) } {}
		constexpr Version(uint8_t major, uint8_t minor, uint8_t patch, PreRelease pr = Unspecified) : m_data{pr, patch, minor, major} {}
		constexpr Version(const uint8_t (&arr)[4]) : m_data{arr[3], arr[2], arr[1], arr[0]} {}
		
		constexpr Version(const Version& other) : m_data{other.m_data[0], other.m_data[1], other.m_data[2], other.m_data[3]} {}
		
		Version(const Version& other, PreRelease new_pr) : m_data{new_pr, other.m_data[1], other.m_data[2], other.m_data[3]} {}
		
		Version& operator=(const Version& other)
		{
			memcpy(m_data, other.m_data, sizeof(m_data));
			return *this;
		}
		
		constexpr explicit operator uint32_t() const
		{
			return (Major() << 24) | (Minor() << 16) | (Patch() << 8) | PreRelease();
		}
		
		[[gnu::always_inline]] constexpr uint8_t Major() const { return m_data[3]; }
		[[gnu::always_inline]] constexpr uint8_t Minor() const { return m_data[2]; }
		[[gnu::always_inline]] constexpr uint8_t Patch() const { return m_data[1]; }
		[[gnu::always_inline]] constexpr uint8_t PreRelease() const { return m_data[0]; }
		
		constexpr bool Valid() const
		{
			const auto pr = PreRelease();
			return (pr >= Alpha0 && pr <= RC31) || pr == Normal;
		}
		
		constexpr bool Complete() const
		{
			return PreRelease();
		}
		
		constexpr bool IsAlpha() const { return PreRelease() >= Alpha0 && PreRelease() <= Alpha31; }
		
		constexpr bool IsBeta() const { return PreRelease() >= Beta0 && PreRelease() <= Beta31; }
		
		constexpr bool IsRC() const { return PreRelease() >= RC0 && PreRelease() <= RC31; }
		
		constexpr bool IsNormal() const { return PreRelease() == Normal; }
		
		constexpr bool operator==(const Version& other) const
		{
			constexpr uint32_t NO_PR_MASK = 0xFFFF'FF00;
			
			const uint32_t t = uint32_t(*this), o = uint32_t(other);
			
			if (Complete() && other.Complete())
				return t == o;
			
			return (t & NO_PR_MASK) == (o & NO_PR_MASK);
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
			int len = sprintf(s_buf_strv, "%hhu.%hhu.%hhu", Major(), Minor(), Patch());
			
			const uint8_t pr = PreRelease();
			
			if ((pr > 0 && pr < Alpha) || (pr > RC31 && pr < Normal))
				len += sprintf(s_buf_strv + len, "-inv");
			else if (pr == Normal)
				len += 0;
			else if (pr >= Alpha)
			{
				uint8_t n;
				if (pr >= RC)
				{
					len += sprintf(s_buf_strv + len, "-rc");
					n = pr - RC0;
				}
				else if (pr >= Beta)
				{
					len += sprintf(s_buf_strv + len, "-beta");
					n = pr - Beta0;
				}
				else
				{
					len += sprintf(s_buf_strv + len, "-alpha");
					n = pr - Alpha0;
				}
				
				if (n)
					len += sprintf(s_buf_strv + len, "%hhu", n);
			}
			else
				len += sprintf(s_buf_strv + len, "-x");
			
			return strv(s_buf_strv, len);
		}
		
		const char * c_str() const
		{
			return operator strv().data();
		}
		
		operator wstrv() const
		{
			static wchar_t s_buf[20];
			
			int len = swprintf(s_buf, std::size(s_buf), L"%hhu.%hhu.%hhu", Major(), Minor(), Patch());
			
			const uint8_t pr = PreRelease();
			
			if ((pr > 0 && pr < Alpha0) || (pr > RC31 && pr < Normal))
				len += swprintf(s_buf + len, std::size(s_buf) - len, L"-inv");
			else if (pr == Normal)
				len += 0;
			else if (pr >= Alpha)
			{
				uint8_t n;
				if (pr >= RC)
				{
					len += swprintf(s_buf + len, std::size(s_buf) - len, L"-rc");
					n = pr - RC0;
				}
				else if (pr >= Beta)
				{
					len += swprintf(s_buf + len, std::size(s_buf) - len, L"-beta");
					n = pr - Beta0;
				}
				else
				{
					len += swprintf(s_buf + len, std::size(s_buf) - len, L"-alpha");
					n = pr - Alpha0;
				}
				
				if (n)
					len += swprintf(s_buf + len, std::size(s_buf) - len, L"%hhu", n);
				
			}
			else
				len += swprintf(s_buf + len, std::size(s_buf) - len, L"-x");
			
			return wstrv(s_buf, len);
		}
		
		strv NormalName() const
		{
			const int len = sprintf(s_buf_strv, "%hhu.%hhu.%hhu", Major(), Minor(), Patch());
			
			return strv(s_buf_strv, len);
		}
		
		static Version from_strv(strv ver)
		{
			uint8_t arr[4];
			
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
				
				// todo: use strv::to_num
				uint8_t val;
				const std::from_chars_result result = std::from_chars(ver.data(), ver.data() + sep_idx, val);
				
				if (result.ec != std::errc())
					return Version();
				
				arr[sizeof(arr) - 1 - i] = val;
				ver.remove_prefix(std::min(sep_idx + 1, ver.size()));
			}
			
			if (ver.remove_prefix("alpha"sv))
				arr[0] = Alpha0;
			else if (ver.remove_prefix("beta"sv))
				arr[0] = Beta0;
			else if (ver.remove_prefix("rc"sv))
				arr[0] = RC0;
			else if (ver == "x"sv)
				arr[0] = Unspecified;
			else if (ver.empty())
				arr[0] = Normal;
			else
				return Version();
			
			if (!ver.empty() && arr[0] != Unspecified && arr[0] != Normal)
			{
				uint8_t val;
				if (!ver.to_num_raw(val) || val > 31)
					return Version();
				
				arr[0] += val;
			}
			
			return Version(arr);
		}
		
		void Next()
		{
			const uint8_t pr = PreRelease();
			
			if (pr == Normal)
			{
				m_data[0] = Alpha0;
				
				for (uint8_t i = 1; i < 4 && ++m_data[i] == 0; i++);
			}
			else if (pr >= Alpha0 && pr < RC31)
				m_data[0]++;
			else if (pr == RC31)
				m_data[0] = Normal;
			else
				m_data[0] = Alpha0;
		}
		
		void NextNormal()
		{
			m_data[0] = Normal;
			for (uint8_t i = 1; i < 4 && ++m_data[i] == 0; ++i);
		}
	};
	
	// todo: Check what needs to be removed (deprecated) for each version. Search for "vx.y.z". Also update README.md.
	constexpr Version VER = (0, 4, 0, Version::Normal);		///< Current version of the framework
}

#include "./Log.hpp"



namespace STM32T::RT
{
	inline std::vector<Log::output_t> _g_handlers;
	
	/**
	@note The @p logger must not write to `stdout` or `stderr` or use `ttywrch`.
	*/
	inline void AddHandler(const Log::output_t logger)
	{
		if (logger != Log::default_output_stdout)
		{
			auto it = std::find(_g_handlers.begin(), _g_handlers.end(), logger);
			if (it == _g_handlers.end())
				_g_handlers.push_back(logger);
		}
	}
	
	inline void RemoveHandler(const Log::output_t logger)
	{
		auto it = std::remove(_g_handlers.begin(), _g_handlers.end(), logger);
		_g_handlers.erase(it, _g_handlers.end());
	}
	
	inline void _Handle(const uint8_t *buf, uint32_t len)
	{
		for (auto handler : _g_handlers)
			handler({reinterpret_cast<const char *>(buf), len}, true);
	}
}



#ifdef STM32T_RT

#include <rt_sys.h>

extern "C"
{
	constexpr int FH_STDIN = 0x8001, FH_STDOUT = 0x8002, FH_STDERR = 0x8003;
	
	const char __stdin_name[]  = ":stdin", __stdout_name[] = ":stdout", __stderr_name[] = ":stderr";
	
	FILEHANDLE _sys_open(const char *name, int openmode)
	{
		if (strcmp(name, __stdin_name) == 0)
			return FH_STDIN;
		
		if (strcmp(name, __stdout_name) == 0)
			return FH_STDOUT;
		
		if (strcmp(name, __stderr_name) == 0)
			return FH_STDERR;
		
		return -1;
	}

	int _sys_close(FILEHANDLE fh) { return 0; }

	int _sys_write(int fh, const uint8_t *buf, uint32_t len, int mode)
	{
		if (fh == FH_STDOUT)
			STM32T::RT::_Handle(buf, len);
		else if (fh == FH_STDERR)
			STM32T::RT::_Handle(buf, len);
		else
			return -1;

		return 0;
	}

	int _sys_read(FILEHANDLE fh, unsigned char *buf, unsigned len, int mode) { return -1; }

	void _ttywrch(int ch) { STM32T::RT::_Handle(reinterpret_cast<const uint8_t *>(&ch), 1); }

	int _sys_istty(FILEHANDLE fh) { return fh == FH_STDIN || fh == FH_STDOUT || fh == FH_STDERR; }

	int _sys_seek(FILEHANDLE fh, long pos) { return -1; }

	long _sys_flen(FILEHANDLE fh) { return -1; }
}

#endif	// STM32T_RT

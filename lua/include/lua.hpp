// lua.hpp
// Lua header files for C++
// <<extern "C">> not supplied automatically because Lua also compiles as C++

extern "C" {
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
	#ifdef _WIN32
		#pragma comment(lib, "lua53.lib")
	#endif
}

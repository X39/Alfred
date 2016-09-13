#pragma once

#include "lua\include\lua.h"

int lh_panic(lua_State*);
int lh_print(lua_State*);
int lh_registerraw(lua_State*);
int lh_registermsg(lua_State*);
int luaopen_alfred_functions(lua_State*);
int lh_registermsg_callback(IRCHANDLE, const irc_command*, unsigned int, const char**, char*, unsigned int, long);
int lh_registerraw_callback(IRCHANDLE, const irc_command*);
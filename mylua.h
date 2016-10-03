#pragma once

#include "lua/include/lua.h"
#include "irc.h"

int lh_panic(lua_State*);
int lh_print(lua_State*);
int lh_registerraw(lua_State*);
int lh_registermsg(lua_State*);
bool lh_registermsg_callback(IRCHANDLE, const irc_command*, unsigned int, const char**, char*, unsigned int, long);
int lh_registerraw_callback(IRCHANDLE, const irc_command*);
int lh_sendprivmsg(lua_State*);
int lh_respond(lua_State*);
int lh_getRandomResponse(lua_State*);
int lh_setChannelVar(lua_State*);
int lh_getChannelVar(lua_State*);
int lh_getChannelList(lua_State*);
int lh_handle_PRIVMSG(IRCHANDLE, const irc_command*);
int luaopen_alfred_functions(lua_State*);
int lh_registermsg_callback(IRCHANDLE, const irc_command*, unsigned int, const char**, char*, unsigned int, long);
int lh_register_callback(IRCHANDLE, const irc_command*);

int lh_load_lua_modules(lua_State*);
void lua_clear_handles();

lua_State *currentL;


int *lh_raw_callback_ids;
int lh_raw_callback_ids_size;
int lh_raw_callback_ids_head;

int *lh_privmsg_callback_ids;
int lh_privmsg_callback_ids_size;
int lh_privmsg_callback_ids_head;
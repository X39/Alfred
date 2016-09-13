#include "mylua.h"
#include "lua\include\lua.h"
#include "lua\include\lauxlib.h"
#include "lua\include\lualib.h"
#include "irc_chatCommands.h"

#include <stdlib.h>

int lh_print(lua_State *L)
{
	int nargs = lua_gettop(L);
	int i;
	char* c;
	if (nargs > 1)
	{
		printf("[LUA ]\t");
	}
	for (i = 1; i <= nargs; i++)
	{
		if (lua_isstring(L, i))
		{
			printf(lua_tostring(L, i));
		}
		else
		{
			luaL_error(L, "Expected String");
		}
	}
	if (nargs > 1)
	{
		printf("\n");
	}
	return 0;
}

int *lh_raw_callback_ids = NULL;
int lh_raw_callback_ids_size = 0;
int lh_raw_callback_ids_head = 0;

lua_State *currentL;

int lh_registerraw(lua_State *L)
{
	if (lh_raw_callback_ids_head == lh_raw_callback_ids_size)
	{
		lh_raw_callback_ids_size += BUFF_INCREASE;
		lh_raw_callback_ids = realloc(lh_raw_callback_ids, sizeof(int) * lh_raw_callback_ids_size);
	}
	lh_raw_callback_ids[lh_raw_callback_ids_head] = luaL_ref(L, LUA_REGISTRYINDEX);
	lh_raw_callback_ids_head++;
}
int lh_registermsg(lua_State *L)
{

}
bool lh_registermsg_callback(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size, long cmdArg)
{
	int i;
	lua_rawgeti(currentL, LUA_REGISTRYINDEX, lh_raw_callback_ids[cmdArg]);

	lua_newtable(currentL);
		lua_pushstring(currentL, "cmd");
		lua_newtable(currentL);
			lua_pushstring(currentL, "content");
			lua_pushstring(currentL, cmd->content);
			lua_settable(currentL, -3);
			lua_pushstring(currentL, "receiver");
			lua_pushstring(currentL, cmd->receiver);
			lua_settable(currentL, -3);
			lua_pushstring(currentL, "sender");
			lua_pushstring(currentL, cmd->sender);
			lua_settable(currentL, -3);
			lua_pushstring(currentL, "type");
			lua_pushinteger(currentL, cmd->type);
			lua_settable(currentL, -3);
		lua_settable(currentL, -3);

		lua_pushstring(currentL, "args");
		lua_createtable(currentL, argc, 0);
			for (i = 0; i < argc; i++)
			{
				lua_pushstring(currentL, args[i]);
				lua_rawseti(currentL, -2, i);
			}
		lua_settable(currentL, -3);

	lua_pcall(currentL, 1, 0, 0);
	return false;
}
int lh_registerraw_callback(IRCHANDLE handle, const irc_command* cmd)
{
	int i;
	for (i = 0; i < lh_raw_callback_ids_head; i++)
	{
		lua_rawgeti(currentL, LUA_REGISTRYINDEX, lh_raw_callback_ids[i]);

		lua_newtable(currentL);
		lua_pushstring(currentL, "content");
		lua_pushstring(currentL, cmd->content);
		lua_settable(currentL, -3);
		lua_pushstring(currentL, "receiver");
		lua_pushstring(currentL, cmd->receiver);
		lua_settable(currentL, -3);
		lua_pushstring(currentL, "sender");
		lua_pushstring(currentL, cmd->sender);
		lua_settable(currentL, -3);
		lua_pushstring(currentL, "type");
		lua_pushinteger(currentL, cmd->type);
		lua_settable(currentL, -3);

		lua_pcall(currentL, 1, 0, 0);
	}
}

int luaopen_alfred_functions(lua_State *L)
{
	currentL = L;
	static const struct luaL_Reg globallib[] = {
		{ "print", lh_print },
		{ NULL, NULL }
	};
	static const struct luaL_Reg alfredlib[] = {
		{ "registerraw", lh_registerraw },	//Alfred.registerraw(callbackfunction)
											//callbackfunction => arg0.content, arg0.receiver, arg0.sender, arg0.type
		{ "registermsg", lh_registermsg },	//Alfred.registermsg(callbackfunction, commandname, formatprovider, requiresauth, directmessageonly)
											//callbackfunction => arg0.cmd.content, arg0.cmd.receiver, arg0.cmd.sender, arg0.cmd.type, arg0.args[]
		{ NULL, NULL }
	};
	lua_getglobal(L, "_G");
	luaL_setfuncs(L, globallib, 0);
	lua_pop(L, 1);

	luaL_newlib(L, "Alfred");
	luaL_setfuncs(L, alfredlib, 0);
	lua_pop(L, 1);
	lh_raw_callback_ids_size = BUFF_SIZE_TINY;
	lh_raw_callback_ids = realloc(lh_raw_callback_ids, sizeof(int) * lh_raw_callback_ids_size);
	lh_raw_callback_ids_head = 0;
}
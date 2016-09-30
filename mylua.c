#include "global.h"
#include "mylua.h"
#include "lua/include/lua.h"
#include "lua/include/lauxlib.h"
#include "lua/include/lualib.h"
#include "irc_chatCommands.h"
#include "irc_user.h"
#include "string_op.h"

#include <malloc.h>
#include <stdlib.h>

#if UNICODE && _UNICODE
#undef UNICODE
#undef _UNICODE
#include "tinydir/tinydir.h"
#define UNICODE
#define _UNICODE
#else
	#ifdef UNICODE
		#undef UNICODE
		#include "tinydir/tinydir.h"
		#define UNICODE
	#else
		#ifdef _UNICODE
			#undef _UNICODE
			#include "tinydir/tinydir.h"
			#define _UNICODE
		#else
			#include "tinydir/tinydir.h"
		#endif
	#endif
#endif

extern IRCHANDLE handle;

int lh_print(lua_State *L)
{
	int nargs = lua_gettop(L);
	int i;
	char* c;
	if (nargs >= 1)
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

extern int *lh_raw_callback_ids = NULL;
extern int lh_raw_callback_ids_size = 0;
extern int lh_raw_callback_ids_head = 0;

extern int *lh_privmsg_callback_ids = NULL;
extern int lh_privmsg_callback_ids_size = 0;
extern int lh_privmsg_callback_ids_head = 0;

extern int *lh_msg_callback_ids = NULL;
extern int lh_msg_callback_ids_size = 0;
extern int lh_msg_callback_ids_head = 0;

extern lua_State *LUAVM;



int lh_registerraw(lua_State *L)
{
	if (lh_raw_callback_ids_head == lh_raw_callback_ids_size)
	{
		lh_raw_callback_ids_size += BUFF_INCREASE;
		lh_raw_callback_ids = realloc(lh_raw_callback_ids, sizeof(int) * lh_raw_callback_ids_size);
	}
	lh_raw_callback_ids[lh_raw_callback_ids_head] = luaL_ref(L, LUA_REGISTRYINDEX);
	lh_raw_callback_ids_head++;
	return 0;
}
int lh_registermsg(lua_State *L)
{
	lua_Integer ref;
	const char* name;
	const char* format;
	int reqAuth;
	int reqDirect;
	
	if (lh_msg_callback_ids_head == lh_msg_callback_ids_size)
	{
		lh_msg_callback_ids_size += BUFF_INCREASE;
		lh_msg_callback_ids = realloc(lh_msg_callback_ids, sizeof(int) * lh_msg_callback_ids_size);
	}

	luaL_checktype(L, 1, LUA_TFUNCTION);
	luaL_checktype(L, 2, LUA_TSTRING);
	luaL_checktype(L, 3, LUA_TSTRING);
	luaL_checktype(L, 4, LUA_TBOOLEAN);
	luaL_checktype(L, 5, LUA_TBOOLEAN);

	name = luaL_tolstring(L, 2, NULL);
	format = luaL_tolstring(L, 3, NULL);
	reqAuth = lua_toboolean(L, 4);
	reqDirect = lua_toboolean(L, 5);
	lua_settop(L, 1);
	ref = luaL_ref(L, LUA_REGISTRYINDEX);
	if (ref == LUA_REFNIL || ref == LUA_NOREF)
		return 0;
	irc_chat_commands_add_command(lh_registermsg_callback, name, format, reqAuth, reqDirect, ref);

	lh_msg_callback_ids[lh_msg_callback_ids_head] = ref;
	lh_msg_callback_ids_head++;
	return 0;
}
int lh_registerprivmsg(lua_State *L)
{
	if (lh_privmsg_callback_ids_head == lh_privmsg_callback_ids_size)
	{
		lh_privmsg_callback_ids_size += BUFF_INCREASE;
		lh_privmsg_callback_ids = realloc(lh_privmsg_callback_ids, sizeof(int) * lh_privmsg_callback_ids_size);
	}
	lh_privmsg_callback_ids[lh_privmsg_callback_ids_head] = luaL_ref(L, LUA_REGISTRYINDEX);
	lh_privmsg_callback_ids_head++;
}
bool lh_registermsg_callback(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size, long cmdArg)
{
	int i;
	lua_rawgeti(LUAVM, LUA_REGISTRYINDEX, (lua_Integer)cmdArg);

	lua_pushstring(LUAVM, cmd->content);
	lua_pushstring(LUAVM, cmd->receiver);
	lua_pushstring(LUAVM, cmd->sender);
	lua_pushinteger(LUAVM, cmd->type);
	for (i = 0; i < argc; i++)
	{
		lua_pushstring(LUAVM, args[i]);
	}

	i = lua_pcall(LUAVM, 4 + argc, 0, 0);
	if (i == LUA_ERRRUN)
	{
		printf("[LERR]\t%s\n", lua_tostring(LUAVM, -1));
	}
	return false;
}
int lh_registerraw_callback(IRCHANDLE handle, const irc_command* cmd)
{
	int i, j;
	for (i = 0; i < lh_raw_callback_ids_head; i++)
	{
		lua_rawgeti(LUAVM, LUA_REGISTRYINDEX, lh_raw_callback_ids[i]);

		lua_pushstring(LUAVM, cmd->content);
		lua_pushstring(LUAVM, cmd->receiver);
		lua_pushstring(LUAVM, cmd->sender);
		lua_pushinteger(LUAVM, cmd->type);

		j = lua_pcall(LUAVM, 4, 0, 0);
		if (j == LUA_ERRRUN)
		{
			printf("[LERR]\t%s\n", lua_tostring(LUAVM, -1));
		}
	}
	return 1;
}

int lh_sendprivmsg(lua_State *L)
{
	const char *msg = luaL_checklstring(L, 1, NULL);
	const char *recv = luaL_checklstring(L, 2, NULL);
	irc_client_send_PRIVMSG(handle, msg, recv);
	return 0;
}
int lh_respond(lua_State *L)
{
	const char *msg = luaL_checklstring(L, 1, NULL);
	const char *recv = luaL_checklstring(L, 2, NULL);
	const char *sender = luaL_checklstring(L, 3, NULL);
	char *buff;
	int i;
	const char *responsee;

	if (recv[0] == '#')
	{
		responsee = recv;
	}
	else
	{
		i = (strchr(sender, '!') - sender) + 1;
		buff = alloca(sizeof(char) * i);
		strncpy(buff, sender, i);
		buff[i - 1] = '\0';
		responsee = buff;
	}
	irc_client_send_PRIVMSG(handle, msg, responsee);
	return 0;
}
int lh_getRandomResponse(lua_State *L)
{
	const char *kind = luaL_checklstring(L, 1, NULL);
	lua_pushstring(L, random_response(kind));
	return 1;
}
int lh_setChannelVar(lua_State *L)
{
	const char *channel = luaL_checklstring(L, 1, NULL);
	const char *namespace = luaL_checklstring(L, 2, NULL);
	const char *variable = luaL_checklstring(L, 3, NULL);
	const char *value = luaL_checklstring(L, 4, NULL);
	USER* user = irc_user_get_user(channel, namespace);
	irc_user_set_variable(user, variable, value);
	return 0;
}
int lh_getChannelVar(lua_State *L)
{
	const char *channel = luaL_checklstring(L, 1, NULL);
	const char *namespace = luaL_checklstring(L, 2, NULL);
	const char *variable = luaL_checklstring(L, 3, NULL);
	const char *result;
	USER* user = irc_user_get_user(channel, namespace);
	result = irc_user_get_variable(user, variable);
	if (result == NULL)
	{
		lua_pushnil(L);
	}
	else
	{
		lua_pushstring(L, result);
	}
	return 1;
}
extern CHANNEL** channels;
extern unsigned int channels_size;
int lh_getChannelList(lua_State *L)
{
	int i;
	lua_newtable(L);
	for (i = channels_size - 1; i >= 0; i--)
	{
		if (channels[i] == NULL)
			continue;
		if (channels[i]->channel_name[0] != '#')
			continue;
		lua_pushnumber(L, i);
		lua_pushstring(L, channels[i]->channel_name);
		lua_settable(L, -3);
	}
	return 1;
}

int lh_handle_PRIVMSG(IRCHANDLE handle, const irc_command *cmd)
{
	int i, j;
	if (cmd->type != IRC_PRIVMSG)
		return;
	for (i = 0; i < lh_privmsg_callback_ids_head; i++)
	{
		lua_rawgeti(LUAVM, LUA_REGISTRYINDEX, lh_privmsg_callback_ids[i]);

		lua_pushstring(LUAVM, cmd->content);
		lua_pushstring(LUAVM, cmd->receiver);
		lua_pushstring(LUAVM, cmd->sender);

		j = lua_pcall(LUAVM, 3, 0, 0);
		if (j == LUA_ERRRUN)
		{
			printf("[LERR]\t%s\n", lua_tostring(LUAVM, -1));
		}
	}
	return 0;
}

int luaopen_alfred_functions_inner(lua_State *L)
{
	static const struct luaL_Reg alfredlib[] = {
		{ "registerRaw", lh_registerraw },				//alfred.registerRaw(callbackfunction)
														//callbackfunction => content, receiver, sender, kind
		{ "registerCmd", lh_registermsg },				//alfred.registerCmd(callbackfunction, commandname, formatprovider, requiresauth, directmessageonly)
														//callbackfunction => content, receiver, sender, kind, arg0, arg1, argN
		{ "registerMsg", lh_registerprivmsg },			//alfred.registerMsg(callbackfunction)
														//callbackfunction => content, receiver, sender
		{ "sendPrivMsg", lh_sendprivmsg },				//alfred.sendprivmsg(message, receiver)
		{ "respond", lh_respond },						//alfred.respond(message, receiver, sender)
		{ "getRandomResponse", lh_getRandomResponse },	//alfred.getRandomResponse(responseType)
		{ "setChannelVar", lh_setChannelVar },			//alfred.setChannelVar(channel, namespace, variable, value)
		{ "getChannelVar", lh_getChannelVar },			//alfred.getChannelVar(channel, namespace, variable)
		{ "getChannelList", lh_getChannelList },		//alfred.getChannelList()
		{ NULL, NULL }
	};
	luaL_newlib(L, alfredlib);
	return 1;
}

int luaopen_alfred_functions(lua_State *L)
{
	LUAVM = L;
	lua_register(L, "print", lh_print);
	luaL_requiref(L, "alfred", luaopen_alfred_functions_inner, 1);
	lh_raw_callback_ids_size = BUFF_SIZE_TINY;
	lh_raw_callback_ids = realloc(lh_raw_callback_ids, sizeof(int) * lh_raw_callback_ids_size);
	lh_raw_callback_ids_head = 0;
	return 1;
}

int lh_load_lua_modules(lua_State *L)
{
	tinydir_dir dir;
	unsigned int modCount = 0;
	
	if (tinydir_open(&dir, TINYDIR_STRING("modules")))
	{
		printf("[LMOD]\tFailed to open module folder 'modules\\'\n");
		return -1;
	}
	printf("[LMOD]\tLoading modules ...\n");
	while (dir.has_next)
	{
		tinydir_file file;
		tinydir_readfile(&dir, &file);
		if (!file.is_dir && file.extension[0] == 'l' && file.extension[1] == 'u' && file.extension[2] == 'a')
		{
			if (luaL_loadfile(L, file.path))
			{
				printf("[LMOD]\tFailed to load module %s (%s)\n", file.name, lua_tostring(L, -1));
			}
			else
			{
				if (lua_pcall(L, 0, 0, 0))
				{
					printf("[LMOD]\tFailed to load module %s (%s)\n", file.name, lua_tostring(L, -1));
				}
				else
				{
					printf("[LMOD]\tSuccessfuly loaded module %s\n", file.name);
					modCount++;
				}
			}
		}
		;
		if (tinydir_next(&dir))
		{
			printf("[LMOD]\tFailed to get next in modules dir (IO error?)\n");
			break;
		}
	}

	printf("[LMOD]\tLoaded %d modules\n", modCount);
	tinydir_close(&dir);
}

void lua_clear_handles()
{
	int i;
	for (i = 0; i < lh_raw_callback_ids_head; i++)
	{
		luaL_unref(LUAVM, LUA_REGISTRYINDEX, lh_raw_callback_ids[i]);
	}
	lh_raw_callback_ids_head = 0;


	for (i = 0; i < lh_privmsg_callback_ids_head; i++)
	{
		luaL_unref(LUAVM, LUA_REGISTRYINDEX, lh_privmsg_callback_ids[i]);
	}
	lh_privmsg_callback_ids_head = 0;


	for (i = 0; i < lh_msg_callback_ids_head; i++)
	{
		luaL_unref(LUAVM, LUA_REGISTRYINDEX, lh_msg_callback_ids[i]);
	}
	lh_msg_callback_ids_head = 0;
}
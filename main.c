#include "global.h"
#include "main.h"
#include "networking.h"
#include "irc.h"
#include "config.h"
#include "irc_chatCommands.h"
#include "string_op.h"
#include "irc_user.h"
#include "mylua.h"
#include "lua/include/lua.h"
#include "lua/include/lauxlib.h"
#include "lua/include/lualib.h"

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#ifdef WIN32
#pragma comment(lib, "lua\\lua53")
#else
#include <execinfo.h>
#endif




IRCHANDLE handle;
CONFIG config;
time_t startTime;
unsigned long serveCount = 0;


/* Returns the node key that contains the string key of given channel or NULL if nothing was found. */
KEY channellist_contains(const char* channel)
{
	unsigned int i, j;
	KEY key, child, child2;
	key = config_get_key(config, "root/channels");
	if (key != NULL)
	{
		for (i = 0; i < config_key_get_size(key); i++)
		{
			child = config_key_get_children(key)[i];
			if (!strcmpi(config_key_get_name(child), "channel"))
			{
				for (j = 0; j < config_key_get_size(child); j++)
				{
					child2 = config_key_get_children(child)[j];
					switch (config_key_get_type(child2))
					{
						case DATATYPE_STRING:
						if (!strcmpi(config_key_get_string(child2), channel))
						{
							return child;
						}
						break;
					}
				}
			}
		}
	}
	return NULL;
}
/* extracts string subnode from provided key */
const char* extract_string_from_key(KEY* key)
{
	unsigned int i;
	KEY child;
	for (i = 0; (child = config_key_get_children(key)[i]) != NULL && i < config_key_get_size(key); i++)
	{
		switch (config_key_get_type(child))
		{
			case DATATYPE_STRING:
			return config_key_get_string(child);
			break;
		}
	}
	return NULL;
}

int handle_ENDOFMOTD(IRCHANDLE handle, const irc_command* cmd)
{
	KEY key;
	KEY child, child2;
	const char *value, *action;
	char* buffer;
	unsigned int i, j;
	if (cmd->type == RPL_ENDOFMOTD)
	{
		buffer = alloca(sizeof(char) * BUFF_SIZE_MEDIUM);

		/* Send Init messages */
		key = config_get_key(config, "root/init");
		if (key != NULL)
		{
			for (i = 0; i < config_key_get_size(key) && (child = config_key_get_children(key)[i]) != NULL; i++)
			{
				if (config_key_get_type(child) != DATATYPE_NODE || strcmpi(config_key_get_name(child), "action"))
					continue;
				value = NULL;
				action = NULL;
				for (j = 0;  j < config_key_get_size(child) && (child2 = config_key_get_children(child)[j]) != NULL; j++)
				{
					switch (config_key_get_type(child2))
					{
						case DATATYPE_ARG:
						if (!strcmpi(config_key_get_name(child2), "type"))
						{
							action = config_key_get_string(child2);
						}
						break;
						case DATATYPE_STRING:
						value = config_key_get_string(child2);
						break;
					}
				}
				if (action != NULL)
				{
					if (!strcmpi(action, "send"))
					{
						snprintf(buffer, BUFF_SIZE_MEDIUM, "%s\r\n", value);
						irc_client_send(handle, buffer, strlen(buffer));
					}
					else if(!strcmpi(action, "poll"))
					{
						irc_client_poll(handle, buffer, strlen(buffer));
					}
				}
			}
		}

		Sleep(500);

		/* Join channels placed in config */
		key = config_get_key(config, "root/channels");
		if (key != NULL)
		{
			for (i = 0; i < config_key_get_size(key) && (child = config_key_get_children(key)[i]) != NULL; i++)
			{
				if (config_key_get_type(child) != DATATYPE_NODE || strcmpi(config_key_get_name(child), "channel"))
					continue;
				for (j = 0; j < config_key_get_size(child) && (child2 = config_key_get_children(child)[j]) != NULL; j++)
				{
					switch (config_key_get_type(child2))
					{
						case DATATYPE_STRING:
						snprintf(buffer, BUFF_SIZE_MEDIUM, "JOIN %s\r\n", config_key_get_string(child2));
						irc_client_send(handle, buffer, strlen(buffer));
						irc_client_poll(handle, buffer, BUFF_SIZE_MEDIUM);
						break;
					}
				}
			}
		}

		return true;
	}
	return false;
}
int handle_INVITE(IRCHANDLE handle, const irc_command* cmd)
{
	char* buffer;
	int buffSize;
	KEY key;
	if (cmd->type == IRC_INVITE)
	{
		buffSize = sizeof("JOIN \r\n") + strlen(cmd->content);
		buffer = alloca(sizeof(char) * buffSize);
		snprintf(buffer, buffSize, "JOIN %s\r\n", cmd->content);
		irc_client_send(handle, buffer, buffSize);

		if (!channellist_contains(cmd->content))
		{
			key = config_key_create_child(config_get_or_create_key(config, "root/channels"));
			config_key_set_name(key, "channel");
			key = config_key_create_child(key);
			config_key_set_string(key, cmd->content);
		}
		return true;
	}
	return false;
}
int handle_KICK(IRCHANDLE handle, const irc_command* cmd)
{
	KEY key;
	if (cmd->type == IRC_KICK)
	{
		if ((key = channellist_contains(cmd->content)))
		{
			config_key_drop_child(config_get_key(config, "root/channels"), key);
		}
	}
	return false;
}

bool chatcmd_join(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size, long cmdArg)
{
	KEY key;
	snprintf(buffer, buffer_size, "JOIN %s\r\n", args[0]);
	irc_client_send(handle, buffer, strlen(buffer));
	if (args[1][0] == 't')
	{
		if (!channellist_contains(args[0]))
		{
			key = config_key_create_child(config_get_or_create_key(config, "root/channels"));
			config_key_set_name(key, "channel");
			key = config_key_create_child(key);
			config_key_set_string(key, args[0]);
		}
		snprintf(buffer, buffer_size, "You can always find me there from now on Master!");
	}
	else
	{
		snprintf(buffer, buffer_size, "You will find me there Master!");
	}
	serveCount++;
	return true;
}
bool chatcmd_leave(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size, long cmdArg)
{
	KEY key;
	snprintf(buffer, buffer_size, "PART %s : As you wish Master.\r\n", cmd->receiver);
	irc_client_send(handle, buffer, strlen(buffer));
	if ((key = channellist_contains(cmd->content)))
	{
		config_key_drop_child(config_get_key(config, "root/channels"), key);
	}
	serveCount++;
	return false;
}
bool chatcmd_save(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size, long cmdArg)
{
	snprintf(buffer, buffer_size, "Tried to save config. Result: %d", config_save(config, CONFIG_PATH));
	serveCount++;
	return true;
}
bool chatcmd_authed(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size, long cmdArg)
{
	strncpy(buffer, is_auth_user(cmd->sender) > 0 ? "Authorized" : "Non-Authorized", buffer_size);
	return true;
}

#ifdef WIN32
BOOL WINAPI handle_SIGTERM(DWORD val)
{
	int err;
	if (val == CTRL_C_EVENT)
	{
		irc_client_send(handle, "QUIT :SIGTERM ... sorry .. must go :/\r\n", sizeof("QUIT :SIGTERM ... sorry .. must go :/\r\n"));
		irc_client_close(&handle);
		irc_user_uninit();
		irc_chat_commands_uninit();
		config_save(config, CONFIG_PATH);
		config_destroy(config);
		if (err = socket_cleanup())
		{
			printf("[ERRO]\tsocket Init failed with error code %d\n", err);
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}
#else
void handle_SIGINT(int val)
{
	int err;
	irc_client_send(handle, "QUIT :My Master has callen me\r\n", sizeof("QUIT :My Master has callen me\r\n"));
	irc_client_close(&handle);
	irc_user_uninit();
	irc_chat_commands_uninit();
	config_save(config, CONFIG_PATH);
	config_destroy(config);
	if (err = socket_cleanup())
	{
		printf("[ERRO]\tsocket Init failed with error code %d\n", err);
		return;
	}
	exit(0);
}
void handle_SIGSEGV(int val)
{
	void *array[20];
	size_t size;
	size = backtrace(array, 20);
	fprintf(stderr, "Error: signal %d:\n", val);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
	exit(1);
}
#endif

bool validate_config_requirements(void)
{
	unsigned int p = 0;
	char* key;

	key = "root/connection/botname"; if ((p += config_get_key(config, key) == NULL)) printf("[ERRO]\tMissing key '%s'\n");
	key = "root/connection/ircport"; if ((p += config_get_key(config, key) == NULL)) printf("[ERRO]\tMissing key '%s'\n");
	key = "root/connection/ircaddr"; if ((p += config_get_key(config, key) == NULL)) printf("[ERRO]\tMissing key '%s'\n");

	return p == 0;
}

int lh_panic(lua_State *L)
{

	return 0;
}

int main(int argc, char** argv)
{
	int err;
	char buffer[BUFF_SIZE_LARGE];
	lua_State *L;
	#ifdef WIN32
	SetConsoleCtrlHandler(handle_SIGTERM, TRUE);
	#else
	struct sigaction action_SIGINT, action_SIGSEGV;

	memset(&action_SIGINT, 0, sizeof(struct sigaction));
	action_SIGINT.sa_handler = handle_SIGINT;
	sigaction(SIGINT, &action_SIGINT, NULL);

	memset(&action_SIGSEGV, 0, sizeof(struct sigaction));
	action_SIGSEGV.sa_handler = handle_SIGSEGV;
	sigaction(SIGSEGV, &action_SIGSEGV, NULL);
	#endif
	
	config = config_create();

	
	srand((unsigned int)time(NULL));
	startTime = time(NULL);
	config_load(config, CONFIG_PATH);
	if (!validate_config_requirements())
	{
		printf("\n\nCannot continue, %s is not setted up correctly\n", CONFIG_PATH);
		return 1;
	}

	irc_chat_commands_init(config);
	irc_user_init();

	irc_chat_commands_add_command(chatcmd_authed, "authed", "", false, true, 0);
	irc_chat_commands_add_command(chatcmd_save, "save", "", true, true, 0);
	irc_chat_commands_add_command(chatcmd_join, "join", "channel;perma=f;", true, false, 0);
	irc_chat_commands_add_command(chatcmd_leave, "leave", "", true, false, 0);


	L = luaL_newstate();
	luaL_openlibs(L);
	luaopen_alfred_functions(L);
	lua_atpanic(L, lh_panic);
	lh_load_lua_modules(L);


	if (err = socket_init())
	{
		printf("\n\nSocket Init failed with error code %d\n", err);
		lua_close(L);
		return 2;
	}
	err = irc_client_connect(extract_string_from_key(config_get_key(config, "root/connection/ircaddr")), extract_string_from_key(config_get_key(config, "root/connection/ircport")), extract_string_from_key(config_get_key(config, "root/connection/botname")), &handle);
	if (err)
	{
		printf("\n\nCreating client failed with error code %d\n", err);
		lua_close(L);
		return 3;
	}
	if (handle == NULL)
	{
		lua_close(L);
		return 4;
	}

	irc_client_register_callback(handle, handle_INVITE);
	irc_client_register_callback(handle, handle_KICK);
	irc_client_register_callback(handle, handle_ENDOFMOTD);
	irc_client_register_callback(handle, irc_chat_handle_chatcommands);
	irc_client_register_callback(handle, irc_user_handleUserFlow);
	irc_client_register_callback(handle, lh_registerraw_callback);


	while (1)
	{
		err = irc_client_poll(handle, buffer, BUFF_SIZE_LARGE);
		if (err < 0)
		{
			if (handle == NULL)
			{
				lua_close(L);
				return 5;
			}
			printf("[ERRO]\tPolling failed with %d\n", err);
			break;
		}
	}
	irc_client_close(&handle);
	irc_user_uninit();
	irc_chat_commands_uninit();
	config_save(config, CONFIG_PATH);
	config_destroy(config);

	if (err = socket_cleanup())
	{
		printf("\n\nSocket Init failed with error code %d\n", err);
		lua_close(L);
		return 6;
	}
	irc_chat_commands_uninit();
	return 0;
}

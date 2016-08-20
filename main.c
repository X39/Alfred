#include "global.h"
#include "main.h"
#include "networking.h"
#include "irc.h"
#include "config.h"
#include "irc_chatCommands.h"
#include "string_op.h"
#include "irc_user.h"

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>


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
			for (i = 0; (child = config_key_get_children(key)[i]) != NULL && i < config_key_get_size(key); i++)
			{
				if (strcmpi(config_key_get_name(child), "action"))
					continue;
				value = NULL;
				action = NULL;
				for (j = 0; (child2 = config_key_get_children(child)[j]) != NULL && j < config_key_get_size(child); j++)
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
			for (i = 0; (child = config_key_get_children(key)[i]) != NULL && i < config_key_get_size(key); i++)
			{
				if (!strcmpi(config_key_get_name(child), "channel"))
				{
					for (j = 0; (child2 = config_key_get_children(child)[j]) != NULL && j < config_key_get_size(child); j++)
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

bool chatcmd_roll(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
{
	long lowVal, highVal;
	lowVal = atol(args[0]);
	highVal = atol(args[1]);
	if (lowVal == highVal || highVal < 0)
	{
		strncpy(buffer, random_error_message(), buffer_size);
		return true;
	}
	highVal -= lowVal;
	highVal = rand() % highVal + 1;
	highVal += lowVal;
	snprintf(buffer, buffer_size, "%ld", highVal);
	serveCount++;
	return true;
}
bool chatcmd_join(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
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
bool chatcmd_leave(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
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
bool chatcmd_save(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
{
	snprintf(buffer, buffer_size, "Tried to save config. Result: %d", config_save(config, CONFIG_PATH));
	serveCount++;
	return true;
}
bool chatcmd_whoareyou(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
{
	strncpy(buffer, "I am a bot. You can see my src at https://github.com/X39/Alfred/", buffer_size);
	serveCount++;
	return true;
}
bool chatcmd_howareyou(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
{
	size_t size;
	size = strftime(buffer, buffer_size, "I am fine sir. I am running since %d.%m.%Y %X UTC%z. ", localtime(&startTime));
	buffer += size;
	buffer_size -= size;
	serveCount++;
	snprintf(buffer, buffer_size, "I also tried to serve as butler %lu times now. Thanks for asking.", serveCount);
	return true;
}
bool chatcmd_fact(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
{
	int size;
	KEY key = config_get_key(config, "root/facts");

	size = config_key_get_size(key);
	if (size == 0)
		strncpy(buffer, random_error_message(), buffer_size);
	else
		snprintf(buffer, buffer_size, "With pleasure Sir. Did you know that %s?", extract_string_from_key(config_key_get_children(key)[rand() % size]));
	serveCount++;
	return true;
}
bool chatcmd_authed(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
{
	strncpy(buffer, is_auth_user(cmd->sender) > 0 ? "Authorized" : "Non-Authorized", buffer_size);
	return true;
}
bool chatcmd_reload(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
{
	config_destroy(&config);
	config = config_create();
	snprintf(buffer, buffer_size, "Tried to reload config. Result: %d", config_load(config, CONFIG_PATH));
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
		irc_chat_commands_uninit();
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
	irc_chat_commands_uninit();
	if (err = socket_cleanup())
	{
		printf("[ERRO]\tsocket Init failed with error code %d\n", err);
		return;
	}
	exit(0);
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

int main(int argc, char** argv)
{
	int err;
	char buffer[BUFF_SIZE_LARGE];
	config = config_create();
	#ifndef WIN32
	struct sigaction action;
	#endif
	
	
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

	irc_chat_commands_add_command(chatcmd_roll, "roll", "from=0;to=30000;", false, false);
	irc_chat_commands_add_command(chatcmd_join, "join", "channel;perma=f;", true, false);
	irc_chat_commands_add_command(chatcmd_leave, "leave", "", true, false);
	irc_chat_commands_add_command(chatcmd_whoareyou, "who are you", "", false, false);
	irc_chat_commands_add_command(chatcmd_howareyou, "how are you", "", false, false);
	irc_chat_commands_add_command(chatcmd_fact, "fact", "", false, false);
	irc_chat_commands_add_command(chatcmd_save, "save", "", true, true);
	irc_chat_commands_add_command(chatcmd_authed, "authed", "", false, true);
	irc_chat_commands_add_command(chatcmd_reload, "reload", "", true, true);

	#ifdef WIN32
	SetConsoleCtrlHandler(handle_SIGTERM, TRUE);
	#else
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = handle_SIGINT;
	sigaction(SIGINT, &action, NULL);
	#endif

	if (err = socket_init())
	{
		printf("\n\nSocket Init failed with error code %d\n", err);
		return 2;
	}
	err = irc_client_connect(extract_string_from_key(config_get_key(config, "root/connection/ircaddr")), extract_string_from_key(config_get_key(config, "root/connection/ircport")), extract_string_from_key(config_get_key(config, "root/connection/botname")), &handle);
	if (err)
	{
		printf("\n\nCreating client failed with error code %d\n", err);
		return 3;
	}
	if (handle == NULL)
		return 4;

	irc_client_register_callback(handle, handle_INVITE);
	irc_client_register_callback(handle, handle_KICK);
	irc_client_register_callback(handle, handle_ENDOFMOTD);
	irc_client_register_callback(handle, irc_chat_handle_chatcommands);
	irc_client_register_callback(handle, irc_user_handleUserFlow);


	while (1)
	{
		err = irc_client_poll(handle, buffer, BUFF_SIZE_LARGE);
		if (err < 0)
		{
			if (handle == NULL)
				return 5;
			printf("[ERRO]\tPolling failed with %d\n", err);
			break;
		}
	}
	irc_client_close(&handle);

	if (err = socket_cleanup())
	{
		printf("\n\nSocket Init failed with error code %d\n", err);
		return 6;
	}
	irc_chat_commands_uninit();
	return 0;
}

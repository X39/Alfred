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


int handle_ENDOFMOTD(IRCHANDLE handle, const irc_command* cmd)
{
	int index;
	char* value;
	char* buffer;
	if (cmd->type == RPL_ENDOFMOTD)
	{
		buffer = alloca(sizeof(char) * BUFF_SIZE_SMALL);

		/* Send Init messages */
		index = 0;
		while ((value = config_get_key(config, "initmsg", index)) != NULL)
		{
			index++;
			snprintf(buffer, BUFF_SIZE_SMALL, "%s\r\n", value);
			irc_client_send(handle, buffer, strlen(buffer));
		}

		Sleep(500);

		/* Join channels placed in config */
		index = 0;
		while ((value = config_get_key(config, "channel", index)) != NULL)
		{
			index++;
			snprintf(buffer, BUFF_SIZE_SMALL, "JOIN %s\r\n", value);
			irc_client_send(handle, buffer, strlen(buffer));
			irc_client_poll(handle, buffer, BUFF_SIZE_SMALL);
		}
		return true;
	}
	return false;
}
int handle_INVITE(IRCHANDLE handle, const irc_command* cmd)
{
	int i;
	char* buffer;
	int buffSize;
	if (cmd->type == IRC_INVITE)
	{
		buffSize = sizeof("JOIN \r\n") + strlen(cmd->content);
		buffer = alloca(sizeof(char) * buffSize);
		snprintf(buffer, buffSize, "JOIN %s\r\n", cmd->content);
		irc_client_send(handle, buffer, buffSize);

		for (i = config_get_key_size(config, "channel"); i >= 0; i--)
		{
			if (strcmpi(config_get_key(config, "channel", i), cmd->content))
			{
				break;
			}
		}
		if (i < 0)
		{
			config_set_key(config, "channel", cmd->content, -1);
		}
		return true;
	}
	return false;
}

bool chatcmd_roll(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
{
	unsigned int val;
	val = atoi(args[0]);
	if (val <= 0)
	{
		strncpy(buffer, random_error_message(), buffer_size);
		return true;
	}
	val = rand() % val + 1;
	snprintf(buffer, buffer_size, "%d", val);
	serveCount++;
	return true;
}

bool chatcmd_join(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
{
	int i;
	snprintf(buffer, buffer_size, "JOIN %s\r\n", args[0]);
	irc_client_send(handle, buffer, strlen(buffer));
	if (args[1][0] == 't')
	{
		for (i = config_get_key_size(config, "channel") - 1; i >= 0; i--)
		{
			if (strcmpi(config_get_key(config, "channel", i), args[0]))
			{
				break;
			}
		}
		if (i < 0)
		{
			config_set_key(config, "channel", args[0], -1);
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
	int i;
	snprintf(buffer, buffer_size, "PART %s : As you wish Master.\r\n", cmd->receiver);
	irc_client_send(handle, buffer, strlen(buffer));
	for (i = config_get_key_size(config, "channel") - 1; i >= 0; i--)
	{
		if (strcmpi(config_get_key(config, "channel", i), cmd->receiver))
		{
			config_remove_key(config, "channel", i);
			break;
		}
	}
	serveCount++;
	return false;
}

bool chatcmd_save(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
{
	snprintf(buffer, buffer_size, "Tried to save config. Result: %d", config_save(config));
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
	int size = config_get_key_size(config, "fact");
	if (size == 0)
		strncpy(buffer, random_error_message(), buffer_size);
	else
		snprintf(buffer, buffer_size, "With pleasure Sir. Did you know that %s?", config_get_key(config, "fact", rand() % size));
	serveCount++;
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
			printf("socket Init failed with error code %d\n", err);
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
		printf("socket Init failed with error code %d\n", err);
		return;
	}
	exit(0);
}
#endif

int main(int argc, char** argv)
{
	int err;
	unsigned int index = 0;
	const char* value;
	char buffer[BUFF_SIZE_LARGE];
	config = config_create("config.xml");
	#ifndef WIN32
	struct sigaction action;
	#endif
	
	
	srand(time(NULL));
	startTime = time(NULL);
	config_load(config);

	irc_chat_commands_init(config);
	irc_chat_commands_add_command(chatcmd_roll, "roll", "limit=32000;", false);
	irc_chat_commands_add_command(chatcmd_join, "join", "channel;perma=f;", true);
	irc_chat_commands_add_command(chatcmd_leave, "leave", "", true);
	irc_chat_commands_add_command(chatcmd_save, "save", "", true);
	irc_chat_commands_add_command(chatcmd_whoareyou, "who are you", "", false);
	irc_chat_commands_add_command(chatcmd_howareyou, "how are you", "", false);
	irc_chat_commands_add_command(chatcmd_fact, "fact", "", false);

	#ifdef WIN32
	SetConsoleCtrlHandler(handle_SIGTERM, TRUE);
	#else
	memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = handle_SIGINT;
	sigaction(SIGINT, &action, NULL);
	#endif

	if (err = socket_init())
	{
		printf("Socket Init failed with error code %d\n", err);
		return 1;
	}
	err = irc_client_connect(config_get_key(config, "ircaddr", index), config_get_key(config, "ircport", index), config_get_key(config, "botname", index), &handle);
	if (err)
	{
		printf("creating client failed with error code %d\n", err);
		return 2;
	}
	if (handle == NULL)
		return 3;

	irc_client_register_callback(handle, irc_chat_handle_chatcommands);
	irc_client_register_callback(handle, handle_INVITE);
	irc_client_register_callback(handle, handle_ENDOFMOTD);


	while (1)
	{
		err = irc_client_poll(handle, buffer, BUFF_SIZE_LARGE);
		if (err < 0)
		{
			if (handle == NULL)
				return 4;
			printf("polling failed with %d\n", err);
			break;
		}
	}
	irc_client_close(&handle);

	if (err = socket_cleanup())
	{
		printf("socket Init failed with error code %d\n", err);
		return 5;
	}
	irc_chat_commands_uninit();
	return 0;
}

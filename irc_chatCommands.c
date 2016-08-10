#define IRC_CHATCOMMANDS_PRIVATE
#include "global.h"
#include "string_op.h"
#include "irc_chatCommands.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>


#ifndef WIN32
#define alloca alloca
#endif

CONFIG config;
const char* botname;
unsigned int botname_length;
COMMANDCONTAINER* containers;
unsigned int containers_index;
unsigned int containers_size;


const char* random_error_message(void)
{
	int size = config_get_key_size(config, "errormsg");
	if (size == 0)
		return "NO MESSAGE SET";
	return config_get_key(config, "errormsg", rand() % size);
}
const char* random_unknowncommand_message(void)
{
	int size = config_get_key_size(config, "unknowncommand");
	if (size == 0)
		return "NO MESSAGE SET";
	return config_get_key(config, "unknowncommand", rand() % size);
}
const char* random_notallowed_message(void)
{
	int size = config_get_key_size(config, "notallowed");
	if (size == 0)
		return "NO MESSAGE SET";
	return config_get_key(config, "notallowed", rand() % size);
}
bool is_auth_user(const char* user)
{
	int size = config_get_key_size(config, "authuser");
	for (size--; size >= 0; size--)
	{
		if (!strcmp(config_get_key(config, "authuser", size), user))
			return true;
	}
	return false;
}

bool chatcmd_help(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size)
{
	char* b = (char*)alloca(sizeof(char) * BUFF_SIZE_LARGE);
	unsigned int b_size = BUFF_SIZE_LARGE;
	unsigned int i;
	unsigned int j;
	unsigned int len;

	len = snprintf(b, b_size, "PRIVMSG %s :*Non-Auth*     ", cmd->receiver);
	b += len;
	b_size -= len;

	for (i = 0; i < containers_index; i++)
	{
		if (containers[i].requires_auth)
			continue;
		len = snprintf(b, b_size, containers[i].args_size ? " - '%s' args: " : " - '%s'", containers[i].name);
		b += len;
		b_size -= len;
		for (j = 0; j < containers[i].args_size; j++)
		{
			if (containers[i].args_defaults[j] != NULL)
			{
				len = snprintf(b, b_size, j ? ", '%s' (%s)" : "'%s' (%s)", containers[i].args[j], containers[i].args_defaults[j]);
				b += len;
				b_size -= len;
			}
			else
			{
				len = snprintf(b, b_size, j ? ", '%s'" : "'%s'", containers[i].args[j]);
				b += len;
				b_size -= len;
			}
		}
	}


	len = snprintf(b, b_size, "\r\nPRIVMSG %s :*Auth-Required*", cmd->receiver);
	b += len;
	b_size -= len;

	for (i = 0; i < containers_index; i++)
	{
		if (!containers[i].requires_auth)
			continue;
		len = snprintf(b, b_size, containers[i].args_size ? " - '%s' args: " : " - '%s'", containers[i].name);
		b += len;
		b_size -= len;
		for (j = 0; j < containers[i].args_size; j++)
		{
			if (containers[i].args_defaults[j] != NULL)
			{
				len = snprintf(b, b_size, j ? ", '%s' (%s)" : "'%s' (%s)", containers[i].args[j], containers[i].args_defaults[j]);
				b += len;
				b_size -= len;
			}
			else
			{
				len = snprintf(b, b_size, j ? ", '%s'" : "'%s'", containers[i].args[j]);
				b += len;
				b_size -= len;
			}
		}
	}
	snprintf(b, b_size, "\r\n", cmd->receiver);
	b -= BUFF_SIZE_LARGE - b_size;
	irc_client_send(handle, b, strlen(b));
	return false;
}

bool irc_chat_handle_chatcommands(IRCHANDLE handle, const irc_command* cmd)
{
	unsigned int i, j = 0;
	const char* res, *res2;
	char** args;
	char* buffer, *buffer2;
	const char* content = cmd->content + 1;
	int flag;
	const char* spotFound = NULL;
	if (cmd->type == IRC_PRIVMSG)
	{
		if (cmd->receiver[0] == '#')
		{
			if (str_swi(content, botname))
				return false;
			content += botname_length;
			for (i = 0; i < containers_index; i++)
			{
				if ((res = str_strwrdi(content, containers[i].name, NULL)))
				{
					if (res < spotFound || spotFound == NULL)
					{
						j = i;
						spotFound = res;
					}
				}
			}
			i = j;
			if(spotFound != NULL)
			{
				res = spotFound;
				flag = 0;
				if (containers[i].requires_auth && !is_auth_user(cmd->sender))
				{
					res = random_notallowed_message();
					flag = sizeof("PRIVMSG  :\r") + strlen(res) + strlen(cmd->receiver);
					buffer2 = (char*)alloca(sizeof(char) * flag);
					snprintf(buffer2, flag, "PRIVMSG %s :%s\r\n", cmd->receiver, res);
					irc_client_send(handle, buffer2, strlen(buffer2));
					return true;
				}
				for (j = 0; j < containers[i].args_size; j++)
				{
					res2 = str_strwrdi(res, containers[i].args[j], NULL);
					if (!res2 && containers[i].args_defaults[j] == NULL)
					{
						flag = 1;
						break;
					}
				}
				if (flag)
				{
					res = random_error_message();
					flag = sizeof("PRIVMSG  :\r") + strlen(cmd->receiver) + strlen(res);
					buffer2 = (char*)alloca(sizeof(char) * flag);
					snprintf(buffer2, flag, "PRIVMSG %s :%s\r\n", cmd->receiver, res);
					irc_client_send(handle, buffer2, flag);
					return true;
				}
				buffer = (char*)alloca(sizeof(char) * BUFF_SIZE_SMALL);
				args = (char**)alloca(sizeof(char*) * containers[i].args_size);
				for (j = 0; j < containers[i].args_size; j++)
				{
					res2 = str_strwrdi(res, containers[i].args[j], NULL);
					if (!res2)
					{
						args[j] = containers[i].args_defaults[j];
					}
					else
					{
						res = strchr(res2 + 1, ' ');
						if (res + 1 == '"')
						{
							res++;
							res2 = strchr(res + 1, '"');
						}
						else
						{
							res2 = strchr(res + 1, ' ');
						}
						if (res2 == NULL)
							res2 = strlen(res) + res;
						flag = res2 - res;
						args[j] = (char*)alloca(sizeof(char) * flag);
						strncpy(args[j], res + 1, res2 - res);
						args[j][flag - 1] = '\0';
					}
				}
				if (containers[i].cmd(handle, cmd, containers[i].args_size, args, buffer, BUFF_SIZE_SMALL))
				{
					flag = BUFF_SIZE_SMALL + sizeof("PRIVMSG  :\r") + strlen(cmd->receiver);
					buffer2 = (char*)alloca(sizeof(char) * flag);
					snprintf(buffer2, flag, "PRIVMSG %s :%s\r\n", cmd->receiver, buffer);
					irc_client_send(handle, buffer2, strlen(buffer2));
				}
				return true;
			}
			res = random_unknowncommand_message();
			flag = sizeof("PRIVMSG  :\r") + strlen(res) + strlen(cmd->receiver);
			buffer2 = (char*)alloca(sizeof(char) * flag);
			snprintf(buffer2, flag, "PRIVMSG %s :%s\r\n", cmd->receiver, res);
			irc_client_send(handle, buffer2, strlen(buffer2));
		}
	}
	return false;
}
void irc_chat_commands_init(CONFIG cfg)
{
	config = cfg;
	containers = (COMMANDCONTAINER*)malloc(sizeof(COMMANDCONTAINER) * BUFF_SIZE_TINY);
	containers_index = 0;
	containers_size = BUFF_SIZE_TINY;
	botname = config_get_key(config, "botname", 0);
	botname_length = strlen(botname);
	irc_chat_commands_add_command(chatcmd_help, "help", "", false);
}
void irc_chat_commands_uninit(void)
{
	unsigned int i;
	unsigned int j;
	for (i = 0; i < containers_index; i++)
	{
		for (j = 0; j < containers[i].args_size; j++)
		{
			free(containers[i].args[j]);
		}
		free(containers[i].name);
		free(containers[i].args);
	}
	free(containers);
}


///format structure:
///	{ LABEL ['=' DEFAULTVALUE] ';' }
///new labels are comma separated
///
///Example:
///	foo;bar;foobar=something;
void irc_chat_commands_add_command(CHATCOMMAND cmd, const char* command, const char* format, bool auth)
{
	char* strstrres;
	char* strstrres2;
	char* strstrres3;
	char tmp[BUFF_SIZE_SMALL];
	unsigned int args_index = 0;
	unsigned int size;
	if (containers_index == containers_size)
	{
		containers_size += BUFF_INCREASE;
		containers = (COMMANDCONTAINER*)realloc(containers, sizeof(COMMANDCONTAINER) * containers_size);
	}
	containers[containers_index].cmd = cmd;
	containers[containers_index].name = (char*)malloc(sizeof(char) * strlen(command) + 1);
	strcpy(containers[containers_index].name, command);
	containers[containers_index].args_size = 0;
	containers[containers_index].requires_auth = auth;

	strstrres = strchr(format, ';');
	strstrres2 = format;
	if (strstrres == NULL)
	{
		containers_index++;
		return;
	}
	do
	{
		strncpy(tmp, strstrres2, strstrres - format);
		strstrres2 = strstrres + 1;
		tmp[strstrres - format] = '\0';

		containers[containers_index].args_size++;
	}
	while ((strstrres = strchr(strstrres + 1, ';')));

	containers[containers_index].args = (char**)malloc(sizeof(char*) * containers[containers_index].args_size);
	containers[containers_index].args_defaults = (char**)malloc(sizeof(char*) * containers[containers_index].args_size);
	strstrres = strchr(format, ';');
	strstrres2 = format;
	do
	{
		strncpy(tmp, strstrres2, strstrres - format);
		strstrres2 = strstrres + 1;
		tmp[strstrres - format] = '\0';

		if ((strstrres3 = strchr(tmp, '=')))
		{
			size = strstrres3 - tmp + 1;
			containers[containers_index].args[args_index] = (char*)malloc(sizeof(char) * size);
			strncpy(containers[containers_index].args[args_index], tmp, size);
			containers[containers_index].args[args_index][size - 1] = '\0';

			size = strlen(strstrres3 + 1);
			containers[containers_index].args_defaults[args_index] = (char*)malloc(sizeof(char) * size);
			strcpy(containers[containers_index].args_defaults[args_index], strstrres3 + 1);
			containers[containers_index].args_defaults[args_index][size - 1] = '\0';
			args_index++;
		}
		else
		{
			size = strlen(tmp) + 1;
			containers[containers_index].args[args_index] = (char*)malloc(sizeof(char) * size);
			strcpy(containers[containers_index].args[args_index], tmp);
			containers[containers_index].args_defaults[args_index] = NULL;
			args_index++;
		}
	}
	while ((strstrres = strchr(strstrres + 1, ';')));
	containers_index++;
}
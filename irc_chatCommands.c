#define IRC_CHATCOMMANDS_PRIVATE
#include "global.h"
#include "string_op.h"
#include "irc_chatCommands.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


CONFIG config;
const char* botname;
unsigned int botname_length;
COMMANDCONTAINER* containers;
unsigned int containers_index;
unsigned int containers_size;

extern const char* extract_string_from_key(KEY* key);

const char* random_response(const char* kind)
{
	KEY responses = config_get_key(config, "root/responses");
	int responses_size = config_key_get_size(responses);
	unsigned int i, j, k = 0;
	KEY child, child2;
	bool flag = false;
	for (i = 0; i < responses_size; i++)
	{
		child = config_key_get_children(responses)[i];
		for (j = 0; j < config_key_get_size(child); j++)
		{
			child2 = config_key_get_children(child)[i];
			switch (config_key_get_type(child2))
			{
				case DATATYPE_ARG:
				if (!strcmpi(config_key_get_string(child2), kind))
				{
					k++;
				}
				break;
			}
		}
	}
	if(k == 0)
		return "NO MESSAGE SET";
	k = rand() % k;
	for (i = 0; i < responses_size; i++)
	{
		child = config_key_get_children(responses)[i];
		for (j = 0; j < config_key_get_size(child); j++)
		{
			child2 = config_key_get_children(child)[i];
			switch (config_key_get_type(child2))
			{
				case DATATYPE_ARG:
				if (!strcmpi(config_key_get_string(child2), kind))
				{
					k--;
					if (k == 0)
						flag = true;
				}
				break;
				case DATATYPE_STRING:
				if (flag)
				{
					return config_key_get_string(child2);
				}
				break;
			}
		}
	}
	return "ERR0R WHILE EXTRACTING RANDOM MESSAGE";
}
const char* random_error_message(void) { return random_response("error"); }
const char* random_unknowncommand_message(void) { return random_response("unknown"); }
const char* random_notallowed_message(void) { return random_response("unauthorized"); }
bool is_auth_user(const char* user)
{
	KEY admins = config_get_key(config, "root/admins");
	int admins_size = config_key_get_size(admins);
	unsigned int i;
	KEY child;
	for (i = 0; i < admins_size; i++)
	{
		child = config_key_get_children(admins)[i];
		if (strcmpi(config_key_get_name(child), "user"))
			continue;
		if (!strcmp(extract_string_from_key(child), user))
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
	snprintf(b, b_size, "\r\n");
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
	botname = extract_string_from_key(config_get_key(config, "root/connection/botname"));
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
		if(containers[i].args_size > 0)
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
	const char* strstrres;
	const char* strstrres2;
	const char* strstrres3;
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
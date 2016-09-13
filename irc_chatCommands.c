#define IRC_CHATCOMMANDS_PRIVATE
#include "global.h"
#include "string_op.h"
#include "irc_user.h"
#include "irc_chatCommands.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


CONFIG config;
char* botname;
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
			child2 = config_key_get_children(child)[j];
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
	k = rand() % k + 1;
	for (i = 0; i < responses_size; i++)
	{
		child = config_key_get_children(responses)[i];
		for (j = 0; j < config_key_get_size(child); j++)
		{
			child2 = config_key_get_children(child)[j];
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

bool chatcmd_help(IRCHANDLE handle, const irc_command* cmd, unsigned int argc, const char** args, char* buffer, unsigned int buffer_size, long cmdArg)
{
	char* b = (char*)alloca(sizeof(char) * BUFF_SIZE_LARGE);
	unsigned int b_size = BUFF_SIZE_LARGE;
	unsigned int i;
	unsigned int j;
	unsigned int len;
	bool isDirect = cmd->receiver[0] != '#';
	char* receiver;
	if (!isDirect)
	{
		#ifdef DEBUG
		printf("[DEBU]\tHelp Command - Is channel message\n");
		#endif
		receiver = cmd->receiver;
	}
	else
	{
		#ifdef DEBUG
		printf("[DEBU]\tHelp Command - Is direct message\n");
		#endif
		i = (strchr(cmd->sender, '!') - cmd->sender) + 1;
		receiver = alloca(sizeof(char) * i);
		strncpy(receiver, cmd->sender, i);
		receiver[i - 1] = '\0';
	}

	#ifdef DEBUG
	printf("[DEBU]\tHelp Command - Start to build Non-Auth command list\n");
	#endif
	len = snprintf(b, b_size, "PRIVMSG %s :*Non-Auth*     ", receiver);
	b += len;
	b_size -= len;

	for (i = 0; i < containers_index; i++)
	{
		if (containers[i].requires_auth || (!isDirect && containers[i].direct_only))
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
	if (is_auth_user(cmd->sender))
	{
		#ifdef DEBUG
		printf("[DEBU]\tHelp Command - Start to build Auth-Required command list\n");
		#endif
		len = snprintf(b, b_size, "\r\nPRIVMSG %s :*Auth-Required*", receiver);
		b += len;
		b_size -= len;

		for (i = 0; i < containers_index; i++)
		{
			if (!containers[i].requires_auth || (!isDirect && containers[i].direct_only))
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
	}
	snprintf(b, b_size, "\r\n");
	b -= BUFF_SIZE_LARGE - b_size;
	irc_client_send(handle, b, strlen(b));
	#ifdef DEBUG
	printf("[DEBU]\tHelp Command - Execution finished\n");
	#endif
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
	char* responsee;
	bool isDirectMessage;
	if (cmd->type == IRC_PRIVMSG)
	{
		if (str_swi(content, botname))
			return false;
		if (cmd->receiver[0] == '#')
		{
			responsee = cmd->receiver;
			isDirectMessage = false;
		}
		else
		{
			i = (strchr(cmd->sender, '!') - cmd->sender) + 1;
			responsee = alloca(sizeof(char) * i);
			strncpy(responsee, cmd->sender, i);
			responsee[i - 1] = '\0';
			isDirectMessage = true;
		}
		if (flag = irc_user_check_antiflood(responsee, cmd->sender))
		{
			res = random_response("antiflood");
			if (!isDirectMessage)
			{
				i = (strchr(cmd->sender, '!') - cmd->sender) + 1;
				responsee = alloca(sizeof(char) * i);
				strncpy(responsee, cmd->sender, i);
				responsee[i - 1] = '\0';
			}
			i = sizeof("PRIVMSG  :\r") + strlen(responsee) + strlen(res) + 1;
			buffer = alloca(sizeof(char) * i);
			irc_client_send(handle, buffer, snprintf(buffer, i, "PRIVMSG %s :%s\r\n", responsee, res));
			return true;
		}
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
		if (spotFound != NULL)
		{
			res = spotFound;
			flag = 0;
			if (containers[i].requires_auth && !is_auth_user(cmd->sender))
			{
				res = random_notallowed_message();
				flag = sizeof("PRIVMSG  :\r") + strlen(res) + strlen(responsee);
				buffer2 = alloca(sizeof(char) * flag);
				snprintf(buffer2, flag, "PRIVMSG %s :%s\r\n", responsee, res);
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
				flag = sizeof("PRIVMSG  :\r") + strlen(responsee) + strlen(res);
				buffer2 = alloca(sizeof(char) * flag);
				snprintf(buffer2, flag, "PRIVMSG %s :%s\r\n", responsee, res);
				irc_client_send(handle, buffer2, flag);
				return true;
			}
			buffer = alloca(sizeof(char) * BUFF_SIZE_SMALL);
			args = alloca(sizeof(char*) * containers[i].args_size);
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
					if (res[1] == '"')
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
					args[j] = alloca(sizeof(char) * flag);
					strncpy(args[j], res + 1, res2 - res);
					args[j][flag - 1] = '\0';
				}
			}
			if (containers[i].cmd(handle, cmd, containers[i].args_size, (const char**)args, buffer, BUFF_SIZE_SMALL, containers[i].cmdArg))
			{
				flag = BUFF_SIZE_SMALL + sizeof("PRIVMSG  :\r") + strlen(responsee);
				buffer2 = alloca(sizeof(char) * flag);
				snprintf(buffer2, flag, "PRIVMSG %s :%s\r\n", responsee, buffer);
				irc_client_send(handle, buffer2, strlen(buffer2));
			}
			return true;
		}
		res = random_unknowncommand_message();
		flag = sizeof("PRIVMSG  :\r") + strlen(res) + strlen(responsee);
		buffer2 = alloca(sizeof(char) * flag);
		snprintf(buffer2, flag, "PRIVMSG %s :%s\r\n", responsee, res);
		irc_client_send(handle, buffer2, strlen(buffer2));

	}
	return false;
}
void irc_chat_commands_init(CONFIG cfg)
{
	const char* botname_const;
	config = cfg;
	containers = (COMMANDCONTAINER*)malloc(sizeof(COMMANDCONTAINER) * BUFF_SIZE_TINY);
	containers_index = 0;
	containers_size = BUFF_SIZE_TINY;
	botname_const = extract_string_from_key(config_get_key(config, "root/connection/bottrigger"));
	botname_length = strlen(botname_const);
	botname = malloc(sizeof(char) * botname_length + 1);
	strcpy(botname, botname_const);
	irc_chat_commands_add_command(chatcmd_help, "help", "", false, false, 0);
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
	botname_length = 0;
	free(botname);
	botname = NULL;
}


///format structure:
///	{ LABEL ['=' DEFAULTVALUE] ';' }
///new labels are comma separated
///
///Example:
///	foo;bar;foobar=something;
void irc_chat_commands_add_command(CHATCOMMAND cmd, const char* command, const char* format, bool auth, bool direct_only, long cmdArg)
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
	containers[containers_index].direct_only = direct_only;
	containers[containers_index].cmdArg = cmdArg;

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
		strncpy(tmp, strstrres2, strstrres - strstrres2);
		tmp[strstrres - strstrres2] = '\0';

		if ((strstrres3 = strchr(tmp, '=')))
		{
			size = strstrres3 - tmp + 1;
			containers[containers_index].args[args_index] = (char*)malloc(sizeof(char) * size);
			strncpy(containers[containers_index].args[args_index], tmp, size);
			containers[containers_index].args[args_index][size - 1] = '\0';

			size = strlen(strstrres3 + 1);
			containers[containers_index].args_defaults[args_index] = (char*)malloc(sizeof(char) * (size + 1));
			strcpy(containers[containers_index].args_defaults[args_index], strstrres3 + 1);
			containers[containers_index].args_defaults[args_index][size] = '\0';
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
		strstrres2 = strstrres + 1;
	}
	while ((strstrres = strchr(strstrres + 1, ';')));
	containers_index++;
}
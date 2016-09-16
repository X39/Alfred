#include "global.h"
#include "irc_user.h"
#include "string_op.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

CHANNEL** channels;
unsigned int channels_size;
void irc_user_remove_user(const char* channel, const char* username)
{
	unsigned int i, j;
	CHANNEL* c = NULL;
	for (i = 0; i < channels_size; i++)
	{
		if (channels[i] == NULL)
		{
			continue;
		}
		c = channels[i];
		if (!strcmpi(channels[i]->channel_name, channel))
		{
			for (j = 0; j < channels[i]->users_size; j++)
			{
				if (channels[i]->users[j] == NULL)
				{
					continue;
				}
				if (!strcmpi(channels[i]->users[j]->username, username))
				{
					irc_user_free_user(channels[i]->users + j);
					channels[i]->users_last--;
					channels[i]->users[j] = channels[i]->users[channels[i]->users_last];
					break;
				}
			}
		}
		break;
	}
}
USER* irc_user_get_user(const char* channel, const char* username)
{
	unsigned int i, j;
	int k = -1;
	CHANNEL* c = NULL;
	for (i = 0; i < channels_size; i++)
	{
		if (channels[i] == NULL)
		{
			if(k < 0)
				k = i;
			continue;
		}
		c = channels[i];
		if (!strcmpi(channels[i]->channel_name, channel))
		{
			k = -1;
			for (j = 0; j < channels[i]->users_size; j++)
			{
				if (channels[i]->users[j] == NULL)
				{
					if (k < 0)
						k = j;
					continue;
				}
				if (!strcmpi(channels[i]->users[j]->username, username))
				{
					return channels[i]->users[j];
				}
			}
		}
		break;
	}
	
	if (c == NULL)
	{
		if (k < 0)
		{
			k = channels_size;
			channels_size += BUFF_INCREASE;
			channels = realloc(channels, sizeof(CHANNEL*) * channels_size);
		}
		channels[k] = malloc(sizeof(CHANNEL));
		channels[k]->channel_name = (char*)malloc(sizeof(char*) * strlen(channel) + 1);
		channels[k]->channel_name[strlen(channel)] = '\0';
		strcpy(channels[k]->channel_name, channel);
		channels[k]->users = malloc(sizeof(USER*) * BUFF_SIZE_SMALL);
		channels[k]->users_size = BUFF_SIZE_SMALL;
		channels[k]->users_last = 1;
		memset(channels[k]->users, 0, sizeof(USER*) * BUFF_SIZE_SMALL);

		channels[k]->users[0] = malloc(sizeof(USER));
		channels[k]->users[0]->last_request = 0;
		channels[k]->users[0]->var_names = malloc(sizeof(char**) * BUFF_SIZE_TINY);
		channels[k]->users[0]->var_values = malloc(sizeof(char**) * BUFF_SIZE_TINY);
		channels[k]->users[0]->var_size = BUFF_SIZE_TINY;
		channels[k]->users[0]->var_head = 0;
		channels[k]->users[0]->username = malloc(sizeof(char) * strlen(username) + 1);
		strcpy(channels[k]->users[0]->username, username);
		channels[k]->users[0]->username[strlen(username)] = '\0';
		return channels[k]->users[0];
	}
	else
	{
		if (channels[i]->users_last == channels[i]->users_size)
		{
			channels[i]->users_size += BUFF_INCREASE;
			channels[i]->users = (USER**)realloc(channels[i]->users, sizeof(USER*) * channels[i]->users_size);
		}
		k = channels[i]->users_last;
		channels[i]->users[k] = (USER*)malloc(sizeof(USER));
		channels[i]->users[k]->last_request = 0;
		channels[i]->users[k]->username = (char*)malloc(sizeof(char) * strlen(username) + 1);
		strcpy(channels[i]->users[k]->username, username);
		channels[i]->users[k]->username[strlen(username)] = '\0';
		channels[i]->users[k]->var_names = malloc(sizeof(char**) * BUFF_SIZE_TINY);
		channels[i]->users[k]->var_values = malloc(sizeof(char**) * BUFF_SIZE_TINY);
		channels[i]->users[k]->var_size = BUFF_SIZE_TINY;
		channels[i]->users[k]->var_head = 0;
		channels[i]->users_last++;
		return channels[i]->users[k];
	}
}
void irc_user_free_user(USER** user)
{
	USER* u;
	unsigned int i;
	if (user == NULL)
		return;
	u = *user;
	for (i = 0; i < u->var_head; i++)
	{
		free(u->var_names[i]);
		free(u->var_values[i]);
	}
	free(u->var_values);
	free(u->username);
	free(u);
}
void irc_user_free_channel(CHANNEL** channel)
{
	CHANNEL* c;
	unsigned int i;
	if (channel == NULL)
		return;
	c = *channel;
	for (i = 0; i < c->users_last; i++)
	{
		irc_user_free_user(&(c->users[i]));
	}
	free(c);
	*channel = NULL;
}


void irc_user_init(void)
{
	channels = (CHANNEL**)malloc(sizeof(CHANNEL*) * BUFF_SIZE_TINY);
	channels_size = BUFF_SIZE_TINY;
	memset(channels, 0, sizeof(CHANNEL*) * BUFF_SIZE_TINY);
}
void irc_user_uninit(void)
{
	unsigned int i;
	for (i = 0; i < channels_size; i++)
	{
		irc_user_free_channel(&(channels[i]));
	}
	free(channels);
}

int irc_user_check_antiflood(const char* channel, const char* username)
{
	USER* u = (USER*)irc_user_get_user(channel, username);
	time_t t = time(NULL), t2 = u->last_request;
	u->last_request = t;
	if (difftime(t, t2) < IRC_USER_ANTIFLOOD)
	{
		return true;
	}
	return false;
}
int irc_user_handleUserFlow(IRCHANDLE handle, const irc_command* cmd)
{
	unsigned int i;
	const char* content = cmd->content;
	const char* res;
	char* buffer, *buffer2;
	if (cmd->type == IRC_JOIN)
	{
		irc_user_get_user(cmd->receiver, cmd->sender);
	}
	else if (cmd->type == IRC_PART)
	{
		irc_user_remove_user(cmd->receiver, cmd->sender);
	}
	else if (cmd->type == RPL_NAMREPLY)
	{
		content = strchr(content, ' ') + 1;
		res = strchr(content, ':');
		i = res - content - 1;
		buffer = (char*)alloca(sizeof(char) * i + 1);
		buffer2 = (char*)alloca(sizeof(char) * BUFF_SIZE_TINY);
		buffer[i] = '\0';
		strncpy(buffer, content, i);
		while (res = strchr((content = res + 1), ' '))
		{
			i = (res - content);
			buffer2[i] = '\0';
			strncpy(buffer2, content, i);
			irc_user_get_user(buffer, buffer2);
		}
	}
	return false;
}
void irc_user_set_variable(USER* usr_hndl, const char* variable, const char* value)
{
	unsigned int i;
	for (i = 0; i < usr_hndl->var_head; i++)
	{
		if (strcmp(variable, usr_hndl->var_names[i]))
			continue;
		if (value == NULL)
		{
			free(usr_hndl->var_values[i]);
			free(usr_hndl->var_names[i]);
			usr_hndl->var_head--;
			usr_hndl->var_names[i] = usr_hndl->var_names[usr_hndl->var_head];
			usr_hndl->var_names[usr_hndl->var_head] = NULL;
			usr_hndl->var_values[i] = usr_hndl->var_values[usr_hndl->var_head];
			usr_hndl->var_values[usr_hndl->var_head] = NULL;
		}
		else
		{
			usr_hndl->var_values[i] = realloc(usr_hndl->var_values[i], sizeof(char) * strlen(value) + 1);
			strcpy(usr_hndl->var_values[i], value);
		}
		return;
	}
	if (value == NULL)
		return;
	if (usr_hndl->var_head == usr_hndl->var_size)
	{
		usr_hndl->var_size += BUFF_INCREASE;
		usr_hndl->var_names = realloc(usr_hndl->var_names, sizeof(char**) * usr_hndl->var_size);
		usr_hndl->var_values = realloc(usr_hndl->var_values, sizeof(char**) * usr_hndl->var_size);
	}
	usr_hndl->var_names[usr_hndl->var_head] = malloc(sizeof(char) * strlen(variable) + 1);
	strcpy(usr_hndl->var_names[usr_hndl->var_head], value);
	usr_hndl->var_values[usr_hndl->var_head] = malloc(sizeof(char) * strlen(value) + 1);
	strcpy(usr_hndl->var_values[usr_hndl->var_head], value);
	usr_hndl->var_head++;
}
const char* irc_user_get_variable(USER* usr_hndl, const char* variable)
{
	unsigned int i;
	const char* value = NULL;
	for (i = 0; i < usr_hndl->var_head; i++)
	{
		if (strcmp(variable, usr_hndl->var_names[i]))
			continue;
		value = usr_hndl->var_values[i];
		break;
	}
	return value;
}
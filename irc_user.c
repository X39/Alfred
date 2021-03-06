#include "global.h"
#include "irc_user.h"
#include "string_op.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

extern CHANNEL** channels;
extern unsigned int channels_size;
void irc_user_remove_user(const char* channel, const char* username)
{
	unsigned int i, j;
	CHANNEL* c = NULL;
	for (i = 0; i < channels_size; i++)
	{
		c = channels[i];
		if (c == NULL)
		{
			continue;
		}
		if (!strcmpi(c->channel_name, channel))
		{
			for (j = 0; j < c->users_last; j++)
			{
				if (!strcmpi(c->users[j]->username, username))
				{
					irc_user_free_user(c->users + j);
					c->users_last--;
					c->users[j] = c->users[c->users_last];
					break;
				}
			}
		}
		break;
	}
}
void irc_user_rename(USER* user, const char *newName)
{
	unsigned int len = strlen(newName);
	free(user->username);
	user->username = malloc(sizeof(char) * (len + 1));
	strcpy(user->username, newName);
	user->username[len] = '\0';
}
CHANNEL* irc_user_try_get_channel(const char *channel)
{
	unsigned int i;
	CHANNEL* c = NULL;
	for (i = 0; i < channels_size; i++)
	{
		if (channels[i] == NULL)
		{
			continue;
		}
		if (!strcmp(channels[i]->channel_name, channel))
		{
			c = channels[i];
			break;
		}
	}
	return c;
}
USER* irc_user_try_get_user(const char *channel, const char *username)
{
	unsigned int i;
	CHANNEL *c = irc_user_try_get_channel(channel);
	if(c == NULL)
		return NULL;
	
	for (i = 0; i < c->users_last; i++)
	{
		if (c->users[i] == NULL)
		{
			continue;
		}
		if (!strcmp(c->users[i]->username, username))
		{
			return c->users[i];
		}
	}
	return NULL;
}
USER* irc_user_get_user(const char* channel, const char* username)
{
	unsigned int i;
	int k = -1;
	CHANNEL* c = NULL;
	for (i = 0; i < channels_size; i++)
	{
		if (channels[i] == NULL)
		{
			if (k < 0)
				k = i;
			continue;
		}
		if (!strcmp(channels[i]->channel_name, channel))
		{
			c = channels[i];
			break;
		}
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
		c = channels[k];
	}
	for (i = 0; i < c->users_last; i++)
	{
		if (c->users[i] == NULL)
		{
			if (k < 0)
				k = i;
			continue;
		}
		if (!strcmp(c->users[i]->username, username))
		{
			return c->users[i];
		}
	}
	if (c->users_last == c->users_size)
	{
		k = c->users_last;
		c->users_size += BUFF_INCREASE;
		c->users = (USER**)realloc(c->users, sizeof(USER*) * c->users_size);
	}
	k = c->users_last;
	c->users[k] = (USER*)malloc(sizeof(USER));
	c->users[k]->last_request = 0;
	c->users[k]->username = (char*)malloc(sizeof(char) * strlen(username) + 1);
	c->users[k]->username[strlen(username)] = '\0';
	strcpy(c->users[k]->username, username);
	c->users[k]->var_names = malloc(sizeof(char**) * BUFF_SIZE_TINY);
	c->users[k]->var_values = malloc(sizeof(char**) * BUFF_SIZE_TINY);
	c->users[k]->var_size = BUFF_SIZE_TINY;
	c->users[k]->var_head = 0;
	c->users_last++;
	return c->users[k];
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
	CHANNEL* c = *channel;
	unsigned int i;
	if (c == NULL)
		return;
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
	USER* u;
	time_t t, t2;

	if (channel[0] != '#')
	{
		u = (USER*)irc_user_get_user("~QUERY~", username);
	}
	else
	{
		u = (USER*)irc_user_get_user(channel, username);
	}
	t = time(NULL);
	t2 = u->last_request;
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
	USER* usr;
	if (cmd->type == IRC_JOIN)
	{
		irc_user_get_user(cmd->receiver, cmd->sender);
	}
	else if (cmd->type == IRC_PART || cmd->type == IRC_QUIT)
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
	else if(cmd->type == IRC_NICK)
	{
		for(i = 0; i < channels_size; i++)
		{
			if(channels[i] == NULL)
				break;
			usr = irc_user_try_get_user(channels[i]->channel_name, cmd->sender);
			if(usr != NULL)
			{
				irc_user_rename(usr, cmd->content + 1);
			}
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
	strcpy(usr_hndl->var_names[usr_hndl->var_head], variable);
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
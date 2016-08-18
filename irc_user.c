#include "global.h"
#include "irc_user.h"
#include "string_op.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>

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
					break;
				}
			}
		}
		break;
	}
}
const USER* irc_user_get_user(const char* channel, const char* username)
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
			channels = (CHANNEL**)realloc(channels, sizeof(CHANNEL*) * channels_size);
		}
		channels[k] = (CHANNEL*)malloc(sizeof(CHANNEL));
		channels[k]->channel_name = (char*)malloc(sizeof(char*) * strlen(channel) + 1);
		channels[k]->channel_name[strlen(channel)] = '\0';
		strcpy(channels[k]->channel_name, channel);
		channels[k]->users = (USER**)malloc(sizeof(USER*) * BUFF_SIZE_SMALL);
		channels[k]->users_size = BUFF_SIZE_SMALL;
		memset(channels[k]->users, 0, sizeof(USER*) * BUFF_SIZE_SMALL);

		channels[k]->users[0] = (USER*)malloc(sizeof(USER));
		channels[k]->users[0]->last_request = 0;
		channels[k]->users[0]->username = (char*)malloc(sizeof(char) * strlen(username) + 1);
		strcpy(channels[k]->users[0]->username, username);
		channels[k]->users[0]->username[strlen(username)] = '\0';
		return channels[k]->users[0];
	}
	else
	{
		if (k < 0)
		{
			k = channels_size;
			channels[i]->users_size += BUFF_INCREASE;
			channels[i]->users = (USER**)realloc(channels[i]->users, sizeof(USER*) * channels[i]->users_size);
		}
		channels[i]->users[k] = (USER*)malloc(sizeof(USER));
		channels[i]->users[k]->last_request = 0;
		channels[i]->users[k]->username = (char*)malloc(sizeof(char) * strlen(username) + 1);
		strcpy(channels[i]->users[k]->username, username);
		channels[i]->users[k]->username[strlen(username)] = '\0';
		return channels[i]->users[k];
	}
}
void irc_user_free_user(USER** user)
{
	USER* u;
	if (user == NULL)
		return;
	u = *user;
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
	for (i = 0; i < c->users_size; i++)
	{
		irc_user_free_user(c->users + i);
	}
	free(c);
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
		irc_user_free_channel(channels + i);
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
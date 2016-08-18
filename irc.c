#include "global.h"
#include "irc.h"
#include "networking.h"
#include "string_op.h"

#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <time.h>

#define CONST_CALLBACK_INITIAL 10
#define CONST_CALLBACK_INC 10
#define CONST_BUFFER_SIZE 256
#define CONST_ANTIFLOOD_THRESHOLD_BYTES 1024
#define CONST_ANTIFLOOD_THRESHOLD_TIME 1

//https://tools.ietf.org/html/rfc2812

typedef struct
{
	IRCCALLBACK* callbacks;
	size_t callbacks_next;
	size_t callbacks_size;
	IRCCALLBACKRAW* callbacks_raw;
	size_t callbacks_raw_next;
	size_t callbacks_raw_size;
	SOCKET socket;

	time_t antiflood_last;
	unsigned long antiflood_out;
} IRC;


int FNC(connect)(const char* ip, const char* port, const char* nick, IRCHANDLE* outHandle)
{
	IRC* irc = (IRC*)malloc(sizeof(IRC));
	int res;
	char buffer[CONST_BUFFER_SIZE];
	#ifdef DEBUG
	printf("[DEBU]\tirc_client_connect - ip{0}: %s port{1}: %s nick{2}: %s\n", ip, port, nick);
	#endif
	if (ip == NULL || port == NULL || nick == NULL)
	{
		return -1;
	}
	*outHandle = irc;
	irc->callbacks = (IRCCALLBACK*)malloc(sizeof(IRCCALLBACK) * CONST_CALLBACK_INITIAL);
	irc->callbacks_next = 0;
	irc->callbacks_size = CONST_CALLBACK_INITIAL;
	irc->callbacks_raw = (IRCCALLBACKRAW*)malloc(sizeof(IRCCALLBACKRAW) * CONST_CALLBACK_INITIAL);
	irc->callbacks_raw_next = 0;
	irc->callbacks_raw_size = CONST_CALLBACK_INITIAL;
	FNC(register_callback_raw)(irc, FNC(handle_ping));
	res = socket_create_client(ip, port, &irc->socket);

	if (res)
	{
		free(irc);
		#ifdef DEBUG
		printf("[DEBU]\tirc_client_connect - error: %d\n", res);
		#endif
		return -2;
	}
	for (res = 0; res < 3; res += FNC(poll)(irc, buffer, CONST_BUFFER_SIZE));

	//handshake
	snprintf(buffer, CONST_BUFFER_SIZE, "NICK %s\r\n", nick); FNC(send)(irc, buffer, strlen(buffer));
	snprintf(buffer, CONST_BUFFER_SIZE, "USER %s 0 * :Mr Bot\r\n", nick); FNC(send)(irc, buffer, strlen(buffer));
	FNC(poll)(irc, buffer, CONST_BUFFER_SIZE);
	FNC(poll)(irc, buffer, CONST_BUFFER_SIZE);


	FNC(register_callback_raw)(irc, FNC(handle_commandCallbacks));
	irc->antiflood_last = time(NULL);
	irc->antiflood_out = 0;

	return 0;
}
void FNC(close)(IRCHANDLE* handle)
{
	if (handle == NULL)
		return;
	IRC* irc = (IRC*)*handle;
	#ifdef DEBUG
	printf("[DEBU]\tirc_client_close - closing socket\n");
	#endif
	socket_close(irc->socket);
	#ifdef DEBUG
	printf("[DEBU]\tirc_client_close - freeing irc->callbacks\n");
	#endif
	free(irc->callbacks);
	#ifdef DEBUG
	printf("[DEBU]\tirc_client_close - freeing irc->callbacks_raw\n");
	#endif
	free(irc->callbacks_raw);
	#ifdef DEBUG
	printf("[DEBU]\tirc_client_close - irc\n");
	#endif
	free(irc);
	(*handle) = NULL;
}
void FNC(register_callback)(IRCHANDLE handle, IRCCALLBACK callback)
{
	IRC* irc = (IRC*)handle;
	if (irc->callbacks_next == irc->callbacks_size)
	{
		irc->callbacks_size += CONST_CALLBACK_INC;
		irc->callbacks = (IRCCALLBACK*)realloc(irc->callbacks, sizeof(IRCCALLBACK) * irc->callbacks_size);
	}
	irc->callbacks[irc->callbacks_next] = callback;
	irc->callbacks_next++;
}
void FNC(register_callback_raw)(IRCHANDLE handle, IRCCALLBACKRAW callback)
{
	IRC* irc = (IRC*)handle;
	if (irc->callbacks_raw_next == irc->callbacks_raw_size)
	{
		irc->callbacks_raw_size += CONST_CALLBACK_INC;
		irc->callbacks_raw = (IRCCALLBACKRAW*)realloc(irc->callbacks_raw, sizeof(IRCCALLBACKRAW) * irc->callbacks_raw_size);
	}
	irc->callbacks_raw[irc->callbacks_raw_next] = callback;
	irc->callbacks_raw_next++;
}
int FNC(poll)(IRCHANDLE handle, char* buffer, unsigned int bufferSize)
{
	IRC* irc = (IRC*)handle;
	int res;
	int i;
	IRCCALLBACKRAW cb;
	char* index = 0;
	int count = 0;

	#ifdef DEBUG
	printf("[DEBU]\tirc_client_poll - Start polling ...\n");
	#endif

	res = recv(irc->socket, buffer, bufferSize - 2, 0);
	if (res <= 0)
	{
		return -1;
	}
	buffer[res] = '\0';
	buffer[res + 1] = '\0';
	#ifdef DEBUG
	printf("[DEBU]\tirc_client_poll - Received %d bytes\n", res);
	#endif
	while ((index = strchr(buffer, '\n')) != NULL)
	{
		index -= (long)buffer;
		count++;
		buffer[(long)index] = '\0';
		str_repchr(buffer, '\r', ' ', (int)(long)index);
		printf("[ <--]\t%s\n", buffer);
		#ifdef DEBUG
		printf("[DEBU]\tirc_client_poll - Processing Message ...\n", res);
		#endif
		for (i = 0; i < irc->callbacks_raw_next; i++)
		{
			cb = irc->callbacks_raw[i];
			if (cb(irc, buffer, res))
			{
				break;
			}
		}
		buffer += (long)index + 1;
	}
	#ifdef DEBUG
	printf("[DEBU]\tirc_client_poll - End polling with %d messages processed\n", count);
	#endif
	return count;
}
int FNC(send)(IRCHANDLE handle, const char* buffer, unsigned int bufferSize)
{
	IRC* irc = (IRC*)handle;
	int res;
	double dtime = difftime(time(NULL), irc->antiflood_last);
	if (dtime > CONST_ANTIFLOOD_THRESHOLD_TIME)
	{
		irc->antiflood_last = time(NULL);
		irc->antiflood_out = 0;
	}
	if (irc->antiflood_out > CONST_ANTIFLOOD_THRESHOLD_BYTES)
	{
		Sleep((CONST_ANTIFLOOD_THRESHOLD_TIME - dtime) * 1000);
	}
	irc->antiflood_out += bufferSize;
	printf("[--> ]\t%s", buffer);
	res = send(irc->socket, buffer, bufferSize, 0);
	if (res <= 0)
	{
		return -1;
	}
	return 0;
}

int FNC(handle_ping)(IRCHANDLE handle, const char* msg, unsigned int msgLen)
{
	char* buffer;
	if (msgLen < 4)
		return 0;
	if (msg[0] == 'P' && msg[1] == 'I' && msg[2] == 'N' && msg[3] == 'G')
	{
		buffer = alloca(sizeof(char) * msgLen + 1);
		memcpy(buffer, msg, msgLen + 1);
		buffer[msgLen - 1] = '\n';
		buffer[1] = 'O';
		FNC(send)(handle, buffer, msgLen);
		return 1;
	}
	return 0;
}
int FNC(handle_commandCallbacks)(IRCHANDLE handle, const char* msg, unsigned int msgLen)
{
	int index;
	char* type;
	irc_command cmd;
	IRC* irc = (IRC*)handle;
	IRCCALLBACK cb;
	int i;
	if (msg[0] == ':')
	{
		msg++;
		index = strchr(msg, ' ') - msg;
		cmd.sender = alloca(sizeof(char) * index + 1);
		cmd.sender[index] = '\0';
		memcpy(cmd.sender, msg, index);

		msg += index + 1;
		index = strchr(msg, ' ') - msg;
		type = alloca(sizeof(char) * index + 1);
		type[index] = '\0';
		memcpy(type, msg, index);

		msg += index + 1;
		index = strchr(msg, ' ') - msg;
		if (index < 0)
			index = strlen(msg);
		cmd.receiver = alloca(sizeof(char) * index + 1);
		cmd.receiver[index] = '\0';
		memcpy(cmd.receiver, msg, index);

		if ((msg + index)[0] != '\0')
		{
			msg += index + 1;
			index = strlen(msg);
			cmd.content = alloca(sizeof(char) * index + 1);
			cmd.content[index] = '\0';
			memcpy(cmd.content, msg, index);
		}
		else
		{
			cmd.content = alloca(sizeof(char));
			cmd.content[0] = '\0';
		}
		if (type[0] >= '0' && type[0] <= '9')
		{
			cmd.type = atoi(type);
		}
		else
		{
			if (strstr(type, "PRIVMSG"))
			{
				cmd.type = IRC_PRIVMSG;
			}
			else if (strstr(type, "MODE"))
			{
				cmd.type = IRC_MODE;
			}
			else if (strstr(type, "INVITE"))
			{
				cmd.type = IRC_INVITE;
			}
			else if (strstr(type, "PART"))
			{
				cmd.type = IRC_PART;
			}
			else if (strstr(type, "JOIN"))
			{
				cmd.type = IRC_JOIN;
			}
			else if (strstr(type, "KICK"))
			{
				cmd.type = IRC_KICK;
			}
			else if (strstr(type, "NICK"))
			{
				cmd.type = IRC_NICK;
			}
			else
			{
				printf("[INFO]\tUnknown type '%s'\n", type);
				cmd.type = -1;
			}
		}

		for (i = 0; i < irc->callbacks_next; i++)
		{
			cb = irc->callbacks[i];
			if (cb(handle, &cmd))
			{
				break;
			}
		}
	}
	return 0;
}
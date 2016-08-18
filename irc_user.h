#pragma once
#include "irc.h"

#include <time.h>

#define IRC_USER_ANTIFLOOD 1000

typedef struct
{
	char* username;
	time_t last_request;
} USER;

typedef struct
{
	char* channel_name;
	USER** users;
	unsigned int users_size;
} CHANNEL;

void irc_user_init(void);

/*
Checks if given users message is allowed to be parsed by the bot or if he is trying to flood.

returns 0 if he is not flooding
returns 1 if he currently tries to flood the bot (last request time is < const value)
*/
int irc_user_check_antiflood(const char*, const char*);

/*
Callback function for <irc.h>
*/
int irc_user_handleUserFlow(IRCHANDLE, const irc_command*);

void irc_user_remove_user(const char*, const char*);
const USER* irc_user_get_user(const char*, const char*);
void irc_user_free_user(USER**);
void irc_user_free_channel(CHANNEL**);
void irc_user_uninit(void);
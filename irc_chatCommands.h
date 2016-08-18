#pragma once
#include "irc.h"
#include "config.h"


typedef bool(*CHATCOMMAND)(IRCHANDLE, const irc_command*, unsigned int, const char**, char*, unsigned int);


bool irc_chat_handle_chatcommands(IRCHANDLE, const irc_command*);
void irc_chat_commands_init(CONFIG);
void irc_chat_commands_uninit(void);

void irc_chat_commands_add_command(CHATCOMMAND, const char*, const char*, bool, bool);


const char* random_response(const char*);
const char* random_error_message(void);
const char* random_unknowncommand_message(void);
const char* random_notallowed_message(void);
bool is_auth_user(const char*);

#ifdef IRC_CHATCOMMANDS_PRIVATE
typedef struct
{
	CHATCOMMAND cmd;
	char* name;
	char** args;
	char** args_defaults;
	unsigned int args_size;
	bool requires_auth;
	bool direct_only;
}COMMANDCONTAINER;
#endif
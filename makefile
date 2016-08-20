COMPILER = gcc
ARGS =  -g -gdwarf-2
CARGS = -c

prog: main.o irc.o networking.o yxml.o config.o irc_chatCommands.o string_op.o irc_user.o
	$(COMPILER) $(ARGS) -o prog main.o irc.o networking.o yxml.o config.o irc_chatCommands.o string_op.o irc_user.o

main.o: main.c
	$(COMPILER) $(ARGS) $(CARGS) main.c

irc.o: irc.c
	$(COMPILER) $(ARGS) $(CARGS) irc.c
	
networking.o: networking.c
	$(COMPILER) $(ARGS) $(CARGS) networking.c

yxml.o: yxml.c
	$(COMPILER) $(ARGS) $(CARGS) yxml.c
	
config.o: config.c
	$(COMPILER) $(ARGS) $(CARGS) config.c
	
irc_chatCommands.o: irc_chatCommands.c
	$(COMPILER) $(ARGS) $(CARGS) irc_chatCommands.c
	
string_op.o: string_op.c
	$(COMPILER) $(ARGS) $(CARGS) string_op.c

irc_user.o: irc_user.c
	$(COMPILER) $(ARGS) $(CARGS) irc_user.c

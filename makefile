prog: main.o irc.o networking.o yxml.o config.o irc_chatCommands.o string_op.o
	gcc -o prog main.o irc.o networking.o yxml.o config.o irc_chatCommands.o string_op.o

main.o: main.c
	gcc -c main.c

irc.o: irc.c
	gcc -c irc.c
	
networking.o: networking.c
	gcc -c networking.c

yxml.o: yxml.c
	gcc -c yxml.c
	
config.o: config.c
	gcc -c config.c
	
irc_chatCommands.o: irc_chatCommands.c
	gcc -c irc_chatCommands.c

string_op.o: string_op.c
	gcc -c string_op.c
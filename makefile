COMPILER = gcc
ARGS =  -g -rdynamic
CARGS = -c
LIBS = lua/liblua53.a

prog:
	$(COMPILER) $(ARGS) $(CARGS) main.c
	$(COMPILER) $(ARGS) $(CARGS) irc.c
	$(COMPILER) $(ARGS) $(CARGS) networking.c
	$(COMPILER) $(ARGS) $(CARGS) yxml.c
	$(COMPILER) $(ARGS) $(CARGS) config.c
	$(COMPILER) $(ARGS) $(CARGS) irc_chatCommands.c
	$(COMPILER) $(ARGS) $(CARGS) string_op.c
	$(COMPILER) $(ARGS) $(CARGS) irc_user.c
	$(COMPILER) $(ARGS) -o prog main.o irc.o networking.o yxml.o config.o irc_chatCommands.o string_op.o irc_user.o $(LIBS)

clean:
	rm -f haupt
	rm -f haupt.o up.o db.o
	rm -f *.bak *~

remake: clean prog
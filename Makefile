
CC = gcc
CFLAGS = -g -Wall -O2

compile: y.tab.c lex.yy.c typecheckfuncs.h header.h parsetreefuncs.h codegenfuncs.h
	$(CC) $(CFLAGS) lex.yy.c y.tab.c -o compile

lex.yy.c : scanner.l
	flex scanner.l

y.tab.c : parser.y
	yacc -d parser.y


.PHONY: clean
clean:
	/bin/rm -f compile lex.yy.c y.tab.c y.tab.h

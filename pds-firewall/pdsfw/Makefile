LOGIN=xsruba03

obj-m += pdsfw.o

all: pdscli
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf example lex.yy.c grammar.tab.{c,h} grammar.output pdscli

pdscli: grammar.tab.c lex.yy.c pdscli.c
	gcc -g lex.yy.c grammar.tab.c pdscli.c -o pdscli

lex.yy.c: tokens.lex grammar.tab.h
	flex tokens.lex

grammar.tab.c: grammar.y
	bison -d --verbose grammar.y

zip:
	tar -czf $(LOGIN).tar.gz grammar.y pdsfw.c pdscli.c tokens.lex

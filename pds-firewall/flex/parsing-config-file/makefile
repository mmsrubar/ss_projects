all: lex.c grammar.tab.c
	gcc lex.yy.c grammar.tab.c -o example

lex.c:		
	flex-2.5.4a tokens.lex

grammar.tab.c:
	bison -d grammar.y

clean:
	rm -rf example lex.yy.c grammar.tab.{c,h}

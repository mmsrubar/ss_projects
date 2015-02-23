all: lex.yy.c grammar.tab.c
	gcc -g lex.yy.c grammar.tab.c -lfl -o example

lex.yy.c:
	flex-2.5.4a tokens.lex

grammar.tab.c:
	bison -d --verbose grammar.y

clean:
	rm -rf example lex.yy.c grammar.tab.{c,h}

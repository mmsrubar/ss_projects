all: lex.c sample.tab.c
	gcc lex.yy.c sample.tab.c -o example

lex.c:		
	flex-2.5.4a sample.lex

sample.tab.c:
	bison -d sample.y

clean:
	rm -rf example lex.yy.c sample.tab.{c,h}

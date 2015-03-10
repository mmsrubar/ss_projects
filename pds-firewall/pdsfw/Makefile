LOGIN=xsruba03

obj-m += pdsfw.o

all: pdscli
	# -C will step into the kernel tree
	# -M will jump back to you pwd
	#make -C /media/sf_pds-kernel/linux-3.18.8	M=$(PWD) modules
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	cp pdsfw.ko pdsfw
clean:
	#make -C /media/sf_pds-kernel/linux-3.18.8	M=$(PWD) clean
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf $(LOGIN).tar.gz example lex.yy.c grammar.tab.{c,h} grammar.output pdscli pdsfw

pdscli: grammar.tab.c lex.yy.c pdscli.c
	gcc -Wextra  lex.yy.c grammar.tab.c pdscli.c -o pdscli

lex.yy.c: tokens.lex grammar.tab.h
	flex tokens.lex

grammar.tab.c: grammar.y
	bison -d --verbose grammar.y

zip:
	tar -czf $(LOGIN).tar.gz grammar.y pdsfw.c pdscli.c tokens.lex doc/doc.pdf
	@echo "!!!!!!! NEZAPOME ZMENIT FLEX-2... NA FLEX !!!!!!! "

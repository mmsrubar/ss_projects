#pri sestavovani binarky pro testovani zapni optimalizace
all:	cgp solaris

cgp:	cgp.cpp cgp.h
		g++ cgp.cpp -g -O3 -o bin

solaris:	cgp.cpp cgp.h
	gcc -Wl,-rpath,/usr/sfw/lib -O3 -o krok-bin cgp.cpp -lstdc++


clean:
		rm cgp core.* log* bin krok-bin

cleanlog:
		rm log_*.*

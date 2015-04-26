#pri sestavovani binarky pro testovani zapni optimalizace
all:	linux solaris

linux:	cgp_psnr.cpp cgp.cpp cgp.h
		g++ cgp.cpp -g -o linux-bin -lm
		g++ cgp_psnr.cpp -g -o linux-bin-psnr -lm

solaris:	cgp.cpp cgp.h
	gcc -Wl,-rpath,/usr/sfw/lib -O3 -o krok-bin cgp.cpp -lstdc++ 


clean:
		rm core.* log* krok-bin linux-bin

cleanlog:
		rm log_*.*

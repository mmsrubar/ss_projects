#pri sestavovani binarky pro testovani zapni optimalizace
all:	linux

linux:	cgp.cpp cgp.h
		g++ cgp.cpp -g -o bin -lm
		#g++ cgp_psnr.cpp -g -o bin-psnr -lm

#solaris:	cgp.cpp cgp.h
#	gcc -Wl,-rpath,/usr/sfw/lib -O3 -o krok-bin cgp.cpp -lstdc++ 


clean:
		rm core.* log* bin bin-psnr

cleanlog:
		rm log_*.*

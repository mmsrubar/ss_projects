all:	cgp

cgp:	cgp.cpp cgp.h
		#g++ cgp.cpp -lm -g -o cgp
		g++ cgp.cpp -g -O3 -o bin
		echo "pri sestavovani binarky pro testovani zapni optimalizace"

clean:
		rm cgp core.* log* bin

cleanlog:
		rm log_*.*

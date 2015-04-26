PRG=plg-2-nka
LOGIN=xsruba03
ZIPNAME=flp-fun-$(LOGIN).zip

$(PRG): $(PRG).hs
	ghc --make $(PRG).hs -o $(PRG)

clean:
	rm -rf $(PRG)  $(PRG).{hi,o} $(ZIPNAME)

zip:
	zip -r $(ZIPNAME) $(PRG).hs README Makefile test_me.sh tests/*.{in,ok}

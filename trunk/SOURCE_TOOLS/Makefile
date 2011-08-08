
all:
		@echo "use make clean"

clean:
		rm -f *.stamp valgrind2grep.lst postcompile.sav

valgrind_update:
		./arb_valgrind update

# --------------------------------------------------------------------------------

dep.clean:
	rm -f dep.*

% : %.tmp
	./mv_if_diff $< $@

dep.alltargets.tmp:
	./needed_libs.pl -U -I -F CORE/libCORE.so | sed -e 's/ /\n/g' >$@

dep.libtargets: dep.alltargets
	grep -E '\.(a|o|so)' $< > $@ || true

dep.4dummy: dep.libtargets
	./gen_dep.pl dummy <$< >$@

libdepends: dep.4dummy



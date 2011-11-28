.SUFFIXES: .o .cxx 

all:
		@echo "use make clean"

clean:
		rm -f *.stamp valgrind2grep.lst postcompile.sav arb_main.o

valgrind_update:
		./arb_valgrind update

# --------------------------------------------------------------------------------

.cxx.o:
	$(CPP) $(cflags) -c -o $@ $(@:.o=.cxx)  $(CPPINCLUDES) $(POST_COMPILE)

mainwrapper: arb_main.o

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



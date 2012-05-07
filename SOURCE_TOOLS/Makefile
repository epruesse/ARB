.SUFFIXES: .o .cxx 

all:
		@echo "use make clean"

# --------------------------------------------------------------------------------

OBJECTS=arb_main_cpp.o arb_main_c.o

arb_main_cpp.o: arb_main.cxx Makefile
	$(CPP) $(cflags) -c $< -o $@ $(CPPINCLUDES) $(POST_COMPILE)

arb_main_c.o: arb_main.c arb_main.cxx Makefile
	$(ACC) $(cflags) -c $< -o $@ $(CPPINCLUDES) $(POST_COMPILE)

mainwrapper: $(OBJECTS)

# --------------------------------------------------------------------------------

clean: dep.clean
		rm -f stamp.* valgrind2grep.lst postcompile.sav $(OBJECTS)

valgrind_update:
		./arb_valgrind update

# --------------------------------------------------------------------------------

dep.clean:
	rm -f dep.*

% : %.tmp
	./mv_if_diff $< $@

CORE_LIB=lib/libCORE.so

dep.alltargets.tmp:
	./needed_libs.pl -U -I -F $(CORE_LIB) | sed -e 's/ /\n/g' >$@
	echo $(CORE_LIB) >>$@

dep.libtargets: dep.alltargets
	grep -E '\.(a|o|so)' $< > $@ || true

dep.4dummy.tmp: dep.libtargets 
	./gen_dep.pl dummy <$< >$@

libdepends: dep.4dummy



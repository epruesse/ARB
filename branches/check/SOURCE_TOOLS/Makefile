.SUFFIXES: .o .cxx 

SED:=$(ARBHOME)/SH/arb_sed

all:
		@echo "use make clean"

# --------------------------------------------------------------------------------

OBJECTS=arb_main_cpp.o arb_main_c.o

ARB_MAIN_DEPENDS=Makefile arb_main.h

arb_main_cpp.o: arb_main.cxx $(ARB_MAIN_DEPENDS) 
	$(A_CXX) $(cflags) $(cxxflags) -c $< -o $@ $(CXX_INCLUDES) $(POST_COMPILE) $<

arb_main_c.o: arb_main.c $(ARB_MAIN_DEPENDS) arb_main.cxx
	$(A_CC) $(cflags) -c $< -o $@ $(CC_INCLUDES) $(POST_COMPILE) $<

mainwrapper: $(OBJECTS)

# --------------------------------------------------------------------------------

clean: 
	rm -f stamp.* valgrind2grep.lst postcompile.sav $(OBJECTS)

valgrind_update:
	./arb_valgrind update

VECTORIZED_SOURCE=vectorized.source

vectorize_checks:
	( echo "# [generated] list of source-files for which loop vectorization shall be checked"; \
	  ( cd $(ARBHOME); \
	    grep --recursive --files-with-matches "--include=*.c" "--include=*.cxx" LOOP_VECTORIZED . \
	    | sort \
	    | $(SED) -e 's/^\.\///' \
          ) \
	) > $(VECTORIZED_SOURCE).tmp
	./mv_if_diff $(VECTORIZED_SOURCE).tmp $(VECTORIZED_SOURCE)

# --------------------------------------------------------------------------------

# library dependencies generated here always use dynamic libnames,
# even if LINK_STATIC=1 defined in ../Makefile@LINK_STATIC
#
# The files generated here are not used yet (so the above does not matter)
# They are intended to be used to link all ARB binaries
# (instead of using explicit link commands in ../Makefile)

% : %.tmp
	./mv_if_diff $< $@

CORE_LIB=lib/libCORE.so

TMP=dep.alltargets.tmp.tmp

dep.alltargets.tmp: Makefile needed_libs.pl
	LINK_STATIC=0;./needed_libs.pl -U -I -F $(CORE_LIB) > $(TMP)
	$(SED) -e 's/ /\n/g' < $(TMP) > $@
	rm $(TMP)
	echo $(CORE_LIB) >>$@

dep.libtargets: dep.alltargets
	grep -E '\.(a|o|so)' $< > $@ || true

dep.4dummy.tmp: dep.libtargets 
	./gen_dep.pl dummy <$< >$@

libdepends: dep.4dummy



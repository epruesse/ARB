.SUFFIXES: .o .c .cxx .depend

OBJECTS = $(subst .cxx,.o,$(wildcard *.cxx))

LIB_SHARED=../lib/$(MAIN:.a=).$(SHARED_LIB_SUFFIX)

it: do_$(MAIN)

do_WINDOW_GTK.a:
	@echo "Error: MAIN='$(MAIN)' is set wrong"
	false

do_libWINDOW_GTK.a: $(LIB_SHARED)

$(LIB_SHARED): $(OBJECTS)
	$(LINK_SHARED_LIB) $(LIB_SHARED) $(OBJECTS)

.cxx.o:
	$(CPPLIB) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

.c.o:
	$(ACCLIB) $(cflags) -c $< $(CPPINCLUDES) $(POST_COMPILE)

clean:
	rm -f $(OBJECTS) *.a *.so

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

aw_awar.o: aw_awar.hxx
aw_awar.o: aw_base.hxx
aw_awar.o: $(ARBHOME)/INCLUDE/arb_assert.h
aw_awar.o: $(ARBHOME)/INCLUDE/arb_core.h
aw_awar.o: $(ARBHOME)/INCLUDE/arbdb_base.h
aw_awar.o: $(ARBHOME)/INCLUDE/arbtools.h
aw_awar.o: $(ARBHOME)/INCLUDE/attributes.h
aw_awar.o: $(ARBHOME)/INCLUDE/cb.h
aw_awar.o: $(ARBHOME)/INCLUDE/dupstr.h
aw_awar.o: $(ARBHOME)/INCLUDE/test_global.h

aw_root.o: aw_base.hxx
aw_root.o: aw_root.hxx
aw_root.o: $(ARBHOME)/INCLUDE/arb_assert.h
aw_root.o: $(ARBHOME)/INCLUDE/arb_core.h
aw_root.o: $(ARBHOME)/INCLUDE/arb_error.h
aw_root.o: $(ARBHOME)/INCLUDE/arbtools.h
aw_root.o: $(ARBHOME)/INCLUDE/attributes.h
aw_root.o: $(ARBHOME)/INCLUDE/cb.h
aw_root.o: $(ARBHOME)/INCLUDE/dupstr.h
aw_root.o: $(ARBHOME)/INCLUDE/smartptr.h
aw_root.o: $(ARBHOME)/INCLUDE/test_global.h

# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt
.SUFFIXES: .o .c .cxx .depend

# --------------------------------------------------------------------------------
# modify cflags 

RAISE_WARNINGS=0# no warnings in this subtree
LOCAL_DEFINES=-DSIMPLE_ARB_ASSERT

LOCAL_MAKEDEPENDFLAGS=$(MAKEDEPENDFLAGS) $(LOCAL_DEFINES)
LOCAL_CFLAGS:=$(cflags) $(LOCAL_DEFINES)
ifeq ('$(RAISE_WARNINGS)','0')
LOCAL_CFLAGS:=$(subst -W -Wall,-w,$(LOCAL_CFLAGS))
endif

# --------------------------------------------------------------------------------


CPP_OBJECTS = island_hopping.o i-hopper.o align.o mem.o trnsprob.o

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)
.c.o:
	$(A_CC) $(LOCAL_CFLAGS) -DARB -c $<  $(CC_INCLUDES) $(POST_COMPILE) $<
.cxx.o:
	$(A_CXX) $(LOCAL_CFLAGS) $(cxxflags) -DARB -c $<  $(CXX_INCLUDES) $(POST_COMPILE) $<

clean:
	rm -f $(CPP_OBJECTS) *.a

DEPENDS = $(CPP_OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(LOCAL_MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.c.depend:
	$(MAKEDEPEND) -f- $(LOCAL_MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
	$(MAKEDEPEND) -f- $(LOCAL_MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

align.o: defs.h
align.o: i-hopper.h
align.o: mem.h
align.o: trnsprob.h

i-hopper.o: defs.h
i-hopper.o: i-hopper.h
i-hopper.o: mem.h

island_hopping.o: defs.h
island_hopping.o: i-hopper.h
island_hopping.o: island_hopping.h
island_hopping.o: mem.h
island_hopping.o: $(ARBHOME)/INCLUDE/arb_assert.h
island_hopping.o: $(ARBHOME)/INCLUDE/arb_core.h
island_hopping.o: $(ARBHOME)/INCLUDE/arbtools.h
island_hopping.o: $(ARBHOME)/INCLUDE/attributes.h
island_hopping.o: $(ARBHOME)/INCLUDE/cxxforward.h
island_hopping.o: $(ARBHOME)/INCLUDE/dupstr.h
island_hopping.o: $(ARBHOME)/INCLUDE/gccver.h
island_hopping.o: $(ARBHOME)/INCLUDE/pos_range.h
island_hopping.o: $(ARBHOME)/INCLUDE/test_global.h

mem.o: mem.h
mem.o: $(ARBHOME)/INCLUDE/attributes.h
mem.o: $(ARBHOME)/INCLUDE/gccver.h

trnsprob.o: defs.h
trnsprob.o: mem.h
trnsprob.o: trnsprob.h
trnsprob.o: $(ARBHOME)/INCLUDE/arb_assert.h
trnsprob.o: $(ARBHOME)/INCLUDE/arb_simple_assert.h
trnsprob.o: $(ARBHOME)/INCLUDE/test_global.h

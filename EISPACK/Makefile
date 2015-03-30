# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend


CPP_OBJECTS =  eispack.o d_sign.o

$(MAIN): $(CPP_OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)

clean:
	rm -f $(CPP_OBJECTS) *.a

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE)

DEPENDS = $(CPP_OBJECTS:.o=.depend)
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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

d_sign.o: f2c.h

eispack.o: f2c.h

.SUFFIXES: .o .c .cxx .depend

#Add additional subdirectories to this list.
SUBDIRS = devices




#############################################################
###END OF CONFIG AREA. DO NOT CHANGE ANYTHING BELOW THIS LINE
#############################################################

vpath %.cxx $(SUBDIRS)

#Add objects from main directory
OBJECTS = $(subst .cxx,.o,$(wildcard *.cxx)) 

#Add objects from subdirs
OBJECTS += $(foreach dir,$(SUBDIRS),$(notdir $(subst .cxx,.o,$(wildcard $(dir)/*.cxx)))) 

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

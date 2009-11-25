# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .c .cxx .depend

C_OBJECTS = 
CPP_OBJECTS = \
	Translate.o \

OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

$(MAIN): $(OBJECTS)
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(AINCLUDES)

clean:
	rm -f $(OBJECTS) *.a

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

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

Translate.o: Translate.hxx
Translate.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
Translate.o: $(ARBHOME)/INCLUDE/ad_prot.h
Translate.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
Translate.o: $(ARBHOME)/INCLUDE/AP_codon_table.hxx
Translate.o: $(ARBHOME)/INCLUDE/AP_pro_a_nucs.hxx
Translate.o: $(ARBHOME)/INCLUDE/arb_assert.h
Translate.o: $(ARBHOME)/INCLUDE/arbdb.h
Translate.o: $(ARBHOME)/INCLUDE/arbdb_base.h
Translate.o: $(ARBHOME)/INCLUDE/arbdbt.h
Translate.o: $(ARBHOME)/INCLUDE/arbtools.h
Translate.o: $(ARBHOME)/INCLUDE/attributes.h
Translate.o: $(ARBHOME)/INCLUDE/aw_device.hxx
Translate.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
Translate.o: $(ARBHOME)/INCLUDE/aw_position.hxx
Translate.o: $(ARBHOME)/INCLUDE/aw_root.hxx
Translate.o: $(ARBHOME)/INCLUDE/aw_window.hxx
Translate.o: $(ARBHOME)/INCLUDE/awt.hxx

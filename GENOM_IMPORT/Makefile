# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

# --------------------------------------------------------------------------------

# objects for library
CPP_OBJECTS =   \
		GenomeImport.o \
		Importer.o \
		DBwriter.o \
		SequenceBuffer.o \
		MetaInfo.o \
		Feature.o \
		Location.o \
		tools.o \

# --------------------------------------------------------------------------------

$(MAIN): $(CPP_OBJECTS)
	@$(ARBHOME)/SOURCE_TOOLS/binuptodate.pl $(MAIN) $(CPP_OBJECTS) || ( \
		echo "$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)"; \
		$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS) ; \
	)

.cxx.o:
	$(CPP) $(cflags) -c $< $(CPPINCLUDES)

.c.o:
	$(ACC) $(cflags) -c $< $(CPPINCLUDES)

clean:
	-rm *.o
	-rm *.a
	-rm *.bak
	-rm *~
	-rm *#
	-rm *\%

#.IGNORE:
#	clean

DEPENDS = $(CPP_OBJECTS:.o=.depend)
depends: $(DEPENDS)
	@cat $(DEPENDS) | grep -v '^#' >>Makefile
	@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
	$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
.cxx.depend:
	$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

DBwriter.o: DBwriter.h
DBwriter.o: defs.h
DBwriter.o: Feature.h
DBwriter.o: GenomeImport.h
DBwriter.o: Importer.h
DBwriter.o: Location.h
DBwriter.o: MetaInfo.h
DBwriter.o: MetaTag.h
DBwriter.o: SequenceBuffer.h
DBwriter.o: types.h
DBwriter.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
DBwriter.o: $(ARBHOME)/INCLUDE/ad_prot.h
DBwriter.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
DBwriter.o: $(ARBHOME)/INCLUDE/adGene.h
DBwriter.o: $(ARBHOME)/INCLUDE/arb_assert.h
DBwriter.o: $(ARBHOME)/INCLUDE/arb_error.h
DBwriter.o: $(ARBHOME)/INCLUDE/arbdb.h
DBwriter.o: $(ARBHOME)/INCLUDE/arbdb_base.h
DBwriter.o: $(ARBHOME)/INCLUDE/arbdbt.h
DBwriter.o: $(ARBHOME)/INCLUDE/arbtools.h
DBwriter.o: $(ARBHOME)/INCLUDE/attributes.h
DBwriter.o: $(ARBHOME)/INCLUDE/aw_device.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/aw_keysym.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/aw_position.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/aw_question.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/aw_root.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/aw_window.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/dupstr.h
DBwriter.o: $(ARBHOME)/INCLUDE/FileBuffer.h
DBwriter.o: $(ARBHOME)/INCLUDE/GEN.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/smartptr.h
DBwriter.o: $(ARBHOME)/INCLUDE/Translate.hxx

Feature.o: defs.h
Feature.o: Feature.h
Feature.o: Location.h
Feature.o: types.h
Feature.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
Feature.o: $(ARBHOME)/INCLUDE/arb_assert.h
Feature.o: $(ARBHOME)/INCLUDE/arbdb_base.h
Feature.o: $(ARBHOME)/INCLUDE/arbtools.h
Feature.o: $(ARBHOME)/INCLUDE/attributes.h
Feature.o: $(ARBHOME)/INCLUDE/dupstr.h
Feature.o: $(ARBHOME)/INCLUDE/smartptr.h

GenomeImport.o: DBwriter.h
GenomeImport.o: defs.h
GenomeImport.o: Feature.h
GenomeImport.o: GenomeImport.h
GenomeImport.o: Importer.h
GenomeImport.o: Location.h
GenomeImport.o: MetaInfo.h
GenomeImport.o: MetaTag.h
GenomeImport.o: SequenceBuffer.h
GenomeImport.o: tools.h
GenomeImport.o: types.h
GenomeImport.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
GenomeImport.o: $(ARBHOME)/INCLUDE/ad_prot.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arb_assert.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arb_error.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arbdb.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arbtools.h
GenomeImport.o: $(ARBHOME)/INCLUDE/attributes.h
GenomeImport.o: $(ARBHOME)/INCLUDE/aw_question.hxx
GenomeImport.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
GenomeImport.o: $(ARBHOME)/INCLUDE/aw_root.hxx
GenomeImport.o: $(ARBHOME)/INCLUDE/dupstr.h
GenomeImport.o: $(ARBHOME)/INCLUDE/FileBuffer.h
GenomeImport.o: $(ARBHOME)/INCLUDE/smartptr.h

Importer.o: DBwriter.h
Importer.o: defs.h
Importer.o: Feature.h
Importer.o: GenomeImport.h
Importer.o: Importer.h
Importer.o: Location.h
Importer.o: MetaInfo.h
Importer.o: MetaTag.h
Importer.o: SequenceBuffer.h
Importer.o: tools.h
Importer.o: types.h
Importer.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
Importer.o: $(ARBHOME)/INCLUDE/ad_prot.h
Importer.o: $(ARBHOME)/INCLUDE/arb_assert.h
Importer.o: $(ARBHOME)/INCLUDE/arb_error.h
Importer.o: $(ARBHOME)/INCLUDE/arbdb.h
Importer.o: $(ARBHOME)/INCLUDE/arbdb_base.h
Importer.o: $(ARBHOME)/INCLUDE/arbtools.h
Importer.o: $(ARBHOME)/INCLUDE/attributes.h
Importer.o: $(ARBHOME)/INCLUDE/dupstr.h
Importer.o: $(ARBHOME)/INCLUDE/FileBuffer.h
Importer.o: $(ARBHOME)/INCLUDE/smartptr.h

Location.o: defs.h
Location.o: Location.h
Location.o: tools.h
Location.o: types.h
Location.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
Location.o: $(ARBHOME)/INCLUDE/ad_prot.h
Location.o: $(ARBHOME)/INCLUDE/adGene.h
Location.o: $(ARBHOME)/INCLUDE/arb_assert.h
Location.o: $(ARBHOME)/INCLUDE/arb_error.h
Location.o: $(ARBHOME)/INCLUDE/arbdb.h
Location.o: $(ARBHOME)/INCLUDE/arbdb_base.h
Location.o: $(ARBHOME)/INCLUDE/arbtools.h
Location.o: $(ARBHOME)/INCLUDE/attributes.h
Location.o: $(ARBHOME)/INCLUDE/dupstr.h
Location.o: $(ARBHOME)/INCLUDE/smartptr.h

MetaInfo.o: defs.h
MetaInfo.o: MetaInfo.h
MetaInfo.o: MetaTag.h
MetaInfo.o: types.h
MetaInfo.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
MetaInfo.o: $(ARBHOME)/INCLUDE/arb_assert.h
MetaInfo.o: $(ARBHOME)/INCLUDE/arbdb_base.h
MetaInfo.o: $(ARBHOME)/INCLUDE/arbtools.h
MetaInfo.o: $(ARBHOME)/INCLUDE/attributes.h
MetaInfo.o: $(ARBHOME)/INCLUDE/dupstr.h
MetaInfo.o: $(ARBHOME)/INCLUDE/RegExpr.hxx

SequenceBuffer.o: defs.h
SequenceBuffer.o: SequenceBuffer.h
SequenceBuffer.o: types.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/arb_assert.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/arbdb_base.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/arbtools.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/attributes.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/dupstr.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/smartptr.h

tools.o: defs.h
tools.o: tools.h
tools.o: types.h
tools.o: $(ARBHOME)/INCLUDE/ad_k_prot.h
tools.o: $(ARBHOME)/INCLUDE/arb_assert.h
tools.o: $(ARBHOME)/INCLUDE/arbdb_base.h
tools.o: $(ARBHOME)/INCLUDE/arbtools.h
tools.o: $(ARBHOME)/INCLUDE/attributes.h
tools.o: $(ARBHOME)/INCLUDE/dupstr.h

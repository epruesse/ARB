# for variables passed from parent makefile see ../SOURCE_TOOLS/parent_make.txt

.SUFFIXES: .o .cxx .depend

# --------------------------------------------------------------------------------

# objects for library
CPP_OBJECTS=$(subst .cxx,.o,$(wildcard *.cxx))

# --------------------------------------------------------------------------------

$(MAIN): $(CPP_OBJECTS)
	@$(ARBHOME)/SOURCE_TOOLS/binuptodate.pl $(MAIN) $(CPP_OBJECTS) || ( \
		echo "$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS)"; \
		$(LINK_STATIC_LIB) $(MAIN) $(CPP_OBJECTS) ; \
	)

.cxx.o:
	$(A_CXX) $(cflags) $(cxxflags) -c $< $(CXX_INCLUDES) $(POST_COMPILE) $<

.c.o:
	$(A_CC) $(cflags) -c $< $(CC_INCLUDES) $(POST_COMPILE) $<

clean:
	-rm -f *.[ao] *.bak *~

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
# For formatting issues see SOURCE_TOOLS/fix_depends.pl (from main)

DBwriter.o: DBwriter.h
DBwriter.o: defs.h
DBwriter.o: Feature.h
DBwriter.o: GenomeImport.h
DBwriter.o: Importer.h
DBwriter.o: MetaInfo.h
DBwriter.o: MetaTag.h
DBwriter.o: SequenceBuffer.h
DBwriter.o: types.h
DBwriter.o: $(ARBHOME)/INCLUDE/ad_prot.h
DBwriter.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
DBwriter.o: $(ARBHOME)/INCLUDE/adGene.h
DBwriter.o: $(ARBHOME)/INCLUDE/arb_assert.h
DBwriter.o: $(ARBHOME)/INCLUDE/arb_core.h
DBwriter.o: $(ARBHOME)/INCLUDE/arb_error.h
DBwriter.o: $(ARBHOME)/INCLUDE/arb_msg.h
DBwriter.o: $(ARBHOME)/INCLUDE/arb_string.h
DBwriter.o: $(ARBHOME)/INCLUDE/arbdb.h
DBwriter.o: $(ARBHOME)/INCLUDE/arbdb_base.h
DBwriter.o: $(ARBHOME)/INCLUDE/arbdbt.h
DBwriter.o: $(ARBHOME)/INCLUDE/arbtools.h
DBwriter.o: $(ARBHOME)/INCLUDE/attributes.h
DBwriter.o: $(ARBHOME)/INCLUDE/aw_base.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/aw_question.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
DBwriter.o: $(ARBHOME)/INCLUDE/cb.h
DBwriter.o: $(ARBHOME)/INCLUDE/cb_base.h
DBwriter.o: $(ARBHOME)/INCLUDE/cbtypes.h
DBwriter.o: $(ARBHOME)/INCLUDE/cxxforward.h
DBwriter.o: $(ARBHOME)/INCLUDE/downcast.h
DBwriter.o: $(ARBHOME)/INCLUDE/dupstr.h
DBwriter.o: $(ARBHOME)/INCLUDE/gccver.h
DBwriter.o: $(ARBHOME)/INCLUDE/GEN.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/items.h
DBwriter.o: $(ARBHOME)/INCLUDE/Location.h
DBwriter.o: $(ARBHOME)/INCLUDE/smartptr.h
DBwriter.o: $(ARBHOME)/INCLUDE/static_assert.h
DBwriter.o: $(ARBHOME)/INCLUDE/test_global.h
DBwriter.o: $(ARBHOME)/INCLUDE/Translate.hxx
DBwriter.o: $(ARBHOME)/INCLUDE/ttypes.h

Feature.o: defs.h
Feature.o: Feature.h
Feature.o: types.h
Feature.o: $(ARBHOME)/INCLUDE/arb_assert.h
Feature.o: $(ARBHOME)/INCLUDE/arb_core.h
Feature.o: $(ARBHOME)/INCLUDE/arbtools.h
Feature.o: $(ARBHOME)/INCLUDE/attributes.h
Feature.o: $(ARBHOME)/INCLUDE/cxxforward.h
Feature.o: $(ARBHOME)/INCLUDE/dupstr.h
Feature.o: $(ARBHOME)/INCLUDE/gccver.h
Feature.o: $(ARBHOME)/INCLUDE/Location.h
Feature.o: $(ARBHOME)/INCLUDE/smartptr.h
Feature.o: $(ARBHOME)/INCLUDE/test_global.h

GenomeImport.o: DBwriter.h
GenomeImport.o: defs.h
GenomeImport.o: Feature.h
GenomeImport.o: GenomeImport.h
GenomeImport.o: Importer.h
GenomeImport.o: MetaInfo.h
GenomeImport.o: MetaTag.h
GenomeImport.o: SequenceBuffer.h
GenomeImport.o: tools.h
GenomeImport.o: types.h
GenomeImport.o: $(ARBHOME)/INCLUDE/ad_prot.h
GenomeImport.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arb_assert.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arb_core.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arb_diff.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arb_error.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arb_file.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arb_msg.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arb_stdstr.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arb_string.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arbdb.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arbdb_base.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arbdbt.h
GenomeImport.o: $(ARBHOME)/INCLUDE/arbtools.h
GenomeImport.o: $(ARBHOME)/INCLUDE/attributes.h
GenomeImport.o: $(ARBHOME)/INCLUDE/aw_base.hxx
GenomeImport.o: $(ARBHOME)/INCLUDE/aw_question.hxx
GenomeImport.o: $(ARBHOME)/INCLUDE/AW_rename.hxx
GenomeImport.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
GenomeImport.o: $(ARBHOME)/INCLUDE/cxxforward.h
GenomeImport.o: $(ARBHOME)/INCLUDE/downcast.h
GenomeImport.o: $(ARBHOME)/INCLUDE/dupstr.h
GenomeImport.o: $(ARBHOME)/INCLUDE/gccver.h
GenomeImport.o: $(ARBHOME)/INCLUDE/Location.h
GenomeImport.o: $(ARBHOME)/INCLUDE/smartptr.h
GenomeImport.o: $(ARBHOME)/INCLUDE/static_assert.h
GenomeImport.o: $(ARBHOME)/INCLUDE/test_global.h
GenomeImport.o: $(ARBHOME)/INCLUDE/test_unit.h

Importer.o: DBwriter.h
Importer.o: defs.h
Importer.o: Feature.h
Importer.o: GenomeImport.h
Importer.o: Importer.h
Importer.o: MetaInfo.h
Importer.o: MetaTag.h
Importer.o: SequenceBuffer.h
Importer.o: tools.h
Importer.o: types.h
Importer.o: $(ARBHOME)/INCLUDE/ad_prot.h
Importer.o: $(ARBHOME)/INCLUDE/arb_assert.h
Importer.o: $(ARBHOME)/INCLUDE/arb_core.h
Importer.o: $(ARBHOME)/INCLUDE/arb_error.h
Importer.o: $(ARBHOME)/INCLUDE/arb_msg.h
Importer.o: $(ARBHOME)/INCLUDE/arb_stdstr.h
Importer.o: $(ARBHOME)/INCLUDE/arb_string.h
Importer.o: $(ARBHOME)/INCLUDE/arbdb.h
Importer.o: $(ARBHOME)/INCLUDE/arbdb_base.h
Importer.o: $(ARBHOME)/INCLUDE/arbtools.h
Importer.o: $(ARBHOME)/INCLUDE/attributes.h
Importer.o: $(ARBHOME)/INCLUDE/BufferedFileReader.h
Importer.o: $(ARBHOME)/INCLUDE/cxxforward.h
Importer.o: $(ARBHOME)/INCLUDE/dupstr.h
Importer.o: $(ARBHOME)/INCLUDE/gccver.h
Importer.o: $(ARBHOME)/INCLUDE/Location.h
Importer.o: $(ARBHOME)/INCLUDE/smartptr.h
Importer.o: $(ARBHOME)/INCLUDE/test_global.h

MetaInfo.o: defs.h
MetaInfo.o: MetaInfo.h
MetaInfo.o: MetaTag.h
MetaInfo.o: types.h
MetaInfo.o: $(ARBHOME)/INCLUDE/arb_assert.h
MetaInfo.o: $(ARBHOME)/INCLUDE/arb_core.h
MetaInfo.o: $(ARBHOME)/INCLUDE/arbtools.h
MetaInfo.o: $(ARBHOME)/INCLUDE/attributes.h
MetaInfo.o: $(ARBHOME)/INCLUDE/cxxforward.h
MetaInfo.o: $(ARBHOME)/INCLUDE/dupstr.h
MetaInfo.o: $(ARBHOME)/INCLUDE/gccver.h
MetaInfo.o: $(ARBHOME)/INCLUDE/RegExpr.hxx
MetaInfo.o: $(ARBHOME)/INCLUDE/test_global.h

SequenceBuffer.o: defs.h
SequenceBuffer.o: SequenceBuffer.h
SequenceBuffer.o: types.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/arb_assert.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/arb_core.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/arbtools.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/attributes.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/cxxforward.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/dupstr.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/gccver.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/smartptr.h
SequenceBuffer.o: $(ARBHOME)/INCLUDE/test_global.h

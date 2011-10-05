# INCLUDE und GLOBALS MAIN CC cflags werden von aussen uebergeben

.SUFFIXES: .o .cxx .depend

C_OBJECTS =  ptpan_main.o ptpan_legacy.o huffman.o bin_tree.o dlist.o

CPP_OBJECTS = ptpan.o \
    ptpan_base.o \
	ptpan_tree.o \
	ptpan_tree_builder.o \
	ptpan_filter.o \
	ptpan_build_settings.o \
	dna5_alphabet_specifics.o \
	abstract_alphabet_specifics.o \
	arbdb_data_retriever.o \
	abstract_data_retriever.o

OBJECTS=$(C_OBJECTS) $(CPP_OBJECTS)

#AOBJECTS = #BI_helix.o

# --------------------------------------------------------------------------------

pan_cflags:=$(cflags) -I../HEADERLIBS

# --------------------------------------------------------------------------------

#$(MAIN): $(OBJECTS) $(AOBJECTS) Makefile
# $(LINK_STATIC_LIB) $(MAIN) $(OBJECTS) $(AOBJECTS)
$(MAIN): $(OBJECTS) 
	$(LINK_STATIC_LIB) $(MAIN) $(OBJECTS)
 
ifeq ('$(RAISE_WARNINGS)','0')
	@echo ----------------- warnings enabled again
endif

# $(OBJECTS): Makefile

.c.o:
		$(ACC) $(pan_cflags) -c $< $(AINCLUDES)  -I./../probe_bank/COM $(POST_COMPILE)
.cxx.o:
		$(CPP) $(pan_cflags) -c $<  $(CPPINCLUDES) -I./../probe_bank/COM $(POST_COMPILE)
proto:
		../AISC_MKPTPS/aisc_mkpt -E -z -A -w pt_prototypes.h $(OBJECTS:.o=.cxx) >pt_prototypes.h.tmp
		../SOURCE_TOOLS/mv_if_diff pt_prototypes.h.tmp pt_prototypes.h

clean:
		rm -f $(OBJECTS) *.a

DEPENDS = $(OBJECTS:.o=.depend)
depends: $(DEPENDS)
		@cat $(DEPENDS) | grep -v '^#' >>Makefile
		@rm $(DEPENDS)
$(DEPENDS): depend.init
depend.init:
		$(MAKEDEPEND) $(MAKEDEPENDFLAGS) 2>/dev/null # remove dependencies
		-@rm Makefile.bak # get rid of bak (generated for some dubious reason only in this directory)
.c.depend:
		$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@
.cxx.depend:
		$(MAKEDEPEND) -f- $(MAKEDEPENDFLAGS) $< 2>/dev/null >$@

# DO NOT DELETE THIS LINE -- make depend depends on it.

# Do not add dependencies manually - use 'make depend' in $ARBHOME
# For formatting issues see SOURCE_TOOLS/fix_depends.pl

abstract_alphabet_specifics.o: abstract_alphabet_specifics.h
abstract_alphabet_specifics.o: bin_tree.h
abstract_alphabet_specifics.o: dlist.h
abstract_alphabet_specifics.o: hash_key.h
abstract_alphabet_specifics.o: huffman.h
abstract_alphabet_specifics.o: ptpan.h
abstract_alphabet_specifics.o: types.h

abstract_data_retriever.o: abstract_alphabet_specifics.h
abstract_data_retriever.o: abstract_data_retriever.h
abstract_data_retriever.o: bin_tree.h
abstract_data_retriever.o: dlist.h
abstract_data_retriever.o: hash_key.h
abstract_data_retriever.o: huffman.h
abstract_data_retriever.o: ptpan.h
abstract_data_retriever.o: types.h

arbdb_data_retriever.o: abstract_alphabet_specifics.h
arbdb_data_retriever.o: abstract_data_retriever.h
arbdb_data_retriever.o: arbdb_data_retriever.h
arbdb_data_retriever.o: bin_tree.h
arbdb_data_retriever.o: dlist.h
arbdb_data_retriever.o: hash_key.h
arbdb_data_retriever.o: huffman.h
arbdb_data_retriever.o: ptpan.h
arbdb_data_retriever.o: types.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/ad_prot.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/adGene.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/arb_assert.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/arb_core.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/arb_error.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/arb_msg.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/arb_string.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/arbdb.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/arbdb_base.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/arbdbt.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/arbtools.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/attributes.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/dupstr.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/smartptr.h
arbdb_data_retriever.o: $(ARBHOME)/INCLUDE/test_global.h

bin_tree.o: bin_tree.h
bin_tree.o: dlist.h
bin_tree.o: types.h

dlist.o: dlist.h
dlist.o: types.h

dna5_alphabet_specifics.o: abstract_alphabet_specifics.h
dna5_alphabet_specifics.o: bin_tree.h
dna5_alphabet_specifics.o: dlist.h
dna5_alphabet_specifics.o: dna5_alphabet_specifics.h
dna5_alphabet_specifics.o: hash_key.h
dna5_alphabet_specifics.o: huffman.h
dna5_alphabet_specifics.o: ptpan.h
dna5_alphabet_specifics.o: types.h

huffman.o: huffman.h
huffman.o: types.h

ptpan.o: bin_tree.h
ptpan.o: dlist.h
ptpan.o: hash_key.h
ptpan.o: huffman.h
ptpan.o: ptpan.h
ptpan.o: types.h

ptpan_base.o: bin_tree.h
ptpan_base.o: dlist.h
ptpan_base.o: hash_key.h
ptpan_base.o: huffman.h
ptpan_base.o: ptpan.h
ptpan_base.o: ptpan_base.h
ptpan_base.o: types.h

ptpan_build_settings.o: abstract_alphabet_specifics.h
ptpan_build_settings.o: bin_tree.h
ptpan_build_settings.o: dlist.h
ptpan_build_settings.o: hash_key.h
ptpan_build_settings.o: huffman.h
ptpan_build_settings.o: ptpan.h
ptpan_build_settings.o: ptpan_build_settings.h
ptpan_build_settings.o: types.h

ptpan_filter.o: bin_tree.h
ptpan_filter.o: dlist.h
ptpan_filter.o: hash_key.h
ptpan_filter.o: huffman.h
ptpan_filter.o: ptpan.h
ptpan_filter.o: ptpan_filter.h
ptpan_filter.o: types.h

ptpan_legacy.o: abstract_alphabet_specifics.h
ptpan_legacy.o: bin_tree.h
ptpan_legacy.o: dlist.h
ptpan_legacy.o: hash_key.h
ptpan_legacy.o: huffman.h
ptpan_legacy.o: ptpan.h
ptpan_legacy.o: ptpan_base.h
ptpan_legacy.o: ptpan_legacy.h
ptpan_legacy.o: ptpan_tree.h
ptpan_legacy.o: types.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/ad_prot.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/aisc_global.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/arb_assert.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/arb_core.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/arb_defs.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/arb_error.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/arb_msg.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/arb_strbuf.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/arb_string.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/arbdb.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/arbtools.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/attributes.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/bytestring.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/dupstr.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/PT_com.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/PT_server.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/smartptr.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/struct_man.h
ptpan_legacy.o: $(ARBHOME)/INCLUDE/test_global.h

ptpan_main.o: abstract_alphabet_specifics.h
ptpan_main.o: abstract_data_retriever.h
ptpan_main.o: arbdb_data_retriever.h
ptpan_main.o: bin_tree.h
ptpan_main.o: dlist.h
ptpan_main.o: dna5_alphabet_specifics.h
ptpan_main.o: hash_key.h
ptpan_main.o: huffman.h
ptpan_main.o: ptpan.h
ptpan_main.o: ptpan_base.h
ptpan_main.o: ptpan_build_settings.h
ptpan_main.o: ptpan_legacy.h
ptpan_main.o: ptpan_tree.h
ptpan_main.o: ptpan_tree_builder.h
ptpan_main.o: types.h
ptpan_main.o: $(ARBHOME)/INCLUDE/ad_prot.h
ptpan_main.o: $(ARBHOME)/INCLUDE/ad_t_prot.h
ptpan_main.o: $(ARBHOME)/INCLUDE/aisc_func_types.h
ptpan_main.o: $(ARBHOME)/INCLUDE/aisc_global.h
ptpan_main.o: $(ARBHOME)/INCLUDE/arb_assert.h
ptpan_main.o: $(ARBHOME)/INCLUDE/arb_core.h
ptpan_main.o: $(ARBHOME)/INCLUDE/arb_error.h
ptpan_main.o: $(ARBHOME)/INCLUDE/arb_msg.h
ptpan_main.o: $(ARBHOME)/INCLUDE/arb_string.h
ptpan_main.o: $(ARBHOME)/INCLUDE/arbdb.h
ptpan_main.o: $(ARBHOME)/INCLUDE/arbdb_base.h
ptpan_main.o: $(ARBHOME)/INCLUDE/arbdbt.h
ptpan_main.o: $(ARBHOME)/INCLUDE/arbtools.h
ptpan_main.o: $(ARBHOME)/INCLUDE/attributes.h
ptpan_main.o: $(ARBHOME)/INCLUDE/bytestring.h
ptpan_main.o: $(ARBHOME)/INCLUDE/client.h
ptpan_main.o: $(ARBHOME)/INCLUDE/client_types.h
ptpan_main.o: $(ARBHOME)/INCLUDE/dupstr.h
ptpan_main.o: $(ARBHOME)/INCLUDE/PT_com.h
ptpan_main.o: $(ARBHOME)/INCLUDE/PT_server.h
ptpan_main.o: $(ARBHOME)/INCLUDE/PT_server_prototypes.h
ptpan_main.o: $(ARBHOME)/INCLUDE/server.h
ptpan_main.o: $(ARBHOME)/INCLUDE/servercntrl.h
ptpan_main.o: $(ARBHOME)/INCLUDE/smartptr.h
ptpan_main.o: $(ARBHOME)/INCLUDE/struct_man.h
ptpan_main.o: $(ARBHOME)/INCLUDE/test_global.h

ptpan_tree.o: abstract_alphabet_specifics.h
ptpan_tree.o: bin_tree.h
ptpan_tree.o: dlist.h
ptpan_tree.o: dna5_alphabet_specifics.h
ptpan_tree.o: hash_key.h
ptpan_tree.o: huffman.h
ptpan_tree.o: ptpan.h
ptpan_tree.o: ptpan_base.h
ptpan_tree.o: ptpan_filter.h
ptpan_tree.o: ptpan_tree.h
ptpan_tree.o: types.h

ptpan_tree_builder.o: abstract_alphabet_specifics.h
ptpan_tree_builder.o: abstract_data_retriever.h
ptpan_tree_builder.o: bin_tree.h
ptpan_tree_builder.o: dlist.h
ptpan_tree_builder.o: hash_key.h
ptpan_tree_builder.o: huffman.h
ptpan_tree_builder.o: ptpan.h
ptpan_tree_builder.o: ptpan_base.h
ptpan_tree_builder.o: ptpan_build_settings.h
ptpan_tree_builder.o: ptpan_tree.h
ptpan_tree_builder.o: ptpan_tree_builder.h
ptpan_tree_builder.o: types.h

#!/bin/tcsh

# Generates some directories as well:
mkdir -p INCLUDE
mkdir -p INCLUDE/GL

mkdir -p NAMES_COM/GENC
mkdir -p NAMES_COM/GENH
mkdir -p NAMES_COM/O

mkdir -p ORS_COM/GENC
mkdir -p ORS_COM/GENH
mkdir -p ORS_COM/O

mkdir -p PROBE_COM/GENC
mkdir -p PROBE_COM/GENH
mkdir -p PROBE_COM/O

test -d lib/pts || mkdir -p lib/pts

# Liblink

ln -s ../ARBDB/libARBDB.sl LIBLINK/libARBDB.sl
ln -s ../ARBDB/libARBDB.so LIBLINK/libARBDB.so
ln -s ../ARBDB/libARBDB.so.2.0 LIBLINK/libARBDB.so.2.0
ln -s ../ARBDB2/libARBDB.a LIBLINK/libARBDO.a
ln -s ../ARBDB2/libARBDB.sl LIBLINK/libARBDO.sl
ln -s ../ARBDB2/libARBDB.so LIBLINK/libARBDO.so
ln -s ../ARBDB2/libARBDB.so.2.0 LIBLINK/libARBDO.so.2.0
ln -s ../ARBDBPP/libARBDBPP.a LIBLINK/libARBDBPP.a
ln -s ../ARBDBPP/libARBDBPP.sl LIBLINK/libARBDBPP.sl
ln -s ../ARBDBPP/libARBDBPP.so LIBLINK/libARBDBPP.so
ln -s ../ARBDBPP/libARBDBPP.so.2.0 LIBLINK/libARBDBPP.so.2.0
ln -s ../ARBDBS/libARBDB.a LIBLINK/libARBDB.a
ln -s ../AWT/libAWT.a LIBLINK/libAWT.a
ln -s ../AWT/libAWT.sl LIBLINK/libAWT.sl
ln -s ../AWT/libAWT.so LIBLINK/libAWT.so
ln -s ../AWT/libAWT.so.2.0 LIBLINK/libAWT.so.2.0
ln -s ../LIBLINK LIBLINK/LIBLINK
ln -s ../WINDOW/libAW.a LIBLINK/libAW.a
ln -s ../WINDOW/libAW.sl LIBLINK/libAW.sl
ln -s ../WINDOW/libAW.so LIBLINK/libAW.so
ln -s ../WINDOW/libAW.so.2.0 LIBLINK/libAW.so.2.0

# Motif stuff
ln -s $MOTIF_LIBPATH LIBLINK/libXm.so.3
ln -s $LIBDIR/libXm.1.dylib LIBLINK/libXm.sl

# Links in bin directory
cd bin
make all
cd ..

# ...COMS

ln -s ../AISC_COM/AISC          NAMES_COM/AISC
ln -s ../AISC_COM/C             NAMES_COM/C
ln -s GENH/aisc_com.h           NAMES_COM/names_client.h
ln -s GENH/aisc_server_proto.h  NAMES_COM/names_prototypes.h
ln -s GENH/aisc.h               NAMES_COM/names_server.h

ln -s ../AISC_COM/AISC          PROBE_COM/AISC
ln -s ../AISC_COM/C             PROBE_COM/C
ln -s GENH/aisc_com.h           PROBE_COM/PT_com.h
ln -s GENH/aisc_server_proto.h  PROBE_COM/PT_server_prototypes.h
ln -s GENH/aisc.h               PROBE_COM/PT_server.h

ln -s ../AISC_COM/AISC          ORS_COM/AISC
ln -s ../AISC_COM/C             ORS_COM/C
ln -s GENH/aisc_com.h           ORS_COM/ors_client.h
ln -s GENH/aisc_server_proto.h  ORS_COM/ors_prototypes.h
ln -s GENH/aisc.h               ORS_COM/ors_server.h

# TEMPLATES directory

ln -s ../TEMPLATES/smartptr.h INCLUDE/smartptr.h
ln -s ../TEMPLATES/output.h INCLUDE/output.h
ln -s ../TEMPLATES/inline.h INCLUDE/inline.h
ln -s ../TEMPLATES/arbtools.h INCLUDE/arbtools.h
ln -s ../TEMPLATES/config_parser.h INCLUDE/config_parser.h
ln -s ../TEMPLATES/SIG_PF.h INCLUDE/SIG_PF.h
ln -s ../TEMPLATES/perf_timer.h INCLUDE/perf_timer.h

# INCLUDE directory

ln -s ../AISC_COM/C/aisc_func_types.h INCLUDE/aisc_func_types.h
ln -s ../AISC_COM/C/client.h INCLUDE/client.h
ln -s ../AISC_COM/C/client_privat.h INCLUDE/client_privat.h
ln -s ../AISC_COM/C/server.h INCLUDE/server.h
ln -s ../AISC_COM/C/struct_man.h INCLUDE/struct_man.h
ln -s ../ARBDB/adGene.h INCLUDE/adGene.h
ln -s ../ARBDB/ad_prot.h INCLUDE/ad_prot.h
ln -s ../ARBDB/ad_t_prot.h INCLUDE/ad_t_prot.h
ln -s ../ARBDB/arb_assert.h INCLUDE/arb_assert.h
ln -s ../ARBDB/arbdb.h INCLUDE/arbdb.h
ln -s ../ARBDB/arbdbt.h INCLUDE/arbdbt.h
ln -s ../ARBDBPP/adtools.hxx INCLUDE/adtools.hxx
ln -s ../ARBDBPP/arbdb++.hxx INCLUDE/arbdb++.hxx
ln -s ../ARB_GDE/gde.hxx INCLUDE/gde.hxx
ln -s ../AWT/BI_helix.hxx INCLUDE/BI_helix.hxx
ln -s ../AWT/awt.hxx INCLUDE/awt.hxx
ln -s ../AWT/awt_advice.hxx INCLUDE/awt_advice.hxx
ln -s ../AWT/awt_asciiprint.hxx INCLUDE/awt_asciiprint.hxx
ln -s ../AWT/awt_attributes.hxx INCLUDE/awt_attributes.hxx
ln -s ../AWT/awt_canvas.hxx INCLUDE/awt_canvas.hxx
ln -s ../AWT/awt_codon_table.hxx INCLUDE/awt_codon_table.hxx
ln -s ../AWT/awt_config_manager.hxx INCLUDE/awt_config_manager.hxx
ln -s ../AWT/awt_csp.hxx INCLUDE/awt_csp.hxx
ln -s ../AWT/awt_dtree.hxx INCLUDE/awt_dtree.hxx
ln -s ../AWT/awt_hotkeys.hxx INCLUDE/awt_hotkeys.hxx
ln -s ../AWT/awt_imp_exp.hxx INCLUDE/awt_imp_exp.hxx
ln -s ../AWT/awt_input_mask.hxx INCLUDE/awt_input_mask.hxx
ln -s ../AWT/awt_iupac.hxx INCLUDE/awt_iupac.hxx
ln -s ../AWT/awt_map_key.hxx INCLUDE/awt_map_key.hxx
ln -s ../AWT/awt_nds.hxx INCLUDE/awt_nds.hxx
ln -s ../AWT/awt_preset.hxx INCLUDE/awt_preset.hxx
ln -s ../AWT/awt_pro_a_nucs.hxx INCLUDE/awt_pro_a_nucs.hxx
ln -s ../AWT/awt_seq_colors.hxx INCLUDE/awt_seq_colors.hxx
ln -s ../AWT/awt_seq_dna.hxx INCLUDE/awt_seq_dna.hxx
ln -s ../AWT/awt_seq_protein.hxx INCLUDE/awt_seq_protein.hxx
ln -s ../AWT/awt_seq_simple_pro.hxx INCLUDE/awt_seq_simple_pro.hxx
ln -s ../AWT/awt_tree.hxx INCLUDE/PH_phylo.hxx
ln -s ../AWT/awt_tree.hxx INCLUDE/awt_tree.hxx
ln -s ../AWT/awt_tree_cb.hxx INCLUDE/awt_tree_cb.hxx
ln -s ../AWT/awt_tree_cmp.hxx INCLUDE/awt_tree_cmp.hxx
ln -s ../AWT/awt_www.hxx INCLUDE/awt_www.hxx
ln -s ../AWT/awtlocal.hxx INCLUDE/awtlocal.hxx
ln -s ../AWTC/awtc_constructSequence.hxx INCLUDE/awtc_constructSequence.hxx
ln -s ../AWTC/awtc_fast_aligner.hxx INCLUDE/awtc_fast_aligner.hxx
ln -s ../AWTC/awtc_next_neighbours.hxx INCLUDE/awtc_next_neighbours.hxx
ln -s ../AWTC/awtc_rename.hxx INCLUDE/awtc_rename.hxx
ln -s ../AWTC/awtc_seq_search.hxx INCLUDE/awtc_seq_search.hxx
ln -s ../AWTC/awtc_submission.hxx INCLUDE/awtc_submission.hxx
ln -s ../AWTI/awti_export.hxx INCLUDE/awti_export.hxx
ln -s ../AWTI/awti_import.hxx INCLUDE/awti_import.hxx
ln -s ../BUGEX/bugex.h INCLUDE/bugex.h
ln -s ../CAT/cat_tree.hxx INCLUDE/cat_tree.hxx
ln -s ../CONSENSUS_TREE/CT_ctree.hxx INCLUDE/CT_ctree.hxx
ln -s ../DIST/dist.hxx INCLUDE/dist.hxx
ln -s ../GENOM/EXP.hxx INCLUDE/EXP.hxx
ln -s ../GENOM/GEN.hxx INCLUDE/GEN.hxx
ln -s ../GENOM/GEN_extern.h INCLUDE/GEN_extern.h
ln -s ../GENOM_IMPORT/GAGenomImport.h INCLUDE/GAGenomImport.h
ln -s ../ISLAND_HOPPING/island_hopping.h INCLUDE/island_hopping.h
ln -s ../MERGE/mg_merge.hxx INCLUDE/mg_merge.hxx
ln -s ../NAMES_COM/names_client.h INCLUDE/names_client.h
ln -s ../NAMES_COM/names_prototypes.h INCLUDE/names_prototypes.h
ln -s ../NAMES_COM/names_server.h INCLUDE/names_server.h
ln -s ../NTREE/ntree.hxx INCLUDE/ntree.hxx
ln -s ../ORS_CGI/ors_lib.h INCLUDE/ors_lib.h
ln -s ../ORS_COM/ors_client.h INCLUDE/ors_client.h
ln -s ../ORS_COM/ors_prototypes.h INCLUDE/ors_prototypes.h
ln -s ../ORS_COM/ors_server.h INCLUDE/ors_server.h
ln -s ../PRIMER_DESIGN/primer_design.hxx INCLUDE/primer_design.hxx
ln -s ../PROBE_COM/PT_com.h INCLUDE/PT_com.h
ln -s ../PROBE_COM/PT_server.h INCLUDE/PT_server.h
ln -s ../PROBE_COM/PT_server_prototypes.h INCLUDE/PT_server_prototypes.h
ln -s ../PROBE_DESIGN/probe_design.hxx INCLUDE/probe_design.hxx
ln -s ../SECEDIT/sec_graphic.hxx INCLUDE/sec_graphic.hxx
ln -s ../SECEDIT/secedit.hxx INCLUDE/secedit.hxx
ln -s ../SEER/seer.hxx INCLUDE/seer.hxx
ln -s ../SERVERCNTRL/servercntrl.h INCLUDE/servercntrl.h
ln -s ../SEQ_QUALITY/seq_quality.h INCLUDE/seq_quality.h
ln -s ../STAT/st_window.hxx INCLUDE/st_window.hxx
ln -s ../WINDOW/aw_awars.hxx INCLUDE/aw_awars.hxx
ln -s ../WINDOW/aw_color_groups.hxx INCLUDE/aw_color_groups.hxx
ln -s ../WINDOW/aw_device.hxx INCLUDE/aw_device.hxx
ln -s ../WINDOW/aw_global_awars.hxx INCLUDE/aw_global_awars.hxx
ln -s ../WINDOW/aw_keysym.hxx INCLUDE/aw_keysym.hxx
ln -s ../WINDOW/aw_preset.hxx INCLUDE/aw_preset.hxx
ln -s ../WINDOW/aw_question.hxx INCLUDE/aw_question.hxx
ln -s ../WINDOW/aw_root.hxx INCLUDE/aw_root.hxx
ln -s ../WINDOW/aw_window.hxx INCLUDE/aw_window.hxx
ln -s ../WINDOW/aw_global.hxx INCLUDE/aw_global.hxx
ln -s ../XML/xml.hxx INCLUDE/xml.hxx

# gl stuff
ln -s ../../GL/glpng/glpng.h INCLUDE/GL/glpng.h 

# arbdb dirs

ln -s ../ARBDB/AD_MOBJECTS.h ARBDBS/AD_MOBJECTS.h
ln -s ../ARBDB/adReference.c ARBDBS/adReference.c
ln -s ../ARBDB/adTest.c ARBDBS/adTest.c
ln -s ../ARBDB/ad_core.c ARBDBS/ad_core.c
ln -s ../ARBDB/ad_load.c ARBDBS/ad_load.c
ln -s ../ARBDB/ad_lpro.h ARBDBS/ad_lpro.h
ln -s ../ARBDB/ad_prot.h ARBDBS/ad_prot.h
ln -s ../ARBDB/ad_save_load.c ARBDBS/ad_save_load.c
ln -s ../ARBDB/ad_t_lpro.h ARBDBS/ad_t_lpro.h
ln -s ../ARBDB/ad_t_prot.h ARBDBS/ad_t_prot.h
ln -s ../ARBDB/adcomm.c ARBDBS/adcomm.c
ln -s ../ARBDB/adcompr.c ARBDBS/adcompr.c
ln -s ../ARBDB/adhash.c ARBDBS/adhash.c
ln -s ../ARBDB/adindex.c ARBDBS/adindex.c
ln -s ../ARBDB/adlang1.c ARBDBS/adlang1.c
ln -s ../ARBDB/adlink.c ARBDBS/adlink.c
ln -s ../ARBDB/adlmacros.h ARBDBS/adlmacros.h
ln -s ../ARBDB/adlocal.h ARBDBS/adlocal.h
ln -s ../ARBDB/adlundo.h ARBDBS/adlundo.h
ln -s ../ARBDB/admalloc.c ARBDBS/admalloc.c
ln -s ../ARBDB/admap.c ARBDBS/admap.c
ln -s ../ARBDB/admap.h ARBDBS/admap.h
ln -s ../ARBDB/admath.c ARBDBS/admath.c
ln -s ../ARBDB/adoptimize.c ARBDBS/adoptimize.c
ln -s ../ARBDB/adperl.c ARBDBS/adperl.c
ln -s ../ARBDB/adquery.c ARBDBS/adquery.c
ln -s ../ARBDB/adregexp.h ARBDBS/adregexp.h
ln -s ../ARBDB/adseqcompr.c ARBDBS/adseqcompr.c
ln -s ../ARBDB/adseqcompr.h ARBDBS/adseqcompr.h
ln -s ../ARBDB/adsocket.c ARBDBS/adsocket.c
ln -s ../ARBDB/adsort.c ARBDBS/adsort.c
ln -s ../ARBDB/adstring.c ARBDBS/adstring.c
ln -s ../ARBDB/adsystem.c ARBDBS/adsystem.c
ln -s ../ARBDB/adtables.c ARBDBS/adtables.c
ln -s ../ARBDB/adtools.c ARBDBS/adtools.c
ln -s ../ARBDB/adtune.c ARBDBS/adtune.c
ln -s ../ARBDB/adtune.h ARBDBS/adtune.h
ln -s ../ARBDB/arbdb.c ARBDBS/arbdb.c
ln -s ../ARBDB/arbdb.h ARBDBS/arbdb.h
ln -s ../ARBDB/arbdbpp.cxx ARBDBS/arbdbpp.cxx
ln -s ../ARBDB/arbdbt.h ARBDBS/arbdbt.h
ln -s ../ARBDB/adRevCompl.c ARBDBS/adRevCompl.c
ln -s ../ARBDB/adGene.c ARBDBS/adGene.c
ln -s ../ARBDB/adGene.h ARBDBS/adGene.h

ln -s ../ARBDB/AD_MOBJECTS.h ARBDB2/AD_MOBJECTS.h
ln -s ../ARBDB/adReference.c ARBDB2/adReference.c
ln -s ../ARBDB/adTest.c ARBDB2/adTest.c
ln -s ../ARBDB/ad_core.c ARBDB2/ad_core.c
ln -s ../ARBDB/ad_load.c ARBDB2/ad_load.c
ln -s ../ARBDB/ad_lpro.h ARBDB2/ad_lpro.h
ln -s ../ARBDB/ad_prot.h ARBDB2/ad_prot.h
ln -s ../ARBDB/ad_save_load.c ARBDB2/ad_save_load.c
ln -s ../ARBDB/ad_t_lpro.h ARBDB2/ad_t_lpro.h
ln -s ../ARBDB/ad_t_prot.h ARBDB2/ad_t_prot.h
ln -s ../ARBDB/adcomm.c ARBDB2/adcomm.c
ln -s ../ARBDB/adcompr.c ARBDB2/adcompr.c
ln -s ../ARBDB/adhash.c ARBDB2/adhash.c
ln -s ../ARBDB/adindex.c ARBDB2/adindex.c
ln -s ../ARBDB/adlang1.c ARBDB2/adlang1.c
ln -s ../ARBDB/adlink.c ARBDB2/adlink.c
ln -s ../ARBDB/adlmacros.h ARBDB2/adlmacros.h
ln -s ../ARBDB/adlocal.h ARBDB2/adlocal.h
ln -s ../ARBDB/adlundo.h ARBDB2/adlundo.h
ln -s ../ARBDB/admalloc.c ARBDB2/admalloc.c
ln -s ../ARBDB/admap.c ARBDB2/admap.c
ln -s ../ARBDB/admap.h ARBDB2/admap.h
ln -s ../ARBDB/admath.c ARBDB2/admath.c
ln -s ../ARBDB/adoptimize.c ARBDB2/adoptimize.c
ln -s ../ARBDB/adperl.c ARBDB2/adperl.c
ln -s ../ARBDB/adquery.c ARBDB2/adquery.c
ln -s ../ARBDB/adregexp.h ARBDB2/adregexp.h
ln -s ../ARBDB/adseqcompr.c ARBDB2/adseqcompr.c
ln -s ../ARBDB/adseqcompr.h ARBDB2/adseqcompr.h
ln -s ../ARBDB/adsocket.c ARBDB2/adsocket.c
ln -s ../ARBDB/adsort.c ARBDB2/adsort.c
ln -s ../ARBDB/adstring.c ARBDB2/adstring.c
ln -s ../ARBDB/adsystem.c ARBDB2/adsystem.c
ln -s ../ARBDB/adtables.c ARBDB2/adtables.c
ln -s ../ARBDB/adtools.c ARBDB2/adtools.c
ln -s ../ARBDB/adtune.c ARBDB2/adtune.c
ln -s ../ARBDB/adtune.h ARBDB2/adtune.h
ln -s ../ARBDB/arbdb.c ARBDB2/arbdb.c
ln -s ../ARBDB/arbdb.h ARBDB2/arbdb.h
ln -s ../ARBDB/arbdbpp.cxx ARBDB2/arbdbpp.cxx
ln -s ../ARBDB/arbdbt.h ARBDB2/arbdbt.h
ln -s ../ARBDB/adRevCompl.c ARBDB2/adRevCompl.c
ln -s ../ARBDB/adGene.c ARBDB2/adGene.c
ln -s ../ARBDB/adGene.h ARBDB2/adGene.h

# small dirs

ln -s ../WINDOW/AW_preset.cxx AWT/AWT_preset.cxx

ln -s ../EDIT/edit_naligner.cxx EDIT4/edit_naligner.cxx
ln -s ../EDIT/edit_naligner.hxx EDIT4/edit_naligner.hxx

ln -s ../AWT/BI_helix.cxx PROBE/BI_helix.cxx
ln -s ../AWT/BI_helix.hxx PROBE/BI_helix.hxx

ln -s ../AWT/BI_helix.cxx NALIGNER/BI_helix.cxx
ln -s ../AWT/BI_helix.hxx NALIGNER/BI_helix.hxx
ln -s ../LIBLINK          NALIGNER/LIBLINK

ln -s ../LIBLINK          TOOLS/LIBLINK

ln -s ../AISC/aisc             MAKEBIN/aisc
ln -s ../AISC_MKPTPS/aisc_mkpt MAKEBIN/aisc_mkpt

# help files (make sure the file is present in user distribution!)

ln -s ../help/input_mask_format.hlp     lib/inputMasks/format.readme
ln -s ../../GDEHELP                     lib/help/GDEHELP

# links related to probe web service
ln -s ../PROBE_WEB/SERVER PROBE_SERVER/ps_cgi


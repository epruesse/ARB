#!/bin/bash

symlink() {
    test -h $2 || (ln -sf $1 $2 || exit 1)
}
makedir() {
    mkdir -p $1 || exit 1
}

# SYMLINK="ln -sf"
# MKDIR="mkdir -p"

# Generates some directories as well:
makedir INCLUDE
makedir INCLUDE/GL

makedir NAMES_COM/GENC
makedir NAMES_COM/GENH
makedir NAMES_COM/O

makedir ORS_COM/GENC
makedir ORS_COM/GENH
makedir ORS_COM/O

makedir PROBE_COM/GENC
makedir PROBE_COM/GENH
makedir PROBE_COM/O

test -d lib/pts || makedir lib/pts

# Liblink

# symlink ../ARBDB/libARBDB.sl LIBLINK/libARBDB.sl
symlink ../ARBDB/libARBDB.so LIBLINK/libARBDB.so
symlink ../ARBDB/libARBDB.so.2.0 LIBLINK/libARBDB.so.2.0
symlink ../ARBDB2/libARBDB.a LIBLINK/libARBDO.a
# symlink ../ARBDB2/libARBDB.sl LIBLINK/libARBDO.sl
symlink ../ARBDB2/libARBDB.so LIBLINK/libARBDO.so
symlink ../ARBDB2/libARBDB.so.2.0 LIBLINK/libARBDO.so.2.0
symlink ../ARBDBPP/libARBDBPP.a LIBLINK/libARBDBPP.a
# symlink ../ARBDBPP/libARBDBPP.sl LIBLINK/libARBDBPP.sl
symlink ../ARBDBPP/libARBDBPP.so LIBLINK/libARBDBPP.so
symlink ../ARBDBPP/libARBDBPP.so.2.0 LIBLINK/libARBDBPP.so.2.0
symlink ../ARBDBS/libARBDB.a LIBLINK/libARBDB.a
symlink ../AWT/libAWT.a LIBLINK/libAWT.a
# symlink ../AWT/libAWT.sl LIBLINK/libAWT.sl
symlink ../AWT/libAWT.so LIBLINK/libAWT.so
symlink ../AWT/libAWT.so.2.0 LIBLINK/libAWT.so.2.0
symlink ../WINDOW/libAW.a LIBLINK/libAW.a
# symlink ../WINDOW/libAW.sl LIBLINK/libAW.sl
symlink ../WINDOW/libAW.so LIBLINK/libAW.so
symlink ../WINDOW/libAW.so.2.0 LIBLINK/libAW.so.2.0

# Motif stuff
symlink $MOTIF_LIBPATH LIBLINK/libXm.so.3
# symlink $LIBDIR/libXm.1.dylib LIBLINK/libXm.sl

# Links in bin directory
cd bin
make all
cd ..

# ...COMS

symlink ../AISC_COM/AISC          NAMES_COM/AISC
symlink ../AISC_COM/C             NAMES_COM/C
symlink GENH/aisc_com.h           NAMES_COM/names_client.h
symlink GENH/aisc_server_proto.h  NAMES_COM/names_prototypes.h
symlink GENH/aisc.h               NAMES_COM/names_server.h

symlink ../AISC_COM/AISC          PROBE_COM/AISC
symlink ../AISC_COM/C             PROBE_COM/C
symlink GENH/aisc_com.h           PROBE_COM/PT_com.h
symlink GENH/aisc_server_proto.h  PROBE_COM/PT_server_prototypes.h
symlink GENH/aisc.h               PROBE_COM/PT_server.h

symlink ../AISC_COM/AISC          ORS_COM/AISC
symlink ../AISC_COM/C             ORS_COM/C
symlink GENH/aisc_com.h           ORS_COM/ors_client.h
symlink GENH/aisc_server_proto.h  ORS_COM/ors_prototypes.h
symlink GENH/aisc.h               ORS_COM/ors_server.h

# TEMPLATES directory

symlink ../TEMPLATES/smartptr.h INCLUDE/smartptr.h
symlink ../TEMPLATES/output.h INCLUDE/output.h
symlink ../TEMPLATES/inline.h INCLUDE/inline.h
symlink ../TEMPLATES/arbtools.h INCLUDE/arbtools.h
symlink ../TEMPLATES/config_parser.h INCLUDE/config_parser.h
symlink ../TEMPLATES/SIG_PF.h INCLUDE/SIG_PF.h
symlink ../TEMPLATES/perf_timer.h INCLUDE/perf_timer.h

# INCLUDE directory

symlink ../AISC_COM/C/aisc_func_types.h INCLUDE/aisc_func_types.h
symlink ../AISC_COM/C/client.h INCLUDE/client.h
symlink ../AISC_COM/C/client_privat.h INCLUDE/client_privat.h
symlink ../AISC_COM/C/server.h INCLUDE/server.h
symlink ../AISC_COM/C/struct_man.h INCLUDE/struct_man.h
symlink ../ARBDB/adGene.h INCLUDE/adGene.h
symlink ../ARBDB/ad_prot.h INCLUDE/ad_prot.h
symlink ../ARBDB/ad_t_prot.h INCLUDE/ad_t_prot.h
symlink ../ARBDB/arb_assert.h INCLUDE/arb_assert.h
symlink ../ARBDB/arbdb.h INCLUDE/arbdb.h
symlink ../ARBDB/arbdbt.h INCLUDE/arbdbt.h
symlink ../ARBDBPP/adtools.hxx INCLUDE/adtools.hxx
symlink ../ARBDBPP/arbdb++.hxx INCLUDE/arbdb++.hxx
symlink ../ARB_GDE/gde.hxx INCLUDE/gde.hxx
symlink ../AWT/awt.hxx INCLUDE/awt.hxx
symlink ../AWT/awt_advice.hxx INCLUDE/awt_advice.hxx
symlink ../AWT/awt_asciiprint.hxx INCLUDE/awt_asciiprint.hxx
symlink ../AWT/awt_attributes.hxx INCLUDE/awt_attributes.hxx
symlink ../AWT/awt_canvas.hxx INCLUDE/awt_canvas.hxx
symlink ../AWT/awt_codon_table.hxx INCLUDE/awt_codon_table.hxx
symlink ../AWT/awt_config_manager.hxx INCLUDE/awt_config_manager.hxx
symlink ../AWT/awt_csp.hxx INCLUDE/awt_csp.hxx
symlink ../AWT/awt_dtree.hxx INCLUDE/awt_dtree.hxx
symlink ../AWT/awt_hotkeys.hxx INCLUDE/awt_hotkeys.hxx
symlink ../AWT/awt_imp_exp.hxx INCLUDE/awt_imp_exp.hxx
symlink ../AWT/awt_input_mask.hxx INCLUDE/awt_input_mask.hxx
symlink ../AWT/awt_iupac.hxx INCLUDE/awt_iupac.hxx
symlink ../AWT/awt_map_key.hxx INCLUDE/awt_map_key.hxx
symlink ../AWT/awt_nds.hxx INCLUDE/awt_nds.hxx
symlink ../AWT/awt_preset.hxx INCLUDE/awt_preset.hxx
symlink ../AWT/awt_pro_a_nucs.hxx INCLUDE/awt_pro_a_nucs.hxx
symlink ../AWT/awt_seq_colors.hxx INCLUDE/awt_seq_colors.hxx
symlink ../AWT/awt_seq_dna.hxx INCLUDE/awt_seq_dna.hxx
symlink ../AWT/awt_seq_protein.hxx INCLUDE/awt_seq_protein.hxx
symlink ../AWT/awt_seq_simple_pro.hxx INCLUDE/awt_seq_simple_pro.hxx
symlink ../AWT/awt_tree.hxx INCLUDE/PH_phylo.hxx
symlink ../AWT/awt_tree.hxx INCLUDE/awt_tree.hxx
symlink ../AWT/awt_tree_cb.hxx INCLUDE/awt_tree_cb.hxx
symlink ../AWT/awt_tree_cmp.hxx INCLUDE/awt_tree_cmp.hxx
symlink ../AWT/awt_www.hxx INCLUDE/awt_www.hxx
symlink ../AWT/awtlocal.hxx INCLUDE/awtlocal.hxx
symlink ../AWTC/awtc_constructSequence.hxx INCLUDE/awtc_constructSequence.hxx
symlink ../AWTC/awtc_fast_aligner.hxx INCLUDE/awtc_fast_aligner.hxx
symlink ../AWTC/awtc_next_neighbours.hxx INCLUDE/awtc_next_neighbours.hxx
symlink ../AWTC/awtc_rename.hxx INCLUDE/awtc_rename.hxx
symlink ../AWTC/awtc_seq_search.hxx INCLUDE/awtc_seq_search.hxx
symlink ../AWTC/awtc_submission.hxx INCLUDE/awtc_submission.hxx
symlink ../AWTI/awti_export.hxx INCLUDE/awti_export.hxx
symlink ../AWTI/awti_import.hxx INCLUDE/awti_import.hxx
symlink ../BUGEX/bugex.h INCLUDE/bugex.h
symlink ../CAT/cat_tree.hxx INCLUDE/cat_tree.hxx
symlink ../CONSENSUS_TREE/CT_ctree.hxx INCLUDE/CT_ctree.hxx
symlink ../DIST/dist.hxx INCLUDE/dist.hxx
symlink ../GENOM/EXP.hxx INCLUDE/EXP.hxx
symlink ../GENOM/GEN.hxx INCLUDE/GEN.hxx
symlink ../GENOM/GEN_extern.h INCLUDE/GEN_extern.h
symlink ../GENOM_IMPORT/GAGenomImport.h INCLUDE/GAGenomImport.h
symlink ../ISLAND_HOPPING/island_hopping.h INCLUDE/island_hopping.h
symlink ../MERGE/mg_merge.hxx INCLUDE/mg_merge.hxx
symlink ../NAMES_COM/names_client.h INCLUDE/names_client.h
symlink ../NAMES_COM/names_prototypes.h INCLUDE/names_prototypes.h
symlink ../NAMES_COM/names_server.h INCLUDE/names_server.h
symlink ../NTREE/ntree.hxx INCLUDE/ntree.hxx
symlink ../ORS_CGI/ors_lib.h INCLUDE/ors_lib.h
symlink ../ORS_COM/ors_client.h INCLUDE/ors_client.h
symlink ../ORS_COM/ors_prototypes.h INCLUDE/ors_prototypes.h
symlink ../ORS_COM/ors_server.h INCLUDE/ors_server.h
symlink ../PRIMER_DESIGN/primer_design.hxx INCLUDE/primer_design.hxx
symlink ../PROBE_COM/PT_com.h INCLUDE/PT_com.h
symlink ../PROBE_COM/PT_server.h INCLUDE/PT_server.h
symlink ../PROBE_COM/PT_server_prototypes.h INCLUDE/PT_server_prototypes.h
symlink ../PROBE_DESIGN/probe_design.hxx INCLUDE/probe_design.hxx
symlink ../SECEDIT/sec_graphic.hxx INCLUDE/sec_graphic.hxx
symlink ../SECEDIT/secedit.hxx INCLUDE/secedit.hxx
symlink ../SEER/seer.hxx INCLUDE/seer.hxx
symlink ../SERVERCNTRL/servercntrl.h INCLUDE/servercntrl.h
symlink ../SEQ_QUALITY/seq_quality.h INCLUDE/seq_quality.h
symlink ../SL/AW_HELIX/AW_helix.hxx INCLUDE/AW_helix.hxx
symlink ../SL/HELIX/BI_helix.hxx INCLUDE/BI_helix.hxx
symlink ../STAT/st_window.hxx INCLUDE/st_window.hxx
symlink ../WINDOW/aw_awars.hxx INCLUDE/aw_awars.hxx
symlink ../WINDOW/aw_color_groups.hxx INCLUDE/aw_color_groups.hxx
symlink ../WINDOW/aw_device.hxx INCLUDE/aw_device.hxx
symlink ../WINDOW/aw_global_awars.hxx INCLUDE/aw_global_awars.hxx
symlink ../WINDOW/aw_keysym.hxx INCLUDE/aw_keysym.hxx
symlink ../WINDOW/aw_preset.hxx INCLUDE/aw_preset.hxx
symlink ../WINDOW/aw_question.hxx INCLUDE/aw_question.hxx
symlink ../WINDOW/aw_root.hxx INCLUDE/aw_root.hxx
symlink ../WINDOW/aw_window.hxx INCLUDE/aw_window.hxx
symlink ../WINDOW/aw_global.hxx INCLUDE/aw_global.hxx
symlink ../WINDOW/aw_font_group.hxx INCLUDE/aw_font_group.hxx
symlink ../XML/xml.hxx INCLUDE/xml.hxx

# gl stuff
symlink ../../GL/glpng/glpng.h INCLUDE/GL/glpng.h 

# arbdb dirs

symlink ../ARBDB/AD_MOBJECTS.h ARBDBS/AD_MOBJECTS.h
symlink ../ARBDB/adReference.c ARBDBS/adReference.c
symlink ../ARBDB/adTest.c ARBDBS/adTest.c
symlink ../ARBDB/ad_core.c ARBDBS/ad_core.c
symlink ../ARBDB/ad_load.c ARBDBS/ad_load.c
symlink ../ARBDB/ad_lpro.h ARBDBS/ad_lpro.h
symlink ../ARBDB/ad_prot.h ARBDBS/ad_prot.h
symlink ../ARBDB/ad_save_load.c ARBDBS/ad_save_load.c
symlink ../ARBDB/ad_t_lpro.h ARBDBS/ad_t_lpro.h
symlink ../ARBDB/ad_t_prot.h ARBDBS/ad_t_prot.h
symlink ../ARBDB/adcomm.c ARBDBS/adcomm.c
symlink ../ARBDB/adcompr.c ARBDBS/adcompr.c
symlink ../ARBDB/adhash.c ARBDBS/adhash.c
symlink ../ARBDB/adindex.c ARBDBS/adindex.c
symlink ../ARBDB/adlang1.c ARBDBS/adlang1.c
symlink ../ARBDB/adlink.c ARBDBS/adlink.c
symlink ../ARBDB/adlmacros.h ARBDBS/adlmacros.h
symlink ../ARBDB/adlocal.h ARBDBS/adlocal.h
symlink ../ARBDB/adlundo.h ARBDBS/adlundo.h
symlink ../ARBDB/admalloc.c ARBDBS/admalloc.c
symlink ../ARBDB/admap.c ARBDBS/admap.c
symlink ../ARBDB/admap.h ARBDBS/admap.h
symlink ../ARBDB/admath.c ARBDBS/admath.c
symlink ../ARBDB/adoptimize.c ARBDBS/adoptimize.c
symlink ../ARBDB/adperl.c ARBDBS/adperl.c
symlink ../ARBDB/adquery.c ARBDBS/adquery.c
symlink ../ARBDB/adregexp.h ARBDBS/adregexp.h
symlink ../ARBDB/adseqcompr.c ARBDBS/adseqcompr.c
symlink ../ARBDB/adseqcompr.h ARBDBS/adseqcompr.h
symlink ../ARBDB/adsocket.c ARBDBS/adsocket.c
symlink ../ARBDB/adsort.c ARBDBS/adsort.c
symlink ../ARBDB/adstring.c ARBDBS/adstring.c
symlink ../ARBDB/adsystem.c ARBDBS/adsystem.c
symlink ../ARBDB/adtables.c ARBDBS/adtables.c
symlink ../ARBDB/adtools.c ARBDBS/adtools.c
symlink ../ARBDB/adtune.c ARBDBS/adtune.c
symlink ../ARBDB/adtune.h ARBDBS/adtune.h
symlink ../ARBDB/arbdb.c ARBDBS/arbdb.c
symlink ../ARBDB/arbdb.h ARBDBS/arbdb.h
symlink ../ARBDB/arbdbpp.cxx ARBDBS/arbdbpp.cxx
symlink ../ARBDB/arbdbt.h ARBDBS/arbdbt.h
symlink ../ARBDB/adRevCompl.c ARBDBS/adRevCompl.c
symlink ../ARBDB/adGene.c ARBDBS/adGene.c
symlink ../ARBDB/adGene.h ARBDBS/adGene.h

symlink ../ARBDB/AD_MOBJECTS.h ARBDB2/AD_MOBJECTS.h
symlink ../ARBDB/adReference.c ARBDB2/adReference.c
symlink ../ARBDB/adTest.c ARBDB2/adTest.c
symlink ../ARBDB/ad_core.c ARBDB2/ad_core.c
symlink ../ARBDB/ad_load.c ARBDB2/ad_load.c
symlink ../ARBDB/ad_lpro.h ARBDB2/ad_lpro.h
symlink ../ARBDB/ad_prot.h ARBDB2/ad_prot.h
symlink ../ARBDB/ad_save_load.c ARBDB2/ad_save_load.c
symlink ../ARBDB/ad_t_lpro.h ARBDB2/ad_t_lpro.h
symlink ../ARBDB/ad_t_prot.h ARBDB2/ad_t_prot.h
symlink ../ARBDB/adcomm.c ARBDB2/adcomm.c
symlink ../ARBDB/adcompr.c ARBDB2/adcompr.c
symlink ../ARBDB/adhash.c ARBDB2/adhash.c
symlink ../ARBDB/adindex.c ARBDB2/adindex.c
symlink ../ARBDB/adlang1.c ARBDB2/adlang1.c
symlink ../ARBDB/adlink.c ARBDB2/adlink.c
symlink ../ARBDB/adlmacros.h ARBDB2/adlmacros.h
symlink ../ARBDB/adlocal.h ARBDB2/adlocal.h
symlink ../ARBDB/adlundo.h ARBDB2/adlundo.h
symlink ../ARBDB/admalloc.c ARBDB2/admalloc.c
symlink ../ARBDB/admap.c ARBDB2/admap.c
symlink ../ARBDB/admap.h ARBDB2/admap.h
symlink ../ARBDB/admath.c ARBDB2/admath.c
symlink ../ARBDB/adoptimize.c ARBDB2/adoptimize.c
symlink ../ARBDB/adperl.c ARBDB2/adperl.c
symlink ../ARBDB/adquery.c ARBDB2/adquery.c
symlink ../ARBDB/adregexp.h ARBDB2/adregexp.h
symlink ../ARBDB/adseqcompr.c ARBDB2/adseqcompr.c
symlink ../ARBDB/adseqcompr.h ARBDB2/adseqcompr.h
symlink ../ARBDB/adsocket.c ARBDB2/adsocket.c
symlink ../ARBDB/adsort.c ARBDB2/adsort.c
symlink ../ARBDB/adstring.c ARBDB2/adstring.c
symlink ../ARBDB/adsystem.c ARBDB2/adsystem.c
symlink ../ARBDB/adtables.c ARBDB2/adtables.c
symlink ../ARBDB/adtools.c ARBDB2/adtools.c
symlink ../ARBDB/adtune.c ARBDB2/adtune.c
symlink ../ARBDB/adtune.h ARBDB2/adtune.h
symlink ../ARBDB/arbdb.c ARBDB2/arbdb.c
symlink ../ARBDB/arbdb.h ARBDB2/arbdb.h
symlink ../ARBDB/arbdbpp.cxx ARBDB2/arbdbpp.cxx
symlink ../ARBDB/arbdbt.h ARBDB2/arbdbt.h
symlink ../ARBDB/adRevCompl.c ARBDB2/adRevCompl.c
symlink ../ARBDB/adGene.c ARBDB2/adGene.c
symlink ../ARBDB/adGene.h ARBDB2/adGene.h

# small dirs

symlink ../WINDOW/AW_preset.cxx AWT/AWT_preset.cxx
symlink ../WINDOW/aw_def.hxx AWT/aw_def.hxx

symlink ../EDIT/edit_naligner.cxx EDIT4/edit_naligner.cxx
symlink ../EDIT/edit_naligner.hxx EDIT4/edit_naligner.hxx

symlink ../LIBLINK          NALIGNER/LIBLINK

symlink ../LIBLINK          TOOLS/LIBLINK

symlink ../AISC/aisc             MAKEBIN/aisc
symlink ../AISC_MKPTPS/aisc_mkpt MAKEBIN/aisc_mkpt

# help files (make sure the file is present in user distribution!)

symlink ../help/input_mask_format.hlp     lib/inputMasks/format.readme
symlink ../../GDEHELP                     lib/help/GDEHELP

# links related to probe web service
symlink ../PROBE_WEB/SERVER PROBE_SERVER/ps_cgi

exit 0

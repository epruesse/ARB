#!/bin/bash

SELF=$ARBHOME/SOURCE_TOOLS/generate_all_links.sh

finderr() {
    FOUND=`grep -Hn "$1" $SELF`
    if [ -z $FOUND ]; then
        echo "$SELF:8: $2 ($1 not located -- search manually)"
    else
        echo "$FOUND $2"
    fi
    false
}

symlink_maybe_no_target() {
    test -h $2 || ln -sf $1 $2 || finderr $2 "Failed to link '$1->$2'"
}

symlink() {
    if [ -z $2 ]; then
        if [ -z $1 ]; then
            echo "$SELF:22: Missing arguments in call to symlink()"
            exit 1
        else
            finderr $1 "Second argument missing in call to symlink()"
            exit 1
        fi
    fi

    DIR=`dirname $2`
    if [ -z $DIR ]; then
        DIR=.
    fi

    (test -e $DIR/$1 || finderr $1 "Target '$DIR/$1 does not exists (anymore)" ) &&
    (test -e $2 || (test -h $2 &&
                    (finderr $2 "Warning Symlink '$2' points to nowhere -- removing wrong link";ls -al $2;rm $2;true)
                    ) || true) &&
    symlink_maybe_no_target $1 $2
}

arbdb_symlink() {
    symlink ../ARBDB/$1 ARBDBS/$1 &&
    symlink ../ARBDB/$1 ARBDB2/$1
}

makedir() {
    mkdir -p $1 || finderr $1 "Failed to create directory '$1'"
}

# Generates some directories as well:
makedir INCLUDE &&
makedir INCLUDE/GL &&

makedir NAMES_COM/GENC &&
makedir NAMES_COM/GENH &&
makedir NAMES_COM/O &&

makedir ORS_COM/GENC &&
makedir ORS_COM/GENH &&
makedir ORS_COM/O &&

makedir PROBE_COM/GENC &&
makedir PROBE_COM/GENH &&
makedir PROBE_COM/O &&

makedir LIBLINK &&
makedir MAKEBIN &&
makedir lib/help &&

(test -d lib/pts || makedir lib/pts) &&

# Liblink

# symlink ../ARBDB/libARBDB.sl LIBLINK/libARBDB.sl
symlink_maybe_no_target ../ARBDB/libARBDB.so LIBLINK/libARBDB.so &&
symlink_maybe_no_target ../ARBDB/libARBDB.so.2.0 LIBLINK/libARBDB.so.2.0 &&
symlink_maybe_no_target ../ARBDB2/libARBDB.a LIBLINK/libARBDO.a &&
# symlink ../ARBDB2/libARBDB.sl LIBLINK/libARBDO.sl
symlink_maybe_no_target ../ARBDB2/libARBDB.so LIBLINK/libARBDO.so &&
symlink_maybe_no_target ../ARBDB2/libARBDB.so.2.0 LIBLINK/libARBDO.so.2.0 &&
symlink_maybe_no_target ../ARBDBPP/libARBDBPP.a LIBLINK/libARBDBPP.a &&
# symlink ../ARBDBPP/libARBDBPP.sl LIBLINK/libARBDBPP.sl
symlink_maybe_no_target ../ARBDBPP/libARBDBPP.so LIBLINK/libARBDBPP.so &&
symlink_maybe_no_target ../ARBDBPP/libARBDBPP.so.2.0 LIBLINK/libARBDBPP.so.2.0 &&
symlink_maybe_no_target ../ARBDBS/libARBDB.a LIBLINK/libARBDB.a &&
symlink_maybe_no_target ../AWT/libAWT.a LIBLINK/libAWT.a &&
# symlink ../AWT/libAWT.sl LIBLINK/libAWT.sl
symlink_maybe_no_target ../AWT/libAWT.so LIBLINK/libAWT.so &&
symlink_maybe_no_target ../AWT/libAWT.so.2.0 LIBLINK/libAWT.so.2.0 &&
symlink_maybe_no_target ../WINDOW/libAW.a LIBLINK/libAW.a &&
# symlink ../WINDOW/libAW.sl LIBLINK/libAW.sl
symlink_maybe_no_target ../WINDOW/libAW.so LIBLINK/libAW.so &&
symlink_maybe_no_target ../WINDOW/libAW.so.2.0 LIBLINK/libAW.so.2.0 &&

# Motif stuff
(test -z $MOTIF_LIBPATH || symlink $MOTIF_LIBPATH LIBLINK/libXm.so.3) &&
# symlink $LIBDIR/libXm.1.dylib LIBLINK/libXm.sl

# Links in bin directory
( cd bin ; make all; cd .. ) &&

# ...COMS

symlink ../AISC_COM/AISC          NAMES_COM/AISC &&
symlink ../AISC_COM/C             NAMES_COM/C &&

symlink_maybe_no_target GENH/aisc_com.h           NAMES_COM/names_client.h &&
symlink_maybe_no_target GENH/aisc_server_proto.h  NAMES_COM/names_prototypes.h &&
symlink_maybe_no_target GENH/aisc.h               NAMES_COM/names_server.h &&

symlink ../AISC_COM/AISC          PROBE_COM/AISC &&
symlink ../AISC_COM/C             PROBE_COM/C &&

symlink_maybe_no_target GENH/aisc_com.h           PROBE_COM/PT_com.h &&
symlink_maybe_no_target GENH/aisc_server_proto.h  PROBE_COM/PT_server_prototypes.h &&
symlink_maybe_no_target GENH/aisc.h               PROBE_COM/PT_server.h &&

symlink ../AISC_COM/AISC          ORS_COM/AISC &&
symlink ../AISC_COM/C             ORS_COM/C &&

symlink_maybe_no_target GENH/aisc_com.h           ORS_COM/ors_client.h &&
symlink_maybe_no_target GENH/aisc_server_proto.h  ORS_COM/ors_prototypes.h &&
symlink_maybe_no_target GENH/aisc.h               ORS_COM/ors_server.h &&

# TEMPLATES directory

symlink ../TEMPLATES/smartptr.h INCLUDE/smartptr.h &&
symlink ../TEMPLATES/output.h INCLUDE/output.h &&
symlink ../TEMPLATES/inline.h INCLUDE/inline.h &&
symlink ../TEMPLATES/arbtools.h INCLUDE/arbtools.h &&
symlink ../TEMPLATES/config_parser.h INCLUDE/config_parser.h &&
symlink ../TEMPLATES/SIG_PF.h INCLUDE/SIG_PF.h &&
symlink ../TEMPLATES/perf_timer.h INCLUDE/perf_timer.h &&
symlink ../TEMPLATES/arb_version.h INCLUDE/arb_version.h &&
symlink ../TEMPLATES/arb_debug.h INCLUDE/arb_debug.h &&
symlink_maybe_no_target ../TEMPLATES/arb_build.h INCLUDE/arb_build.h &&

# INCLUDE directory

symlink_maybe_no_target ../NAMES_COM/names_client.h         INCLUDE/names_client.h &&
symlink_maybe_no_target ../NAMES_COM/names_prototypes.h     INCLUDE/names_prototypes.h &&
symlink_maybe_no_target ../NAMES_COM/names_server.h         INCLUDE/names_server.h &&

symlink_maybe_no_target ../ORS_CGI/ors_lib.h                INCLUDE/ors_lib.h &&
symlink_maybe_no_target ../ORS_COM/ors_client.h             INCLUDE/ors_client.h &&
symlink_maybe_no_target ../ORS_COM/ors_prototypes.h         INCLUDE/ors_prototypes.h &&
symlink_maybe_no_target ../ORS_COM/ors_server.h             INCLUDE/ors_server.h &&

symlink_maybe_no_target ../PROBE_COM/PT_com.h               INCLUDE/PT_com.h &&
symlink_maybe_no_target ../PROBE_COM/PT_server.h            INCLUDE/PT_server.h &&
symlink_maybe_no_target ../PROBE_COM/PT_server_prototypes.h INCLUDE/PT_server_prototypes.h &&

symlink ../AISC_COM/C/aisc_func_types.h INCLUDE/aisc_func_types.h &&
symlink ../AISC_COM/C/client.h INCLUDE/client.h &&
symlink ../AISC_COM/C/client_privat.h INCLUDE/client_privat.h &&
symlink ../AISC_COM/C/server.h INCLUDE/server.h &&
symlink ../AISC_COM/C/struct_man.h INCLUDE/struct_man.h &&
symlink ../AISC_COM/C/aisc_global.h INCLUDE/aisc_global.h &&
symlink ../ARBDB/adGene.h INCLUDE/adGene.h &&
symlink ../ARBDB/ad_prot.h INCLUDE/ad_prot.h &&
symlink ../ARBDB/ad_config.h INCLUDE/ad_config.h &&
symlink ../ARBDB/ad_t_prot.h INCLUDE/ad_t_prot.h &&
symlink ../ARBDB/arb_assert.h INCLUDE/arb_assert.h &&
symlink ../ARBDB/arbdb.h INCLUDE/arbdb.h &&
symlink ../ARBDB/arbdbt.h INCLUDE/arbdbt.h &&
symlink ../ARBDBPP/adtools.hxx INCLUDE/adtools.hxx &&
symlink ../ARBDBPP/arbdb++.hxx INCLUDE/arbdb++.hxx &&
symlink ../ARB_GDE/gde.hxx INCLUDE/gde.hxx &&
symlink ../AWT/awt.hxx INCLUDE/awt.hxx &&
symlink ../AWT/awt_advice.hxx INCLUDE/awt_advice.hxx &&
symlink ../AWT/awt_asciiprint.hxx INCLUDE/awt_asciiprint.hxx &&
symlink ../AWT/awt_attributes.hxx INCLUDE/awt_attributes.hxx &&
symlink ../AWT/awt_canvas.hxx INCLUDE/awt_canvas.hxx &&
symlink ../AWT/awt_changekey.hxx INCLUDE/awt_changekey.hxx &&
symlink ../AWT/awt_codon_table.hxx INCLUDE/awt_codon_table.hxx &&
symlink ../AWT/awt_config_manager.hxx INCLUDE/awt_config_manager.hxx &&
symlink ../AWT/awt_csp.hxx INCLUDE/awt_csp.hxx &&
symlink ../AWT/awt_dtree.hxx INCLUDE/awt_dtree.hxx &&
symlink ../AWT/awt_hotkeys.hxx INCLUDE/awt_hotkeys.hxx &&
symlink ../AWT/awt_imp_exp.hxx INCLUDE/awt_imp_exp.hxx &&
symlink ../AWT/awt_input_mask.hxx INCLUDE/awt_input_mask.hxx &&
symlink ../AWT/awt_iupac.hxx INCLUDE/awt_iupac.hxx &&
symlink ../AWT/awt_macro.hxx INCLUDE/awt_macro.hxx &&
symlink ../AWT/awt_map_key.hxx INCLUDE/awt_map_key.hxx &&
symlink ../AWT/awt_nds.hxx INCLUDE/awt_nds.hxx &&
symlink ../AWT/awt_preset.hxx INCLUDE/awt_preset.hxx &&
symlink ../AWT/awt_pro_a_nucs.hxx INCLUDE/awt_pro_a_nucs.hxx &&
symlink ../AWT/awt_sel_boxes.hxx INCLUDE/awt_sel_boxes.hxx &&
symlink ../AWT/awt_seq_colors.hxx INCLUDE/awt_seq_colors.hxx &&
symlink ../AWT/awt_seq_dna.hxx INCLUDE/awt_seq_dna.hxx &&
symlink ../AWT/awt_seq_protein.hxx INCLUDE/awt_seq_protein.hxx &&
symlink ../AWT/awt_seq_simple_pro.hxx INCLUDE/awt_seq_simple_pro.hxx &&
symlink ../AWT/awt_translate.hxx INCLUDE/awt_translate.hxx &&
# symlink ../AWT/awt_tree.hxx INCLUDE/PH_phylo.hxx &&
symlink ../AWT/awt_tree.hxx INCLUDE/awt_tree.hxx &&
symlink ../AWT/awt_tree_cb.hxx INCLUDE/awt_tree_cb.hxx &&
symlink ../AWT/awt_tree_cmp.hxx INCLUDE/awt_tree_cmp.hxx &&
symlink ../AWT/awt_www.hxx INCLUDE/awt_www.hxx &&
symlink ../AWT/awtlocal.hxx INCLUDE/awtlocal.hxx &&
symlink ../AWTC/awtc_constructSequence.hxx INCLUDE/awtc_constructSequence.hxx &&
symlink ../AWTC/awtc_fast_aligner.hxx INCLUDE/awtc_fast_aligner.hxx &&
symlink ../AWTC/awtc_next_neighbours.hxx INCLUDE/awtc_next_neighbours.hxx &&
# symlink ../AWTC/awtc_rename.hxx INCLUDE/awtc_rename.hxx &&
symlink ../AWTC/awtc_seq_search.hxx INCLUDE/awtc_seq_search.hxx &&
symlink ../AWTC/awtc_submission.hxx INCLUDE/awtc_submission.hxx &&
symlink ../AWTI/awti_export.hxx INCLUDE/awti_export.hxx &&
symlink ../AWTI/awti_import.hxx INCLUDE/awti_import.hxx &&
symlink ../BUGEX/bugex.h INCLUDE/bugex.h &&
symlink ../CAT/cat_tree.hxx INCLUDE/cat_tree.hxx &&
symlink ../CONSENSUS_TREE/CT_ctree.hxx INCLUDE/CT_ctree.hxx &&
symlink ../DIST/dist.hxx INCLUDE/dist.hxx &&
symlink ../GENOM/EXP.hxx INCLUDE/EXP.hxx &&
symlink ../GENOM/GEN.hxx INCLUDE/GEN.hxx &&
# symlink ../GENOM/GEN_tools.hxx INCLUDE/GEN_tools.hxx &&
# symlink ../GENOM/GEN_extern.h INCLUDE/GEN_extern.h &&
symlink ../GENOM_IMPORT/GAGenomImport.h INCLUDE/GAGenomImport.h &&
# symlink ../GENOM_IMPORT/GenomeImport.h INCLUDE/GenomeImport.h &&
symlink ../ISLAND_HOPPING/island_hopping.h INCLUDE/island_hopping.h &&
symlink ../MERGE/mg_merge.hxx INCLUDE/mg_merge.hxx &&
symlink ../NTREE/ntree.hxx INCLUDE/ntree.hxx &&
symlink ../PRIMER_DESIGN/primer_design.hxx INCLUDE/primer_design.hxx &&
symlink ../PROBE_DESIGN/probe_design.hxx INCLUDE/probe_design.hxx &&
symlink ../SECEDIT/sec_graphic.hxx INCLUDE/sec_graphic.hxx &&
symlink ../SECEDIT/secedit.hxx INCLUDE/secedit.hxx &&
symlink ../SEER/seer.hxx INCLUDE/seer.hxx &&
symlink ../SERVERCNTRL/servercntrl.h INCLUDE/servercntrl.h &&
symlink ../SEQ_QUALITY/seq_quality.h INCLUDE/seq_quality.h &&
symlink ../SL/AW_HELIX/AW_helix.hxx INCLUDE/AW_helix.hxx &&
symlink ../SL/AW_NAME/AW_rename.hxx INCLUDE/AW_rename.hxx &&
symlink ../SL/DB_SCANNER/db_scanner.hxx INCLUDE/db_scanner.hxx &&
symlink ../SL/FILE_BUFFER/FileBuffer.h INCLUDE/FileBuffer.h &&
symlink ../SL/HELIX/BI_helix.hxx INCLUDE/BI_helix.hxx &&
symlink ../STAT/st_window.hxx INCLUDE/st_window.hxx &&
symlink ../WINDOW/aw_awars.hxx INCLUDE/aw_awars.hxx &&
symlink ../WINDOW/aw_color_groups.hxx INCLUDE/aw_color_groups.hxx &&
symlink ../WINDOW/aw_device.hxx INCLUDE/aw_device.hxx &&
symlink ../WINDOW/aw_global_awars.hxx INCLUDE/aw_global_awars.hxx &&
symlink ../WINDOW/aw_keysym.hxx INCLUDE/aw_keysym.hxx &&
symlink ../WINDOW/aw_preset.hxx INCLUDE/aw_preset.hxx &&
symlink ../WINDOW/aw_question.hxx INCLUDE/aw_question.hxx &&
symlink ../WINDOW/aw_root.hxx INCLUDE/aw_root.hxx &&
symlink ../WINDOW/aw_window.hxx INCLUDE/aw_window.hxx &&
symlink ../WINDOW/aw_global.hxx INCLUDE/aw_global.hxx &&
symlink ../WINDOW/aw_font_group.hxx INCLUDE/aw_font_group.hxx &&
symlink ../XML/xml.hxx INCLUDE/xml.hxx &&

# gl stuff
symlink ../../GL/glpng/glpng.h INCLUDE/GL/glpng.h &&

# arbdb dirs

arbdb_symlink AD_MOBJECTS.h &&
arbdb_symlink adGene.c &&
arbdb_symlink adGene.h &&
arbdb_symlink adReference.c &&
arbdb_symlink adRevCompl.c &&
arbdb_symlink adTest.c &&
arbdb_symlink ad_core.c &&
arbdb_symlink ad_load.c &&
arbdb_symlink ad_lpro.h &&
arbdb_symlink ad_prot.h &&
arbdb_symlink ad_save_load.c &&
arbdb_symlink ad_t_lpro.h &&
arbdb_symlink ad_t_prot.h &&
arbdb_symlink adcomm.c &&
arbdb_symlink ad_config.c &&
arbdb_symlink ad_config.h &&
arbdb_symlink adcompr.c &&
arbdb_symlink adhash.c &&
arbdb_symlink adindex.c &&
arbdb_symlink adlang1.c &&
arbdb_symlink adlink.c &&
arbdb_symlink adlmacros.h &&
arbdb_symlink adlocal.h &&
arbdb_symlink adlundo.h &&
arbdb_symlink admalloc.c &&
arbdb_symlink admap.c &&
arbdb_symlink admap.h &&
arbdb_symlink admath.c &&
arbdb_symlink adoptimize.c &&
arbdb_symlink adperl.c &&
arbdb_symlink adquery.c &&
arbdb_symlink adregexp.h &&
arbdb_symlink adseqcompr.c &&
arbdb_symlink adseqcompr.h &&
arbdb_symlink adsocket.c &&
arbdb_symlink adsort.c &&
arbdb_symlink adstring.c &&
arbdb_symlink adsystem.c &&
arbdb_symlink adtables.c &&
arbdb_symlink adtcp.c &&
arbdb_symlink adtools.c &&
arbdb_symlink adtune.c &&
arbdb_symlink adtune.h &&
arbdb_symlink arbdb.c &&
arbdb_symlink arbdb.h &&
arbdb_symlink arbdbpp.cxx &&
arbdb_symlink arbdbt.h &&

# small dirs

symlink ../WINDOW/AW_preset.cxx AWT/AWT_preset.cxx &&
symlink ../WINDOW/aw_def.hxx AWT/aw_def.hxx &&

symlink ../EDIT/edit_naligner.cxx EDIT4/edit_naligner.cxx &&
symlink ../EDIT/edit_naligner.hxx EDIT4/edit_naligner.hxx &&

symlink ../LIBLINK          NALIGNER/LIBLINK &&

symlink ../LIBLINK          TOOLS/LIBLINK &&

symlink_maybe_no_target ../AISC/aisc             MAKEBIN/aisc &&
symlink_maybe_no_target ../AISC_MKPTPS/aisc_mkpt MAKEBIN/aisc_mkpt &&

# help files (make sure the file is present in user distribution!)

symlink_maybe_no_target ../help/input_mask_format.hlp     lib/inputMasks/format.readme &&
symlink ../../GDEHELP                     lib/help/GDEHELP &&

# links related to probe web service
symlink ../PROBE_WEB/SERVER PROBE_SERVER/ps_cgi &&

echo "generate_all_links.sh done."

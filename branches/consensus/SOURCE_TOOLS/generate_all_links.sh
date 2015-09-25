#!/bin/bash

SELF=$ARBHOME/SOURCE_TOOLS/generate_all_links.sh
READLINK=${ARBHOME}/SH/arb_readlink


finderr() {
    FOUND=`grep -Hn "$1" $SELF | perl -ne '/^[^:]+:[^:]+:/; print $&."[here]\n";'`
    if [ -z "$FOUND" ]; then
        echo "$SELF:8: $2 ($1 not located -- search manually)"
    else
        echo "$FOUND $2"
    fi
    false
}

may_create_link() {
    # $1 is the link to create
    # $2 ==1 -> warn about links to nowhere
    if [ -h $1 ]; then
        if [ -e $1 ]; then
            # points to sth existing, assume link already present and valid
            true
        else
            # points to nothing
            if [ $2 = 1 ]; then
                finderr $1 "Note: Symlink '$1' pointed to nowhere -- removing wrong link"
                ls -al $1
            fi
            rm $1 # remove wrong link
            true # allow regeneration
        fi
    else
        if [ -e $1 ]; then
            finderr $1 "$1 is in the way (and is not a link)"
        else
            true # link simply missing, allow creation
        fi
    fi
}

assert_links_to_target() {
    # $1 target
    # $2 link
    local LINKTARGET=`$READLINK -f $2`
    local LINKDIR=`dirname $2`
    local TARGET=`$READLINK -f $LINKDIR/$1`

    [ "$LINKTARGET" = "$TARGET" ] || (finderr $2 "$2 links not to $TARGET")
}

create_symlink_unverified() {
    # $1 target
    # $2 link
    test -h $2 || ln -sf $1 $2 || finderr $2 "Failed to link '$1->$2'"
}

create_symlink() {
    # $1 target
    # $2 link
    create_symlink_unverified $1 $2 && assert_links_to_target $1 $2
}

symlink_maybe_no_target() {
    may_create_link $2 0 && create_symlink_unverified $1 $2
}

symlink_typed() {
    # $1 is target
    # $2 is the created link
    # $3 is the expected target type (e.g. -d or -f)
    if [ -z $2 ]; then
        if [ -z $1 ]; then
            echo "$SELF:25: Missing arguments in call to symlink_typed()"
            exit 1
        else
            finderr $1 "Second argument missing in call to symlink_typed()"
            exit 1
        fi
    fi

    DIR=`dirname $2`
    if [ -z $DIR ]; then
        DIR=.
    fi

    (test -e $DIR/$1 || finderr $2 "Target '$DIR/$1 does not exists (anymore)" ) &&
    (test $3 $DIR/$1 || finderr $2 "Target '$DIR/$1 has wrong type (expected $3)" ) &&
    
    may_create_link $2 1 && create_symlink $1 $2
}

symlink_dir() {
    # $1 is target (a directory)
    # $2 is the created link
    symlink_typed $1 $2 -d
}

symlink_file() {
    # $1 is target (a file)
    # $2 is the created link
    symlink_typed $1 $2 -f
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

makedir PROBE_COM/GENC &&
makedir PROBE_COM/GENH &&
makedir PROBE_COM/O &&

makedir lib/help &&

(test -d lib/pts || makedir lib/pts) &&

# Motif stuff
(test -z $MOTIF_LIBPATH || symlink_file $MOTIF_LIBPATH lib/libXm.so.3) &&

# Links in bin directory
( cd bin ; make all; cd .. ) &&

# ...COMS

symlink_dir ../AISC_COM/AISC NAMES_COM/AISC &&
symlink_dir ../AISC_COM/C    NAMES_COM/C &&

symlink_maybe_no_target GENH/aisc_com.h           NAMES_COM/names_client.h &&
symlink_maybe_no_target GENH/aisc_server_proto.h  NAMES_COM/names_prototypes.h &&
symlink_maybe_no_target GENH/aisc.h               NAMES_COM/names_server.h &&

symlink_dir ../AISC_COM/AISC PROBE_COM/AISC &&
symlink_dir ../AISC_COM/C    PROBE_COM/C &&

symlink_maybe_no_target GENH/aisc_com.h           PROBE_COM/PT_com.h &&
symlink_maybe_no_target GENH/aisc_server_proto.h  PROBE_COM/PT_server_prototypes.h &&
symlink_maybe_no_target GENH/aisc.h               PROBE_COM/PT_server.h &&

# TEMPLATES directory

symlink_file ../TEMPLATES/arb_algo.h INCLUDE/arb_algo.h &&
symlink_file ../TEMPLATES/arb_backtrace.h INCLUDE/arb_backtrace.h &&
symlink_file ../TEMPLATES/arb_debug.h INCLUDE/arb_debug.h &&
symlink_file ../TEMPLATES/arb_defs.h INCLUDE/arb_defs.h &&
symlink_file ../TEMPLATES/arb_early_check.h INCLUDE/arb_early_check.h &&
symlink_file ../TEMPLATES/arb_error.h INCLUDE/arb_error.h &&
symlink_file ../TEMPLATES/arb_forward_list.h INCLUDE/arb_forward_list.h &&
symlink_file ../TEMPLATES/arb_global_defs.h INCLUDE/arb_global_defs.h &&
symlink_file ../TEMPLATES/arb_simple_assert.h INCLUDE/arb_simple_assert.h &&
symlink_file ../TEMPLATES/arb_sleep.h INCLUDE/arb_sleep.h &&
symlink_file ../TEMPLATES/arb_stdstr.h INCLUDE/arb_stdstr.h &&
symlink_file ../TEMPLATES/arb_str.h INCLUDE/arb_str.h &&
symlink_file ../TEMPLATES/arb_unit_test.h INCLUDE/arb_unit_test.h &&
symlink_file ../TEMPLATES/arb_unordered_map.h INCLUDE/arb_unordered_map.h &&
symlink_file ../TEMPLATES/arb_version.h INCLUDE/arb_version.h &&
symlink_file ../TEMPLATES/arbtools.h INCLUDE/arbtools.h &&
symlink_file ../TEMPLATES/attributes.h INCLUDE/attributes.h &&
symlink_file ../TEMPLATES/bytestring.h INCLUDE/bytestring.h &&
symlink_file ../TEMPLATES/cache.h INCLUDE/cache.h &&
symlink_file ../TEMPLATES/ChecksumCollector.h INCLUDE/ChecksumCollector.h &&
symlink_file ../TEMPLATES/command_output.h INCLUDE/command_output.h &&
symlink_file ../TEMPLATES/config_parser.h INCLUDE/config_parser.h &&
symlink_file ../TEMPLATES/consensus_config.h INCLUDE/consensus_config.h &&
symlink_file ../TEMPLATES/cxxforward.h INCLUDE/cxxforward.h &&
symlink_file ../TEMPLATES/downcast.h INCLUDE/downcast.h &&
symlink_file ../TEMPLATES/dupstr.h INCLUDE/dupstr.h &&
symlink_file ../TEMPLATES/gccver.h INCLUDE/gccver.h &&
symlink_file ../TEMPLATES/Keeper.h INCLUDE/Keeper.h &&
symlink_file ../TEMPLATES/malloc.h INCLUDE/malloc.h &&
symlink_file ../TEMPLATES/mode_text.h INCLUDE/mode_text.h &&
symlink_file ../TEMPLATES/output.h INCLUDE/output.h &&
symlink_file ../TEMPLATES/perf_timer.h INCLUDE/perf_timer.h &&
symlink_file ../TEMPLATES/SigHandler.h INCLUDE/SigHandler.h &&
symlink_file ../TEMPLATES/smartptr.h INCLUDE/smartptr.h &&
symlink_file ../TEMPLATES/static_assert.h INCLUDE/static_assert.h &&
symlink_file ../TEMPLATES/SuppressOutput.h INCLUDE/SuppressOutput.h &&
symlink_file ../TEMPLATES/ttypes.h INCLUDE/ttypes.h &&
symlink_file ../TEMPLATES/ut_valgrinded.h INCLUDE/ut_valgrinded.h &&
symlink_file ../TEMPLATES/valgrind.h INCLUDE/valgrind.h &&

symlink_maybe_no_target ../TEMPLATES/arb_build.h INCLUDE/arb_build.h &&
symlink_maybe_no_target ../TEMPLATES/svn_revision.h INCLUDE/svn_revision.h &&

# INCLUDE directory

symlink_maybe_no_target ../NAMES_COM/names_client.h         INCLUDE/names_client.h &&
symlink_maybe_no_target ../NAMES_COM/names_prototypes.h     INCLUDE/names_prototypes.h &&
symlink_maybe_no_target ../NAMES_COM/names_server.h         INCLUDE/names_server.h &&

symlink_maybe_no_target ../PROBE_COM/PT_com.h               INCLUDE/PT_com.h &&
symlink_maybe_no_target ../PROBE_COM/PT_server.h            INCLUDE/PT_server.h &&
symlink_maybe_no_target ../PROBE_COM/PT_server_prototypes.h INCLUDE/PT_server_prototypes.h &&

symlink_file ../AISC_COM/C/aisc_func_types.h INCLUDE/aisc_func_types.h &&
symlink_file ../AISC_COM/C/aisc_global.h INCLUDE/aisc_global.h &&
symlink_file ../AISC_COM/C/client.h INCLUDE/client.h &&
symlink_file ../AISC_COM/C/client_types.h INCLUDE/client_types.h &&
symlink_file ../AISC_COM/C/client_privat.h INCLUDE/client_privat.h &&
symlink_file ../AISC_COM/C/server.h INCLUDE/server.h &&
symlink_file ../AISC_COM/C/struct_man.h INCLUDE/struct_man.h &&
symlink_file ../ARBDB/ad_cb.h INCLUDE/ad_cb.h &&
symlink_file ../ARBDB/ad_cb_prot.h INCLUDE/ad_cb_prot.h &&
symlink_file ../ARBDB/ad_config.h INCLUDE/ad_config.h &&
symlink_file ../ARBDB/ad_colorset.h INCLUDE/ad_colorset.h &&
symlink_file ../ARBDB/ad_p_prot.h INCLUDE/ad_p_prot.h &&
symlink_file ../ARBDB/ad_prot.h INCLUDE/ad_prot.h &&
symlink_file ../ARBDB/ad_remote.h INCLUDE/ad_remote.h &&
symlink_file ../ARBDB/ad_t_prot.h INCLUDE/ad_t_prot.h &&
symlink_file ../ARBDB/adGene.h INCLUDE/adGene.h &&
symlink_file ../ARBDB/adperl.h INCLUDE/adperl.h &&
symlink_file ../ARBDB/arbdb.h INCLUDE/arbdb.h &&
symlink_file ../ARBDB/arbdb_base.h INCLUDE/arbdb_base.h &&
symlink_file ../ARBDB/arbdbt.h INCLUDE/arbdbt.h &&
symlink_file ../ARBDB/dbitem_set.h INCLUDE/dbitem_set.h &&
symlink_file ../ARBDB/TreeNode.h INCLUDE/TreeNode.h &&
symlink_file ../ARB_GDE/gde.hxx INCLUDE/gde.hxx &&
symlink_file ../AWT/awt.hxx INCLUDE/awt.hxx &&
symlink_file ../AWT/awt_asciiprint.hxx INCLUDE/awt_asciiprint.hxx &&
symlink_file ../AWT/awt_canvas.hxx INCLUDE/awt_canvas.hxx &&
symlink_file ../AWT/awt_config_manager.hxx INCLUDE/awt_config_manager.hxx &&
symlink_file ../AWT/awt_hotkeys.hxx INCLUDE/awt_hotkeys.hxx &&
symlink_file ../AWT/awt_input_mask.hxx INCLUDE/awt_input_mask.hxx &&
symlink_file ../AWT/awt_map_key.hxx INCLUDE/awt_map_key.hxx &&
symlink_file ../AWT/awt_misc.hxx INCLUDE/awt_misc.hxx &&
symlink_file ../AWT/awt_modules.hxx INCLUDE/awt_modules.hxx &&
symlink_file ../AWT/awt_sel_boxes.hxx INCLUDE/awt_sel_boxes.hxx &&
symlink_file ../AWT/awt_TreeAwars.hxx INCLUDE/awt_TreeAwars.hxx &&
symlink_file ../AWT/awt_www.hxx INCLUDE/awt_www.hxx &&
symlink_file ../AWT/awtlocal.hxx INCLUDE/awtlocal.hxx &&
symlink_file ../AWTC/awtc_next_neighbours.hxx INCLUDE/awtc_next_neighbours.hxx &&
symlink_file ../AWTC/awtc_submission.hxx INCLUDE/awtc_submission.hxx &&
symlink_file ../AWTI/awti_export.hxx INCLUDE/awti_export.hxx &&
symlink_file ../AWTI/awti_import.hxx INCLUDE/awti_import.hxx &&
symlink_file ../BUGEX/bugex.h INCLUDE/bugex.h &&
symlink_file ../CONSENSUS_TREE/CT_ctree.hxx INCLUDE/CT_ctree.hxx &&
symlink_file ../CORE/arb_assert.h INCLUDE/arb_assert.h &&
symlink_file ../CORE/arb_core.h INCLUDE/arb_core.h &&
symlink_file ../CORE/arb_cs.h INCLUDE/arb_cs.h &&
symlink_file ../CORE/arb_diff.h INCLUDE/arb_diff.h &&
symlink_file ../CORE/arb_file.h INCLUDE/arb_file.h &&
symlink_file ../CORE/arb_handlers.h INCLUDE/arb_handlers.h &&
symlink_file ../CORE/arb_match.h INCLUDE/arb_match.h &&
symlink_file ../CORE/arb_mem.h INCLUDE/arb_mem.h &&
symlink_file ../CORE/arb_misc.h INCLUDE/arb_misc.h &&
symlink_file ../CORE/arb_msg.h INCLUDE/arb_msg.h &&
symlink_file ../CORE/arb_msg_fwd.h INCLUDE/arb_msg_fwd.h &&
symlink_file ../CORE/arb_pathlen.h INCLUDE/arb_pathlen.h &&
symlink_file ../CORE/arb_progress.h INCLUDE/arb_progress.h &&
symlink_file ../CORE/arb_signal.h INCLUDE/arb_signal.h &&
symlink_file ../CORE/arb_sort.h INCLUDE/arb_sort.h &&
symlink_file ../CORE/arb_strarray.h INCLUDE/arb_strarray.h &&
symlink_file ../CORE/arb_strbuf.h INCLUDE/arb_strbuf.h &&
symlink_file ../CORE/arb_string.h INCLUDE/arb_string.h &&
symlink_file ../CORE/BufferedFileReader.h INCLUDE/BufferedFileReader.h &&
symlink_file ../CORE/MultiFileReader.h INCLUDE/MultiFileReader.h &&
symlink_file ../CORE/FileContent.h INCLUDE/FileContent.h &&
symlink_file ../CORE/pos_range.h INCLUDE/pos_range.h &&
symlink_file ../DIST/dist.hxx INCLUDE/dist.hxx &&
symlink_file ../EDIT4/ed4_extern.hxx INCLUDE/ed4_extern.hxx &&
symlink_file ../EDIT4/ed4_plugins.hxx INCLUDE/ed4_plugins.hxx &&
symlink_file ../GENOM/EXP.hxx INCLUDE/EXP.hxx &&
symlink_file ../GENOM/GEN.hxx INCLUDE/GEN.hxx &&
symlink_file ../GENOM_IMPORT/GenomeImport.h INCLUDE/GenomeImport.h &&
symlink_file ../ISLAND_HOPPING/island_hopping.h INCLUDE/island_hopping.h &&
symlink_file ../MERGE/mg_merge.hxx INCLUDE/mg_merge.hxx &&
symlink_file ../MULTI_PROBE/multi_probe.hxx INCLUDE/multi_probe.hxx &&
symlink_file ../PRIMER_DESIGN/primer_design.hxx INCLUDE/primer_design.hxx &&
symlink_file ../PROBE_DESIGN/probe_gui.hxx INCLUDE/probe_gui.hxx &&
symlink_file ../PROBE/PT_global_defs.h INCLUDE/PT_global_defs.h &&
symlink_file ../SECEDIT/secedit_extern.hxx INCLUDE/secedit_extern.hxx &&
symlink_file ../RNA3D/rna3d_extern.hxx INCLUDE/rna3d_extern.hxx &&
symlink_file ../SEQ_QUALITY/seq_quality.h INCLUDE/seq_quality.h &&
symlink_file ../SERVERCNTRL/servercntrl.h INCLUDE/servercntrl.h &&
symlink_file ../SL/ALILINK/AliAdmin.h INCLUDE/AliAdmin.h &&
symlink_file ../SL/ALIVIEW/AliView.hxx INCLUDE/AliView.hxx &&
symlink_file ../SL/AP_TREE/AP_Tree.hxx INCLUDE/AP_Tree.hxx &&
symlink_file ../SL/ARB_TREE/ARB_Tree.hxx INCLUDE/ARB_Tree.hxx &&
symlink_file ../SL/AW_HELIX/AW_helix.hxx INCLUDE/AW_helix.hxx &&
symlink_file ../SL/AW_NAME/AW_rename.hxx INCLUDE/AW_rename.hxx &&
symlink_file ../SL/CB/cb.h INCLUDE/cb.h &&
symlink_file ../SL/CB/cb_base.h INCLUDE/cb_base.h &&
symlink_file ../SL/CB/cb_base.h INCLUDE/cb_base.h &&
symlink_file ../SL/CB/cbtypes.h INCLUDE/cbtypes.h &&
symlink_file ../SL/CB/rootAsWin.h INCLUDE/rootAsWin.h &&
symlink_file ../SL/CONSENSUS/consensus.h INCLUDE/consensus.h &&
symlink_file ../SL/DB_QUERY/db_query.h INCLUDE/db_query.h &&
symlink_file ../SL/DB_SCANNER/db_scanner.hxx INCLUDE/db_scanner.hxx &&
symlink_file ../SL/DB_UI/dbui.h INCLUDE/dbui.h &&
symlink_file ../SL/DB_UI/info_window.h INCLUDE/info_window.h &&
symlink_file ../SL/FAST_ALIGNER/fast_aligner.hxx INCLUDE/fast_aligner.hxx &&
symlink_file ../SL/FILTER/AP_filter.hxx INCLUDE/AP_filter.hxx &&
symlink_file ../SL/FILTER/RangeList.h INCLUDE/RangeList.h &&
symlink_file ../SL/GUI_ALIVIEW/awt_filter.hxx INCLUDE/awt_filter.hxx &&
symlink_file ../SL/GUI_ALIVIEW/ColumnStat.hxx INCLUDE/ColumnStat.hxx &&
symlink_file ../SL/GUI_ALIVIEW/gui_aliview.hxx INCLUDE/gui_aliview.hxx &&
symlink_file ../SL/HELIX/BI_basepos.hxx INCLUDE/BI_basepos.hxx &&
symlink_file ../SL/HELIX/BI_helix.hxx INCLUDE/BI_helix.hxx &&
symlink_file ../SL/INSDEL/insdel.h INCLUDE/insdel.h &&
symlink_file ../SL/ITEMS/item_sel_list.h INCLUDE/item_sel_list.h &&
symlink_file ../SL/ITEMS/items.h INCLUDE/items.h &&
symlink_file ../SL/LOCATION/Location.h INCLUDE/Location.h &&
symlink_file ../SL/MACROS/macros.hxx INCLUDE/macros.hxx &&
symlink_file ../SL/MATRIX/AP_matrix.hxx INCLUDE/AP_matrix.hxx &&
symlink_file ../SL/NDS/nds.h INCLUDE/nds.h &&
symlink_file ../SL/NEIGHBOURJOIN/neighbourjoin.hxx INCLUDE/neighbourjoin.hxx &&
symlink_file ../SL/PRONUC/AP_codon_table.hxx INCLUDE/AP_codon_table.hxx &&
symlink_file ../SL/PRONUC/AP_pro_a_nucs.hxx INCLUDE/AP_pro_a_nucs.hxx &&
symlink_file ../SL/PRONUC/iupac.h INCLUDE/iupac.h &&
symlink_file ../SL/PTCLEAN/ptclean.h INCLUDE/ptclean.h &&
symlink_file ../SL/REFENTRIES/refentries.h INCLUDE/refentries.h &&
symlink_file ../SL/REGEXPR/RegExpr.hxx INCLUDE/RegExpr.hxx &&
symlink_file ../SL/SEQIO/seqio.hxx INCLUDE/seqio.hxx &&
symlink_file ../SL/SEQUENCE/AP_seq_dna.hxx INCLUDE/AP_seq_dna.hxx &&
symlink_file ../SL/SEQUENCE/AP_seq_protein.hxx INCLUDE/AP_seq_protein.hxx &&
symlink_file ../SL/SEQUENCE/AP_seq_simple_pro.hxx INCLUDE/AP_seq_simple_pro.hxx &&
symlink_file ../SL/SEQUENCE/AP_sequence.hxx INCLUDE/AP_sequence.hxx &&
symlink_file ../SL/TRANSLATE/Translate.hxx INCLUDE/Translate.hxx &&
symlink_file ../SL/TREE_ADMIN/TreeAdmin.h INCLUDE/TreeAdmin.h &&
symlink_file ../SL/TREE_READ/TreeRead.h INCLUDE/TreeRead.h &&
symlink_file ../SL/TREE_WRITE/TreeWrite.h INCLUDE/TreeWrite.h &&
symlink_file ../SL/TREEDISP/TreeCallbacks.hxx INCLUDE/TreeCallbacks.hxx &&
symlink_file ../SL/TREEDISP/TreeDisplay.hxx INCLUDE/TreeDisplay.hxx &&
symlink_file ../STAT/st_window.hxx INCLUDE/st_window.hxx &&
symlink_file ../UNIT_TESTER/test_unit.h INCLUDE/test_unit.h &&
symlink_file ../UNIT_TESTER/test_global.h INCLUDE/test_global.h &&
symlink_file ../UNIT_TESTER/test_helpers.h INCLUDE/test_helpers.h &&
symlink_file ../WINDOW/aw_advice.hxx INCLUDE/aw_advice.hxx &&
symlink_file ../WINDOW/aw_awar.hxx INCLUDE/aw_awar.hxx &&
symlink_file ../WINDOW/aw_awar_defs.hxx INCLUDE/aw_awar_defs.hxx &&
symlink_file ../WINDOW/aw_awars.hxx INCLUDE/aw_awars.hxx &&
symlink_file ../WINDOW/aw_base.hxx INCLUDE/aw_base.hxx &&
symlink_file ../WINDOW/aw_color_groups.hxx INCLUDE/aw_color_groups.hxx &&
symlink_file ../WINDOW/aw_device.hxx INCLUDE/aw_device.hxx &&
symlink_file ../WINDOW/aw_device_click.hxx INCLUDE/aw_device_click.hxx &&
symlink_file ../WINDOW/aw_edit.hxx INCLUDE/aw_edit.hxx &&
symlink_file ../WINDOW/aw_file.hxx INCLUDE/aw_file.hxx &&
symlink_file ../WINDOW/aw_font_group.hxx INCLUDE/aw_font_group.hxx &&
symlink_file ../WINDOW/aw_global.hxx INCLUDE/aw_global.hxx &&
symlink_file ../WINDOW/aw_global_awars.hxx INCLUDE/aw_global_awars.hxx &&
symlink_file ../WINDOW/aw_keysym.hxx INCLUDE/aw_keysym.hxx &&
symlink_file ../WINDOW/aw_msg.hxx INCLUDE/aw_msg.hxx &&
symlink_file ../WINDOW/aw_position.hxx INCLUDE/aw_position.hxx &&
symlink_file ../WINDOW/aw_preset.hxx INCLUDE/aw_preset.hxx &&
symlink_file ../WINDOW/aw_question.hxx INCLUDE/aw_question.hxx &&
symlink_file ../WINDOW/aw_root.hxx INCLUDE/aw_root.hxx &&
symlink_file ../WINDOW/aw_scalar.hxx INCLUDE/aw_scalar.hxx &&
symlink_file ../WINDOW/aw_select.hxx INCLUDE/aw_select.hxx &&
symlink_file ../WINDOW/aw_window.hxx INCLUDE/aw_window.hxx &&
symlink_file ../WINDOW/aw_window_Xm_interface.hxx INCLUDE/aw_window_Xm_interface.hxx &&
symlink_file ../XML/xml.hxx INCLUDE/xml.hxx &&

# gl stuff
symlink_file ../../GL/glpng/glpng.h INCLUDE/GL/glpng.h &&
symlink_file ../../GL/glAW/aw_window_ogl.hxx INCLUDE/GL/aw_window_ogl.hxx &&

# help files (make sure the file is present in user distribution!)
symlink_maybe_no_target ../help/input_mask_format.hlp     lib/inputMasks/format.readme &&

echo "generate_all_links.sh done."

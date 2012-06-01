// =============================================================== //
//                                                                 //
//   File      : ED4_tools.cxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in January 2010   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ed4_class.hxx"
#include "ed4_edit_string.hxx"
#include "ed4_tools.hxx"
#include "ed4_awars.hxx"

#include <arbdbt.h>
#include <aw_awar.hxx>
#include <aw_root.hxx>
#include <fast_aligner.hxx>
#include <iupac.h>

#include <cctype>

char ED4_encode_iupac(const char bases[], GB_alignment_type ali) {
    return iupac::encode(bases, ali);
}

const char *ED4_decode_iupac(char iupac, GB_alignment_type ali) {
    return iupac::decode(iupac, ali, ED4_ROOT->aw_root->awar(ED4_AWAR_CONSENSUS_GROUP)->read_int()); // IUPAC shown in consensus!
}

void ED4_set_clipping_rectangle(AW_screen_area *rect)
{
    // The following can't be replaced by set_cliprect (set_cliprect clears font overlaps):

    current_device()->set_top_clip_border(rect->t);
    current_device()->set_bottom_clip_border(rect->b);
    current_device()->set_left_clip_border(rect->l);
    current_device()->set_right_clip_border(rect->r);
}

void ED4_aws_init(AW_root *root, AW_window_simple *aws, GB_CSTR macro_format, GB_CSTR window_format, GB_CSTR typeId) {
    int typeIdLen = strlen(typeId);
    int macro_format_len = strlen(macro_format);
    int window_format_len = strlen(window_format);
    char *macro_buffer = GB_give_buffer(macro_format_len+window_format_len+2*typeIdLen);
    char *window_buffer = macro_buffer+macro_format_len+typeIdLen;

    sprintf(macro_buffer, macro_format, typeId);
    sprintf(window_buffer, window_format, typeId);

    char *macro = GBS_string_2_key(macro_buffer);
    aws->init(root, macro, window_buffer);
    free(macro);
}

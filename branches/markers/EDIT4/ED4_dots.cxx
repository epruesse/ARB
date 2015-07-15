// ================================================================ //
//                                                                  //
//   File      : ED4_dots.cxx                                       //
//   Purpose   : Insert dots where bases may be missing             //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in December 2008   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "ed4_dots.hxx"
#include "ed4_class.hxx"

#include <awt_sel_boxes.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <arbdbt.h>
#include <cctype>
#include <awt_config_manager.hxx>

using namespace std;

#define AWAR_DOT_BASE      "dotmiss/"
#define AWAR_DOT_SAI       AWAR_DOT_BASE "sainame" // selected SAI
#define AWAR_DOT_SAI_CHARS AWAR_DOT_BASE "chars"   // handle columns where SAI contains one of these chars
#define AWAR_DOT_MARKED    AWAR_DOT_BASE "marked"  // handle marked only?

struct dot_insert_stat {
    size_t  pos_count;
    size_t *position;           // contains 'pos_count' positions, where dots get inserted if sequence contains '-'

    bool marked_only;

    // statistics:
    size_t dots_inserted;
    size_t already_there;
    size_t sequences_checked;
};

static ARB_ERROR dot_sequence_by_consensus(ED4_base *base, AW_CL cl_insert_stat) {
    ARB_ERROR error = 0;

    if (base->is_sequence_info_terminal()) {
        ED4_sequence_info_terminal *seq_term = base->to_sequence_info_terminal();
        if (seq_term) {
            GBDATA *gb_ali = seq_term->data();
            if (gb_ali) {
                GBDATA           *gb_species = GB_get_grandfather(gb_ali);
                bool              marked     = GB_read_flag(gb_species);
                dot_insert_stat&  stat       = *reinterpret_cast<dot_insert_stat *>(cl_insert_stat);

                if (marked || !stat.marked_only) {
                    char *sequence = GB_read_string(gb_ali);

                    if (!sequence) {
                        GB_ERROR err = GB_await_error();
                        error        = GBS_global_string("No sequence found for '%s'\n(Reason: %s)",
                                                         GBT_read_name(gb_species), err);
                    }
                    else {
                        size_t length            = GB_read_string_count(gb_ali);
                        size_t old_dots_inserted = stat.dots_inserted;

                        for (size_t p = 0; p<stat.pos_count; p++) {
                            size_t pos = stat.position[p];

                            if (pos<length) {
                                switch (sequence[pos]) {
                                    case '-':
                                        sequence[pos] = '.';
                                        stat.dots_inserted++;
                                        break;

                                    case '.':
                                        stat.already_there++;
                                        break;

                                    default:
                                        break;
                                }
                            }
                        }

                        if (stat.dots_inserted > old_dots_inserted) { // did sequence change?
                            error = GB_write_string(gb_ali, sequence);
                        }

                        free(sequence);
                        stat.sequences_checked++;
                    }
                }
            }
        }
    }

    return error;
}

static void dot_missing_bases(AW_window *aww) {
    ED4_MostRecentWinContext context;

    ED4_cursor *cursor = &current_cursor();
    ARB_ERROR   error  = 0;

    if (!cursor->in_consensus_terminal()) {
        error = "No consensus selected";
    }
    else {
        AW_root *aw_root = aww->get_root();

        dot_insert_stat stat;
        stat.dots_inserted     = 0;
        stat.already_there     = 0;
        stat.position          = 0;
        stat.sequences_checked = 0;
        stat.marked_only       = aw_root->awar(AWAR_DOT_MARKED)->read_int();

        ED4_group_manager *group_manager = cursor->owner_of_cursor->get_parent(ED4_L_GROUP)->to_group_manager();
        {
            // build list of positions where consensus contains upper case characters:
            char *consensus = group_manager->table().build_consensus_string();
            for (int pass = 1; pass <= 2; pass++) {
                stat.pos_count = 0;
                for (int pos = 0; consensus[pos]; pos++) {
                    if (isupper(consensus[pos])) {
                        if (pass == 2) stat.position[stat.pos_count] = pos;
                        stat.pos_count++;
                    }
                }

                if (pass == 1) stat.position = (size_t*)malloc(stat.pos_count * sizeof(*stat.position));
            }
            free(consensus);
        }

        if (!stat.pos_count) {
            error = "No consensus column contains upper case characters";
        }
        else {
            // if SAI is selected, reduce list of affected positions
            char   *sai     = 0;
            size_t  sai_len = -1;
            {
                GB_transaction  ta(GLOBAL_gb_main);
                char           *sai_name = aw_root->awar(AWAR_DOT_SAI)->read_string();

                if (sai_name && sai_name[0]) {
                    GBDATA *gb_sai     = GBT_expect_SAI(GLOBAL_gb_main, sai_name);
                    if (!gb_sai) error = GB_await_error();
                    else {
                        GBDATA *gb_ali = GBT_find_sequence(gb_sai, ED4_ROOT->alignment_name);
                        if (!gb_ali) {
                            error = GBS_global_string("SAI '%s' has no data in '%s'", sai_name, ED4_ROOT->alignment_name);
                        }
                        else {
                            sai     = GB_read_string(gb_ali);
                            sai_len = GB_read_string_count(gb_ali);
                        }
                    }
                }
                free(sai_name);
                error = ta.close(error);
            }

            if (sai) { // SAI is selected
                if (!error) {
                    char *sai_chars = aw_root->awar(AWAR_DOT_SAI_CHARS)->read_string();
                    if (sai_chars[0] == 0) error = "No SAI characters given -> no column selectable";
                    else {
                        size_t k = 0;
                        size_t p;
                        for (p = 0; p<stat.pos_count && stat.position[p]<sai_len; p++) {
                            size_t  pos = stat.position[p];
                            if (strchr(sai_chars, sai[pos]) != NULL) { // SAI contains one of the 'sai_chars'
                                stat.position[k++] = pos; // use current position
                            }
                        }
                        stat.pos_count = k;

                        if (!stat.pos_count) error = "SAI selects other columns than consensus. Nothing to do.";
                    }
                    free(sai_chars);
                }
                free(sai);
            }
        }

        if (!error) {
            e4_assert(stat.pos_count);
            GB_transaction ta(GLOBAL_gb_main);
            error = group_manager->route_down_hierarchy(dot_sequence_by_consensus, (AW_CL)&stat);

            if (stat.sequences_checked == 0 && !error) {
                error = GBS_global_string("Group contains no %ssequences", stat.marked_only ? "marked " : "");
            }

            if (!error) {
                const char *present = "";
                if (stat.already_there) {
                    present = GBS_global_string("Dots already present: %zu  ", stat.already_there);
                }

                const char *changed = stat.dots_inserted
                    ? GBS_global_string("Gaps changed into dots: %zu", stat.dots_inserted)
                    : "No gaps were changed into dots.";

                aw_message(GBS_global_string("%s%s", present, changed));
            }

            error = ta.close(error);
        }
    }

    aw_message_if(error);
}

void ED4_create_dot_missing_bases_awars(AW_root *aw_root, AW_default aw_def) {
    aw_root->awar_int(AWAR_DOT_MARKED, 0, aw_def);
    aw_root->awar_string(AWAR_DOT_SAI, "", aw_def);
    aw_root->awar_string(AWAR_DOT_SAI_CHARS, "", aw_def);
}

static AWT_config_mapping_def dotbases_config_mapping[] = {
    { AWAR_DOT_MARKED,    "marked" },
    { AWAR_DOT_SAI,       "sai" },
    { AWAR_DOT_SAI_CHARS, "saichars" },

    { 0, 0 }
};

void ED4_popup_dot_missing_bases_window(AW_window *editor_window, AW_CL, AW_CL) {
    AW_root                 *aw_root = editor_window->get_root();
    static AW_window_simple *aws     = 0;

    ED4_LocalWinContext uses(editor_window);

    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aw_root, "DOT_MISS_BASES", "Dot potentially missing bases");
        aws->load_xfig("edit4/missbase.fig");

        aws->button_length(10);
        
        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("missbase.hlp"));
        aws->create_button("HELP", "HELP", "H");

        aws->at("marked");
        aws->label("Marked species only");
        aws->create_toggle(AWAR_DOT_MARKED);

        aws->at("SAI");
        aws->button_length(30);
        awt_create_SAI_selection_button(GLOBAL_gb_main, aws, AWAR_DOT_SAI);

        aws->at("SAI_chars");
        aws->label("contains one of");
        aws->create_input_field(AWAR_DOT_SAI_CHARS, 20);

        aws->button_length(10);

        aws->at("cons_def");
        aws->label("Change definition of");
        aws->callback(ED4_create_consensus_definition_window);
        aws->create_button("CONS_DEF", "Consensus", "C");

        aws->at("go");
        aws->callback(dot_missing_bases);
        aws->create_button("GO", "GO", "G");

        aws->at("config");
        AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "dotbases", dotbases_config_mapping);
    }

    e4_assert(aws);

    aws->activate();
}


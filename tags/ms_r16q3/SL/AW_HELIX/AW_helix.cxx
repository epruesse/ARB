// ==================================================================== //
//                                                                      //
//   File      : AW_helix.cxx                                           //
//   Purpose   : Wrapper for BI_helix + AW-specific functions           //
//                                                                      //
//                                                                      //
// Coded by Ralf Westram (coder@reallysoft.de) in December 2004         //
// Copyright Department of Microbiology (Technical University Munich)   //
//                                                                      //
// Visit our web site at: http://www.arb-home.de/                       //
//                                                                      //
// ==================================================================== //

#include "AW_helix.hxx"
#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awar.hxx>
#include <aw_device.hxx>
#include <arbdbt.h>
#include <cctype>
#include <awt_config_manager.hxx>

#define HELIX_AWAR_ENABLE          "Helix/enable"
#define HELIX_AWAR_SYMBOL_TEMPLATE "Helix/symbols/%s"
#define HELIX_AWAR_PAIR_TEMPLATE   "Helix/pairs/%s"

struct helix_pair_def {
    const char *awar;
    BI_PAIR_TYPE pair_type;
};

static helix_pair_def helix_awars[] = {
    { "Strong_Pair",      HELIX_STRONG_PAIR },
    { "Normal_Pair",      HELIX_PAIR },
    { "Weak_Pair",        HELIX_WEAK_PAIR },
    { "No_Pair",          HELIX_NO_PAIR },
    { "User_Pair",        HELIX_USER0 },
    { "User_Pair2",       HELIX_USER1 },
    { "User_Pair3",       HELIX_USER2 },
    { "User_Pair4",       HELIX_USER3 },
    { "Default",          HELIX_DEFAULT },
    { "Non_Standard_aA",  HELIX_NON_STANDARD0 },
    { "Non_Standard1",    HELIX_NON_STANDARD1 },
    { "Non_Standard2",    HELIX_NON_STANDARD2 },
    { "Non_Standard3",    HELIX_NON_STANDARD3 },
    { "Non_Standard4",    HELIX_NON_STANDARD4 },
    { "Non_Standard5",    HELIX_NON_STANDARD5 },
    { "Non_Standard6",    HELIX_NON_STANDARD6 },
    { "Non_Standard7",    HELIX_NON_STANDARD7 },
    { "Non_Standard8",    HELIX_NON_STANDARD8 },
    { "Non_Standard9",    HELIX_NON_STANDARD9 },
    { "Not_Non_Standard", HELIX_NO_MATCH },
    { 0,                  HELIX_NONE },
};

inline const char *helix_symbol_awar(int idx) { return GBS_global_string(HELIX_AWAR_SYMBOL_TEMPLATE, helix_awars[idx].awar); }
inline const char *helix_pair_awar  (int idx) { return GBS_global_string(HELIX_AWAR_PAIR_TEMPLATE,   helix_awars[idx].awar); }

AW_helix::AW_helix(AW_root * aw_root)
    : enabled(0)
{
    for (int j=0; helix_awars[j].awar; j++) {
        int i = helix_awars[j].pair_type;
        aw_root->awar_string(helix_pair_awar(j),   pairs[i])    ->add_target_var(&pairs[i]);
        aw_root->awar_string(helix_symbol_awar(j), char_bind[i])->add_target_var(&char_bind[i]);
    }
    aw_root->awar_int(HELIX_AWAR_ENABLE, 1)->add_target_var(&enabled);
}

char AW_helix::get_symbol(char left, char right, BI_PAIR_TYPE pair_type) {
    left  = toupper(left);
    right = toupper(right);

    int erg;
    if (pair_type < HELIX_NON_STANDARD0) {
        erg = *char_bind[HELIX_DEFAULT];
        for (int i = HELIX_STRONG_PAIR; i< HELIX_NON_STANDARD0; i++) {
            if (is_pairtype(left, right, (BI_PAIR_TYPE)i)) {
                erg = *char_bind[i];
                break;
            }
        }
    }
    else {
        erg = *char_bind[HELIX_NO_MATCH];
        if (is_pairtype(left, right, pair_type)) erg = *char_bind[pair_type];
    }
    if (!erg) erg = ' ';
    return erg;
}

char *AW_helix::seq_2_helix(char *sequence, char undefsymbol) {
    size_t size2 = strlen(sequence);
    bi_assert(size2<=size()); // if this fails there is a sequence longer than the alignment
    char *helix = ARB_calloc<char>(size()+1);
    size_t i, j;
    for (i=0; i<size2; i++) {
        BI_PAIR_TYPE pairType = pairtype(i);

        if (pairType == HELIX_NONE) {
            helix[i] = undefsymbol;
        }
        else {
            j        = opposite_position(i);
            char sym = get_symbol(sequence[i], sequence[j], pairType);
            helix[i] = sym == ' ' ? undefsymbol : sym;
        }
    }
    return helix;
}

static bool BI_show_helix_on_device(AW_device *device, int gc, const char *opt_string, size_t opt_string_size, size_t start, size_t size,
                                    AW_pos x, AW_pos y, AW_pos /*opt_ascent*/, AW_pos /*opt_descent*/, AW_CL cduser)
{
    AW_helix *helix = (AW_helix *)cduser;
    char *buffer = GB_give_buffer(size+1);
    register unsigned long i, j, k;

    for (k=0; k<size; k++) {
        i = k+start;

        BI_PAIR_TYPE pairType = helix->pairtype(i);
        if (pairType == HELIX_NONE) {
            buffer[k] = ' ';
        }
        else {
            j             = helix->opposite_position(i);
            char pairchar = j<opt_string_size ? opt_string[j] : '.';
            buffer[k]     = helix->get_symbol(opt_string[i], pairchar, pairType);
        }
    }
    buffer[size] = 0;
    return device->text(gc, buffer, x, y);
}

int AW_helix::show_helix(void *devicei, int gc1, const char *sequence, AW_pos x, AW_pos y, AW_bitset filter) {
    if (!has_entries()) return 0;
    AW_device *device = (AW_device *)devicei;
    return device->text_overlay(gc1, sequence, 0, AW::Position(x, y), 0.0,  filter, (AW_CL)this, 1.0, 1.0, BI_show_helix_on_device);
}

static void helix_pairs_changed_cb(AW_window *aww, int changed_idx, const WindowCallback *refreshCallback) {
    static bool recursion = false;

    if (!recursion) {
        AW_root *aw_root   = aww->get_root();
        AW_awar *awar_pair = aw_root->awar(helix_pair_awar(changed_idx));
        char    *pairdef   = awar_pair->read_string();

        {
            LocallyModify<bool> flag(recursion, true);
            for (int i = 0; ; i += 3) {
                char left  = toupper(pairdef[i]); if (!left) break;
                char right = toupper(pairdef[i+1]); if (!right) break;

                pairdef[i]   = left;
                pairdef[i+1] = right;

                for (int j = 0; helix_awars[j].awar; j++) {
                    if (j != changed_idx) {
                        AW_awar *awar_pair2 = aw_root->awar(helix_pair_awar(j));
                        char    *pd2        = awar_pair2->read_string();
                        int      dst        = 0;
                        bool     modified   = false;

                        for (int k = 0; ; k += 3) {
                            char l = toupper(pd2[k]); if (!l) break;
                            char r = toupper(pd2[k+1]); if (!r) break;

                            if ((left == l && right == r) || (left == r && right == l)) {
                                // remove duplicated pair
                                modified = true;
                            }
                            else {
                                pd2[dst]   = l;
                                pd2[dst+1] = r;

                                dst += 3;
                            }
                            if (!pd2[k+2]) break;
                        }

                        if (modified) {
                            pd2[dst-1] = 0;
                            awar_pair2->write_string(pd2);
                        }

                        free(pd2);
                    }
                }

                if (!pairdef[i+2]) break;
            }
            awar_pair->write_string(pairdef); // write back uppercase version
        }
        (*refreshCallback)(aww);

        free(pairdef);
    }
}

void setup_helix_config(AWT_config_definition& cdef) {
    cdef.add(HELIX_AWAR_ENABLE, "enable");
    
    for (int j=0; helix_awars[j].awar; j++) {
        int i = helix_awars[j].pair_type;

        const char *name = helix_awars[j].awar;
        if (i != HELIX_DEFAULT && i != HELIX_NO_MATCH) {
            cdef.add(helix_pair_awar(j), name);
        }
        cdef.add(helix_symbol_awar(j), GBS_global_string("%s_symbol", name));
    }
}

AW_window *create_helix_props_window(AW_root *awr, const WindowCallback *refreshCallback) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(awr, "HELIX_PROPS", "HELIX_PROPERTIES");

        aws->at(10, 10);
        aws->auto_space(3, 3);

        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback("helixsym.hlp"));
        aws->create_button("HELP", "HELP", "H");

        AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "helix", makeConfigSetupCallback(setup_helix_config));

        aws->at_newline();

        const size_t max_awar_len = 18;
        aws->label_length(max_awar_len);

        aws->label("Show helix?");
        aws->callback(*refreshCallback); // @@@ used as TOGGLE_CLICK_CB (see #559)
        aws->create_toggle(HELIX_AWAR_ENABLE);

        aws->at_newline();

        int ex = 0;
        for (int j = 0; helix_awars[j].awar; j++) {
            int  i = helix_awars[j].pair_type;

            aw_assert(strlen(helix_awars[j].awar) <= max_awar_len);

            if (i != HELIX_DEFAULT && i != HELIX_NO_MATCH) {
                aws->label(helix_awars[j].awar);
                aws->callback(makeWindowCallback(helix_pairs_changed_cb, j, refreshCallback)); // @@@ used as INPUTFIELD_CB (see #559)
                aws->create_input_field(helix_pair_awar(j), 20);

                if (j == 0) ex = aws->get_at_xposition();
            }
            else {
                aw_assert(j != 0);
                aws->create_autosize_button(0, helix_awars[j].awar);
                aws->at_x(ex);
            }

            aws->callback(*refreshCallback); // @@@ used as INPUTFIELD_CB (see #559)
            aws->create_input_field(helix_symbol_awar(j), 3);
            aws->at_newline();
        }
        aws->window_fit();
    }
    return aws;
}



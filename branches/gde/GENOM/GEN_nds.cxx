// =============================================================== //
//                                                                 //
//   File      : GEN_nds.cxx                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in 2001           //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "GEN.hxx"

#include <nds.h>
#include <item_sel_list.h>
#include <awt_sel_boxes.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <ad_cb.h>

#define gen_assert(bed) arb_assert(bed)

using namespace std;

// Note: this file is based on ../AWT/AWT_nds.cxx

#define GEN_NDS_COUNT       10
#define GEN_NDS_STRING_SIZE 4000

struct GEN_NodeTextBuilder {
    char  buf[GEN_NDS_STRING_SIZE];
    char  zbuf[GEN_NDS_COUNT];
    long  lengths[GEN_NDS_COUNT];
    long  rek[GEN_NDS_COUNT];
    char *dkeys[GEN_NDS_COUNT];
    char *parsing[GEN_NDS_COUNT];
    long  count;
    int   errorclip;
};

static GEN_NodeTextBuilder *gen_nds_ms = 0;

void GEN_make_node_text_init(GBDATA *gb_main) {
    GBDATA     *gbz, *gbe;
    const char *sf, *sl;
    int         count;

    sf = "flag1";
    sl = "len1";

    if (!gen_nds_ms) gen_nds_ms = (GEN_NodeTextBuilder *) GB_calloc(sizeof(GEN_NodeTextBuilder), 1);

    GBDATA *gb_arb_presets = GB_search(gb_main, "arb_presets", GB_CREATE_CONTAINER);
    count                  = 0;

    for (gbz = GB_entry(gb_arb_presets, "gene_viewkey"); gbz; gbz  = GB_nextEntry(gbz)) {
        // toggle set ?
        if (GB_read_int(GB_entry(gbz, sf))) {
            freeset(gen_nds_ms->dkeys[count], GB_read_string(GB_entry(gbz, "key_text")));
                if (GB_first_non_key_char(gen_nds_ms->dkeys[count])) {
                    gen_nds_ms->rek[count] = 1;
                }
                else {
                    gen_nds_ms->rek[count] = 0;
                }
                gen_nds_ms->lengths[count] = GB_read_int(GB_entry(gbz, sl));
                gbe = GB_entry(gbz, "pars");
                freenull(gen_nds_ms->parsing[count]);
                if (gbe && GB_read_string_count(gbe)>1) gen_nds_ms->parsing[count] = GB_read_string(gbe);
                count++;
            }
        }
    gen_nds_ms->errorclip = 0;
    gen_nds_ms->count = count;
}

char *GEN_make_node_text_nds(GBDATA *gb_main, GBDATA * gbd, int mode) {
    // if mode == 0 compress info else format info

    char *bp = gen_nds_ms->buf;
    gen_assert(gbd);

    long first = 0;
    for (long i = 0; i < gen_nds_ms->count; i++) {
        GBDATA *gbe;
        if (gen_nds_ms->rek[i]) { // hierarchical key
            gbe = GB_search(gbd, gen_nds_ms->dkeys[i], GB_FIND);
        }
        else { // flat entry
            gbe = GB_entry(gbd, gen_nds_ms->dkeys[i]);
        }

        if (!mode && !gbe) continue;
        if (!mode && first) {
            (*bp++) = ',';
        }
        (*bp++) = ' ';
        first++;
        if (gbe) {
            switch (GB_read_type(gbe)) {
                case GB_INT:
                    if (mode) {
                        char buf[20];
                        sprintf(buf, "%%%lii", gen_nds_ms->lengths[i]);
                        sprintf(bp, buf, GB_read_int(gbe));
                    }
                    else {
                        sprintf(bp, "%li", GB_read_int(gbe));
                    }
                    bp += strlen(bp);
                    break;
                case GB_BYTE:
                    if (mode) {
                        char buf[20];
                        sprintf(buf, "%%%lii", gen_nds_ms->lengths[i]);
                        sprintf(bp, buf, GB_read_byte(gbe));
                    }
                    else {
                        sprintf(bp, "%i", GB_read_byte(gbe));
                    }
                    bp += strlen(bp);
                    break;
                case GB_STRING: {
                    char       *pars = 0;
                    const char *p    = 0;

                    if (gen_nds_ms->parsing[i]) {
                        char *p2 = GB_read_string(gbe);
                        pars     = GB_command_interpreter(gb_main, p2, gen_nds_ms->parsing[i], gbd, 0);
                        free(p2);

                        if (!pars) {
                            pars = strdup("<error>");
                            if (!gen_nds_ms->errorclip++) {
                                aw_message(GB_await_error());
                            }
                        }
                        p = pars;
                    }
                    else {
                        p = GB_read_char_pntr(gbe);
                    }

                    long dlen = gen_nds_ms->lengths[i];
                    if (dlen + (bp - gen_nds_ms->buf) + 256 > GEN_NDS_STRING_SIZE) {
                        dlen = GEN_NDS_STRING_SIZE - 256 - (bp - gen_nds_ms->buf);
                    }

                    if (dlen> 0) {
                        int len = strlen(p);
                        long j = len;
                        if (j > dlen)   j = dlen;
                        for (; j; j--) *bp++ = *p++;
                        if (mode) {
                            long post = dlen - len;
                            while (post-- > 0) *(bp++) = ' ';
                        }
                    }
                    free(pars);
                    break;
                }
                case GB_FLOAT:
                    sprintf(bp, "%4.4f", GB_read_float(gbe));
                    bp += strlen(bp);
                    break;
                default:
                    break;
            }
        }
        else if (mode) {
            long j = gen_nds_ms->lengths[i];
            if (j + (bp - gen_nds_ms->buf) + 256 > GEN_NDS_STRING_SIZE) {
                j = GEN_NDS_STRING_SIZE - 256 - (bp - gen_nds_ms->buf);
            }
            for (; j > 0; j--)  *(bp++) = ' ';
        }
    }
    *bp = 0;

    return NDS_mask_nonprintable_chars(gen_nds_ms->buf);
}

void GEN_create_nds_vars(AW_root *aw_root, AW_default awdef, GBDATA *gb_main, const DatabaseCallback& NDS_changed_callback) {
    GB_ERROR  error          = GB_push_transaction(gb_main);
    GBDATA   *gb_arb_presets = GB_search(gb_main, "arb_presets", GB_CREATE_CONTAINER);
    GBDATA   *gb_viewkey     = 0;

    for (int i=0; i<GEN_NDS_COUNT && !error; i++) {
        char  buf[256];
        char *keystart = buf+sprintf(buf, "tmp/gene_viewkey_%i/", i);

        if (gb_viewkey) {
            gen_assert(GB_has_key(gb_viewkey, "gene_viewkey"));
            gb_viewkey = GB_nextEntry(gb_viewkey);
        }
        else {
            gb_viewkey = GB_entry(gb_arb_presets, "gene_viewkey");
        }
        if (!gb_viewkey) {
            gb_viewkey = GB_create_container(gb_arb_presets, "gene_viewkey");
        }

        if (!gb_viewkey) {
            error = GB_await_error();
        }
        else {
            GB_add_callback(gb_viewkey, GB_CB_CHANGED, NDS_changed_callback);

            const char *default_key = 0;
            switch (i) {
                case 1:  default_key = "pos_start"; break;
                case 2:  default_key = "pos_stop"; break;
                case 3:  default_key = "product"; break;
                default: default_key = "name"; break;
            }
            GBDATA *gb_key_text     = GB_searchOrCreate_string(gb_viewkey, "key_text", default_key);
            if (!gb_key_text) error = GB_await_error();
            else {
                strcpy(keystart, "key_text");
                aw_root->awar_string(buf, "", awdef);
                aw_root->awar(buf)->map(gb_key_text);
            }
        }

        if (!error) {
            GBDATA *gb_pars     = GB_searchOrCreate_string(gb_viewkey, "pars", "");
            if (!gb_pars) error = GB_await_error();
            else {
                strcpy(keystart, "pars");
                aw_root->awar_string(buf, "", awdef);
                aw_root->awar(buf)->map(gb_pars);
            }
        }

        if (!error) {
            GBDATA *gb_flag1     = GB_searchOrCreate_int(gb_viewkey, "flag1", i <= 2 ? 1 : 0);
            if (!gb_flag1) error = GB_await_error();
            else {
                strcpy(keystart, "flag1");
                aw_root->awar_int(buf, 0, awdef);
                aw_root->awar(buf)->map(gb_flag1);
            }
        }

        if (!error) {
            GBDATA *gb_len1     = GB_searchOrCreate_int(gb_viewkey, "len1", 30);
            if (!gb_len1) error = GB_await_error();
            else {
                strcpy(keystart, "len1");
                aw_root->awar_int(buf, 0, awdef);
                aw_root->awar(buf)->set_minmax(0, GEN_NDS_STRING_SIZE);
                aw_root->awar(buf)->map(gb_len1);
            }
        }
    }

    if (!error) {
        GBDATA *gb_next;
        gen_assert(GB_has_key(gb_viewkey, "gene_viewkey"));
        while (!error && (gb_next = GB_nextEntry(gb_viewkey))) {
            error = GB_delete(gb_next);
        }
    }

    aw_root->awar_string("tmp/gene_viewkey/key_text", "", awdef);

    error = GB_end_transaction(gb_main, error);
    if (error) aw_message(error);
}

AW_window *GEN_open_nds_window(AW_root *aw_root, GBDATA *gb_main) {
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;

        aws->init(aw_root, "GENE_NDS_PROPS", "Gene NDS");
        aws->load_xfig("awt/nds.fig");
        aws->auto_space(10, 5);

        aws->callback(AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("props_nds.hlp"));
        aws->create_button("HELP", "HELP", "H");

        aws->button_length(13);
        int dummy, closey;
        aws->at_newline();
        aws->get_at_position(&dummy, &closey);

        aws->create_button(0, "K");

        aws->at_newline();

        int showx, fieldx, columnx, srtx, srtux;

        aws->auto_space(10, 0);

        for (int i=0; i<GEN_NDS_COUNT; i++) {
            char buf[256];

            sprintf(buf, "tmp/gene_viewkey_%i/flag1", i);
            aws->get_at_position(&showx, &dummy);
            aws->create_toggle(buf);

            sprintf(buf, "tmp/gene_viewkey_%i/key_text", i);
            aws->get_at_position(&fieldx, &dummy);
            create_itemfield_selection_button(aws, FieldSelDef(buf, gb_main, GEN_get_selector(), FIELD_FILTER_NDS, "display-field"), NULL);

            sprintf(buf, "tmp/gene_viewkey_%i/len1", i);
            aws->get_at_position(&columnx, &dummy);
            aws->create_input_field(buf, 4);

            sprintf(buf, "tmp/gene_viewkey_%i/pars", i);
            const char *inputFieldAwarName = strdup(buf);

            aws->get_at_position(&srtx, &dummy);

            aws->button_length(0);

            aws->callback(makeWindowCallback(AWT_popup_select_srtaci_window, inputFieldAwarName));
            sprintf(buf, "SELECT_SRTACI_%i", i);
            aws->create_button(buf, "S", "S");

            aws->get_at_position(&srtux, &dummy);
            aws->create_input_field(inputFieldAwarName, 50);
            aws->at_newline();
        }
        aws->at(showx, closey);

        aws->at_x(showx);
        aws->create_button(0, "SHOW");

        aws->at_x(fieldx);
        aws->create_button(0, "FIELD");

        aws->at_x(columnx);
        aws->create_button(0, "WIDTH");

        aws->at_x(srtx);
        aws->create_button(0, "SRT");

        aws->at_x(srtux);
        aws->create_button(0, "ACI/SRT PROGRAM");
    }
    return aws;
}


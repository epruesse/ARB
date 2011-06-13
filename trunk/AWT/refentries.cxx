// ============================================================ //
//                                                              //
//   File      : refentries.cxx                                 //
//   Purpose   :                                                //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2011   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "refentries.h"

#include <aw_root.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arb_msg.h>
#include <arbdbt.h>

#include <cctype>

namespace RefEntries {

    static ARB_ERROR addRefsTo(DBItemSet& referred, const ad_item_selector& itemtype, GBDATA *gb_item, const char *refs_field, bool error_if_unknown_ref) {
        ARB_ERROR       error;
        GBDATA         *gb_main = GB_get_root(gb_item);
        GB_transaction  ta(gb_main);

        GBDATA *gb_refs = GB_entry(gb_item, refs_field);
        if (gb_refs) {
            const char *refs       = GB_read_char_pntr(gb_refs);
            if (!refs) error = GB_await_error();
            else {
                size_t   refCount = 0;
                char   **refNames = GBT_split_string(refs, ";, ", true, &refCount);

                for (size_t r = 0; r<refCount && !error; ++r) {
                    GBDATA *gb_referred = itemtype.find_item_by_id(gb_main, refNames[r]);
                    if (gb_referred) {
                        referred.insert(gb_referred);
                    }
                    else if (error_if_unknown_ref) {
                        char *item_id = itemtype.generate_item_id(gb_main, gb_item);

                        error = GBS_global_string("%s '%s' refers to unknown %s '%s' (in field '%s')",
                                                  itemtype.item_name, item_id,
                                                  itemtype.item_name, refNames[r],
                                                  refs_field);

                        free(item_id);
                    }
                }

                GBT_free_names(refNames);
            }
        }

        return error;
    }

    ARB_ERROR ReferringEntriesHandler::with_all_referred_items(AWT_QUERY_RANGE range, const char *refs_field, bool error_if_unknown_ref, referred_item_handler callback) {
        awt_assert(range != QUERY_CURRENT_ITEM); // would need AW_root

        ARB_ERROR  error;
        DBItemSet referred;

        {
            GB_transaction ta(gb_main);

            for (GBDATA *gb_container = itemtype.get_first_item_container(gb_main, NULL, QUERY_ALL_ITEMS);
                 gb_container && !error;
                 gb_container = itemtype.get_next_item_container(gb_container, QUERY_ALL_ITEMS))
            {
                for (GBDATA *gb_item = itemtype.get_first_item(gb_container, range);
                     gb_item && !error;
                     gb_item = itemtype.get_next_item(gb_item, range))
                {
                    error = addRefsTo(referred, itemtype, gb_item, refs_field, error_if_unknown_ref);
                }
            }
        }

        if (!error) error = callback(gb_main, referred);
        return error;
    }

    ARB_ERROR ReferringEntriesHandler::with_all_referred_items(GBDATA *gb_item, const char *refs_field, bool error_if_unknown_ref, referred_item_handler callback) {
        DBItemSet referred;
        ARB_ERROR  error  = addRefsTo(referred, itemtype, gb_item, refs_field, error_if_unknown_ref);
        if (!error) error = callback(gb_main, referred);
        return error;
    }



    // ------------------------
    //      GUI related below


#define AWAR_MARKBYREF                "awt/refs/"
#define AWAR_MARKBYREF_ALL            AWAR_MARKBYREF "which"
#define AWAR_MARKBYREF_FIELD          AWAR_MARKBYREF "field"
#define AWAR_MARKBYREF_IGNORE_UNKNOWN AWAR_MARKBYREF "ignore_unknown"

    static void perform_refentries(AW_window *aww, AW_CL cl_reh, AW_CL cl_ricb) {
        ReferringEntriesHandler *reh  = (ReferringEntriesHandler*)cl_reh;
        referred_item_handler    ricb = (referred_item_handler)cl_ricb;

        AW_root         *awr            = aww->get_root();
        AWT_QUERY_RANGE  range          = awr->awar(AWAR_MARKBYREF_ALL)->read_int() ? QUERY_ALL_ITEMS : QUERY_MARKED_ITEMS;
        char            *refs_field     = awr->awar(AWAR_MARKBYREF_FIELD)->read_string();
        bool             ignore_unknown = awr->awar(AWAR_MARKBYREF_IGNORE_UNKNOWN)->read_int();

        ARB_ERROR error = reh->with_all_referred_items(range, refs_field, !ignore_unknown, ricb);
        aw_message_if(error);

        free(refs_field);
    }

    void create_refentries_awars(AW_root *aw_root, AW_default aw_def) {
        aw_root->awar_int   (AWAR_MARKBYREF_ALL,            0,           aw_def);
        aw_root->awar_string(AWAR_MARKBYREF_FIELD,          "used_rels", aw_def);
        aw_root->awar_int   (AWAR_MARKBYREF_IGNORE_UNKNOWN, 0,           aw_def);
    }

    AW_window *create_refentries_window(AW_root *awr, ReferringEntriesHandler *reh, const char *window_id, const char *title, const char *help, client_area_builder build_client_area, const char *action, referred_item_handler action_cb) {
        AW_window_simple *aws = new AW_window_simple;

        aws->init(awr, window_id, title);
        aws->auto_space(10, 10);

        aws->callback((AW_CB0) AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(AW_POPUP_HELP, (AW_CL)help);
        aws->create_button("HELP", "HELP", "H");

        aws->at_newline();
        aws->label_length(27);

        const ad_item_selector& referring = reh->get_referring_item();
    
        char *items_name = strdup(referring.items_name);
        items_name[0]    = toupper(items_name[0]);

        aws->create_option_menu(AWAR_MARKBYREF_ALL, GBS_global_string("%s to examine", items_name));
        aws->insert_option("Marked", "M", 0);
        aws->insert_option("All",    "A", 1);
        aws->update_option_menu();

        aws->at_newline();

        aws->label("Field containing references");
        aws->create_input_field(AWAR_MARKBYREF_FIELD, 10);

        aws->at_newline();

        aws->label("Ignore unknown references?");
        aws->create_toggle(AWAR_MARKBYREF_IGNORE_UNKNOWN);

        if (build_client_area) build_client_area(aws);

        aws->at_newline();

        aws->callback(perform_refentries, (AW_CL)reh, (AW_CL)action_cb);
        aws->create_autosize_button("ACTION", action, "");

        aws->window_fit();

        free(items_name);

        return aws;
    }

};


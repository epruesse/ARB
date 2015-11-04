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

#include <arb_msg.h>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_awars.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>
#include <arb_strarray.h>

#include <cctype>

namespace RefEntries {

    static ARB_ERROR generate_item_error(const char *format, ItemSelector& itemtype, GBDATA *gb_item) {
        GBDATA *gb_main          = GB_get_root(gb_item);
        char   *item_id          = itemtype.generate_item_id(gb_main, gb_item);
        char   *item_description = GBS_global_string_copy("%s '%s'", itemtype.item_name, item_id);

        re_assert(strstr(format, "%s"));
        ARB_ERROR error = GBS_global_string(format, item_description);

        free(item_description);
        free(item_id);

        return error;
    }

    const char *RefSelector::get_refs(ItemSelector& itemtype, GBDATA *gb_item) const {
        const char *refs    = NULL;
        GBDATA     *gb_refs = GB_entry(gb_item, field);
        if (gb_refs) {
            refs = GB_read_char_pntr(gb_refs);
        }
        else if (error_if_field_missing) {
            GB_export_error(generate_item_error(GBS_global_string("%%s has no field '%s'", field), itemtype, gb_item).deliver());
        }
        return refs;
    }

    char *RefSelector::filter_refs(const char *refs, GBDATA *gb_item) const {
        return refs ? GB_command_interpreter(GB_get_root(gb_item), refs, aci, gb_item, NULL) : NULL;
    }

    static ARB_ERROR addRefsTo(DBItemSet& referred, ItemSelector& itemtype, GBDATA *gb_item, const RefSelector& ref) {
        ARB_ERROR   error;
        const char *refs     = ref.get_refs(itemtype, gb_item);
        char       *filtered = ref.filter_refs(refs, gb_item);

        if (!filtered) {
            if (GB_have_error()) error = GB_await_error();
        }
        else {
            ConstStrArray refNames;
            GBT_split_string(refNames, filtered, ";, ", true);
            size_t   refCount = refNames.size();

            for (size_t r = 0; r<refCount && !error; ++r) {
                GBDATA *gb_main     = GB_get_root(gb_item);
                GBDATA *gb_referred = itemtype.find_item_by_id(gb_main, refNames[r]);
                if (gb_referred) {
                    referred.insert(gb_referred);
                }
                else if (!ref.ignore_unknown_refs()) {
                    error = generate_item_error(GBS_global_string("%%s refers to unknown %s '%s'\n"
                                                                  "in content of field '%s'\n"
                                                                  "(content ='%s',\n"
                                                                  " filtered='%s')\n",
                                                                  itemtype.item_name, refNames[r],
                                                                  ref.get_field(),
                                                                  refs,
                                                                  filtered),
                                                itemtype, gb_item);
                }
            }

            free(filtered);
        }

        return error;
    }

    ARB_ERROR ReferringEntriesHandler::with_all_referred_items(QUERY_RANGE range, const RefSelector& refsel, referred_item_handler callback) {
        re_assert(range != QUERY_CURRENT_ITEM); // would need AW_root

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
                    error = addRefsTo(referred, itemtype, gb_item, refsel);
                }
            }
        }

        if (!error) error = callback(gb_main, referred);
        return error;
    }

    ARB_ERROR ReferringEntriesHandler::with_all_referred_items(GBDATA *gb_item, const RefSelector& refsel, referred_item_handler callback) {
        DBItemSet referred;
        ARB_ERROR  error  = addRefsTo(referred, itemtype, gb_item, refsel);
        if (!error) error = callback(gb_main, referred);
        return error;
    }



    // ------------------------
    //      GUI related below


#define AWAR_MARKBYREF                "awt/refs/"
#define AWAR_MARKBYREF_ALL            AWAR_MARKBYREF "which"
#define AWAR_MARKBYREF_FIELD          AWAR_MARKBYREF "field"
#define AWAR_MARKBYREF_IGNORE_MISSING AWAR_MARKBYREF "ignore_missing"
#define AWAR_MARKBYREF_IGNORE_UNKNOWN AWAR_MARKBYREF "ignore_unknown"
#define AWAR_MARKBYREF_FILTER         AWAR_MARKBYREF "filter"

#define AWAR_MARKBYREF_TEMP "tmp/awt/refs/"
#define AWAR_MARKBYREF_SELECTED AWAR_MARKBYREF_TEMP "selected"
#define AWAR_MARKBYREF_CONTENT  AWAR_MARKBYREF_TEMP "content"
#define AWAR_MARKBYREF_RESULT   AWAR_MARKBYREF_TEMP "result"

    static void perform_refentries(AW_window *aww, ReferringEntriesHandler *reh, referred_item_handler ricb) {
        AW_root     *aw_root = aww->get_root();
        QUERY_RANGE  range   = aw_root->awar(AWAR_MARKBYREF_ALL)->read_int() ? QUERY_ALL_ITEMS : QUERY_MARKED_ITEMS;

        RefSelector refsel(aw_root->awar(AWAR_MARKBYREF_FIELD)->read_char_pntr(),
                           aw_root->awar(AWAR_MARKBYREF_FILTER)->read_char_pntr(),
                           !aw_root->awar(AWAR_MARKBYREF_IGNORE_MISSING)->read_int(),
                           !aw_root->awar(AWAR_MARKBYREF_IGNORE_UNKNOWN)->read_int());

        ARB_ERROR error = reh->with_all_referred_items(range, refsel, ricb);
        aw_message_if(error);
    }

    static void refresh_result_cb(AW_root *aw_root, ReferringEntriesHandler *reh) {
        ItemSelector&   itemtype = reh->get_referring_item();
        GBDATA         *gb_main  = reh->get_gbmain();
        GB_transaction  ta(gb_main);

        GBDATA  *gb_item       = itemtype.get_selected_item(gb_main, aw_root);
        AW_awar *awar_selected = aw_root->awar(AWAR_MARKBYREF_SELECTED);

        if (!gb_item) {
            awar_selected->write_string(GBS_global_string("<no %s selected>", itemtype.item_name));
        }
        else {
            char *id = itemtype.generate_item_id(gb_main, gb_item);
            if (!id) {
                awar_selected->write_string(GB_await_error());
            }
            else {
                awar_selected->write_string(id);

                AW_awar *awar_content = aw_root->awar(AWAR_MARKBYREF_CONTENT);

                RefSelector refsel(aw_root->awar(AWAR_MARKBYREF_FIELD)->read_char_pntr(),
                                   aw_root->awar(AWAR_MARKBYREF_FILTER)->read_char_pntr(),
                                   true, true);

                const char *refs = refsel.get_refs(itemtype, gb_item);

                if (!refs) awar_content->write_string(GB_await_error());
                else {
                    awar_content->write_string(refs);

                    AW_awar *awar_result = aw_root->awar(AWAR_MARKBYREF_RESULT);
                    char    *result      = refsel.filter_refs(refs, gb_item);

                    if (result) {
                        awar_result->write_string(result);
                        free(result);
                    }
                    else {
                        awar_result->write_string(GB_await_error());
                    }
                }

                free(id);
            }
        }
    }

    static void bind_result_refresh_cbs(AW_root *aw_root, ReferringEntriesHandler *reh) {
        RootCallback refreshCb = makeRootCallback(refresh_result_cb, reh);

        aw_root->awar(AWAR_MARKBYREF_FIELD) ->add_callback(refreshCb);
        aw_root->awar(AWAR_MARKBYREF_FILTER)->add_callback(refreshCb);
        aw_root->awar(AWAR_SPECIES_NAME)    ->add_callback(refreshCb);    // @@@ hack
    }

    void create_refentries_awars(AW_root *aw_root, AW_default aw_def) {
        aw_root->awar_string(AWAR_MARKBYREF_FIELD, "used_rels",       aw_def);
        aw_root->awar_string(AWAR_MARKBYREF_FILTER,   "/[0-9.]+[%]*://", aw_def);
        
        aw_root->awar_string(AWAR_MARKBYREF_SELECTED, "a", aw_def);
        aw_root->awar_string(AWAR_MARKBYREF_CONTENT,  "b", aw_def);
        aw_root->awar_string(AWAR_MARKBYREF_RESULT,   "c", aw_def);

        aw_root->awar_int(AWAR_MARKBYREF_ALL,            0, aw_def);
        aw_root->awar_int(AWAR_MARKBYREF_IGNORE_MISSING, 0, aw_def);
        aw_root->awar_int(AWAR_MARKBYREF_IGNORE_UNKNOWN, 0, aw_def);
    }

    AW_window *create_refentries_window(AW_root *aw_root, ReferringEntriesHandler *reh, const char *window_id, const char *title, const char *help, client_area_builder build_client_area, const char *action, referred_item_handler action_cb) {
        AW_window_simple *aws = new AW_window_simple;

        aws->init(aw_root, window_id, title);
        aws->at(10, 10);
        aws->auto_space(10, 10);

        bind_result_refresh_cbs(aw_root, reh);

        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->callback(makeHelpCallback(help));
        aws->create_button("HELP", "HELP", "H");

        const int LABEL_LENGTH = 27;
        aws->label_length(LABEL_LENGTH);

        ItemSelector& itemtype = reh->get_referring_item();

        char *items_name = strdup(itemtype.items_name);
        items_name[0]    = toupper(items_name[0]);

        aws->at_newline();
        aws->label(GBS_global_string("%s to examine", items_name));
        aws->create_option_menu(AWAR_MARKBYREF_ALL, true);
        aws->insert_option("Marked", "M", 0);
        aws->insert_option("All",    "A", 1);
        aws->update_option_menu();

        aws->at_newline();
        aws->label("Field containing references");
        aws->create_input_field(AWAR_MARKBYREF_FIELD, 10);

        aws->at_newline();
        aws->label("Ignore if field missing?");
        aws->create_toggle(AWAR_MARKBYREF_IGNORE_MISSING);

        aws->at_newline();
        aws->label("Filter content by ACI");
        aws->create_input_field(AWAR_MARKBYREF_FILTER, 30);

        aws->label_length(10);
        aws->button_length(40);
        aws->at_newline(); aws->label("Selected"); aws->create_button(NULL, AWAR_MARKBYREF_SELECTED);
        aws->at_newline(); aws->label("Content");  aws->create_button(NULL, AWAR_MARKBYREF_CONTENT);
        aws->at_newline(); aws->label("Result");   aws->create_button(NULL, AWAR_MARKBYREF_RESULT);
        aws->label_length(LABEL_LENGTH);

        aws->at_newline();
        aws->label("Ignore unknown references?");
        aws->create_toggle(AWAR_MARKBYREF_IGNORE_UNKNOWN);

        if (build_client_area) build_client_area(aws);

        aws->at_newline();
        aws->callback(makeWindowCallback(perform_refentries, reh, action_cb));
        aws->create_autosize_button("ACTION", action, "");

        aws->window_fit();

        free(items_name);

        refresh_result_cb(aw_root, reh);

        return aws;
    }

};


// =============================================================== //
//                                                                 //
//   File      : adTest.cxx                                        //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "gb_storage.h"

const char *GB_get_type_name(GBDATA *gbd) {
    int         type = GB_TYPE(gbd);
    const char *type_name;

    switch (type) {
        case GB_INT:    { type_name = "GB_INT"; break; }
        case GB_FLOAT:  { type_name = "GB_FLOAT"; break; }
        case GB_BYTE:   { type_name = "GB_BYTE"; break; }
        case GB_STRING: { type_name = "GB_STRING"; break; }
        case GB_LINK:   { type_name = "GB_LINK"; break; }
        case GB_BITS:   { type_name = "GB_BITS"; break; }
        case GB_BYTES:  { type_name = "GB_BYTES"; break; }
        case GB_INTS:   { type_name = "GB_INTS"; break; }
        case GB_FLOATS: { type_name = "GB_FLOATS"; break; }
        case GB_DB:     { type_name = "GB_DB"; break; }
        default: {
            static char *unknownType = 0;
            freeset(unknownType, GBS_global_string_copy("<unknown GB_TYPE=%i>", type));
            type_name = unknownType;
            break;
        }
    }

    return type_name;
}

const char *GB_get_db_path(GBDATA *gbd) {
    GBDATA *gb_father = (GBDATA*)GB_FATHER(gbd);

    if (gb_father) {
        const char *father_path = GB_get_db_path(gb_father);
        char       *key         = GB_KEY(gbd);
        RETURN_LOCAL_ALLOC(GBS_global_string_copy("%s/%s", father_path, key ? key : "<gbmain>"));
    }
    return "";
}

void GB_dump_db_path(GBDATA *gbd) {
    printf("Path to GBDATA %p (type=%s) is '%s'\n", gbd, GB_get_type_name(gbd), GB_get_db_path(gbd));
}

static void GB_dump_internal(GBDATA *gbd, int *lines_allowed) {
    static int     indent            = 0;
    int            type              = GB_TYPE(gbd);
    const char    *type_name         = GB_get_type_name(gbd);
    const char    *key_name          = 0;
    const char    *content           = 0;
    unsigned long  content_len       = 0;
    GBCONTAINER   *father            = GB_FATHER(gbd);
    GBDATA        *gb_show_later     = 0;
    char          *whatto_show_later = 0;
    bool           showChildren      = true;

    if (father) {
        int             index_pos = (int)gbd->index; // my index position in father
        gb_header_list *hls       = &(GB_DATA_LIST_HEADER(father->d)[index_pos]);

        if (!hls) {
            key_name = GBS_global_string("<no gb_header_list found for index_pos=%i>", index_pos);
            father   = 0;                           // otherwise crash below
        }
        else {
            GBDATA *gb_self = GB_HEADER_LIST_GBD(*hls);
            if (gb_self != gbd) {
                key_name = GBS_global_string("<element not linked in parent>");
                if (gb_self) {
                    gb_show_later     = gb_self;
                    whatto_show_later = GBS_global_string_copy("Element linked at index pos of %p", gbd);
                }
                father = 0;                         // otherwise crash below
            }
            // otherwise father looks fine
        }
    }

    if (father) {
        bool is_db_server = GB_is_server(gbd);

        if (is_db_server && gbd->server_id != GBTUM_MAGIC_NUMBER) {
            key_name = GBS_global_string("<element with illegal server-id %p>", (void*)gbd->server_id);
        }
        else if (is_db_server && father->server_id != GBTUM_MAGIC_NUMBER) {
            key_name = GBS_global_string("<elements parent has illegal server-id %p>", (void*)father->server_id);
            father   = 0;                           // avoids crashes below
        }
        else {
            key_name = GB_KEY_QUARK(gbd) ? GB_KEY(gbd) : "<illegal quark=0>";
        }
    }

    if (!father && !key_name) {
        key_name     = "<unknown quark - element w/o father>";
        showChildren = false;
    }
    else {                                          // test if we need a transaction
        if (!GB_MAIN(gbd)->transaction) {
            GB_push_transaction(gbd);
            GB_dump_internal(gbd, lines_allowed);
            GB_pop_transaction(gbd);
            return;
        }
    }

    if (indent == 0) {
        printf("\nGB_dump of '%s':\n", father ? GB_get_db_path(gbd) : "<no DB-path - father missing or not inspected>");
        if (lines_allowed) (*lines_allowed)--;
    }

    if (father) {
        if (GB_ARRAY_FLAGS(gbd).changed == GB_DELETED) {
            content = "<can't examine - entry is deleted>";
        }
        else {
            switch (type) {
                case GB_INT:    { content = GBS_global_string("%li", GB_read_int(gbd)); break; }
                case GB_FLOAT:  { content = GBS_global_string("%f", (float)GB_read_float(gbd)); break; }
                case GB_BYTE:   { content = GBS_global_string("%i", GB_read_byte(gbd)); break; }
                case GB_STRING: { content = GB_read_char_pntr(gbd); content_len = GB_read_count(gbd); break; }
                case GB_LINK:   { content = GBS_global_string("link to %p", GB_follow_link(gbd)); break; }
                case GB_BITS:   { break; }
                case GB_BYTES:  { break; }
                case GB_INTS:   { break; }
                case GB_FLOATS: { break; }
                case GB_DB:     { content = "see below"; break; }
                default:        { content = ""; break; }
            }
        }
    }

    if (content==0) {
        if (GB_have_error()) {
            content = GBS_global_string("<failed to read content (error is '%s')>", GB_await_error());
        }
        else {
            content = "<illegal zero content, but no error - severe bug?!>";
        }
    }
    if (content_len == 0) content_len = strlen(content);

    {
        char     *prefix  = GBS_global_string_copy("%*s %-15s gbd=%p type=%s content=", indent, "", key_name, gbd, type_name);
        unsigned  wrappos = 500;
        char     *toFree  = 0;

        if (content_len > wrappos) {
            toFree      = strdup(content);
            content     = toFree;
            content_len = GBS_shorten_repeated_data(toFree);
        }

        if (content_len <= wrappos) {
            printf("%s'%s'\n", prefix, content);
            if (lines_allowed) (*lines_allowed)--;
        }
        else {
            char          *buffer  = (char*)malloc(wrappos+1);
            unsigned long  rest    = content_len;
            const char    *from    = content;
            int            cleared = 0;

            buffer[wrappos] = 0;
            while (rest) {
                memcpy(buffer, from, wrappos);
                rest  = rest>wrappos ? rest-wrappos : 0;
                from += wrappos;
                printf("%s'%s'\n", prefix, buffer);
                if (lines_allowed && --(*lines_allowed) <= 0) break;
                if (!cleared) { memset(prefix, ' ', strlen(prefix)); cleared = 1; }
            }
            free(buffer);
        }
        free(prefix);
        free(toFree);
    }

    if (type==GB_DB && showChildren) {
        GBCONTAINER *gbc = (GBCONTAINER*)gbd;
        GBDATA *gbp;

        if (gbd->flags2.folded_container) gb_unfold(gbc, -1, -1);
        for (gbp = GB_child(gbd); gbp; gbp = GB_nextChild(gbp)) {
            ++indent;
            GB_dump_internal(gbp, lines_allowed);
            --indent;
            if (lines_allowed && (*lines_allowed)<0) break;
        }
    }

    if (gb_show_later) {
        if (!lines_allowed || (*lines_allowed)>0) {
            printf("%*s Showing %s:\n", indent, "", whatto_show_later);
            freenull(whatto_show_later);
            ++indent;
            GB_dump_internal(gb_show_later, lines_allowed);
            --indent;
        }
    }
}

void GB_dump(GBDATA *gbd) {
    int max_lines = 2500;
    GB_dump_internal(gbd, &max_lines);
    if (max_lines <= 0) {
        printf("Warning: Dump has been aborted (too many lines)\n"
               "[use GB_dump_no_limit() if you really want to dump all]\n");
    }
}

void GB_dump_no_limit(GBDATA *gbd) {
    GB_dump_internal(gbd, 0);
}

// ---------------------
//      Fix database

static GB_ERROR gb_fix_recursive(GBDATA *gbd) {
    GBDATA *gbp;
    int     type = GB_TYPE(gbd);

    if (type == GB_DB) {
        for (gbp = GB_child(gbd); gbp; gbp = GB_nextChild(gbp)) {
            gb_fix_recursive(gbp);
        }
    }
    else {
        GBQUARK key_quark = GB_KEY_QUARK(gbd);
        if (key_quark == 0) {
            GB_MAIN_TYPE *Main         = GB_MAIN(gbd);
            const char   *new_key_try  = GBS_global_string("illegal_zero_key_%s", GB_get_type_name(gbd));
            char         *new_key_name = GBS_string_2_key(new_key_try);
            GBQUARK       keyq         = gb_key_2_quark(Main, new_key_name);

            printf("new_key_name='%s'\n", new_key_name);

            gb_assert(keyq != 0);
            {
                long gbm_index    = GB_QUARK_2_GBMINDEX(Main, keyq);
                GB_GBM_INDEX(gbd) = gbm_index;      // set new index

                // @@@ FIXME: above command has no effect

                printf("Fixed zero key_quark of GBDATA at %p\n", gbd);
                GB_dump_db_path(gbd);
            }

            free(new_key_name);
        }
    }

    return 0;
}

GB_ERROR GB_fix_database(GBDATA *gb_main) {
    GB_ERROR err  = GB_begin_transaction(gb_main);
    if (!err) err = gb_fix_recursive(gb_main);
    return GB_end_transaction(gb_main, err);
}


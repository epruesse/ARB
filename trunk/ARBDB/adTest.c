#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>

/*#include "arbdb.h"*/
#include "adlocal.h"
#include "admap.h"

#define VERBOSE 0

static const char *actType = "none";
static void *actData = NULL;
static int  actQuark = 0;

void err_hook() {
    int x=2;
    x=x;
}

#define err(gbd,quark,type,mess) \
    do { \
        fprintf(stderr, "(%s*)0x%p(=%s) %s (in (%s*)0x%p(=%s))\n",\
            type,gbd,Main->keys[quark].key,mess,\
            actType,actData,Main->keys[actQuark].key); \
        err=1; \
        err_hook();\
    } while (0)

#define errGBD(gbd,quark,mess) err(gbd,quark,"GBDATA",mess)
#define errGBC(gbc,quark,mess) err(gbc,quark,"GBCONTAINER",mess)

void testData(GB_MAIN_TYPE *Main, GBDATA *gbd, long server_id, int key_quark)
{
    int err = 0;

#if (VERBOSE>=2)
    printf("Teste (GBDATA*)0x%p (=%s)\n", gbd, Main->keys[key_quark].key);
#endif

    if (gbd->server_id != server_id) errGBD(gbd, key_quark, "illegal server id");
}

void testContainer(GB_MAIN_TYPE *Main,GBCONTAINER *gbc, long server_id, int key_quark)
{
    struct gb_header_list_struct *header;
    int     item,
        err=0;
    const char  *oldType   = actType;
    void    *oldData  = actData;
    int     oldQuark  = actQuark;

#if (VERBOSE>=1)
    printf("Teste (GBCONTAINER*)0x%p (=%s)\n", gbc, Main->keys[key_quark].key);
#endif
    actType  = "GBCONTAINER";
    actData  = gbc;
    actQuark = key_quark;

    if (gbc->server_id != server_id) {
        errGBC(gbc, actQuark, "illegal server id");
    }

    header = GB_DATA_LIST_HEADER(gbc->d);
    for (item=0; item<gbc->d.nheader; item++) {
        GBDATA *gbd = GB_HEADER_LIST_GBD(header[item]);
        GBCONTAINER *father;
        int     type, quark = header[item].flags.key_quark;

        if (!gbd) continue;

        if (quark>=Main->sizeofkeys) {
            fprintf(stderr, "Illegal quark %i\n", quark);
            continue;
        }


        if (!gbd) errGBC(gbc,key_quark,"NULL-GBDATA in header-list");

        type = GB_TYPE(gbd);

        if (type==GB_DB) testContainer(Main,(GBCONTAINER*)gbd, server_id, quark);
        else         testData(Main,gbd,server_id,quark);

        father = GB_FATHER(gbd);
        if (!father) {
            errGBD(gbd, quark, "has no father.");
        }
        else if (father!=gbc) {
            errGBD(gbd, quark,"is not son of..");
        }
        else if (gbd->index!=item) {
            errGBD(gbd, quark,"index mismatch..");
        }
        else if (err) {
            errGBD(gbd, quark,"is correct connected to..");
        }
    }

    actType = oldType;
    actData = oldData;
    actQuark = oldQuark;
}

void gb_testDB(GBDATA *gbd)
{
    GB_MAIN_TYPE    *Main = 0;
    GBCONTAINER     *gbc;
    long        server_id;
    int         err=0;

    actType = "GB_MAIN_TYPE";
    actData = gbd;
    actQuark = 0;

    gbc = GB_FATHER(gbd);
    Main = GBCONTAINER_MAIN(gbc);

    if (!gbc) {
        fprintf(stderr, "(GBDATA*)0x%p has no father\n", gbd);
        err_hook();
    }
    if (!Main) {
        fprintf(stderr, "(GBCONTAINER*)0x%p has no main-entry\n", gbc);
        err_hook();
    }

    gbc = Main->data;
    if (!gbc) {
        err(Main, actQuark, "GB_MAIN_TYPE","has no data");
    }

    server_id = gbc->server_id;

    if (GB_FATHER(gbc)!=Main->dummy_father) {
        errGBC(gbc, actQuark, "mainContainer's father != dummy_father");
    }
    if (Main->dummy_father->server_id != server_id) {
        errGBC(Main->dummy_father, actQuark, "illegal server id");
    }

    if (!err) testContainer(Main,gbc,server_id,0);

    printf("testDB passed.\n");
}

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
            static char *unknownType;
            if (unknownType) free(unknownType);
            unknownType = strdup(GBS_global_string("<unknown GB_TYPE=%i>", type));
            type_name   = unknownType;
            break;
        }
    }

    return type_name;
}

const char *GB_get_db_path(GBDATA *gbd) {
    GBDATA *gb_father = GB_get_father(gbd);

    if (gb_father) {
        static char *result;
        char        *father_path = strdup(GB_get_db_path(gb_father));
        int          key_quark   = GB_KEY_QUARK(gbd);

        if (result) {
            free(result);
            result = 0;
        }

        if (key_quark == 0) { // illegal
            result = strdup(GBS_global_string("%s/<illegal quark=0>", father_path));
        }
        else {
            const char *key = GB_KEY(gbd);
            result = strdup(GBS_global_string("%s/%s", father_path, key));
        }

        return result;
    }
    return "ROOT";
}

void GB_dump_db_path(GBDATA *gbd) {
    printf("Path to GBDATA %p (type=%s) is '%s'\n", gbd, GB_get_type_name(gbd), GB_get_db_path(gbd));
}

void GB_dump(GBDATA *gbd) {    
    static int   indent            = 0;
    int          type              = GB_TYPE(gbd);
    const char  *type_name         = GB_get_type_name(gbd);
    const char  *key_name          = 0;
    const char  *content           = 0;
    GBCONTAINER *father            = GB_FATHER(gbd);
    GBDATA      *gb_show_later     = 0;
    char        *whatto_show_later = 0;

    if (father) {
        int index_pos = (int)gbd->index; /* my index position in father */
        struct gb_header_list_struct *hls = &(GB_DATA_LIST_HEADER(father->d)[index_pos]);

        if (!hls) {
            key_name = GBS_global_string("<no gb_header_list_struct found for index_pos=%i>", index_pos);
            father = 0; // otherwise crash below
        }
        else {
            GBDATA *gb_self = GB_HEADER_LIST_GBD(*hls);
            if (gb_self != gbd) {
                key_name = GBS_global_string("<element not linked in parent>");
                if (gb_self) {
                    gb_show_later     = gb_self;
                    whatto_show_later = GBS_global_string_copy("Element linked at index pos of %p", gbd);
                }
                father = 0; // otherwise crash below
            }
            // otherwise father looks fine
        }
    }

    if (father) {
        GB_MAIN_TYPE *Main        = GBCONTAINER_MAIN(father);
        int          is_db_server = Main->local_mode ? 1 : 0; /* 0 = no (client); 1 = yes (server); */

        if (is_db_server == 1 && gbd->server_id != GBTUM_MAGIC_NUMBER) {
            key_name = GBS_global_string("<element with illegal server-id %p>", (void*)gbd->server_id);
        }
        else if (is_db_server == 1 && father->server_id != GBTUM_MAGIC_NUMBER) {
            key_name = GBS_global_string("<elements parent has illegal server-id %p>", (void*)father->server_id);
            father   = 0; /* avoids crashes below */
        }
        else {
            key_name = GB_KEY_QUARK(gbd) ? GB_KEY(gbd) : "<illegal quark=0>";
        }
    }

    if (!father && !key_name) {
        key_name = "<unknown quark - element w/o father>";
    }

    if (indent == 0) {
        printf("\nGB_dump of '%s':\n", father ? GB_get_db_path(gbd) : "<no DB-path - father missing or not inspected>");
    }

    if (father) {
        if (GB_ARRAY_FLAGS(gbd).changed == gb_deleted) {
            content = "<can't examine - entry is deleted>";
        }
        else {
            switch (type) {
                case GB_INT:    { content = GBS_global_string("%li", GB_read_int(gbd)); break; }
                case GB_FLOAT:  { content = GBS_global_string("%f", (float)GB_read_float(gbd)); break; }
                case GB_BYTE:   { content = GBS_global_string("%i", GB_read_byte(gbd)); break; }
                case GB_STRING: { content = GB_read_char_pntr(gbd); break; }
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

    if (content==0) content = "<unknown - not examined>";

    printf("%*s %-15s gbd=%p type=%s content='%s'\n", indent, "", key_name, gbd, type_name, content);

    if (type==GB_DB) {
        GBCONTAINER *gbc = (GBCONTAINER*)gbd;
        GBDATA *gbp;

        if (gbd->flags2.folded_container) gb_unfold(gbc, -1, -1);
        for (gbp = GB_find(gbd, 0, 0, down_level); gbp; gbp = GB_find(gbp, 0, 0, this_level|search_next)) {
            ++indent;
            GB_dump(gbp);
            --indent;
        }
    }

    if (gb_show_later) {
        printf("%*s Showing %s:\n", indent, "", whatto_show_later);
        free(whatto_show_later); whatto_show_later = 0;
        ++indent;
        GB_dump(gb_show_later);
        --indent;
    }
}

char *GB_ralfs_test(GBDATA *gb_main)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    GB_push_transaction(gb_main);
    GB_push_my_security(gb_main);
    system("date");
    gb_create_dictionaries(Main,10000000);
    system("date");
    GB_pop_my_security(gb_main);
    GB_pop_transaction(gb_main);

    return NULL;
}

char *GB_ralfs_menupoint(GBDATA *main_data)
{
    return GB_ralfs_test(main_data);
}

/* -------------------------------------------------------------------------------- */
/* Fix database                                                                     */
/* -------------------------------------------------------------------------------- */

static GB_ERROR gb_fix_recursive(GBDATA *gbd) {
    GBDATA *gbp;
    int     type = GB_TYPE(gbd);

    if (type == GB_DB) {
        for (gbp = GB_find(gbd, 0, 0, down_level); gbp; gbp = GB_find(gbp, 0, 0, this_level|search_next)) {
            gb_fix_recursive(gbp);
        }
    }
    else {
        int key_quark = GB_KEY_QUARK(gbd);
        if (key_quark == 0) {
            GB_MAIN_TYPE *Main         = GB_MAIN(gbd);
            const char   *new_key_try  = GBS_global_string("illegal_zero_key_%s", GB_get_type_name(gbd));
            char         *new_key_name = GBS_string_2_key(new_key_try);
            GBQUARK       keyq         = gb_key_2_quark(Main, new_key_name);

            printf("new_key_name='%s'\n", new_key_name);

            ad_assert(keyq != 0);
            {
                long gbm_index    = GB_QUARK_2_GBMINDEX(Main, keyq);
                GB_GBM_INDEX(gbd) = gbm_index; // set new index

                /* @@@ FIXME: above command has no effect  */

                printf("Fixed zero key_quark of GBDATA at %p\n", gbd);
                GB_dump_db_path(gbd);
            }

            free(new_key_name);
        }
    }

    return 0;
}

GB_ERROR GB_fix_database(GBDATA *gb_main) {
    GB_ERROR err = 0;
    GB_begin_transaction(gb_main);

    err = gb_fix_recursive(gb_main);

    if (err) GB_abort_transaction(gb_main);
    else GB_commit_transaction(gb_main);

    return err;
}



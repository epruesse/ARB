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
    int 	item,
        err=0;
    const char 	*oldType   = actType;
    void 	*oldData  = actData;
    int 	oldQuark  = actQuark;

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
        int 	type, quark = header[item].flags.key_quark;

        if (!gbd) continue;

        if (quark>=Main->sizeofkeys) {
            fprintf(stderr, "Illegal quark %i\n", quark);
            continue;
        }


        if (!gbd) errGBC(gbc,key_quark,"NULL-GBDATA in header-list");

        type = GB_TYPE(gbd);

        if (type==GB_DB) testContainer(Main,(GBCONTAINER*)gbd, server_id, quark);
        else 		 testData(Main,gbd,server_id,quark);

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
    GB_MAIN_TYPE 	*Main = 0;
    GBCONTAINER 	*gbc;
    long 		server_id;
    int 		err=0;

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

void GB_dump(GBDATA *gbd) {
    long type = GB_TYPE(gbd);
    const char *type_name = 0;
    static int indent;
/*     int i; */
    const char *content = 0;

    switch (type) {
        case GB_INT: {
            type_name = "GB_INT";
            content = GBS_global_string("%i", GB_read_int(gbd));
            break;
        }
        case GB_FLOAT: {
            type_name = "GB_FLOAT";
            content = GBS_global_string("%f", (float)GB_read_float(gbd));
            break;
        }
        case GB_BYTE: {
            type_name = "GB_BYTE";
            content = GBS_global_string("%i", GB_read_byte(gbd));
            break;
        }
        case GB_STRING: {
            type_name = "GB_STRING";
            content = GB_read_char_pntr(gbd);
            break;
        }
        case GB_LINK: {
            type_name = "GB_LINK";
            content = GBS_global_string("link to %p", GB_follow_link(gbd));
            break;
        }
        case GB_BITS: {
            type_name = "GB_BITS";
            break;
        }
        case GB_BYTES: {
            type_name = "GB_BYTES";
            break;
        }
        case GB_INTS: {
            type_name = "GB_INTS";
            break;
        }
        case GB_FLOATS: {
            type_name = "GB_FLOATS";
            break;
        }
        case GB_DB: {
            type_name = "GB_DB";
            content = "see below";
            break;
        }
        default: {
            type_name = "<unknown>";
            content = "";
            break;
        }
    }

    if (content==0) content = "<unknown>";
    /*for (i=0; i<indent; ++i) printf(" ");*/
    printf("%*s gbd=%p type=%s content='%s'\n", indent, "", gbd, type_name, content);

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
}

char *GB_ralfs_test(GBDATA *gb_main)
{
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    GB_push_transaction(gb_main);
    system("date");
    gb_create_dictionaries(Main,10000000);
    system("date");
    GB_pop_transaction(gb_main);

    return NULL;
}

#ifndef NDEBUG
char *GB_ralfs_menupoint(GBDATA *main_data)
{
    return GB_ralfs_test(main_data);
}
#endif





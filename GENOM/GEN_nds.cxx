/*********************************************************************************
 *  Coded by Ralf Westram (coder@reallysoft.de) in 2001                          *
 *  Institute of Microbiology (Technical University Munich)                      *
 *  http://www.mikro.biologie.tu-muenchen.de/                                    *
 *********************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>
#include <awt_nds.hxx>
#include <awt_changekey.hxx>

#include "GEN_interface.hxx"

using namespace std;

// Note: this file is based on ../AWT/AWT_nds.cxx

#define GEN_NDS_COUNT       10
#define GEN_NDS_STRING_SIZE 4000

struct make_node_text_struct {
    char  buf[GEN_NDS_STRING_SIZE];
    char  zbuf[GEN_NDS_COUNT];
    long  lengths[GEN_NDS_COUNT];
    long  rek[GEN_NDS_COUNT];
    char *dkeys[GEN_NDS_COUNT];
    char *parsing[GEN_NDS_COUNT];
    //  long  inherit[GEN_NDS_COUNT];
    long  count;
    int   errorclip;

} *gen_nds_ms = 0;

//  -----------------------------------------------------
//      void GEN_make_node_text_init(GBDATA *gb_main)
//  -----------------------------------------------------
void GEN_make_node_text_init(GBDATA *gb_main) {
    GBDATA     *gbz,*gbe;
    const char *sf, *sl;
    int         count;

    sf = "flag1";
    sl = "len1";

    if (!gen_nds_ms) gen_nds_ms = (struct make_node_text_struct *) GB_calloc(sizeof(struct make_node_text_struct),1);

    GBDATA *gb_arb_presets = GB_search(gb_main,"arb_presets",GB_CREATE_CONTAINER);
    count                  = 0;

    for (gbz = GB_find(gb_arb_presets, "gene_viewkey", NULL, down_level);
         gbz != NULL;
         gbz  = GB_find(gbz, "gene_viewkey", NULL, this_level + search_next))
    {
        /* toggle set ? */
        if (GB_read_int(GB_find(gbz, sf, NULL, down_level))) {
            if (gen_nds_ms->dkeys[count]) free(gen_nds_ms->dkeys[count]);
            gen_nds_ms->dkeys[count] = GB_read_string(GB_find(gbz, "key_text", NULL, down_level));
            if (GB_first_non_key_char(gen_nds_ms->dkeys[count])) {
                gen_nds_ms->rek[count] = 1;
            }
            else {
                gen_nds_ms->rek[count] = 0;
            }
            gen_nds_ms->lengths[count] = GB_read_int(GB_find(gbz, sl, NULL, down_level));
            //          gen_nds_ms->inherit[count] = GB_read_int(GB_find(gbz, "inherit", NULL, down_level));
            gbe = GB_find(gbz, "pars", NULL, down_level);
            if (gen_nds_ms->parsing[count]) {
                free(gen_nds_ms->parsing[count]);
                gen_nds_ms->parsing[count] = 0;
            }
            if (gbe && GB_read_string_count(gbe)>1 ) gen_nds_ms->parsing[count] = GB_read_string(gbe);
            count++;
        }
    }
    gen_nds_ms->errorclip = 0;
    gen_nds_ms->count = count;
}

//  -----------------------------------------------------------------------------
//      char *GEN_make_node_text_nds(GBDATA *gb_main, GBDATA * gbd, int mode)
//  -----------------------------------------------------------------------------
char *GEN_make_node_text_nds(GBDATA *gb_main, GBDATA * gbd, int mode)
{
    /* if mode ==0 compress info else format info */
    char     *bp, *p;
    GBDATA   *gbe;
    long      i, j;
    long      first;
    //     GBT_TREE *father;

    bp = gen_nds_ms->buf;
    gb_assert(gbd);
    //     if (!gbd) {
    //         static char hae[] = "??????";
    //         if (!species) return hae;
    //         sprintf(gen_nds_ms->buf,"<%s>",species->name);
    //         return gen_nds_ms->buf;
    //     }

    first = 0;
    for (i = 0; i < gen_nds_ms->count; i++) {
        if (gen_nds_ms->rek[i]) {       /* hierarchical key */
            gbe = GB_search(gbd,gen_nds_ms->dkeys[i],0);
        }else{              /* flat entry */
            gbe = GB_find(gbd, gen_nds_ms->dkeys[i], NULL, down_level);
        }

        //         if (!gbe && gen_nds_ms->inherit[i] && species ) {
        //             for (    father = species->father;
        //                     father && !gbe;
        //                     father = father->father){
        //                 if (father->gb_node){
        //                     gbe = GB_find(father->gb_node, gen_nds_ms->dkeys[i], NULL, down_level);
        //                 }
        //             }
        //         }

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
                        sprintf(buf,"%%%lii", gen_nds_ms->lengths[i]);
                        sprintf(bp, buf, GB_read_int(gbe));
                    } else {
                        sprintf(bp, "%li", GB_read_int(gbe));
                    }
                    bp += strlen(bp);
                    break;
                case GB_BYTE:
                    if (mode) {
                        char buf[20];
                        sprintf(buf,"%%%lii", gen_nds_ms->lengths[i]);
                        sprintf(bp, buf, GB_read_byte(gbe));
                    } else {
                        sprintf(bp, "%i", GB_read_byte(gbe));
                    }
                    bp += strlen(bp);
                    break;
                case GB_STRING:
                    {
                        long            post;
                        long        dlen;
                        char *pars = 0;
                        if (gen_nds_ms->parsing[i]) {
                            p = GB_read_string(gbe);
                            pars = GB_command_interpreter(gb_main,p, gen_nds_ms->parsing[i],gbd, 0);
                            free(p);
                            if (!pars){
                                pars = strdup("<error>");
                                if (!gen_nds_ms->errorclip++) {
                                    aw_message(GB_get_error());
                                }
                            }
                            p = pars;
                        }else{
                            p = GB_read_char_pntr(gbe);
                        }

                        dlen = gen_nds_ms->lengths[i];
                        if (dlen + (bp - gen_nds_ms->buf) +256 > GEN_NDS_STRING_SIZE) {
                            dlen = GEN_NDS_STRING_SIZE - 256 - (bp - gen_nds_ms->buf);
                        }

                        if (dlen> 0){
                            int len = strlen(p);
                            j = len;
                            if (j > dlen)   j = dlen;
                            for (; j; j--) *bp++ = *p++;
                            if (mode){
                                post = dlen - len;
                                while (post-- > 0) *(bp++) = ' ';
                            }
                        }
                        if (pars) free(pars);
                    }
                    break;
                case GB_FLOAT:
                    sprintf(bp, "%4.4f", GB_read_float(gbe));
                    bp += strlen(bp);
                    break;
                default:
                    break;
            }
        } else if (mode) {
            j = gen_nds_ms->lengths[i];
            if (j + (bp - gen_nds_ms->buf) + 256 > GEN_NDS_STRING_SIZE) {
                j = GEN_NDS_STRING_SIZE - 256 - (bp - gen_nds_ms->buf);
            }
            for (; j > 0; j--)  *(bp++) = ' ';
        }
    }           /* for */
    *bp = 0;
    return gen_nds_ms->buf;
}



//  ---------------------------------------------------------------------------------------------------------------
//      void GEN_create_nds_vars(AW_root *aw_root,AW_default awdef,GBDATA *gb_main, GB_CB NDS_changed_callback)
//  ---------------------------------------------------------------------------------------------------------------
void GEN_create_nds_vars(AW_root *aw_root,AW_default awdef,GBDATA *gb_main, GB_CB NDS_changed_callback)
{
    GBDATA *gb_arb_presets;
    GBDATA *gb_viewkey = 0;
    //  GBDATA *gb_inherit;
    GBDATA *gb_flag1;
    GBDATA *gb_len1;
    GBDATA *gb_pars;
    GBDATA *gb_key_text;

    GB_push_transaction(gb_main);
    gb_arb_presets = GB_search(gb_main,"arb_presets",GB_CREATE_CONTAINER);
    int i;
    for (   i=0;i<GEN_NDS_COUNT; i++) {
        char buf[256];
        memset(buf,0,256);

        if (gb_viewkey) {
            gb_viewkey = GB_find(gb_viewkey,"gene_viewkey",0,this_level|search_next);
        }else{
            gb_viewkey = GB_find(gb_arb_presets,"gene_viewkey",0,down_level);
        }
        if (!gb_viewkey){
            gb_viewkey = GB_create_container(gb_arb_presets,"gene_viewkey");
        }

        gb_assert(gb_viewkey);
        GB_add_callback(gb_viewkey, GB_CB_CHANGED, NDS_changed_callback, 0);

        gb_key_text = GB_find( gb_viewkey, "key_text",0,down_level);
        if (!gb_key_text) {
            gb_key_text = GB_create(gb_viewkey,"key_text",GB_STRING);
            switch(i){
                case 1:
                    GB_write_string(gb_key_text,"pos_begin");
                    break;
                case 2:
                    GB_write_string(gb_key_text,"pos_end");
                    break;
                case 3:
                    GB_write_string(gb_key_text,"product");
                    break;
                default:
                    GB_write_string(gb_key_text,"name");
                    break;
            }
        }
        sprintf(buf,"tmp/gene_viewkey_%i/key_text",i);
        aw_root->awar_string(buf,"",awdef);
        aw_root->awar(buf)->map((void *)gb_key_text);

        gb_pars = GB_find( gb_viewkey, "pars",0,down_level);
        if (!gb_pars) {
            gb_pars = GB_create(gb_viewkey,"pars",GB_STRING);
            GB_write_string(gb_pars,"");
        }
        sprintf(buf,"tmp/gene_viewkey_%i/pars",i);
        aw_root->awar_string(buf,"",awdef);
        aw_root->awar(buf)->map((void *)gb_pars);

        gb_flag1 = GB_find( gb_viewkey, "flag1",0,down_level);
        if (!gb_flag1) {
            gb_flag1 = GB_create(gb_viewkey,"flag1",GB_INT);
            if (i<=2){
                GB_write_int(gb_flag1,1);
            }else{
                GB_write_int(gb_flag1,0);
            }
        }
        sprintf(buf,"tmp/gene_viewkey_%i/flag1",i);
        aw_root->awar_int(buf,0,awdef);
        aw_root->awar(buf)->map((void *)gb_flag1);

        gb_len1 = GB_find( gb_viewkey, "len1",0,down_level);
        if (!gb_len1) {
            gb_len1 = GB_create(gb_viewkey,"len1",GB_INT);
            GB_write_int(gb_len1,30);
        }
        sprintf(buf,"tmp/gene_viewkey_%i/len1",i);
        aw_root->awar_int(buf,0,awdef);
        aw_root->awar(buf)->set_minmax(0, GEN_NDS_STRING_SIZE);
        aw_root->awar(buf)->map((void *)gb_len1);

        //      gb_inherit = GB_find( gb_viewkey, "inherit",0,down_level);
        //      if (!gb_inherit) {
        //          gb_inherit = GB_create(gb_viewkey,"inherit",GB_INT);
        //          GB_write_int(gb_inherit,0);
        //      }
        //      sprintf(buf,"tmp/gene_viewkey_%i/inherit",i);
        //      aw_root->awar_int(buf,0,awdef);
        //      aw_root->awar(buf)->map((void *)gb_inherit);
    }
    GBDATA *gb_next;
    while ( (gb_next = GB_find(gb_viewkey,"gene_viewkey",0,this_level|search_next)) ) {
        GB_ERROR error = GB_delete(gb_next);
        if (error) {
            aw_message(error);
            break;
        }
    }
    aw_root->awar_string("tmp/gene_viewkey/key_text","",awdef);
    GB_pop_transaction(gb_main);
}
//  ---------------------------------------------------------------------------------------
//      void GEN_create_select_nds_window(AW_window *aww,char *key_text,AW_CL cgb_main)
//  ---------------------------------------------------------------------------------------
void GEN_create_select_nds_window(AW_window *aww,char *key_text,AW_CL cgb_main)
{
    static AW_window *win = 0;
    AW_root *aw_root = aww->get_root();
    aw_root->awar("tmp/gene_viewkey/key_text")->map(key_text);
    if (!win) {
        AW_window_simple *aws = new AW_window_simple;
        aws->init( aw_root, "NDS", "NDS_SELECT");
        aws->load_xfig("awt/nds_sel.fig");
        aws->button_length(13);

        aws->callback( AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE","C");

        awt_create_selection_list_on_scandb((GBDATA *)cgb_main,
                                            (AW_window*)aws,"tmp/gene_viewkey/key_text",
                                            AWT_NDS_FILTER,
                                            "scandb","rescandb", &GEN_item_selector, 20, 10);
        //aw_root->awar(key_text)->add_callback((AW_RCB1)awt_pop_down_select_nds,(AW_CL)aws);

        win =  (AW_window*)aws;
    }
    win->show();
}
//  -----------------------------------------------------------------------
//      AW_window *GEN_open_nds_window(AW_root *aw_root,AW_CL cgb_main)
//  -----------------------------------------------------------------------
AW_window *GEN_open_nds_window(AW_root *aw_root,AW_CL cgb_main)
{
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
    
        aws->init( aw_root, "GENE_NDS_PROPS", "Gene NDS");
        aws->load_xfig("awt/nds.fig");
        aws->auto_space(10,5);

        aws->callback( AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE","C");

        aws->at("help");
        aws->callback(AW_POPUP_HELP,(AW_CL)"props_nds.hlp");
        aws->create_button("HELP", "HELP","H");

        aws->button_length(13);
        int dummy,closey;
        aws->at_newline();
        aws->get_at_position( &dummy,&closey );

        aws->create_button(0,"K");

        aws->at_newline();


        int showx,fieldselectx,fieldx, /*inheritx,*/ columnx,srtx,srtux;

        aws->auto_space(10,0);

        int i;
        for (   i=0;i<GEN_NDS_COUNT; i++) {
            char buf[256];

            sprintf(buf,"tmp/gene_viewkey_%i/flag1",i);
            aws->get_at_position( &showx,&dummy );
            aws->create_toggle(buf);

            aws->button_length(20);
            sprintf(buf,"tmp/gene_viewkey_%i/key_text",i);
            aws->get_at_position( &fieldx,&dummy );
            aws->create_input_field(buf,15);

            aws->button_length(0);
            aws->callback((AW_CB)GEN_create_select_nds_window, (AW_CL)strdup(buf),cgb_main);
            aws->get_at_position( &fieldselectx,&dummy );
            aws->create_button("SELECT_NDS","S");

            //      sprintf(buf,"tmp/gene_viewkey_%i/inherit",i);
            //      aws->get_at_position( &inheritx,&dummy );
            //      aws->create_toggle(buf);

            sprintf(buf,"tmp/gene_viewkey_%i/len1",i);
            aws->get_at_position( &columnx,&dummy );
            aws->create_input_field(buf,4);

            sprintf(buf,"tmp/gene_viewkey_%i/pars",i);
            aws->get_at_position( &srtx,&dummy );

            aws->button_length(0);

            aws->callback(AWT_create_select_srtaci_window,(AW_CL)strdup(buf),0);
            aws->create_button("SELECT_SRTACI", "S","S");

            aws->get_at_position( &srtux,&dummy );
            aws->create_input_field(buf,40);
            aws->at_newline();
        }
        aws->at(showx,closey);

        aws->at_x(fieldselectx);
        aws->create_button(0,"SEL");

        aws->at_x(showx);
        aws->create_button(0,"SHOW");

        aws->at_x(fieldx);
        aws->create_button(0,"FIELD");

        //  aws->at_x(inheritx);
        //  aws->create_button(0,"INH.");

        aws->at_x(columnx);
        aws->create_button(0,"WIDTH");

        aws->at_x(srtx);
        aws->create_button(0,"SRT");

        aws->at_x(srtux);
        aws->create_button(0,"ACI/SRT PROGRAM");
    }
    return aws;
}




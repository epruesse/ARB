#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
// #include <malloc.h>
#include <string.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt.hxx>
#include "awt_nds.hxx"
#include "awt_config_manager.hxx"

#define NDS_COUNT 10
#define NDS_STRING_SIZE 4000

struct make_node_text_struct {
    char  buf[NDS_STRING_SIZE];
    char  zbuf[NDS_COUNT];
    long  lengths[NDS_COUNT];
    long  rek[NDS_COUNT];
    char *dkeys[NDS_COUNT];
    char *parsing[NDS_COUNT];
    long  inherit[NDS_COUNT];
    long  count;
    int   errorclip;

} *awt_nds_ms = 0;

void create_nds_vars(AW_root *aw_root,AW_default awdef,GBDATA *gb_main)
{
    GBDATA *gb_arb_presets;
    GBDATA *gb_viewkey = 0;
    GBDATA *gb_inherit;
    GBDATA *gb_flag1;
    GBDATA *gb_len1;
    GBDATA *gb_pars;
    GBDATA *gb_key_text;

    GB_push_transaction(gb_main);
    gb_arb_presets = GB_search(gb_main,"arb_presets",GB_CREATE_CONTAINER);
    int i;
    for (   i=0;i<NDS_COUNT; i++) {
        char buf[256];
        memset(buf,0,256);

        if (gb_viewkey) {
            gb_viewkey = GB_find(gb_viewkey,"viewkey",0,this_level|search_next);
        }else{
            gb_viewkey = GB_find(gb_arb_presets,"viewkey",0,down_level);
        }
        if (!gb_viewkey){
            gb_viewkey = GB_create_container(gb_arb_presets,"viewkey");
        }

        gb_key_text = GB_find( gb_viewkey, "key_text",0,down_level);
        if (!gb_key_text) {
            gb_key_text = GB_create(gb_viewkey,"key_text",GB_STRING);
            switch(i){
                case 1:
                    GB_write_string(gb_key_text,"full_name");
                    break;
                case 2:
                    GB_write_string(gb_key_text,"group_name");
                    break;
                case 3:
                    GB_write_string(gb_key_text,"acc");
                    break;
                case 4:
                    GB_write_string(gb_key_text,"date");
                    break;
                default:
                    GB_write_string(gb_key_text,"name");
                    break;
            }
        }
        sprintf(buf,"tmp/viewkey_%i/key_text",i);
        aw_root->awar_string(buf,"",awdef);
        aw_root->awar(buf)->map((void *)gb_key_text);

        gb_pars = GB_find( gb_viewkey, "pars",0,down_level);
        if (!gb_pars) {
            gb_pars = GB_create(gb_viewkey,"pars",GB_STRING);
            GB_write_string(gb_pars,"");
        }
        sprintf(buf,"tmp/viewkey_%i/pars",i);
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
        sprintf(buf,"tmp/viewkey_%i/flag1",i);
        aw_root->awar_int(buf,0,awdef);
        aw_root->awar(buf)->map((void *)gb_flag1);

        gb_len1 = GB_find( gb_viewkey, "len1",0,down_level);
        if (!gb_len1) {
            gb_len1 = GB_create(gb_viewkey,"len1",GB_INT);
            GB_write_int(gb_len1,30);
        }
        sprintf(buf,"tmp/viewkey_%i/len1",i);
        aw_root->awar_int(buf,0,awdef);
        aw_root->awar(buf)->set_minmax(0,NDS_STRING_SIZE);
        aw_root->awar(buf)->map((void *)gb_len1);

        gb_inherit = GB_find( gb_viewkey, "inherit",0,down_level);
        if (!gb_inherit) {
            gb_inherit = GB_create(gb_viewkey,"inherit",GB_INT);
            GB_write_int(gb_inherit,0);
        }
        sprintf(buf,"tmp/viewkey_%i/inherit",i);
        aw_root->awar_int(buf,0,awdef);
        aw_root->awar(buf)->map((void *)gb_inherit);
    }
    GBDATA *gb_next;
    while ( (gb_next = GB_find(gb_viewkey,"viewkey",0,this_level|search_next)) ) {
        GB_ERROR error = GB_delete(gb_next);
        if (error) {
            aw_message(error);
            break;
        }
    }
    aw_root->awar_string("tmp/viewkey/key_text","",awdef);
    GB_pop_transaction(gb_main);
}
void awt_pop_down_select_nds(AW_root *,AW_window *aww){
    aww->hide();
}

void AWT_create_select_nds_window(AW_window *aww,char *key_text,AW_CL cgb_main)
{
    static AW_window *win = 0;
    AW_root *aw_root = aww->get_root();
    aw_root->awar("tmp/viewkey/key_text")->map(key_text);
    if (!win) {
        AW_window_simple *aws = new AW_window_simple;
        aws->init( aw_root, "NDS", "NDS_SELECT");
        aws->load_xfig("awt/nds_sel.fig");
        aws->button_length(13);

        aws->callback( AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE","C");

        awt_create_selection_list_on_scandb((GBDATA *)cgb_main,
                                            (AW_window*)aws,"tmp/viewkey/key_text",
                                            AWT_NDS_FILTER,
                                            "scandb","rescandb", &AWT_species_selector, 20, 10);
        //aw_root->awar(key_text)->add_callback((AW_RCB1)awt_pop_down_select_nds,(AW_CL)aws);

        win =  (AW_window*)aws;
    }
    win->show();
}
void awt_pre_to_view(AW_root *aw_root){
    char *str = aw_root->awar(AWAR_SELECT_ACISRT_PRE)->read_string();
    char *brk = strchr(str,'#');
    if (brk) {
        *(brk++) = 0;
        aw_root->awar(AWAR_SELECT_ACISRT)->write_string(brk);
    }else{
        aw_root->awar(AWAR_SELECT_ACISRT)->write_string(str);
    }
    free(str);
}
void AWT_create_select_srtaci_window(AW_window *aww,AW_CL awar_acisrt,AW_CL awar_short)
{
    AWUSE(awar_short);
    static AW_window *win = 0;
    AW_root *aw_root = aww->get_root();
    aw_root->awar_string(AWAR_SELECT_ACISRT);
    aw_root->awar(AWAR_SELECT_ACISRT)->map((char *)awar_acisrt);

    if (!win) {
        aw_root->awar_string(AWAR_SELECT_ACISRT_PRE);
        AW_window_simple *aws = new AW_window_simple;
        aws->init( aw_root, "SRT_ACI_SELECT", "SRT_ACI_SELECT");
        aws->load_xfig("awt/srt_select.fig");
        aws->button_length(13);

        aws->callback( AW_POPDOWN);
        aws->at("close");
        aws->create_button("CLOSE", "CLOSE","C");

        aws->callback( AW_POPUP_HELP,(AW_CL)"acisrt.hlp");
        aws->at("help");
        aws->create_button("HELP", "HELP","H");

        aws->at("box");
        AW_selection_list*  id = aws->create_selection_list(AWAR_SELECT_ACISRT_PRE);
        char *filename = AWT_unfold_path("lib/sellists/srt_aci*.sellst","ARBHOME");
        GB_ERROR error = aws->load_selection_list(id,filename);
        free(filename);
        if (error) aw_message(error);

        aws->at("field");
        aws->create_text_field(AWAR_SELECT_ACISRT);

        aw_root->awar(AWAR_SELECT_ACISRT_PRE)->add_callback(awt_pre_to_view);
        awt_pre_to_view(aw_root);
        win =  (AW_window*)aws;
    }
    win->show();
}

static void nds_init_config(AW_window *aww) {
    AWT_reset_configDefinition(aww->get_root());
    for (int i = 0; i<NDS_COUNT; ++i) {
        char buf[256];
        sprintf(buf,"tmp/viewkey_%i/flag1",i); AWT_add_configDefinition(buf, "active", i);
        sprintf(buf,"tmp/viewkey_%i/key_text",i); AWT_add_configDefinition(buf, "key_text", i);
        sprintf(buf,"tmp/viewkey_%i/inherit",i); AWT_add_configDefinition(buf, "inherit", i);
        sprintf(buf,"tmp/viewkey_%i/len1",i); AWT_add_configDefinition(buf, "len1", i);
        sprintf(buf,"tmp/viewkey_%i/pars",i); AWT_add_configDefinition(buf, "pars", i);
    }
}

static char *nds_store_config(AW_window *aww, AW_CL, AW_CL) {
    nds_init_config(aww);
    return AWT_store_configDefinition();
}
static void nds_restore_config(AW_window *aww, const char *stored, AW_CL, AW_CL) {
    nds_init_config(aww);
    AWT_restore_configDefinition(stored);
}


AW_window *AWT_open_nds_window(AW_root *aw_root,AW_CL cgb_main)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "NDS_PROPS", "NDS");
    aws->load_xfig("awt/nds.fig");
    aws->auto_space(10,5);

    aws->callback( AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"props_nds.hlp");
    aws->create_button("HELP", "HELP","H");

    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "nds", nds_store_config, nds_restore_config, 0, 0);

    aws->button_length(13);
    int dummy,closey;
    aws->at_newline();
    aws->get_at_position( &dummy,&closey );

    aws->create_button(0,"K");

    aws->at_newline();


    int showx,fieldselectx,fieldx, inheritx,columnx,srtx,srtux;

    aws->auto_space(10,0);

    int i;
    for (   i=0;i<NDS_COUNT; i++) {
        char buf[256];

        sprintf(buf,"tmp/viewkey_%i/flag1",i);
        aws->get_at_position( &showx,&dummy );
        aws->create_toggle(buf);

        aws->button_length(20);
        sprintf(buf,"tmp/viewkey_%i/key_text",i);
        aws->get_at_position( &fieldx,&dummy );
        aws->create_input_field(buf,15);

        aws->button_length(0);
        aws->callback((AW_CB)AWT_create_select_nds_window, (AW_CL)strdup(buf),cgb_main);
        aws->get_at_position( &fieldselectx,&dummy );
        aws->create_button("SELECT_NDS","S");

        sprintf(buf,"tmp/viewkey_%i/inherit",i);
        aws->get_at_position( &inheritx,&dummy );
        aws->create_toggle(buf);

        sprintf(buf,"tmp/viewkey_%i/len1",i);
        aws->get_at_position( &columnx,&dummy );
        aws->create_input_field(buf,4);

        sprintf(buf,"tmp/viewkey_%i/pars",i);
        aws->get_at_position( &srtx,&dummy );

        aws->button_length(0);

        aws->callback(AWT_create_select_srtaci_window,(AW_CL)strdup(buf),0);
        aws->create_button("SELECT_SRTACI", "S","S");

        aws->get_at_position( &srtux,&dummy );
        aws->at_set_to(AW_TRUE, AW_FALSE, -7, 30);
        aws->create_input_field(buf,40);
        aws->at_unset_to();

        aws->at_newline();
    }
    aws->at(showx,closey);

    aws->at_x(fieldselectx);
    aws->create_button(0,"SEL");

    aws->at_x(showx);
    aws->create_button(0,"SHOW");

    aws->at_x(fieldx);
    aws->create_button(0,"FIELD");

    aws->at_x(inheritx);
    aws->create_button(0,"INH.");

    aws->at_x(columnx);
    aws->create_button(0,"WIDTH");

    aws->at_x(srtx);
    aws->create_button(0,"SRT");

    aws->at_x(srtux);
    aws->create_button(0,"ACI/SRT PROGRAM");


    return (AW_window *)aws;
}



void make_node_text_init(GBDATA *gb_main){
    GBDATA     *gbz,*gbe;
    const char *sf, *sl;
    int         count;

    sf = "flag1";
    sl = "len1";

    if (!awt_nds_ms) awt_nds_ms = (struct make_node_text_struct *) GB_calloc(sizeof(struct make_node_text_struct),1);

    GBDATA *gb_arb_presets = GB_search(gb_main,"arb_presets",GB_CREATE_CONTAINER);
    count                  = 0;

    for (gbz = GB_find(gb_arb_presets, "viewkey", NULL, down_level);
         gbz != NULL;
         gbz  = GB_find(gbz, "viewkey", NULL, this_level + search_next))
    {
        /* toggle set ? */
        if (GB_read_int(GB_find(gbz, sf, NULL, down_level))) {
            if (awt_nds_ms->dkeys[count]) free(awt_nds_ms->dkeys[count]);
            awt_nds_ms->dkeys[count] = GB_read_string(GB_find(gbz, "key_text", NULL, down_level));
            if (GB_first_non_key_char(awt_nds_ms->dkeys[count])) {
                awt_nds_ms->rek[count] = 1;
            }
            else {
                awt_nds_ms->rek[count] = 0;
            }
            awt_nds_ms->lengths[count] = GB_read_int(GB_find(gbz, sl, NULL, down_level));
            awt_nds_ms->inherit[count] = GB_read_int(GB_find(gbz, "inherit", NULL, down_level));
            gbe = GB_find(gbz, "pars", NULL, down_level);
            if (awt_nds_ms->parsing[count]) {
                free(awt_nds_ms->parsing[count]);
                awt_nds_ms->parsing[count] = 0;
            }
            if (gbe && GB_read_string_count(gbe)>1 ) awt_nds_ms->parsing[count] = GB_read_string(gbe);
            count++;
        }
    }
    awt_nds_ms->errorclip = 0;
    awt_nds_ms->count = count;
}

enum { MNTN_COMPRESSED = 0, MNTN_SPACED = 1, MNTN_TABBED = 2 };

#if defined(DEBUG)
// #define QUOTE_NDS_STRING
#endif // DEBUG

char *make_node_text_nds(GBDATA *gb_main, GBDATA * gbd, int mode, GBT_TREE *species)
{
    // mode == MNTN_COMPRESSED      compress info (no tabbing, seperate single fields by komma)
    // mode == MNTN_SPACED          format info (using spaces)
    // mode == MNTN_TABBED          format info (using 1 tab per column - for easy import into star-calc, excel, etc. )

    char     *bp, *p;
    GBDATA   *gbe;
    long      i, j;
    GBT_TREE *father;
    bool      field_was_printed = false;

    bp = awt_nds_ms->buf;
    if (!gbd) {
        static char hae[] = "??????";
        if (!species) return hae;
        sprintf(awt_nds_ms->buf,"<%s>",species->name);
        return awt_nds_ms->buf;
    }

#if defined(QUOTE_NDS_STRING)
    *bp++ = '\'';
#endif // QUOTE_NDS_STRING

    for (i = 0; i < awt_nds_ms->count; i++) {
        if (awt_nds_ms->rek[i]) {       /* hierarchical key */
            gbe = GB_search(gbd,awt_nds_ms->dkeys[i],0);
        }else{              /* flat entry */
            gbe = GB_find(gbd, awt_nds_ms->dkeys[i], NULL, down_level);
        }

        if (!gbe && awt_nds_ms->inherit[i] && species ) {
            for (   father = species->father; father && !gbe; father = father->father) {
                if (father->gb_node){
                    gbe = GB_find(father->gb_node, awt_nds_ms->dkeys[i], NULL, down_level);
                }
            }
        }

        if (mode == MNTN_COMPRESSED && !gbe) continue;

        switch (mode) {
            case MNTN_COMPRESSED:
                if (!field_was_printed) break; // no komma no space if nothing printed yet
                *bp++ = ','; // seperate single fields by komma in compressed mode
                // no break here!!!
            case MNTN_SPACED:
                *bp++ = ' ';    // print at least one space if not using tabs
                break;
            case MNTN_TABBED:
                if (i != 0) *bp++ = '\t'; // tabbed output for star-calc/excel/...
                break;
            default :
                awt_assert(0);
                break;
        }

        //         if (mode == 0 && first) (*bp++) = ',';
        //         if (mode != 2)          (*bp++) = ' '; // print at least one space if not using tabs
        //         first++;

        if (gbe) {
            field_was_printed = true;

            switch (GB_read_type(gbe)) {
                case GB_INT:
                    if (mode == MNTN_SPACED) {
                        //                         char buf[20];
                        //                         sprintf(buf,"%%%lii", awt_nds_ms->lengths[i]);
                        //                         sprintf(bp, buf, GB_read_int(gbe));
                        sprintf(bp, "%-*li", int(awt_nds_ms->lengths[i]), GB_read_int(gbe));
                    }
                    else {
                        sprintf(bp, "%li", GB_read_int(gbe));
                    }
                    bp += strlen(bp);
                    break;
                case GB_BYTE:
                    if (mode == MNTN_SPACED) {
                        //                         char buf[20];
                        //                         sprintf(buf,"%%%lii", awt_nds_ms->lengths[i]);
                        //                         sprintf(bp, buf, GB_read_byte(gbe));
                        sprintf(bp, "%-*i", int(awt_nds_ms->lengths[i]), GB_read_byte(gbe));
                    }
                    else {
                        sprintf(bp, "%i", GB_read_byte(gbe));
                    }
                    bp += strlen(bp);
                    break;
                case GB_STRING:
                    {
                        long  post;
                        long  dlen;
                        char *pars = 0;

                        if (awt_nds_ms->parsing[i]) {
                            p = GB_read_string(gbe);
                            pars = GB_command_interpreter(gb_main,p, awt_nds_ms->parsing[i],gbd);
                            free(p);
                            if (!pars){
                                pars = strdup("<error>");
                                if (!awt_nds_ms->errorclip++) {
                                    aw_message(GB_get_error());
                                }
                            }
                            p = pars;
                        }else{
                            p = GB_read_char_pntr(gbe);
                        }

                        dlen = awt_nds_ms->lengths[i];
                        if (dlen + (bp - awt_nds_ms->buf) +256 > NDS_STRING_SIZE) {
                            dlen = NDS_STRING_SIZE - 256 - (bp - awt_nds_ms->buf);
                        }

                        if (dlen> 0){
                            int len = strlen(p);
                            j = len;
                            if (j > dlen)   j = dlen;
                            for (; j; j--) *bp++ = *p++;
                            if (mode == MNTN_SPACED) {
                                post = dlen - len;
                                while (post-- > 0) *(bp++) = ' ';
                            }
                        }
                        if (pars) free(pars);
                    }
                    break;
                case GB_FLOAT:
                    if (mode == MNTN_SPACED) {
                        char buf[20];
                        sprintf(buf, "%4.4f", GB_read_float(gbe));
                        sprintf(bp, "%-*s", int(awt_nds_ms->lengths[i]), buf);
                    }
                    else {
                        sprintf(bp, "%4.4f", GB_read_float(gbe));
                        if (mode == MNTN_TABBED) { // '.' -> ','
                            char *dot     = strchr(bp, '.');
                            if (dot) *dot = ',';
                        }
                    }
                    bp += strlen(bp);
                    break;
                default:
                    break;
            }
        }
        else if (mode == MNTN_SPACED) { // fill with spaces till start of next column
            j = awt_nds_ms->lengths[i];
            if (j + (bp - awt_nds_ms->buf) + 256 > NDS_STRING_SIZE) {
                j = NDS_STRING_SIZE - 256 - (bp - awt_nds_ms->buf);
            }
            for (; j > 0; j--)  *(bp++) = ' ';
        }
    }
#if defined(QUOTE_NDS_STRING)
    *bp++ = '\'';
#endif // QUOTE_NDS_STRING
    *bp = 0;

    //     if (mode == MNTN_COMPRESSED) { // remove leading and trailing commas in compressed mode
    //
    //     }

    return awt_nds_ms->buf;
}

char *make_node_text_list(GBDATA * gbd, FILE *fp)
{
    /* if mode==0 screen else file */
    char           *bp, *p;
    GBDATA         *gbe;
    long             i;
    long             cp;
    char        c = 0;
    char        fieldname[50];

    bp = awt_nds_ms->buf;
    if (!gbd) {
        *bp = 0;
        return awt_nds_ms->buf;
    }

    fprintf(fp,"\n------------------- %s\n",GB_read_char_pntr(GB_find(gbd, "name", 0, down_level)));

    for (i = 0; i < awt_nds_ms->count; i++) {
        if (awt_nds_ms->rek[i]) {       /* hierarchical key */
            gbe = GB_search(gbd,awt_nds_ms->dkeys[i],0);
        }else{              /* flat entry */
            gbe = GB_find(gbd, awt_nds_ms->dkeys[i], NULL, down_level);
        }
        if (!gbe) continue;
        /*** get field info ***/
        switch (GB_read_type(gbe)) {
            case GB_INT:
                sprintf(bp, "%li", GB_read_int(gbe));
                break;
            case GB_STRING:
                p = GB_read_char_pntr(gbe);
                sprintf(bp,"%s", p);
                break;
            case GB_FLOAT:
                sprintf(bp, "%4.4f", GB_read_float(gbe));
                break;
            default:
                sprintf(bp,"'default:' make_node_text_list!");
                                 break;
        }/*switch*/

        /*** get fieldname ***/
        strcpy(fieldname, awt_nds_ms->dkeys[i]);

        /*** print fieldname+begin of line ***/
        cp = strlen (bp);
        if (cp>=60) {
            c = bp[60];
            bp[60] = 0;
        }
        fprintf(fp,"%18s: %s\n",fieldname+1, bp);
        if (cp>=60) bp[60] = c;

        while(cp > 60) {
            cp -= 60;
            bp += 60;
            if (cp>=60) {
                c = bp[60];
                bp[60] = 0;
            }
            fprintf(fp,"%18s  %s\n","", bp);
            if (cp>=60) bp[60] = c;
        }
    }
    *bp = 0;
    return awt_nds_ms->buf;
}

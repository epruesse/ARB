#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
// #include <malloc.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <awt.hxx>

#include "gde.hxx"
#include "GDE_menu.h"
#include "GDE_def.h"
#include "GDE_extglob.h"

#define META "" // default is 'no hotkey'

AW_CL agde_filtercd;

extern void GDE_startaction_cb(AW_window *aw,AWwindowinfo *AWinfo,AW_CL cd);

Gmenu menu[GDEMAXMENU];
int num_menus = 0,repeat_cnt = 0;
//Frame frame,pframe,infoframe;
//Panel popup,infopanel;
//Panel_item left_foot,right_foot;
//Canvas EditCan,EditNameCan;
int DisplayType;
GmenuItem *current_item;
NA_Alignment *DataSet = NULL;
//NA_Alignment *Clipboard = NULL;
//char **TextClip;
//int TextClipSize = 0,TextClipLength = 0;

char GDEBLANK[] = "\0";

#define SLIDERWIDTH 5           // input field width for numbers

// enum {
//  SLIDERWIDTH    = 5,
//  TEXTFIELDWIDTH = 15
//  };

char *GDE_makeawarname(AWwindowinfo *AWinfo,long i);

int GDE_odd(long a)
{
    if(((a/2)*2)==a) return 0;
    return 1;
}

void GDE_showhelp_cb(AW_window *aw, AWwindowinfo *AWinfo, AW_CL /*cd*/) {
    const char *help_file = AWinfo->gmenuitem->help;
    if (help_file) {
        char *agde_help_file = GBS_string_eval(help_file, "*.help=agde_*1.hlp", 0);
        AW_POPUP_HELP(aw, (AW_CL)agde_help_file);
        free(agde_help_file);
    }
    else {
        aw_message("Sorry - no help available (please report to devel@arb-home.de)");
    }
}

char *GDE_makeawarname(AWwindowinfo *AWinfo,long i)
{
    char name[GB_KEY_LEN_MAX*4+5];
    char *gmenu = GBS_string_2_key(AWinfo->gmenu->label);
    char *gmenuitem = GBS_string_2_key(AWinfo->gmenuitem->label);
    char *arg = GBS_string_2_key(AWinfo->gmenuitem->arg[i].symbol);

    sprintf(name,"gde/%s/%s/%s",gmenu,gmenuitem,arg);
    free(gmenu);
    free(gmenuitem);
    free(arg);

    return(strdup(name));
}

void GDE_slide_awar_int_cb(AW_window *aws, AW_CL cl_awar_name, AW_CL cd_diff)
{
    int      diff = (int)cd_diff;
    AW_awar *awar = aws->get_root()->awar((const char *)cl_awar_name);

    awar->write_int(awar->read_int()+diff);
}
void GDE_slide_awar_float_cb(AW_window *aws, AW_CL cl_awar_name, AW_CL cd_diff)
{
    double   diff    = *(double*)cd_diff;
    AW_awar *awar    = aws->get_root()->awar((const char *)cl_awar_name);
    double   new_val = awar->read_float()+diff;

    if (fabs(new_val) < 0.0001) new_val = 0.0;
    awar->write_float(new_val);

    // do it again (otherwise internal awar-range correction sometimes leads to 1+eXX values)
    new_val = awar->read_float();
    if (fabs(new_val) < 0.0001) new_val = 0.0;
    awar->write_float(new_val);
}

void GDE_create_infieldwithpm(AW_window *aws,char *newawar,long width)
{
    char *awar=strdup(newawar);
    aws->create_input_field(newawar,(int)width);
    if (aws->get_root()->awar(newawar)->get_type() == AW_INT) {
        aws->button_length(3);
        aws->callback(GDE_slide_awar_int_cb,(AW_CL)awar,-1);
        aws->create_button(0,"-","-");
        aws->callback(GDE_slide_awar_int_cb,(AW_CL)awar,+1);
        aws->create_button(0,"+","+");
    }
    else if (aws->get_root()->awar(newawar)->get_type() == AW_FLOAT) {
        aws->button_length(3);
        aws->callback(GDE_slide_awar_float_cb,(AW_CL)awar,(AW_CL)new double(-0.1));
        aws->create_button(0,"-","-");
        aws->callback(GDE_slide_awar_float_cb,(AW_CL)awar,(AW_CL)new double(+0.1));
        aws->create_button(0,"+","+");
    }
}

char *gde_filter_weights(GBDATA *gb_sai,AW_CL ){
    char *ali_name = GBT_get_default_alignment(gb_main);
    GBDATA *gb_ali = GB_find(gb_sai,ali_name,0,down_level);
    delete ali_name;
    if (!gb_ali) return 0;
    GBDATA *gb_type = GB_find(gb_ali, "_TYPE",0,down_level);
    if (!gb_type) return 0;
    char *type = GB_read_char_pntr(gb_type);
    if (GBS_string_cmp( type,"PV?:*",0)) {
        return 0;
    }

    char *name = GBT_read_name(gb_sai);
    const char *res = GBS_global_string("%s: %s",name,type);
    delete name;
    return strdup(res);
}



AW_window *GDE_menuitem_cb(AW_root *aw_root,AWwindowinfo *AWinfo) {
#define BUFSIZE 200
    char bf[BUFSIZE+1];
#if defined(ASSERTION_USED)
    int printed =
#endif // ASSERTION_USED
        sprintf(bf,"GDE / %s / %s",AWinfo->gmenu->label,AWinfo->gmenuitem->label);
    gb_assert(printed<=BUFSIZE);

    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root,bf,bf);

    switch (gde_cgss.wt) {
        case CGSS_WT_DEFAULT:
            aws->load_xfig("gdeitem.fig");
            break;
        case CGSS_WT_EDIT:
            aws->load_xfig("gde2item.fig");
            break;
        case CGSS_WT_EDIT4:
            aws->load_xfig("gde3item.fig");
            break;
        default:
            gde_assert(0);
    }

    aws->set_window_size(1000,2000);
    aws->button_length(10);
    aws->at(10,10);
    aws->auto_space(0,10);

    aws->at("help");
    aws->callback((AW_CB2)GDE_showhelp_cb,(AW_CL)AWinfo,0);
    aws->create_button("GDE_HELP","HELP...","H");

    aws->at("start");
    aws->callback((AW_CB2)GDE_startaction_cb,(AW_CL)AWinfo,0);
    aws->create_button("GO", "GO","O");

    aws->at("cancel");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");


    if (AWinfo->gmenuitem->numinputs>0) {
        switch (gde_cgss.wt) {
            case CGSS_WT_DEFAULT:
                aws->at("which_alignment");
                awt_create_selection_list_on_ad(gb_main, (AW_window *)aws,"tmp/gde/alignment","*=");
                aws->at( "which_species" );
                aws->create_toggle_field( "gde/species" );
                aws->insert_toggle( "all", "a", 0 );
                aws->insert_default_toggle( "marked",  "m", 1 );
                aws->update_toggle_field();
                break;
            case CGSS_WT_EDIT:
                aws->at("bottom"); aws->create_toggle("gde/bottom_area");
                aws->at("bottomsai"); aws->create_toggle("gde/bottom_area_sai");
                aws->at("bottomh"); aws->create_toggle("gde/bottom_area_helix");
                goto both_edits;
            case CGSS_WT_EDIT4:
                aws->at("topk"); aws->create_toggle("gde/top_area_kons");
                aws->at("middlek"); aws->create_toggle("gde/middle_area_kons");
                aws->at("topr"); aws->create_toggle("gde/top_area_remark");
                aws->at("middler"); aws->create_toggle("gde/middle_area_remark");
                goto both_edits;
        both_edits:
                aws->at("top"); aws->create_toggle("gde/top_area");
                aws->at("topsai"); aws->create_toggle("gde/top_area_sai");
                aws->at("toph"); aws->create_toggle("gde/top_area_helix");
                aws->at("middle"); aws->create_toggle("gde/middle_area");
                aws->at("middlesai"); aws->create_toggle("gde/middle_area_sai");
                aws->at("middleh"); aws->create_toggle("gde/middle_area_helix");
                break;
        }

        aws->at("compression");
        aws->create_toggle_field( "gde/compression" );
        aws->insert_toggle( "none", "n",0 );
        aws->insert_default_toggle( "vertical gaps", "v", 1 );
        aws->insert_toggle( "all gaps",  "g", 2 );
        aws->update_toggle_field();

        aws->button_length(12);
        aws->at("filtername");
        agde_filtercd = awt_create_select_filter(aws->get_root(),gb_main,"gde/filter/name");
        aws->callback((AW_CB2)AW_POPUP,(AW_CL)awt_create_select_filter_win,agde_filtercd);
        aws->create_button("SELECT_FILTER", "gde/filter/name");

        aws->at("paramsb");
    }
    else {
        aws->at("paramsb");
    }


    long labellength=0,lablen;
    //char *help;
    labellength=1;
    long i;
    for (i=0;i<AWinfo->gmenuitem->numargs;i++) {
        //help=AWinfo->gmenuitem->help;
        if(!(AWinfo->gmenuitem->arg[i].label)) AWinfo->gmenuitem->arg[i].label=GDEBLANK;
        lablen=strlen(AWinfo->gmenuitem->arg[i].label);
        if(lablen>labellength) labellength=lablen;
    }
    aws->label_length((int)labellength);
    aws->auto_space(0,0);

    for (i=0;i<AWinfo->gmenuitem->numargs;i++) {
        GmenuItemArg itemarg=AWinfo->gmenuitem->arg[i];

        if(itemarg.type==SLIDER) {
            char *newawar=GDE_makeawarname(AWinfo,i);
            if ( int(AWinfo->gmenuitem->arg[i].fvalue) == AWinfo->gmenuitem->arg[i].fvalue &&
                 int(AWinfo->gmenuitem->arg[i].min) == AWinfo->gmenuitem->arg[i].min &&
                 int(AWinfo->gmenuitem->arg[i].max) == AWinfo->gmenuitem->arg[i].max){
                aw_root->awar_int(newawar,(long)AWinfo->gmenuitem->arg[i].fvalue,AW_ROOT_DEFAULT);
            }else{
                aw_root->awar_float(newawar,AWinfo->gmenuitem->arg[i].fvalue,AW_ROOT_DEFAULT);
            }
            aw_root->awar(newawar)->set_minmax(AWinfo->gmenuitem->arg[i].min,AWinfo->gmenuitem->arg[i].max);
            aws->label(AWinfo->gmenuitem->arg[i].label);
            GDE_create_infieldwithpm(aws,newawar,SLIDERWIDTH);
            // maybe bound checking //
            free(newawar);
        }
        else if(itemarg.type==CHOOSER) {
            char    *defopt           = itemarg.choice[0].method;
            char    *newawar          = GDE_makeawarname(AWinfo,i);
            AW_awar *curr_awar        = aw_root->awar_string(newawar,defopt,AW_ROOT_DEFAULT);
            char    *curr_value       = curr_awar->read_string();
            bool     curr_value_legal = false;

            aws->label(AWinfo->gmenuitem->arg[i].label);
            if (    (! GBS_string_cmp(itemarg.choice[0].label,"no",1)) ||
                    (! GBS_string_cmp(itemarg.choice[0].label,"yes",1)))
            {
                aws->create_toggle_field(newawar,1);
            }
            else {
                aws->create_toggle_field(newawar);
            }

            for(long j=0;j<itemarg.numchoices;j++) {
                if (strcmp(itemarg.choice[j].method, curr_value) == 0) curr_value_legal = true;

                if (!j) {
                    aws->insert_default_toggle(itemarg.choice[j].label,"1", itemarg.choice[j].method);
                }
                else {
                    aws->insert_toggle(itemarg.choice[j].label,"1", itemarg.choice[j].method);
                }
            }
            if (!curr_value_legal) curr_awar->write_string(defopt); // if saved value no longer occurs in choice -> overwrite with default
            free(curr_value);
            aws->update_toggle_field();
            free(newawar);
        }
        else if(itemarg.type==CHOICE_MENU) {
            char    *defopt           = itemarg.choice[itemarg.ivalue].method;
            char    *newawar          = GDE_makeawarname(AWinfo,i);
            AW_awar *curr_awar        = aw_root->awar_string(newawar,defopt,AW_ROOT_DEFAULT);
            char    *curr_value       = curr_awar->read_string();
            bool     curr_value_legal = false;

            aws->label(AWinfo->gmenuitem->arg[i].label);
            aws->create_option_menu(newawar,NULL,"");

            for(long j=0;j<itemarg.numchoices;j++) {
                if (strcmp(itemarg.choice[j].method, curr_value) == 0) curr_value_legal = true;
                aws->insert_option(itemarg.choice[j].label,"1", itemarg.choice[j].method);
            }
            if (!curr_value_legal) curr_awar->write_string(defopt); // if saved value no longer occurs in choice -> overwrite with default
            free(curr_value);
            aws->update_option_menu();
            free(newawar);
        }
        else if(itemarg.type==TEXTFIELD) {
            char *defopt  = itemarg.textvalue;
            char *newawar = GDE_makeawarname(AWinfo,i);
            aw_root->awar_string(newawar,defopt,AW_ROOT_DEFAULT);
            aws->label(AWinfo->gmenuitem->arg[i].label);
            aws->create_input_field(newawar,itemarg.textwidth/*TEXTFIELDWIDTH*/);
            free(newawar);
        }
        else if(itemarg.type==CHOICE_TREE) {
            char *defopt=itemarg.textvalue;
            char *newawar=GDE_makeawarname(AWinfo,i);
            aw_root->awar_string(newawar,defopt,AW_ROOT_DEFAULT);
            aws->label(AWinfo->gmenuitem->arg[i].label);
            awt_create_selection_list_on_trees(gb_main,aws,newawar);
            free(newawar);
        }
        else if(itemarg.type==CHOICE_SAI) {
            char *defopt=itemarg.textvalue;
            char *newawar=GDE_makeawarname(AWinfo,i);
            aw_root->awar_string(newawar,defopt,AW_ROOT_DEFAULT);
            aws->label(AWinfo->gmenuitem->arg[i].label);
            awt_create_selection_list_on_extendeds(gb_main,aws,newawar);
            free(newawar);
        }
        else if(itemarg.type==CHOICE_WEIGHTS) {
            char *defopt=itemarg.textvalue;
            char *newawar=GDE_makeawarname(AWinfo,i);
            aw_root->awar_string(newawar,defopt,AW_ROOT_DEFAULT);
            aws->label(AWinfo->gmenuitem->arg[i].label);
            void *id = awt_create_selection_list_on_extendeds(gb_main,aws,newawar,gde_filter_weights);
            free(newawar);
            aw_root->awar("tmp/gde/alignment")->add_callback((AW_RCB1)awt_create_selection_list_on_extendeds_update,(AW_CL)id);
        }

        aws->at_newline();
//         if((AWinfo->gmenuitem->numargs<8) || GDE_odd(i)) aws->at_newline();
//         else aws->at_shift( 50,0 );
    }
    aws->at_newline();

    aws->window_fit();
    return (AW_window *)aws;
#undef BUFSIZE
}


void GDE_load_menu(AW_window *awm,const char *menulabel,const char *menuitemlabel)
{
    //  AW_root *aw_root = awm->get_root();
    char       buffer[1024];
    char      *help;
    long       nitem,num_items;
    GmenuItem *menuitem;

    for(long nmenu = 0; nmenu<num_menus; nmenu++) {
        {
            char *menuname = GBS_string_2_key(menu[nmenu].label);
            if (menulabel){
                if (strcmp(menulabel,menuname)) {
                    free(menuname);
                    continue;
                }
            }
            else {
                awm->insert_sub_menu(0,menuname,META);
            }
            free(menuname);
        }

        num_items = menu[nmenu].numitems;
        for(nitem=0;nitem<num_items;nitem++)
        {
            menuitem=&menu[nmenu].item[nitem];
            if (!menuitemlabel || !GBS_string_cmp(menuitem->label,menuitemlabel,0)){
                AWwindowinfo *AWinfo=new AWwindowinfo;
                AWinfo->gmenu=&menu[nmenu];
                AWinfo->gmenuitem=menuitem;
                if (menuitem->help) {
                    sprintf(buffer,"GDEHELP/%s",menuitem->help);
                    help = strdup(buffer);
                }
                else {
                    help = 0;
                }
                char hotkey[] = "x";
                hotkey[0]     = menuitem->meta;
                awm->insert_menu_topic(0,menuitem->label,hotkey,
                                       help,AWM_ALL,
                                       AW_POPUP, (AW_CL)GDE_menuitem_cb, (AW_CL)AWinfo );
            }
        }
        //  if(!menulabel && nmenu==0)
        //      awm->insert_menu_topic(0,"Close","C",0,(AW_active) -1, (AW_CB)AW_POPDOWN, 0, 0);
        if (!menulabel){
            awm->close_sub_menu();
        }
    }
}

struct choose_get_sequence_struct gde_cgss = { 0, CGSS_WT_DEFAULT, 0 };

void create_gde_var(AW_root  *aw_root, AW_default aw_def,
                    char *(*get_sequences)(void *THIS, GBDATA **&the_species,
                                           uchar **&the_names,
                                           uchar **&the_sequences,
                                           long &numberspecies,long &maxalignlen),
                    gde_cgss_window_type wt,
                    void *THIS)
{
    gde_cgss.get_sequences= get_sequences;
    gde_cgss.wt = wt;
    gde_cgss.THIS= THIS;

    aw_root->awar_string("tmp/gde/helptext","help",aw_def);
    aw_root->awar_string( "tmp/gde/alignment","" ,aw_def );

    switch (gde_cgss.wt)
    {
        case CGSS_WT_EDIT4:
            aw_root->awar_int("gde/top_area_kons",1,aw_def);
            aw_root->awar_int("gde/top_area_remark",1,aw_def);
            aw_root->awar_int("gde/middle_area_kons",1,aw_def);
            aw_root->awar_int("gde/middle_area_remark",1,aw_def);
        case CGSS_WT_EDIT:
            aw_root->awar_int("gde/top_area",1,aw_def);
            aw_root->awar_int("gde/top_area_sai",1,aw_def);
            aw_root->awar_int("gde/top_area_helix",1,aw_def);
            aw_root->awar_int("gde/middle_area",1,aw_def);
            aw_root->awar_int("gde/middle_area_sai",1,aw_def);
            aw_root->awar_int("gde/middle_area_helix",1,aw_def);
            aw_root->awar_int("gde/bottom_area",1,aw_def);
            aw_root->awar_int("gde/bottom_area_sai",1,aw_def);
            aw_root->awar_int("gde/bottom_area_helix",1,aw_def);
        default:
            break;
    }

    aw_root->awar_string( "presets/use","" ,gb_main );
    aw_root->awar_string( "gde/filter/name","",aw_def);
    aw_root->awar_string( "gde/filter/filter","",aw_def);
    aw_root->awar_int( "gde/species",1,aw_def);
    aw_root->awar_int( "gde/compression",1,aw_def);
    aw_root->awar_string( "gde/filter/alignment","",aw_def);
    aw_root->awar("tmp/gde/alignment")->map("presets/use");
    aw_root->awar("gde/filter/alignment")->map("presets/use");

    DataSet = (NA_Alignment *) Calloc(1,sizeof(NA_Alignment));
    DataSet->rel_offset = 0;
    ParseMenu();
}

AW_window *AP_open_gde_window(AW_root *aw_root)
{
    AW_window_menu_modes *awm = new AW_window_menu_modes;
    awm->init(aw_root,"GDE","GDE",700,30);
    awm->at(10,100);awm->callback((AW_CB0)AW_POPDOWN);
    awm->create_button("CLOSE", "CLOSE","C");

    GDE_load_menu(awm);

    return (AW_window *)awm;
}

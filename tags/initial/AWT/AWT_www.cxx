#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <malloc.h>
#include <string.h>

#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>

#include <arbdb.h>
#include <arbdbt.h>
#include <awt.hxx>


#define WWW_COUNT 10
#define AWAR_WWW_BROWSER "www/browser"
#define AWAR_WWW_SELECT "www/url_select"
#define AWAR_WWW_0 "www/url_0/srt"
#define AWAR_WWW_1 "www/url_0/srt"
#define AWAR_WWW_2 "www/url_0/srt"
#define AWAR_WWW_SELECT_TEMPLATE "www/url_%i/select"
#define AWAR_WWW_TEMPLATE "www/url_%i/srt"
#define AWAR_WWW_DESC_TEMPLATE "www/url_%i/desc"



void awt_create_aww_vars(AW_root *aw_root,AW_default aw_def){
    int i;
    for (i=0;i<WWW_COUNT;i++){
        const char *awar_name;
        
        awar_name = GBS_global_string(AWAR_WWW_TEMPLATE,i);         aw_root->awar_string(awar_name,i==WWW_COUNT-1?"\"http://www.ebi.ac.uk/cgi-bin/emblfetch?\";readdb(acc)":"",aw_def);        
        awar_name = GBS_global_string(AWAR_WWW_DESC_TEMPLATE,i);    aw_root->awar_string(awar_name,i==WWW_COUNT-1?"EMBL example":"",aw_def);
        awar_name = GBS_global_string(AWAR_WWW_SELECT_TEMPLATE,i);  aw_root->awar_int(awar_name,0,aw_def);
    }
    
    aw_root->awar_int(AWAR_WWW_SELECT,0,aw_def);
    aw_root->awar_string(AWAR_WWW_BROWSER, "(netscape -remote 'openURL($(URL))' || netscape '$(URL)') &", aw_def);
}

GB_ERROR awt_openURL_by_gbd(AW_root *aw_root,GBDATA *gb_main, GBDATA *gbd, const char *name){
    GB_ERROR error = 0;
    GB_transaction tscope(gb_main);
    
    int url_selected = aw_root->awar(AWAR_WWW_SELECT)->read_int();
    const char *awar_url = GBS_global_string(AWAR_WWW_TEMPLATE,url_selected);
    char *url_srt = aw_root->awar(awar_url)->read_string();
    char *url = GB_command_interpreter(gb_main,name,url_srt,gbd);
    if (!url){
	error = GB_get_error();
    }else{
	char *ka;
	char *browser = aw_root->awar(AWAR_WWW_BROWSER)->read_string();
	while ( (ka = GBS_find_string(browser,"$(URL)",0)) ){
	    char *nb = (char *)GB_calloc(sizeof(char), strlen(browser) + strlen(url));
	    *ka = 0;
	    sprintf(nb,"%s%s%s",browser,url,ka + 6);
	    delete browser;
	    browser = nb;
	}
	if (GBCMC_system(gb_main, browser)){
	    error = GB_get_error();
	}
	delete browser;
	delete url;
    }
    delete url_srt;
    return error;
}

void awt_openURL(AW_window *aww,GBDATA *gb_main){
    GB_transaction tscope(gb_main);
    AW_root *aw_root = aww->get_root();
    GB_ERROR error = 0;
    char *selected_species = aw_root->awar(AWAR_SPECIES_NAME)->read_string();
    GBDATA *gb_species = GBT_find_species(gb_main,selected_species);
    if (!gb_species){
	error = GB_export_error("Cannot find species '%s'",selected_species);
    }else{
	error = awt_openURL_by_gbd(aw_root,gb_main, gb_species, selected_species);
    }
    if (error) aw_message(error);
    delete selected_species;
}
/* a crazy implementation of a toggle field */
void awt_www_select_change(AW_window *aww,AW_CL selected){
    int i;
    AW_root *aw_root = aww->get_root();
    for (i=0;i<WWW_COUNT;i++){
        const char *awar_name = GBS_global_string(AWAR_WWW_SELECT_TEMPLATE,i);
        aw_root->awar(awar_name)->write_int( (i==selected)? 1:0);
    }
    aw_root->awar(AWAR_WWW_SELECT)->write_int(selected);
}

AW_window *AWT_open_www_window(AW_root *aw_root,AW_CL cgb_main){
    
    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "WWW_PROPS", "WWW", 200, 0 );
    aws->load_xfig("awt/nds.fig");
    aws->auto_space(10,5);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"props_www.hlp");
    aws->create_button("HELP", "HELP","H");
    
    aws->at("action");
    aws->callback((AW_CB1)awt_openURL,cgb_main);
    aws->create_button("WWW", "WWW", "W");

    aws->button_length(13);
    int dummy,closey;
    aws->at_newline();
    aws->get_at_position( &dummy,&closey );
    aws->at_newline();

    aws->create_button(0,"K");

    aws->at_newline();
	
    int	fieldselectx,srtx,descx;


    int i;
    aws->get_at_position( &fieldselectx,&dummy );

    aws->auto_space(10,2);
	
    for (i=0;i<WWW_COUNT; i++) {
        char buf[256];
        sprintf(buf,AWAR_WWW_SELECT_TEMPLATE,i);
        aws->callback(awt_www_select_change,i);
        aws->create_toggle(buf);
	
        sprintf(buf,AWAR_WWW_DESC_TEMPLATE,i);
        aws->get_at_position( &descx,&dummy );
        aws->create_input_field(buf,15);

        aws->get_at_position( &srtx,&dummy );
        sprintf(buf,AWAR_WWW_TEMPLATE,i);
        aws->create_input_field(buf,80);
		
        aws->at_newline();
    }
    aws->at_newline();

    aws->create_input_field(AWAR_WWW_BROWSER,100);

    aws->at(fieldselectx,closey);

    aws->at_x(fieldselectx);
    aws->create_button(0,"SEL");	

    aws->at_x(descx);
    aws->create_button(0,"DESCRITPION");

    aws->at_x(srtx);
    aws->create_button(0,"URL");
    
    awt_www_select_change(aws,aw_root->awar(AWAR_WWW_SELECT)->read_int());
    return (AW_window *)aws;
}
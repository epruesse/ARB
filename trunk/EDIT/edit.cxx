#include <stdio.h>
#include <stdlib.h> // wegen exit();
#include <memory.h>
// #include <malloc.h>
#include <string.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <arbdb++.hxx>

#include <aw_root.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <adtools.hxx>
#include <AW_helix.hxx>
#include <st_window.hxx>
#include <edit.hxx>
#include "ed_conf.hxx"
#include <awt_map_key.hxx>
#include "gde.hxx"
#include <awt.hxx>

#include <awtc_fast_aligner.hxx>

AW_HEADER_MAIN
AD_MAIN *ad_main;
GBDATA *gb_main;
ST_ML   *st_ml = 0;

void AD_map_viewer(GBDATA *dummy,AD_MAP_VIEWER_TYPE )
{
    AWUSE(dummy);
}

struct ed_global {
    AW_helix    *helix;
    BI_ecoli_ref    *ref;
    ed_key      *edk;
} edg;


void aed_expose_info_area(AW_window *aw, AW_CL cd1, AW_CL cd2);

AED_dlist::AED_dlist(){
    memset((char *)this,0,sizeof(AED_dlist));
}

AED_root::AED_root(void) {
    ad_main = new AD_MAIN;
    aw_root = new AW_root;
}



AED_window::AED_window(void) {
    memset((char *)this,0,sizeof(AED_window));

    config_window_created = AW_FALSE;

    global_focus_use = AW_FALSE;

    last_slider_position = 0;
    quickdraw = AW_FALSE;

    show_dlist_left_side = new AED_dlist_left_side;
    hide_dlist_left_side = new AED_dlist_left_side;


    area_top = new AED_dlist;
    area_bottom = new AED_dlist;
    area_middle = new AED_dlist;

    alignment = new ADT_ALI;

    cursor_is_managed = AW_FALSE;
    one_area_entry_is_selected = AW_FALSE;

    edit_modus = AED_ALIGN;

    info_area_height = 50;
    edit_info_area = AW_FALSE;

    selected_area_entry_is_visible = AW_FALSE;
}


AED_window::~AED_window(void) {
}


void AED_window::init(AED_root *rootin) {
    root = rootin;
}



/***************************************************************************************************************************/
/***************************************************************************************************************************/
/***************************************************************************************************************************/


static void aed_use_focus(AW_window *aw, AW_CL , AW_CL cd2) {
    //AED_window *aedw = (AED_window *)cd1;
    AW_root *awr = aw->get_root();
    if (cd2) {
        long pos;
        pos = awr->awar(AWAR_CURSOR_POSITION_LOCAL)->read_int();
        awr->awar(AWAR_CURSOR_POSITION_LOCAL)->map(AWAR_CURSOR_POSITION);
        awr->awar(AWAR_CURSOR_POSITION_LOCAL)->write_int(pos);

        // name = awr->awar(AWAR_SPECIES_NAME_LOCAL)->read_string();
        //awr->awar(AWAR_SPECIES_NAME_LOCAL)->map(AWAR_SPECIES_NAME);
        //awr->awar(AWAR_SPECIES_NAME_LOCAL)->write_string(name);

    }else{
        //awr->awar(AWAR_SPECIES_NAME_LOCAL)->unmap();
        awr->awar(AWAR_CURSOR_POSITION_LOCAL)->unmap();
    }
}

static void set_security(AW_window *aw, AW_CL cd1, AW_CL cd2){
    AED_window *aedw = (AED_window *)cd1;
    if (!aedw->selected_area_entry) {
        aw_message("Please select a secuence first");
        return;
    }
    GB_transaction dummy(gb_main);
    GB_push_my_security(gb_main);
    GBDATA *gbd = aedw->selected_area_entry->adt_sequence->get_GBDATA();
    if (gbd) {
        GB_ERROR error = GB_write_security_write(gbd,cd2);
        if (error) aw_message(error);
    }else{
        aw_message("Cannot change security level when there is no sequence");
    }

    GB_pop_my_security(gb_main);
    aedw->expose(aw);
}
static void set_mark_cb(AW_window *aw, AW_CL cd1, AW_CL cd2){
    AWUSE(aw);
    AED_window *aedw = (AED_window *)cd1;
    GB_transaction dummy(gb_main);
    AED_area_entry *ae;
    for (ae = aedw->area_top->first; ae ; ae = ae->next) {
        GBDATA *gbd  = ae->ad_species->get_GBDATA(); if (!gbd) continue;
        GB_write_flag( gbd, cd2 );
    }
    for (ae = aedw->area_middle->first; ae ; ae = ae->next) {
        GBDATA *gbd  = ae->ad_species->get_GBDATA(); if (!gbd) continue;
        GB_write_flag( gbd, cd2 );
    }
    for (ae = aedw->area_bottom->first; ae ; ae = ae->next) {
        GBDATA *gbd  = ae->ad_species->get_GBDATA(); if (!gbd) continue;
        GB_write_flag( gbd, cd2 );
    }
    aedw->expose( aw );
}

static void set_smark_cb(AW_window *aw, AW_CL cd1, AW_CL cd2){
    AED_window *aedw = (AED_window *)cd1;
    GB_transaction dummy(gb_main);
    AED_area_entry *ae = aedw->selected_area_entry;
    if (!ae) return;
    GBDATA *gbd  = ae->ad_species->get_GBDATA(); if (!gbd) return;
    GB_write_flag( gbd, cd2 );
    aedw->expose( aw );
}

static void aed_changesecurity(AW_root *dummy, AW_CL cd1) {
    AWUSE(dummy);
    AED_window *aedw = (AED_window *)cd1;
    long level = aedw->root->aw_root->awar(AWAR_SECURITY_LEVEL)->read_int();
    aedw->root->ad_main->change_security_level((int)level);
}



static AD_ERR *aed_adopen(const char *server,   AD_MAIN *admain ) {
    return admain->open(server,MAXCACH);
}


/*
  void aed_save(AW_window *aw, AW_CL cd1, AW_CL cd2 ) {
  AWUSE(aw);AWUSE(cd2);
  AED_window *aedw = (AED_window *)cd1;
  aedw->root->ad_main->save("ascii");
  }
*/

static void aed_save_bin(AW_window *aw, AW_CL cd1, AW_CL cd2 ) {
    AWUSE(aw);AWUSE(cd2);
    AED_window *aedw = (AED_window *)cd1;
    aedw->root->ad_main->save("bin");
}


static void aed_quit(AW_window *aw, AW_CL cd1, AW_CL cd2) {
    AWUSE(aw);AWUSE(cd2);
    AED_root *root = (AED_root *)cd1;
    root->ad_main->close();
    exit(0);
}

void AED_window::calculate_size(AW_window *awmm)
{
    AW_device           *device;
    AW_device           *size_device;

    device          = awmm->get_device (AW_MIDDLE_AREA  );
    device->set_filter(AED_F_ALL);
    size_device     = awmm->get_size_device (AW_MIDDLE_AREA  );
    size_device->set_filter(AED_F_ALL);

    AED_area_display_struct display_struct;
    display_struct.clear                = AW_FALSE;
    display_struct.calc_size            = AW_TRUE;
    display_struct.visible_control          = AW_TRUE;
    display_struct.top_indent           = 0;
    display_struct.bottom_indent            = 0;
    display_struct.left_indent          = 0;
    display_struct.slider_pos_horizontal        = 0;
    display_struct.slider_pos_vertical      = 0;
    display_struct.picture_l            = 0;
    display_struct.picture_t            = 0;

    size_device->reset();
    show_bottom_data( size_device, awmm, display_struct );
    size_device->get_size_information( &size_information );
    awmm->set_vertical_scrollbar_bottom_indent( (int)(size_information.b - size_information.t +5.0) );
    size_device->reset();
    show_top_data( size_device, awmm, display_struct );
    size_device->get_size_information( &size_information );
    awmm->set_vertical_scrollbar_top_indent( (int)size_information.b );

    show_middle_data( size_device, awmm, display_struct );
    size_device->get_size_information( &size_information );
    size_information.b += 20;           // show helix at bottom
    awmm->tell_scrolled_picture_size( size_information );

    awmm->calculate_scrollbars();
    this->expose(awmm);
}

// static void aed_weg(AW_window *aw, AW_CL cd1, AW_CL cd2) {
//     AWUSE(aw);AWUSE(cd2);
//     AED_window       *aedw = (AED_window *)cd1;
//     AW_window        *awmm = aedw->aww;


//     if ( aedw->one_area_entry_is_selected ) {

//         aedw->selected_area_entry->in_area->remove_entry( aedw->selected_area_entry );
//         aedw->deselect_area_entry();
//         aedw->calculate_size(awmm);
//     }
// }


/***************************************************************************************************************************/
/***************************************************************************************************************************/
/***************************************************************************************************************************/

void AED_window::select_area_entry( AED_area_entry *area_entry,  AW_pos cursor_position ) {
    selected_area_entry =           area_entry;
    area_entry->is_selected =       AW_TRUE;
    one_area_entry_is_selected =    AW_TRUE;
    if (cursor_position >=0)    cursor =            (int)cursor_position;
    root->aw_root->awar( AWAR_SPECIES_NAME_LOCAL)->write_string( area_entry->ad_species->name() );
    root->aw_root->awar( AWAR_CURSOR_POSITION_LOCAL)->write_int( (int)cursor_position );
}



void AED_window::deselect_area_entry( void ) {
    selected_area_entry->is_selected =  AW_FALSE;
    selected_area_entry =           NULL;
    one_area_entry_is_selected =        AW_FALSE;
}


/***************************************************************************************************************************/
/***************************************************************************************************************************/
/***************************************************************************************************************************/

int AED_window::load_data(void) {
    AED_area_entry *help_area_entry;
    int i = 0;
    AD_SPECIES sp;
    AD_SAI ex;
    root->ad_main->begin_transaction();
    area_top->create_hash(gb_main,"top_area_species");
    area_bottom->create_hash(gb_main,"bottom_area_species");
    sp.init(root->ad_main);
    ex.init(root->ad_main);
    alignment->init(root->ad_main);
    alignment->ddefault();
    if (alignment->eof()) {
        aw_message("Error: Cannot start the EDITOR: Please select a valid alignment","OK");
        exit(-1);
    }

    for (ex.first(); !ex.eof() ;ex.next()) {
        help_area_entry = new AED_area_entry;
        help_area_entry->ad_extended = new AD_SAI;
        help_area_entry->ad_species = (AD_SPECIES *)help_area_entry->ad_extended;
        *(help_area_entry->ad_extended) = ex;
        help_area_entry->ad_container = new AD_CONT;
        help_area_entry->ad_container->init(help_area_entry->ad_extended,(AD_ALI *)alignment);
        if (!help_area_entry->ad_container->eof()) {
            help_area_entry->ad_stat = new AD_STAT ;
            help_area_entry->adt_sequence = new ADT_SEQUENCE;
            help_area_entry->ad_stat->init(help_area_entry->ad_container);
            help_area_entry->adt_sequence->init((ADT_ALI *)alignment,help_area_entry->ad_container);
        }else{
            help_area_entry->ad_stat = 0;
            help_area_entry->adt_sequence = 0;
        }
        const char *name = help_area_entry->ad_species->name();
        if (area_top->read_hash(name)){
            area_top->append(help_area_entry);
        }else   if (area_bottom->read_hash(name)){
            area_bottom->append(help_area_entry);
        }else{
            area_middle->append(help_area_entry);
        }
    }
    ex.exit();

    for (sp.firstmarked(); !sp.eof() ;sp.nextmarked()) {
        help_area_entry = new AED_area_entry;
        help_area_entry->ad_species = new AD_SPECIES;
        help_area_entry->ad_extended = 0;
        *(help_area_entry->ad_species) = sp;
        help_area_entry->ad_container = new AD_CONT;
        help_area_entry->ad_container->init(help_area_entry->ad_species,(AD_ALI *)alignment);
        if (!help_area_entry->ad_container->eof()) {
            help_area_entry->ad_stat = new AD_STAT ;
            help_area_entry->adt_sequence = new ADT_SEQUENCE;
            help_area_entry->ad_stat->init(help_area_entry->ad_container);
            help_area_entry->adt_sequence->init((ADT_ALI *)alignment,help_area_entry->ad_container);
        }else{
            help_area_entry->ad_stat = 0;
            help_area_entry->adt_sequence = 0;
        }
        const char *name = help_area_entry->ad_species->name();
        if (area_top->read_hash(name)){
            area_top->append(help_area_entry);
        }else   if (area_bottom->read_hash(name)){
            area_bottom->append(help_area_entry);
        }else{
            area_middle->append(help_area_entry);
        }
        i++;
        if (i== AED_MAX_SPECIES) {
            char buffer[256];
            sprintf(buffer, " You try to load more than %li Sequences:", AED_MAX_SPECIES);
            if(aw_message(buffer,"LOAD ALL,LOAD 100,EXIT"))
                break;
        }
    }
    sp.exit();
    edg.helix = new AW_helix(root->aw_root);
    const char *err = edg.helix->init(gb_main);
    if (err) aw_message(err);

    edg.ref = new BI_ecoli_ref();
    err = edg.ref->init(gb_main);
    if (err){
        AW_POPUP_HELP(aww,(AW_CL)"ecoliref.hlp");
        aw_message(err);
    }
    area_top->optimize_hash();
    area_bottom->optimize_hash();
    root->ad_main->commit_transaction();

    return i;
}

static void reload_helix(AW_window *aww, AED_window *aedw)
{
    AWUSE(aww);
    const char *err = edg.helix->init((GBDATA *)gb_main);
    if (err)    aw_message(err);
    aedw->expose(aww);
}

static void reload_ref(AW_window *aww)
{
    AWUSE(aww);
    const char *err = edg.ref->init((GBDATA *)gb_main);
    if (err)    aw_message(err);
}
/****************************************************************************************************/

static GB_ERROR species_copy_cb(const char *source, char *dest){
    GB_ERROR error = 0;
    GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    GBDATA *gb_species = GBT_find_species_rel_species_data(gb_species_data,source);
    GBDATA *gb_dest = GBT_find_species_rel_species_data(gb_species_data,dest);

    if (gb_dest) {
        error = "Sorry: species already exists";
    }else   if (gb_species) {
        gb_dest = GB_create_container(gb_species_data,"species");
        error = GB_copy(gb_dest,gb_species);
        if (!error) {
            GBDATA *gb_name =
                GB_search(gb_dest,"name",GB_STRING);
            error = GB_write_string(gb_name,dest);
        }
    }else{
        error = "Please select a species first";
    }

    return error;
}

/** searches a species and returns TRUE if species exists */

static AW_BOOL does_species_exists(char *name) {
    GBDATA *gb_species = GBT_find_species(gb_main,name);
    if (gb_species) return AW_TRUE;
    return AW_FALSE;
}


static void create_new_sequence(AW_window *aww,AED_window *aedw, int do_what)       // dowhat = 0 open old      1: create new       2: copy seque
{
    AED_area_entry *help_area_entry;
    AD_SPECIES sp;
    GB_ERROR error = 0;
    aedw->root->ad_main->begin_transaction();
    char *spname = aww->get_root()->awar(AWAR_SPECIES_DEST)->read_string();

    do {
        switch (do_what){
            case 2:
                if (!aedw->selected_area_entry) {
                    error = "Error: Please select a sequence first";
                }else{
                    const char *source = aedw->selected_area_entry->ad_species->name();
                    error = species_copy_cb(source,spname);
                }
                break;
            default:
                if (does_species_exists(spname)) {
                    if (do_what == 1) error = GB_export_error("Species %s already exists",spname);
                }else{
                    if (do_what == 0) error = GB_export_error("Cannot find species %s",spname);
                }
        }
        if (error) break;

        sp.init(aedw->root->ad_main);
        AD_ERR *ad_err = sp.create(spname);
        if (!ad_err) {
            help_area_entry = new AED_area_entry;
            help_area_entry->ad_species = new AD_SPECIES;
            help_area_entry->ad_extended = 0;
            *(help_area_entry->ad_species) = sp;
            help_area_entry->ad_container = new AD_CONT;
            if (do_what != 1) {
                help_area_entry->ad_container->init(help_area_entry->ad_species,
                                                    (AD_ALI *)aedw->alignment);
            }else{
                help_area_entry->ad_container->create(help_area_entry->ad_species,
                                                      (AD_ALI *)aedw->alignment);
            }
            if (!help_area_entry->ad_container->eof()) {
                help_area_entry->ad_stat = new AD_STAT ;
                help_area_entry->adt_sequence = new ADT_SEQUENCE;
                help_area_entry->ad_stat->init(help_area_entry->ad_container);
                help_area_entry->adt_sequence->init((ADT_ALI *)aedw->alignment,help_area_entry->ad_container);
            }else{
                help_area_entry->ad_stat = 0;
                help_area_entry->adt_sequence = 0;
            }
            help_area_entry->in_area = aedw->area_top;
            aedw->area_top->append(help_area_entry);
        }else{
            aw_message(ad_err->show());
            delete ad_err;
        }
        sp.exit();
    } while (0);
    if (error){
        aedw->root->ad_main->abort_transaction();
        aw_message(error);
    }else{
        aedw->root->ad_main->commit_transaction();
        aww->get_root()->awar(AWAR_SPECIES_DEST)->write_string("");
        aww->hide();
    }

    aedw->calculate_size(aedw->aww);
}


static AW_window *create_new_seq_window(AW_root *root, AED_window *aedw)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "CREATE_SEQUENCE","CREATE SEQUENCE");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the name\nof the new species");

    aws->at("input");
    aws->create_input_field(AWAR_SPECIES_DEST,15);

    aws->at("ok");
    aws->callback((AW_CB)create_new_sequence,(AW_CL)aedw,1);
    aws->create_button("GO","GO","g");

    return (AW_window *)aws;
}

static AW_window *create_old_seq_window(AW_root *root, AED_window *aedw)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "OPEN_SEQUENCE", "OPEN SEQUENCE");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the name\nof the existing species");

    aws->at("input");
    aws->create_input_field(AWAR_SPECIES_DEST,15);

    aws->at("ok");
    aws->callback((AW_CB)create_new_sequence,(AW_CL)aedw,0);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

static AW_window *create_new_copy_window(AW_root *root, AED_window *aedw)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "COPY_SELECTED_SEQUENCE", "COPY SELECTED SEQUENCE");
    aws->load_xfig("ad_al_si.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE","C");

    aws->at("label");
    aws->create_autosize_button(0,"Please enter the name\nof the new sequence");

    aws->at("input");
    aws->create_input_field(AWAR_SPECIES_DEST,15);

    aws->at("ok");
    aws->callback((AW_CB)create_new_sequence,(AW_CL)aedw,2);
    aws->create_button("GO","GO","G");

    return (AW_window *)aws;
}

/****************************************************************************************************/
/****************************************************************************************************/

void AED_window::make_left_text( char *string, AED_area_entry *area_entry ) {
    AED_left_side *current_entry_of_dlist;
    char tmp[100];
    char left_text[200];

    left_text[0] = '\0';

    current_entry_of_dlist = show_dlist_left_side->first;
    while ( current_entry_of_dlist ) {
        tmp[0] = '\0';
        current_entry_of_dlist->make_text( this, area_entry, tmp );
        strcat( left_text, tmp );
        current_entry_of_dlist = current_entry_of_dlist->next;
    }

    sprintf( string, "%s", left_text );

}



void AED_window::show_top_data(AW_device *device, AW_window *awmm, AED_area_display_struct& display_struct ) {
    AW_pos y = 0;

    area_top->current = area_top->first;
    while (area_top->current != NULL) {
        this->show_single_top_data( device, awmm, area_top->current, display_struct, &y );
        area_top->current = area_top->current->next;
    }
    if ( area_top->first )
        device->invisible( 1, 0, y+5, AED_F_NAME | AED_F_SEQUENCE, (AW_CL)0, (AW_CL)0 );
}


void AED_window::show_single_top_data(AW_device *device, AW_window *awmm, AED_area_entry *area_entry, AED_area_display_struct& display_struct, AW_pos *y ) {
    AWUSE(awmm);
    AW_pos width;
    AW_pos height;
    AW_rectangle screen;
    char left_text[100];
    AW_BOOL tmp;

    const AW_font_information *font_information = device->get_font_information(AED_GC_SEQUENCE, 'A');
    device->get_area_size( &screen );

    if ( display_struct.calc_size ) {
        *y += AED_TOP_LINE + font_information->max_letter.ascent;
        area_entry->absolut_x = 2;
        area_entry->absolut_y = *y;
    }


    if ( !quickdraw ) {

        // display species names on the left side
        device->push_clip_scale();
        device->shift_x( 0 );
        device->shift_y( 0 );
        device->set_bottom_clip_border( display_struct.top_indent );
        device->set_right_clip_border( display_struct.left_indent );

        device->invisible( AED_GC_NAME, 0, 0, AED_F_NAME | AED_F_SEQUENCE, (AW_CL)0 ,(AW_CL)0 );


        this->make_left_text( left_text, area_entry );

        if( !area_entry->is_selected ) {
            if ( display_struct.clear ) {
                width  = strlen( left_text ) * font_information->max_letter.width + 4;
                height = font_information->max_letter.height + 2;
                device->clear_part(  0, *y - font_information->max_letter.ascent - 1, width+1, height+1, AED_F_NAME);
            }
            device->text( AED_GC_NAME, left_text,
                          4, *y, 0.0, AED_F_NAME, (AW_CL)area_entry, AED_F_NAME );
        }
        else {
            AW_pos help_y = *y + font_information->max_letter.descent + 1;

            width  = strlen( left_text ) * font_information->max_letter.width + 4;
            height = font_information->max_letter.height + 2;

            if ( display_struct.clear )
                device->clear_part(  0, *y - font_information->max_letter.ascent - 1, width+1, height+1, AED_F_ALL);

            if ( (tmp  = (AW_BOOL)device->text( AED_GC_SELECTED, left_text, 4, *y, 0.0,AED_F_NAME, (AW_CL)area_entry, AED_F_NAME )) ) {
                device->line( AED_GC_SELECTED, 2,       help_y,        2+width,  help_y,         AED_F_ALL, (AW_CL)"box", 0 );     // unten
                device->line( AED_GC_SELECTED, 2,       help_y,        2,        help_y-height,  AED_F_ALL, (AW_CL)"box", 0 );     // links
                device->line( AED_GC_SELECTED, 2+width, help_y,        2+width,  help_y-height,  AED_F_ALL, (AW_CL)"box", 0 );     // rechts
                device->line( AED_GC_SELECTED, 2,       help_y-height, 2+width,  help_y-height,  AED_F_ALL, (AW_CL)"box", 0 );     // oben
            }
            if ( display_struct.visible_control ) {
                selected_area_entry_is_visible = tmp;
            }
        }
        device->pop_clip_scale();
    }


    // display sequences of the species on the right side
    device->push_clip_scale();
    device->shift_x( -(display_struct.picture_l + display_struct.slider_pos_horizontal) );
    device->shift_dx( display_struct.left_indent );
    device->shift_y( 0 );

    device->set_bottom_clip_border( display_struct.top_indent );
    device->set_left_clip_border( display_struct.left_indent );
    if ( quickdraw ) {
        device->set_left_clip_border( quickdraw_left_indent );
        device->set_right_clip_border( quickdraw_right_indent );
    }

    if ( display_struct.clear ) {
        width  = screen.r;
        height = font_information->max_letter.height+AED_LINE_SPACING+AED_TOP_LINE;
        device->clear_part((display_struct.picture_l + display_struct.slider_pos_horizontal),
                           *y - font_information->max_letter.ascent - 1,
                           width+1,
                           height+1, AED_F_ALL);
    }
    if (area_entry->adt_sequence) {

        if (aed_root.helix_at_extendeds || !area_entry->ad_extended) {
            edg.helix->show_helix( (void*)device, AED_GC_HELIX ,
                                   area_entry->adt_sequence->show_get(), 0.0,
                                   *y + font_information->max_letter.ascent + AED_CENTER_SPACING,
                                   AED_F_HELIX, (AW_CL)area_entry, AED_F_HELIX);
        }
        device->text(   AED_GC_SEQUENCE, area_entry->adt_sequence->show_get(), 0, *y, 0.0,
                        AED_F_SEQUENCE, (AW_CL)area_entry, AED_F_SEQUENCE, area_entry->adt_sequence->show_len() );  }
    device->pop_clip_scale();

    if ( display_struct.calc_size ) {
        area_entry->in_line = *y;
        *y += AED_LINE_SPACING + font_information->max_letter.descent;
    }

    device->invisible( AED_GC_NAME, 0, *y, AED_F_NAME | AED_F_SEQUENCE, (AW_CL)0, (AW_CL)0 );
}


void AED_window::show_middle_data(AW_device *device, AW_window *awmm, AED_area_display_struct& display_struct ) {
    AW_pos y = 0;

    area_middle->current = area_middle->first;
    while (area_middle->current != NULL) {
        this->show_single_middle_data(device, awmm, area_middle->current, display_struct, &y );
        area_middle->current = area_middle->current->next;
    }
}

// @@@@@
static int AED_show_colored_sequence(AW_device *device, int gc, const char *opt_string, size_t /*opt_strlen*/,size_t start, size_t size,
                                     AW_pos x,AW_pos y, AW_pos /*opt_ascent*/,AW_pos /*opt_descent*/,
                                     AW_CL cduser, AW_CL cd1, AW_CL cd2){
    char *sname = (char *)cduser;   // The name of the species
    ST_ML_Color *colors = st_ml_get_color_string(   st_ml, sname, 0, start, start+size );
    if (colors) {
        const AW_font_information *font_information = device->get_font_information( AED_GC_SEQUENCE, 'A' );
        int    i;
        long   len       = start+size;
        AW_pos height    = font_information->max_letter.ascent;
        AW_pos width     = font_information->max_letter.width;
        AW_pos x2        = x;
        AW_pos y2        = y - height;
        int    old_color = AED_GC_0;
        int    color     = AED_GC_0;

        for ( x2 = x, i = start; i < len; i++,x2 += width) {
            color = colors[i] + AED_GC_0;
            if (color > AED_GC_9) color = AED_GC_9;
            if (color != old_color) {   // draw till oldcolor
                device->box(old_color,x, y2, x2-x, height, -1, cd1,cd2);
                x = x2;
                old_color = color;
            }
        }
        device->box(color,x, y2, x2-x, height, -1, cd1,cd2);
    }
    return device->text(gc, opt_string,0,y,0.0, -1,cd1,cd2,size+start);
}


void AED_window::show_single_middle_data(AW_device *device, AW_window *awmm, AED_area_entry *area_entry, AED_area_display_struct& display_struct, AW_pos *y ) {
    AWUSE(awmm);
    AW_pos width;
    AW_pos height;
    AW_rectangle screen;
    char left_text[100];
    AW_BOOL tmp;

    const AW_font_information *font_information = device->get_font_information( AED_GC_SEQUENCE, 'A' );
    device->get_area_size( &screen );

    if ( display_struct.calc_size ) {
        *y += AED_TOP_LINE + font_information->max_letter.ascent;
        area_entry->absolut_x = 2;
        area_entry->absolut_y = *y - (display_struct.picture_t + display_struct.slider_pos_vertical) + display_struct.top_indent;
    }

    if ( !quickdraw ) {

        // display species names on the left side
        device->push_clip_scale();
        device->shift_x( 0 );
        device->shift_y( -(display_struct.picture_t + display_struct.slider_pos_vertical) );
        device->shift_dy( display_struct.top_indent );
        device->set_top_clip_border( display_struct.top_indent );
        device->set_bottom_clip_margin( display_struct.bottom_indent );
        device->set_right_clip_border( display_struct.left_indent );


        device->invisible(AED_GC_NAME, 0, 0, AED_F_NAME, (AW_CL)0, (AW_CL)0 );


        this->make_left_text( left_text, area_entry );

        if( !area_entry->is_selected ) {
            if ( display_struct.clear ) {
                width  = strlen( left_text ) * font_information->max_letter.width + 4;
                height = font_information->max_letter.height + 2;
                device->clear_part(  2, *y - font_information->max_letter.ascent - 1, width+1, height+1, AED_F_NAME);
            }
            device->text( AED_GC_NAME, left_text, 4, *y, 0.0, AED_F_NAME, (AW_CL)area_entry, AED_F_NAME );
        }
        else {
            AW_pos help_y = *y + font_information->max_letter.descent + 1;

            width  = strlen( left_text ) * font_information->max_letter.width + 4;
            height = font_information->max_letter.height + 2;

            if ( display_struct.clear )
                device->clear_part(  2, *y - font_information->max_letter.ascent - 1, width+1, height+1, AED_F_ALL);

            if ( (tmp = (AW_BOOL)device->text( AED_GC_SELECTED, left_text, 4, *y, 0.0,AED_F_NAME, (AW_CL)area_entry, AED_F_NAME ))  ) {
                device->line( AED_GC_SELECTED, 2,       help_y,        2+width,  help_y,         AED_F_ALL, (AW_CL)"box", 0 );     // unten
                device->line( AED_GC_SELECTED, 2,       help_y,        2,        help_y-height,  AED_F_ALL, (AW_CL)"box", 0 );     // links
                device->line( AED_GC_SELECTED, 2+width, help_y,        2+width,  help_y-height,  AED_F_ALL, (AW_CL)"box", 0 );     // rechts
                device->line( AED_GC_SELECTED, 2,       help_y-height, 2+width,  help_y-height,  AED_F_ALL, (AW_CL)"box", 0 );     // oben
            }
            if ( display_struct.visible_control ) {
                selected_area_entry_is_visible = tmp;
            }
        }
        device->pop_clip_scale();
    }


    // display sequences of the species on the right side
    device->push_clip_scale();
    device->shift_x( -(display_struct.picture_l + display_struct.slider_pos_horizontal) );
    device->shift_dx( display_struct.left_indent );
    device->shift_y( -(display_struct.picture_t + display_struct.slider_pos_vertical) );
    device->shift_dy( display_struct.top_indent );
    device->set_top_clip_border( display_struct.top_indent );
    device->set_bottom_clip_margin( display_struct.bottom_indent );
    device->set_left_clip_border( display_struct.left_indent );
    if ( quickdraw ) {
        device->set_left_clip_border( quickdraw_left_indent );
        device->set_right_clip_border( quickdraw_right_indent );
    }


    device->invisible( AED_GC_NAME, 0, 0, AED_F_SEQUENCE, (AW_CL)0, (AW_CL)0 );
    if ( display_struct.clear ) {
        width  = screen.r;
        height = font_information->max_letter.height+AED_LINE_SPACING+AED_TOP_LINE;

        device->clear_part(  (display_struct.picture_l + display_struct.slider_pos_horizontal), *y - font_information->max_letter.ascent - 1, width+1, height+1, AED_F_ALL);
    }

    if (area_entry->adt_sequence) {
        if (aed_root.helix_at_extendeds || !area_entry->ad_extended) {
            edg.helix->show_helix( (void*)device, AED_GC_HELIX ,
                                   area_entry->adt_sequence->show_get(), 0.0,
                                   *y + font_information->max_letter.ascent + AED_CENTER_SPACING,
                                   AED_F_HELIX, (AW_CL)area_entry, AED_F_HELIX);
        }
        if (st_ml && st_is_inited(st_ml) && device->type() == AW_DEVICE_SCREEN ){
            device->text_overlay(   AED_GC_SEQUENCE, area_entry->adt_sequence->show_get(), area_entry->adt_sequence->show_len(),
                                    0, *y, 0.0,     // x, y, alignment
                                    AED_F_SEQUENCE,     // filter
                                    (AW_CL)area_entry->ad_species->name(),  // cl for AED_show_colored_sequence
                                    (AW_CL)area_entry, AED_F_SEQUENCE,
                                    0,0,                    // use font to get ascent/descent
                                    AED_show_colored_sequence);     // the real drawing function  );
        }else{
            device->text(   AED_GC_SEQUENCE, area_entry->adt_sequence->show_get(), 0, *y, 0.0,
                            AED_F_SEQUENCE, (AW_CL)area_entry, AED_F_SEQUENCE, area_entry->adt_sequence->show_len() );
        }
    }


    if ( display_struct.calc_size ) {
        area_entry->in_line = *y;
        *y += AED_LINE_SPACING + font_information->max_letter.descent;
        AW_pos help;
        help = ( alignment->len()+50) * font_information->max_letter.width;
        //      printf("width %f\n",help);
        device->invisible( AED_GC_NAME, help, *y, AED_F_ALL, (AW_CL)0, (AW_CL)0 );
    }


    device->pop_clip_scale();
}


void AED_window::show_bottom_data(AW_device *device, AW_window *awmm, AED_area_display_struct& display_struct ) {
    AW_pos y = 0;

    area_bottom->current = area_bottom->first;
    while ( area_bottom->current != NULL ) {
        this->show_single_bottom_data( device, awmm, area_bottom->current, display_struct, &y );
        area_bottom->current = area_bottom->current->next;
    }
}


void AED_window::show_single_bottom_data(AW_device *device, AW_window *awmm, AED_area_entry *area_entry, AED_area_display_struct& display_struct, AW_pos *y ) {
    AWUSE(awmm);
    AW_pos width;
    AW_pos height;
    AW_rectangle screen;
    char left_text[100];
    AW_BOOL tmp;

    const AW_font_information *font_information = device->get_font_information(AED_GC_SEQUENCE, 'A');
    device->get_area_size( &screen );

    if ( display_struct.calc_size ) {
        *y += AED_TOP_LINE + font_information->max_letter.ascent;
        area_entry->absolut_x = 2;
        area_entry->absolut_y = *y + screen.b - display_struct.bottom_indent;
    }


    if ( !quickdraw ) {

        // display species names on the left side
        device->push_clip_scale();
        device->shift_x( 0 );
        device->shift_y( screen.b - display_struct.bottom_indent );
        //  device->set_top_clip_border( screen.b - display_struct.bottom_indent );
        device->set_right_clip_border( display_struct.left_indent );

        device->invisible( AED_GC_NAME, 0, 0, AED_F_ALL, (AW_CL)0, (AW_CL)0 );

        this->make_left_text( left_text, area_entry );

        if( !area_entry->is_selected ) {
            if ( display_struct.clear ) {
                width = strlen( left_text ) * font_information->max_letter.width + 4;
                height = font_information->max_letter.height + 2;
                device->clear_part(  2, *y - font_information->max_letter.ascent - 1, width+1, height+1, AED_F_NAME);
            }
            device->text( AED_GC_NAME, left_text, 4, *y , 0.0, AED_F_NAME, (AW_CL)area_entry, AED_F_NAME );
        }
        else {
            AW_pos help_y = *y + font_information->max_letter.descent + 1;

            width = strlen( left_text ) * font_information->max_letter.width + 4;
            height = font_information->max_letter.height + 2;

            if ( display_struct.clear )
                device->clear_part(  2, *y - font_information->max_letter.ascent - 1, width+1, height+1, AED_F_ALL);

            if ( (tmp = (AW_BOOL)device->text( AED_GC_SELECTED, left_text, 4, *y, 0.0,AED_F_NAME, (AW_CL)area_entry, AED_F_NAME )) ) {
                device->line( AED_GC_SELECTED, 2,       help_y,        2+width,  help_y,         AED_F_ALL, (AW_CL)"box", 0 );     // unten
                device->line( AED_GC_SELECTED, 2,       help_y,        2,        help_y-height,  AED_F_ALL, (AW_CL)"box", 0 );     // links
                device->line( AED_GC_SELECTED, 2+width, help_y,        2+width,  help_y-height,  AED_F_ALL, (AW_CL)"box", 0 );     // rechts
                device->line( AED_GC_SELECTED, 2,       help_y-height, 2+width,  help_y-height,  AED_F_ALL, (AW_CL)"box", 0 );     // oben
            }
            if ( display_struct.visible_control ) {
                selected_area_entry_is_visible = tmp;
            }
        }
        device->pop_clip_scale();
    }


    // display sequences of the species on the right side
    device->push_clip_scale();
    device->shift_x( -(display_struct.picture_l + display_struct.slider_pos_horizontal) );
    device->shift_dx( display_struct.left_indent );
    device->shift_y( screen.b - display_struct.bottom_indent );
    //  device->set_top_clip_border( screen.b - display_struct.bottom_indent );
    device->set_left_clip_border( display_struct.left_indent );
    if ( quickdraw ) {
        device->set_left_clip_border( quickdraw_left_indent );
        device->set_right_clip_border( quickdraw_right_indent );
    }

    if ( display_struct.clear ) {
        width = screen.r;
        height = font_information->max_letter.height+AED_LINE_SPACING+AED_TOP_LINE ;
        device->clear_part(  (display_struct.picture_l + display_struct.slider_pos_horizontal), *y - font_information->max_letter.ascent - 1, width+1, height+1, AED_F_ALL);
    }
    if (area_entry->adt_sequence) {

        if (aed_root.helix_at_extendeds || !area_entry->ad_extended) {
            edg.helix->show_helix( (void*)device, AED_GC_HELIX ,
                                   area_entry->adt_sequence->show_get(), 0.0,
                                   *y + font_information->max_letter.ascent + AED_CENTER_SPACING,
                                   AED_F_HELIX, (AW_CL)area_entry, AED_F_HELIX);
        }
        device->text(   AED_GC_SEQUENCE, area_entry->adt_sequence->show_get(), 0, *y, 0.0,
                        AED_F_SEQUENCE, (AW_CL)area_entry, AED_F_SEQUENCE, area_entry->adt_sequence->show_len() );
    }

    if ( display_struct.calc_size ) {
        area_entry->in_line = *y;
        *y += AED_LINE_SPACING + font_information->max_letter.descent;
    }

    device->invisible( AED_GC_NAME, 0, *y, AED_F_ALL, (AW_CL)0, (AW_CL)0 );
    device->pop_clip_scale();
}


void AED_window::show_data( AW_device *device, AW_window *awmm, AW_BOOL visibility_control ) {
    AW_rectangle screen;

    AED_area_display_struct display_struct;
    display_struct.clear                = AW_FALSE;
    display_struct.calc_size            = AW_TRUE;
    display_struct.visible_control          = visibility_control;
    display_struct.top_indent           = awmm->top_indent_of_vertical_scrollbar;
    display_struct.bottom_indent            = awmm->bottom_indent_of_vertical_scrollbar;
    display_struct.left_indent          = awmm->left_indent_of_horizontal_scrollbar;
    display_struct.slider_pos_horizontal        = awmm->slider_pos_horizontal;
    display_struct.slider_pos_vertical      = awmm->slider_pos_vertical;
    display_struct.picture_l            = awmm->picture->l;
    display_struct.picture_t            = awmm->picture->t;

    device->get_area_size( &screen );
    GB_transaction dummy(gb_main);
    this->show_top_data( device, awmm, display_struct );
    this->show_middle_data( device, awmm, display_struct );
    this->show_bottom_data( device, awmm, display_struct );

    device->get_area_size( &screen );
    device->shift_y( 0 );
    device->shift_x( 0 );
    if ( area_top )
        device->line( AED_GC_NAME, screen.l, awmm->top_indent_of_vertical_scrollbar-2, screen.r, awmm->top_indent_of_vertical_scrollbar-2, AED_F_NAME, (AW_CL)"Loben", 0 );
    if ( area_bottom )
        device->line( AED_GC_NAME, screen.l, screen.b-awmm->bottom_indent_of_vertical_scrollbar+1, screen.r,
                      screen.b-awmm->bottom_indent_of_vertical_scrollbar+1, AED_F_NAME, (AW_CL)"Lunten", 0 );

} // end: AED_window::show_data

void AED_window::expose( AW_window *awmm ) {
    AW_device   *device;

    device = awmm->get_device (AW_MIDDLE_AREA  );
    device->reset();
    device->set_filter(AED_F_ALL);
    device->clear(AED_F_ALL);
    this->show_data( device, awmm, AW_TRUE );

    if ( this->cursor_is_managed ) {
        this->manage_cursor( device, awmm, AW_FALSE );
    }

} // end: aed_expose

void AED_undo_cb(AW_window *aw, AW_CL cd1, AW_CL undo_type){
    GB_ERROR error = GB_undo(gb_main,(GB_UNDO_TYPE)undo_type);
    if (error) aw_message(error);
    else{
        GB_begin_transaction(gb_main);
        GB_commit_transaction(gb_main);
        aed_expose(aw,cd1,0);
    }
}



void aed_expose( AW_window *aw, AW_CL cd1, AW_CL cd2 ) {
    AWUSE(cd2);
    AED_window  *aedw = (AED_window *)cd1;
    aedw->expose(aw);
} // end: aed_expose


void aed_resize( AW_window *dummy, AW_CL cd1, AW_CL cd2 ) {
    AWUSE(cd2);
    AWUSE(dummy);
    AW_device               *device;
    AED_window              *aedw = (AED_window *)cd1;
    AW_window   *aw = aedw->aww;

    device = aw->get_device (AW_MIDDLE_AREA  );
    device->set_filter(AED_F_ALL);

    char    left_text[200];
    aedw->make_left_text( left_text, NULL );
    aw->set_horizontal_scrollbar_left_indent( (int)(device->get_string_size( AED_GC_NAME, left_text,0 )) + 20 );

    aw->calculate_scrollbars();
    aedw->expose( aw );


} // end: aed_resize


static void aed_horizontal( AW_window *aw, AW_CL cd1, AW_CL cd2 ) {
    AW_device               *device;
    AW_device               *info_device;
    AED_window              *aedw = (AED_window *)cd1;
    AW_BOOL                 handle_cursor = (AW_BOOL)cd2;
    AW_window   *awmm = aedw->aww;
    AW_rectangle            screen;
    AW_pos                  src_x;
    AW_pos                  src_y;
    AW_pos                  width;
    AW_pos                  height;
    AW_pos                  dest_x;
    AW_pos                  dest_y;

    device = aw->get_device (AW_MIDDLE_AREA  );
    device->reset();
    device->set_filter(AED_F_ALL);
    info_device = aw->get_device (AW_INFO_AREA  );
    device->reset();
    info_device->set_filter(AED_F_ALL);
    device->get_area_size( &screen );

    if ( handle_cursor && aedw->cursor_is_managed ) {
        aedw->manage_cursor( device, awmm, AW_TRUE );
    }

    src_x = awmm->left_indent_of_horizontal_scrollbar + (AW_pos)awmm->slider_pos_horizontal - aedw->last_slider_position;

    if ( src_x < screen.r ) {
        if ( (AW_pos)awmm->slider_pos_horizontal > aedw->last_slider_position ) {      // nach rechts gescrollt
            aedw->quickdraw = AW_TRUE;
            src_y   = 0;
            width       = screen.r - awmm->left_indent_of_horizontal_scrollbar;
            height  = screen.b;
            dest_x  = awmm->left_indent_of_horizontal_scrollbar;
            dest_y  = 0;

            aedw->quickdraw_left_indent = screen.r - ( awmm->slider_pos_horizontal - aedw->last_slider_position ) - 10;
            if ( aedw->quickdraw_left_indent < awmm->left_indent_of_horizontal_scrollbar ) {
                aedw->quickdraw_left_indent = awmm->left_indent_of_horizontal_scrollbar;
            }
            aedw->quickdraw_right_indent = screen.r;

            device->move_region( src_x , src_y, width, height, dest_x, dest_y );
            aedw->show_data( device, awmm, AW_TRUE );
            aedw->quickdraw = AW_FALSE;
        }
        if ( (AW_pos)awmm->slider_pos_horizontal < aedw->last_slider_position ) {      // nach links gescrollt

            aedw->quickdraw = AW_TRUE;

            src_x   = awmm->left_indent_of_horizontal_scrollbar;
            src_y   = 0;
            width       = screen.r - awmm->left_indent_of_horizontal_scrollbar;
            height  = screen.b;
            dest_x  = awmm->left_indent_of_horizontal_scrollbar + aedw->last_slider_position - (AW_pos)awmm->slider_pos_horizontal;
            dest_y  = 0;

            device->move_region( src_x , src_y, width, height, dest_x, dest_y );

            aedw->quickdraw_left_indent = awmm->left_indent_of_horizontal_scrollbar;
            aedw->quickdraw_right_indent = awmm->left_indent_of_horizontal_scrollbar - awmm->slider_pos_horizontal + aedw->last_slider_position + 10;
            device->shift_x( 0 );
            device->shift_y( 0 );
            device->clear_part(  awmm->left_indent_of_horizontal_scrollbar, 0, aedw->last_slider_position - (AW_pos)awmm->slider_pos_horizontal, screen.b, AED_F_ALL);
            aedw->show_data( device, awmm, AW_TRUE );
            aedw->quickdraw = AW_FALSE;
        }
    }
    else {  // mehr als eine Seite gescrollt
        aedw->quickdraw = AW_TRUE;
        device->shift_x( 0 );
        device->shift_y( 0 );
        device->clear_part(  awmm->left_indent_of_horizontal_scrollbar, 0, screen.r - awmm->left_indent_of_horizontal_scrollbar, screen.b, AED_F_ALL);
        aedw->quickdraw_left_indent = awmm->left_indent_of_horizontal_scrollbar;
        aedw->quickdraw_right_indent = screen.r;
        aedw->show_data( device, awmm, AW_TRUE );
        aedw->quickdraw = AW_FALSE;
    }

    aedw->last_slider_position = awmm->slider_pos_horizontal;

    if ( handle_cursor && aedw->cursor_is_managed ) {
        aedw->cursor_is_managed = aedw->manage_cursor( device, awmm, AW_FALSE );
    }


} // end: aed_horizontal



static void aed_vertical( AW_window *aw, AW_CL cd1, AW_CL cd2 ) {
    AED_window              *aedw = (AED_window *)cd1;
    AW_window   *awmm = aedw->aww;
    AW_BOOL                 handle_cursor = (AW_BOOL)cd2;
    AW_device               *device;
    AW_device               *info_device;
    AW_rectangle            screen;
    GB_transaction dummy(gb_main);

    device = aw->get_device (AW_MIDDLE_AREA  );
    device->reset();
    device->set_filter(AED_F_ALL);
    info_device = aw->get_device (AW_INFO_AREA  );
    info_device->reset();
    info_device->set_filter(AED_F_ALL);
    device->get_area_size( &screen );

    if ( handle_cursor && aedw->cursor_is_managed && aedw->selected_area_entry_is_visible ) {
        aedw->manage_cursor( device, awmm, AW_FALSE );
    }

    device->shift_x( 0 );
    device->shift_y( 0 );

    device->clear_part( 0, awmm->top_indent_of_vertical_scrollbar,
                        screen.r, screen.b - awmm->top_indent_of_vertical_scrollbar - awmm->bottom_indent_of_vertical_scrollbar, AED_F_ALL);

    AED_area_display_struct display_struct;
    display_struct.clear                        = AW_FALSE;
    display_struct.calc_size                    = AW_TRUE;
    display_struct.visible_control                  = AW_TRUE;
    display_struct.top_indent                   = awmm->top_indent_of_vertical_scrollbar;
    display_struct.bottom_indent                    = awmm->bottom_indent_of_vertical_scrollbar;
    display_struct.left_indent                  = awmm->left_indent_of_horizontal_scrollbar;
    display_struct.slider_pos_horizontal                = awmm->slider_pos_horizontal;
    display_struct.slider_pos_vertical              = awmm->slider_pos_vertical;
    display_struct.picture_l                    = awmm->picture->l;
    display_struct.picture_t                    = awmm->picture->t;

    aedw->show_middle_data( device, awmm, display_struct );

    if ( handle_cursor && aedw->cursor_is_managed && aedw->selected_area_entry_is_visible ) {
        aedw->manage_cursor( device, awmm, AW_FALSE );
    }


} // end: aed_vertical



void AED_window::hide_cursor( AW_device *device, AW_window *awmm ) {

    if ( cursor_is_managed ) {
        this->manage_cursor( device, awmm, AW_FALSE );
    }

} // end: AED_window::hide_cursor



void AED_window::show_cursor( AW_device *device, AW_window *awmm ) {

    if ( cursor_is_managed ) {
        this->manage_cursor( device, awmm, AW_FALSE );
    }

} // end: AED_window::show_cursor



AW_BOOL AED_window::manage_cursor( AW_device *device, AW_window *awmm, AW_BOOL use_last_slider_position ) {
    AW_rectangle screen;
    AW_pos       slider_position_horizontal;
    AW_BOOL      cursor_drawn;
    
    if (!selected_area_entry) return AW_FALSE;
    const AW_font_information *font_information = device->get_font_information( AED_GC_SEQUENCE, 'A' );
    if ( use_last_slider_position ) {
        slider_position_horizontal = last_slider_position;
    }
    else {
        slider_position_horizontal = awmm->slider_pos_horizontal;
    }

    device->push_clip_scale();
    device->shift_x( -( awmm->picture->l + slider_position_horizontal ) );
    device->shift_dx( awmm->left_indent_of_horizontal_scrollbar );
    device->set_left_clip_border(awmm->left_indent_of_horizontal_scrollbar);

    if ( selected_area_entry->in_area == area_top ) {
        device->shift_y( 0 );
        device->set_bottom_clip_border( awmm->top_indent_of_vertical_scrollbar );
        device->set_left_clip_border( awmm->left_indent_of_horizontal_scrollbar );
    }

    if ( selected_area_entry->in_area == area_middle ) {
        device->shift_y( awmm->top_indent_of_vertical_scrollbar );
        device->shift_dy( -(awmm->picture->t + awmm->slider_pos_vertical ) );
        device->set_top_clip_border( awmm->top_indent_of_vertical_scrollbar );
        device->set_bottom_clip_margin( awmm->bottom_indent_of_vertical_scrollbar );
    }

    if ( selected_area_entry->in_area == area_bottom ) {
        device->get_area_size(&screen);
        device->shift_y( screen.b - awmm->bottom_indent_of_vertical_scrollbar );
        device->set_top_clip_border( screen.b - awmm->bottom_indent_of_vertical_scrollbar );
    }
    cursor_drawn = (AW_BOOL)device->cursor( AED_GC_NAME_DRAG, cursor * font_information->max_letter.width, selected_area_entry->in_line, AW_cursor_insert, AED_F_CURSOR, 0, 0 );
    device->pop_clip_scale();
    root->aw_root->awar( AWAR_CURSOR_POSITION_LOCAL)->write_int( cursor );

    return cursor_drawn;

} // end: AED_window::manage_cursor



void AED_window::show_single_area_entry( AW_device *device, AW_window *awmm, AED_area_entry *area_entry ) {
    AW_pos y = area_entry->in_line;
    GB_transaction dummy(gb_main);
    AED_area_display_struct display_struct;
    display_struct.clear                = AW_TRUE;
    display_struct.calc_size            = AW_FALSE;
    display_struct.visible_control          = AW_TRUE;
    display_struct.top_indent           = awmm->top_indent_of_vertical_scrollbar;
    display_struct.bottom_indent            = awmm->bottom_indent_of_vertical_scrollbar;
    display_struct.left_indent          = awmm->left_indent_of_horizontal_scrollbar;
    display_struct.slider_pos_horizontal        = awmm->slider_pos_horizontal;
    display_struct.slider_pos_vertical      = awmm->slider_pos_vertical;
    display_struct.picture_l            = awmm->picture->l;
    display_struct.picture_t            = awmm->picture->t;

    if( area_entry->in_area == area_top ) {
        this->show_single_top_data( device, awmm, area_entry, display_struct, &y );
    }

    if( area_entry->in_area == area_middle ) {
        this->show_single_middle_data( device, awmm, area_entry, display_struct, &y );
    }

    if( area_entry->in_area == area_bottom ) {
        this->show_single_bottom_data( device, awmm, area_entry, display_struct, &y );
    }

} // end: AED_window::show_single_area_entry



void drag_box(AW_device *device, int gc, AW_pos x, AW_pos y, AW_pos width, AW_pos height, char *str) {
    if( width == 0 ) {
        const AW_font_information *font_information = device->get_font_information( AED_GC_SELECTED_DRAG, 'A' );

        width  = strlen( str ) * font_information->max_letter.width + 4;
        height = font_information->max_letter.height + 4;
    }
    AW_pos y_help = y + 2;
    device->line( gc, x,       y_help,        x+width, y_help,        AED_F_ALL,(AW_CL)"drag_box",0);                       // unten
    device->line( gc, x,       y_help,        x,       y_help-height, AED_F_ALL,(AW_CL)"drag_box",0);                   // links
    device->line( gc, x+width, y_help,        x+width, y_help-height, AED_F_ALL,(AW_CL)"drag_box",0);   // rechts
    device->line( gc, x,       y_help-height, x+width, y_help-height, AED_F_ALL,(AW_CL)"drag_box",0);   // oben
    device->text( gc, str, x+2, y-2, 0, AED_F_ALL, (AW_CL)"drag_box", 0 );                      // Text
}



void AED_dlist::remove_entry(AED_area_entry *to_be_removed) {
    if ( first == to_be_removed ) {
        if(to_be_removed->next) {
            first = to_be_removed->next;
            to_be_removed->next->previous = NULL;
        }
        else
            first = NULL;
    }
    else {
        to_be_removed->previous->next = to_be_removed->next;
        if(to_be_removed->next)
            to_be_removed->next->previous = to_be_removed->previous;
    }
    if( last == to_be_removed ) {
        if( to_be_removed->previous )
            last = to_be_removed->previous;
        else
            last = NULL;
    }
    size--;
    to_be_removed->in_area = 0;
    this->remove_hash(to_be_removed->ad_species->name());
}
void AED_dlist::insert_after_entry(AED_area_entry *add_after_this, AED_area_entry *to_be_inserted) {
    if( last == add_after_this )
        last = to_be_inserted;
    to_be_inserted->next                            = add_after_this->next;
    to_be_inserted->previous                    = add_after_this;
    if ( add_after_this->next )
        add_after_this->next->previous      = to_be_inserted;
    add_after_this->next                            = to_be_inserted;

    size++;
    to_be_inserted->in_area = this;
    this->insert_hash(to_be_inserted->ad_species->name());
}
void AED_dlist::insert_before_entry(AED_area_entry *add_before_this, AED_area_entry *to_be_inserted) {

    if( (add_before_this)->previous )
        (add_before_this)->previous->next   = to_be_inserted;
    else
        first = to_be_inserted;
    to_be_inserted->next                            = add_before_this;
    to_be_inserted->previous                    = add_before_this->previous;
    add_before_this->previous                   = to_be_inserted;

    size++;
    to_be_inserted->in_area = this;
    this->insert_hash(to_be_inserted->ad_species->name());
}
void AED_dlist::append(AED_area_entry *to_be_inserted) {

    if(last) {
        last->next = to_be_inserted;
        to_be_inserted->previous = last;
        to_be_inserted->next = NULL;
    }
    else {
        first = last = to_be_inserted;
        to_be_inserted->previous = NULL;
        to_be_inserted->next = NULL;
    }
    last = to_be_inserted;
    size++;
    to_be_inserted->in_area = this;
    this->insert_hash(to_be_inserted->ad_species->name());
}
/********************************************************************************************************/
/********************************************************************************************************/
void AED_dlist::create_hash(GBDATA *gb_maini,const char *awar_suffix){
    char awar_buffer[1024];
    this->gb_main = gb_maini;
    sprintf(awar_buffer,"arb_edit/%s",awar_suffix);
    this->hash_awar = strdup(awar_buffer);
    char *data  = GBT_read_string2(gb_main, hash_awar, "");
    hash = GBS_create_hash(100,0);
    GBS_string_2_hashtab(hash,data);
    hash_level = 0;
    free(data);
}

long AED_dlist::read_hash(const char *name){
    if (!hash) return 0;
    long val = GBS_read_hash(hash,name);
    if (!val) return val;
    if (!hash_level) GBS_write_hash(hash,name,val+1);
    return val;
}

#ifdef __cplusplus
extern "C" {
#endif

    static long aeddlist_optimizehash(const char *key, long val) {
        if (val>1) return 1;
        key = key;
        return 0;
    }

#ifdef __cplusplus
}
#endif

void AED_dlist::optimize_hash(void){
    if (!hash) return;
    hash_level = 1;
    GBS_hash_do_loop(hash,aeddlist_optimizehash);
}
void AED_dlist::insert_hash(const char *name){
    if (!hash) return;
    GB_transaction dummy(gb_main);
    if (hash_level) {
        GBS_write_hash(hash,name,1);
        char *str = GBS_hashtab_2_string(hash);
        GBT_write_string(gb_main,hash_awar, str);
        delete str;
    }
}
void AED_dlist::remove_hash(const char *name){
    if (!hash) return;
    GB_transaction dummy(gb_main);
    GBS_write_hash(hash,name,0);
    char *str = GBS_hashtab_2_string(hash);
    GBT_write_string(gb_main,hash_awar, str);
    delete str;
}


/********************************************************************************************************/
/********************************************************************************************************/
/********************************************************************************************************/

void set_cursor_to( AED_window *aedw, long cursor, class AED_area_entry *aed ) {
    AW_window   *awmm;
    AW_device   *device;
    const   int  AED_WINBORDER = 50;

    awmm   = aedw->aww;
    device = awmm->get_device (AW_MIDDLE_AREA  );
    device->set_filter(AED_F_ALL);
    
    const AW_font_information *font_information = device->get_font_information(AED_GC_SEQUENCE, 'A' );
    AW_rectangle               screen;
    device->get_area_size(&screen);

    long wincursor          = cursor*font_information->max_letter.width;
    long widthofscrolledwin = (long)(screen.r  - awmm->left_indent_of_horizontal_scrollbar);
    long worldwidth         = (long) aedw->size_information.r;

    aedw->hide_cursor( device, awmm );

    if (aed) {
        if ( aedw->one_area_entry_is_selected ) {
            AED_area_entry *help_area_entry = aedw->selected_area_entry;
            aedw->deselect_area_entry();
            aedw->show_single_area_entry(device, awmm, help_area_entry);
        }
        aedw->select_area_entry( aed,-10 );
        aedw->show_single_area_entry(device, awmm, aed);
    }

    aedw->cursor = (int)cursor;
    aedw->cursor_is_managed = AW_TRUE;

    if(  wincursor < awmm->slider_pos_horizontal + AED_WINBORDER && awmm->slider_pos_horizontal > 0) {
        long h = wincursor - AED_WINBORDER;
        if ( h < 0 ) h = 0;
        if ( h > worldwidth - widthofscrolledwin ) h = (long)(worldwidth - widthofscrolledwin);
        awmm->set_horizontal_scrollbar_position( (int)h );
        aed_horizontal( awmm, (AW_CL)aedw, (AW_CL)AW_FALSE );
    }else if ( wincursor > awmm->slider_pos_horizontal + widthofscrolledwin - AED_WINBORDER ){
        long h = wincursor - (widthofscrolledwin - AED_WINBORDER);
        if ( h < 0 ) h = 0;
        if ( h > worldwidth - widthofscrolledwin ) h = (long)(worldwidth - widthofscrolledwin);
        awmm->set_horizontal_scrollbar_position( (int)h );
        aed_horizontal( awmm, (AW_CL)aedw, (AW_CL)AW_FALSE );
    }
    if (!aed || aedw->area_middle != aed->in_area){
        aedw->show_cursor( device, awmm );
        return;
    }
    aedw->cursor_is_managed = AW_FALSE;

    AW_pos y_pos = aed->absolut_y - awmm->top_indent_of_vertical_scrollbar;
    AW_pos hight_of_scrolled_window = screen.b - awmm->bottom_indent_of_vertical_scrollbar
        - awmm->top_indent_of_vertical_scrollbar;
    if ( y_pos <  AED_WINBORDER &&
         awmm->slider_pos_vertical > 0) {
        y_pos = awmm->slider_pos_vertical + y_pos - AED_WINBORDER;
        if (y_pos <= 0 ) y_pos = 0;
        awmm->set_vertical_scrollbar_position( (int)y_pos );
        aed_vertical( awmm, (AW_CL)aedw, (AW_CL)AW_FALSE );
    }else if (y_pos > hight_of_scrolled_window - AED_WINBORDER){
        y_pos = awmm->slider_pos_vertical + y_pos - hight_of_scrolled_window + AED_WINBORDER;
        if (y_pos <= 0 ) y_pos = 0;
        awmm->set_vertical_scrollbar_position( (int)y_pos );
        aed_vertical( awmm, (AW_CL)aedw, (AW_CL)AW_FALSE );
    }
    aedw->cursor_is_managed = AW_TRUE;
    aedw->show_cursor( device, awmm );
} // end: set_cursor_to

static void jump_to_cursor_position(AW_window *aww, AED_window *aedw, char *awar_name)
{
    AW_root *root = aww->get_root();
    long pos = root->awar(awar_name)->read_int();
    if (strcmp(awar_name,AWAR_CURSOR_POSITION_LOCAL) ){ // rel->abs
        long apos;
        const char *error = edg.ref->rel_2_abs(pos,0,apos);
        AWUSE(error);
        root->awar(AWAR_CURSOR_POSITION_LOCAL)->write_int( apos);
        pos = apos;
    }
    set_cursor_to(aedw,pos,0);
}


static void set_cursor_up_down(AED_window *aedw, int direction)
{
    AW_window    *awmm   = aedw->aww;
    AW_device    *device = awmm->get_device (AW_MIDDLE_AREA  );
    device->set_filter(AED_F_ALL);

    const AW_font_information *font_information = device->get_font_information( AED_GC_SEQUENCE, 'A' );
    AW_rectangle               screen;
    device->get_area_size(&screen);

    int jump = (font_information->max_letter.height + AED_LINE_SPACING) * (direction == -1 ? -1 : 1);
    int d_x  = 0;
    int d_y  = 0;

    if (aedw->selected_area_entry->in_area == aedw->area_top) {
        d_x = awmm->left_indent_of_horizontal_scrollbar - awmm->picture->l - awmm->slider_pos_horizontal;
        d_y = 0;
    }
    if (aedw->selected_area_entry->in_area == aedw->area_middle) {
        d_x = awmm->left_indent_of_horizontal_scrollbar - awmm->picture->l - awmm->slider_pos_horizontal;
        d_y = awmm->top_indent_of_vertical_scrollbar - awmm->picture->t - awmm->slider_pos_vertical;
    }
    if (aedw->selected_area_entry->in_area == aedw->area_bottom) {
        d_x = awmm->left_indent_of_horizontal_scrollbar - awmm->picture->l - awmm->slider_pos_horizontal;
        d_y = screen.b - awmm->bottom_indent_of_vertical_scrollbar;
    }
    
    AW_device *click_device = awmm->get_click_device (AW_MIDDLE_AREA,
                                                      1 + aedw->cursor * font_information->max_letter.width + d_x,
                                                      (int)aedw->selected_area_entry->in_line + d_y + jump,
                                                      10, 5, 0);
    click_device->reset();
    click_device->set_filter(AED_F_NAME | AED_F_SEQUENCE);
    aedw->show_data(click_device, awmm, AW_FALSE);

    AW_clicked_text clicked_text;
    click_device->get_clicked_text(&clicked_text);

    if (clicked_text.exists == AW_TRUE) {
        AED_area_entry *new_selected_area_entry = (AED_area_entry *) clicked_text.client_data1;
        if (clicked_text.client_data2 == AED_F_SEQUENCE) {
            if (aedw->one_area_entry_is_selected) {
                //alten area_entry ent - selektieren
                aedw->hide_cursor(device, awmm);
                AED_area_entry *help_area_entry = aedw->selected_area_entry;
                aedw->deselect_area_entry();
                aedw->show_single_area_entry(device, awmm, help_area_entry);
            }
            aedw->select_area_entry(new_selected_area_entry, clicked_text.cursor);
            aedw->show_single_area_entry(device, awmm, new_selected_area_entry);
            aedw->show_cursor(device, awmm);
        }
    } else {
        aedw->selected_area_entry_is_visible = AW_TRUE;
    }
}

static void change_direction(AW_window *aww,AED_window *aedw)
{
    switch(aedw->edit_direction) {
        case 1:     aedw->edit_direction = 0;break;
        default:    aedw->edit_direction = 1;break;
    }
    aww->get_root()->awar(AWAR_EDIT_DIRECTION)->write_int(aedw->edit_direction);
}

static void change_insert_mode(AW_window *aww,AED_window *aedw)
{
    switch(aedw->edit_modus) {
        case AED_ALIGN:     aedw->edit_modus = AED_INSERT;break;
        case AED_INSERT:    aedw->edit_modus = AED_REPLACE;break;
        case AED_REPLACE:   aedw->edit_modus = AED_ALIGN;break;
    }
    aww->get_root()->awar(AWAR_EDIT_MODE)->write_int(aedw->edit_modus);
}

static void aed_double_click( AW_window *aw, AED_window         *aedw ) {
    AWUSE(aedw);
    AW_root *awr = aw->get_root();
    GB_transaction dummy(gb_main);

    char *name;
    name = awr->awar(AWAR_SPECIES_NAME_LOCAL)->read_string();
    GBDATA *gb_species = GBT_find_species(gb_main,name);
    if (gb_species){
        GB_write_flag(gb_species,1-GB_read_flag(gb_species));
    }
    delete name;
}

static void aed_input( AW_window *aw, AW_CL cd1, AW_CL cd2 ) {
    AWUSE(cd2);
    AW_event         event;
    AW_device       *device,*info_device,*click_device;
    AED_window      *aedw       = (AED_window *)cd1;
    AW_window       *awmm       = aedw->aww;
    AW_clicked_text  clicked_text;
    AED_area_entry  *area_entry = aedw->selected_area_entry;
    AD_ERR          *error_flag = 0;
    char             left_text[100];
    AED_area_entry  *first_entry, *last_entry;

    aw->get_event( &event );

    device  = awmm->get_device (AW_MIDDLE_AREA  );
    device->reset();
    device->set_filter(AED_F_ALL);
    info_device = awmm->get_device (AW_INFO_AREA  );
    info_device->reset();
    info_device->set_filter(AED_F_ALL);
    click_device    = aw->get_click_device (AW_MIDDLE_AREA, event.x, event.y, 10, 10, 0 );
    click_device->reset();
    click_device->set_filter(AED_F_NAME | AED_F_SEQUENCE);

    int direction = 1;
    int changeflag = 0;
    long cursor;
    if (event.type == AW_Keyboard_Press && event.keycode == AW_KEY_F1){
        change_insert_mode(aedw->aww,aedw);
    }else if (event.type == AW_Keyboard_Press && event.keycode == AW_KEY_F2){
        change_direction(aedw->aww,aedw);
    }else if ( event.type == AW_Keyboard_Press && aedw->cursor_is_managed ) {
        switch(aedw->edit_modus) {
            case AED_ALIGN:     area_entry->adt_sequence->changemode(AD_allign);break;
            case AED_INSERT:    area_entry->adt_sequence->changemode(AD_insert);break;
            case AED_REPLACE:   area_entry->adt_sequence->changemode(AD_replace);break;
        }
        switch (event.keycode) {
            case AW_KEY_F1:
                break;
            case AW_KEY_UP:
                direction = -1;
            case AW_KEY_DOWN:
                set_cursor_up_down(aedw,direction);
                break;
            default:
                if (awmm->get_root()->awar(AWAR_EDIT_MULTI_SEQ)->read_int()){
                    first_entry =  area_entry->in_area->first;
                    last_entry =  NULL;
                }else{
                    first_entry =  area_entry;
                    last_entry =  area_entry->next;
                }
                error_flag = NULL;
                for ( area_entry = first_entry; area_entry != last_entry && !error_flag; area_entry = area_entry->next){
                    cursor = aedw->cursor;
                    int changed;
                    error_flag = area_entry->adt_sequence->show_command( event.keymodifier, event.keycode,
                                                                         edg.edk->map_key(event.character),
                                                                         aedw->edit_direction, cursor, changed);
                    changeflag |= changed;
                }

                if (!error_flag && changeflag) {
                    aedw->root->ad_main->begin_transaction();
                    for (   area_entry = first_entry;
                            area_entry != last_entry;
                            area_entry = area_entry->next){
                        AD_ERR *h = area_entry->adt_sequence->show_put();
                        if (!error_flag) error_flag = h;
                    }
                    aedw->owntimestamp = aedw->root->ad_main->time_stamp();
                    aedw->root->ad_main->commit_transaction();
                }else if (changeflag) {
                    aedw->root->ad_main->begin_transaction();
                    for (   area_entry = first_entry;
                            area_entry != last_entry;
                            area_entry = area_entry->next){
                        area_entry->adt_sequence->update();
                        area_entry->adt_sequence->show_update();
                    }
                    aedw->root->ad_main->commit_transaction();
                }

                if (changeflag) {
                    aedw->hide_cursor( device, awmm );
                    for (   area_entry = first_entry;
                            area_entry != last_entry;
                            area_entry = area_entry->next){
                        aedw->show_single_area_entry(device, awmm, area_entry);
                    }
                    aedw->show_cursor( device, awmm );
                }

                set_cursor_to(aedw,cursor,0);
                break;
        }

    } // end: if ( event.type == AW_Keyboard_Press && aedw->cursor_is_managed )



    if ( event.type == AW_Mouse_Press ) {
        aedw->show_data( click_device, awmm, AW_FALSE );
        click_device->get_clicked_text( &clicked_text );

        if ( clicked_text.exists == AW_TRUE ) {
            AED_area_entry *new_selected_area_entry = (AED_area_entry *)clicked_text.client_data1;
            if( clicked_text.client_data2 == AED_F_NAME ) {
                if( aedw->one_area_entry_is_selected ) {
                    if ( aedw->selected_area_entry_is_visible ) {
                        aedw->hide_cursor( device, awmm );
                    }
                    AED_area_entry *help_area_entry = aedw->selected_area_entry;
                    aedw->deselect_area_entry();
                    aedw->show_single_area_entry( device, awmm, help_area_entry );
                }
                aedw->select_area_entry( new_selected_area_entry, clicked_text.cursor );
                aedw->show_single_area_entry(device, awmm, new_selected_area_entry);
                aedw->cursor_is_managed = AW_FALSE;
            }
            if( clicked_text.client_data2 == AED_F_SEQUENCE ) {
                if( aedw->one_area_entry_is_selected ) {
                    if (  aedw->selected_area_entry_is_visible ) {
                        aedw->hide_cursor( device, awmm );
                    }
                    AED_area_entry *help_area_entry = aedw->selected_area_entry;
                    aedw->deselect_area_entry();
                    aedw->show_single_area_entry( device, awmm, help_area_entry );
                }
                aedw->select_area_entry( new_selected_area_entry, clicked_text.cursor );
                aedw->show_single_area_entry( device, awmm, new_selected_area_entry );
                aedw->cursor_is_managed = AW_TRUE;
                aedw->show_cursor( device, awmm );
            }
            aedw->selected_area_entry_is_visible = AW_TRUE;
        }
    } // end: if ( event.type == AW_Mouse_Press )


    if ( event.type == AW_Mouse_Release ) {

        if( aedw->drag ) {
            aedw->drag = AW_FALSE;                                    // drag beendet
            aedw->make_left_text( left_text, aedw->selected_area_entry );
            drag_box( device, AED_GC_NAME_DRAG, aedw->drag_x - aedw->drag_x_correcting, aedw->drag_y - aedw->drag_y_correcting, 0, 0, left_text );          // vorher gemalte Box entfernen
            click_device = aw->get_click_device (AW_MIDDLE_AREA,event.x, event.y, 10, 8, 0);
            click_device->reset();
            click_device->set_filter(AED_F_NAME);

            aedw->show_data( click_device, awmm, AW_FALSE );
            click_device->get_clicked_text(&clicked_text);

            AW_rectangle    screen;
            device->get_area_size(&screen);
            if (event.x > 0 && event.x < screen.r && event.y > 0 && event.y < screen.b) {        // man ist innerhalb der draw area
                if ( clicked_text.exists && clicked_text.client_data2 == AED_F_NAME) {       // drag auf gueltiger Zeile beendet
                    AED_area_entry *destination_area_entry = (AED_area_entry *)clicked_text.client_data1;
                    if( strcmp(destination_area_entry->ad_species->name(),aedw->selected_area_entry->ad_species->name()) != 0 ) {
                        // nicht auf sich selber abgelegt
                        aedw->selected_area_entry->in_area->remove_entry(aedw->selected_area_entry);
                        if( clicked_text.distance > 0 )
                            destination_area_entry->in_area->insert_after_entry(destination_area_entry, aedw->selected_area_entry);
                        else
                            destination_area_entry->in_area->insert_before_entry(destination_area_entry, aedw->selected_area_entry);
                        aedw->calculate_size(awmm);
                    }
                }else {
                    if( event.y > awmm->top_indent_of_vertical_scrollbar &&
                        event.y < screen.b - awmm->bottom_indent_of_vertical_scrollbar ) {
                        // man ist im mittleren Bereich
                        if(!aedw->area_middle->first) {
                            // der mittlere Bereich ist leer, man will da anfuegen
                            aedw->selected_area_entry->in_area->remove_entry(
                                                                             aedw->selected_area_entry);
                            aedw->area_middle->append(aedw->selected_area_entry);
                            aedw->calculate_size(awmm);
                        }
                    }
                }
            }else {             // drag auf ungueltiger Zeile beendet
                if(event.y < screen.t) {    // drag oberhalb des Bildschirmes beendet
                    aedw->selected_area_entry->in_area->remove_entry(aedw->selected_area_entry);
                    aedw->area_top->append(aedw->selected_area_entry);
                    aedw->selected_area_entry->in_area = aedw->area_top;

                    aedw->calculate_size(aw);
                }
                if(event.y > screen.b) {    // drag unterhalb des Bildschirmes beendet
                    aedw->selected_area_entry->in_area->remove_entry(aedw->selected_area_entry);
                    aedw->area_bottom->append(aedw->selected_area_entry);

                    aedw->calculate_size(awmm);
                }
            }
        } // end if ( aedw->drag )
    } // end if ( event.type == AW_Mouse_Release )
    if (error_flag&&error_flag->show()) aw_message(error_flag->show());
    delete error_flag;
} // end: aed_input



static void aed_motion( AW_window *aw, AW_CL cd1, AW_CL cd2 ) {
    AWUSE(cd2);
    AW_event event;
    AW_event motion_event;
    AW_device *device;
    AW_device *click_device;
    AED_window *aedw = (AED_window *)cd1;
    AW_window *awmm = aedw->aww;
    AW_clicked_text clicked_text;
    char left_text[100];

    device = aw->get_device (AW_MIDDLE_AREA  );
    device->set_filter(AED_F_ALL);

    if ( aedw->drag ) {                                                                                                                         // mitten im dragging
        aw->get_event ( &motion_event );
        device->shift_y( 0 );
        device->shift_x( 0 );
        aedw->make_left_text( left_text, aedw->selected_area_entry );
        drag_box( device, AED_GC_NAME_DRAG, aedw->drag_x - aedw->drag_x_correcting, aedw->drag_y - aedw->drag_y_correcting, 0, 0, left_text );          // vorher gemalte Box entfernen
        aedw->drag_x = motion_event.x;
        aedw->drag_y = motion_event.y;
        drag_box( device, AED_GC_NAME_DRAG, motion_event.x - aedw->drag_x_correcting , motion_event.y - aedw->drag_y_correcting, 0, 0, left_text );
    } // end: if ( aedw->drag )


    if( !aedw->drag && aedw->selected_area_entry) {         // Beginn des draggings
        aw->get_event( &event );
        click_device = aw->get_click_device (AW_MIDDLE_AREA, event.x, event.y, 10, 5, 0 );
        click_device->reset();
        click_device->set_filter(AED_F_NAME);
        aedw->show_data( click_device, awmm, AW_FALSE );
        click_device->get_clicked_text( &clicked_text );

        if( clicked_text.exists && clicked_text.client_data2 == AED_F_NAME ) {
            AED_area_entry *area_entry = (AED_area_entry *)clicked_text.client_data1;
            if( strcmp(area_entry->ad_species->name(),aedw->selected_area_entry->ad_species->name()) == 0 ) {
                aedw->drag = AW_TRUE;
                aw->get_event ( &motion_event );
                aedw->drag_x_correcting = event.x - (int)aedw->selected_area_entry->absolut_x;
                aedw->drag_y_correcting = event.y - (int)aedw->selected_area_entry->absolut_y;
                device->shift_y( 0 );
                device->shift_x( 0 );
                aedw->make_left_text( left_text, aedw->selected_area_entry );
                drag_box( device, AED_GC_NAME_DRAG, motion_event.x - aedw->drag_x_correcting , motion_event.y - aedw->drag_y_correcting, 0, 0, left_text );
                aedw->drag_x = motion_event.x;
                aedw->drag_y = motion_event.y;
            }
        }
    } // end: if( !aedw->drag )

} // end: aed_motion



static void aed_timer( AW_root *ar, AW_CL cd1, AW_CL cd2 ) {
    AWUSE(ar);
    AED_window *aedw = (AED_window *)cd1;
    AW_window *aw = (AW_window *)cd2;
    AW_window   *awmm = aedw->aww;
    AW_device *device;
    if ( GB_read_transaction(gb_main) <=0){
        aedw->root->ad_main->begin_transaction();
        GB_tell_server_dont_wait(gb_main);
        if ( aedw->owntimestamp != aedw->root->ad_main->time_stamp() ) {
            aedw->owntimestamp = aedw->root->ad_main->time_stamp();
            aedw->root->ad_main->commit_transaction();
            device = awmm->get_device (AW_MIDDLE_AREA  );
            device->clear(AED_F_ALL);
            device = awmm->get_device (AW_INFO_AREA  );
            aed_expose((AW_window *)cd2,cd1,0);
        }
        else {
            ar->check_for_remote_command((AW_default)gb_main,"ARB_EDIT");
            aedw->root->ad_main->commit_transaction();
        }

        aw->get_root()->add_timed_callback(500,aed_timer,cd1,cd2);
    }
} // end: aed_timer





/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/

// static void Probebutton(AW_window *aw, AW_CL cd1, AW_CL cd2) {
//     AWUSE(aw);
//     printf("Probebutton:%li:%li\n",cd1,cd2);
// }


void aed_initialize_device(AW_device *device) {
    device->new_gc( 0 );
    device->set_line_attributes( 0, 0.3, AW_SOLID );
    device->set_font( 0, AW_DEFAULT_BOLD_FONT/*AW_LUCIDA_SANS_BOLD*/, 12, 0);
    device->set_foreground_color( 0, AW_WINDOW_FG );

    device->new_gc( 1 );
    device->set_font( 1, AW_DEFAULT_BOLD_FONT/*AW_LUCIDA_SANS_BOLD*/, 12, 0);
    device->set_foreground_color( 1, AW_WINDOW_FG );

    device->new_gc( 2 );
    device->set_font( 2, AW_DEFAULT_BOLD_FONT/*AW_LUCIDA_SANS_BOLD*/, 12, 0);
    device->set_foreground_color( 2,AW_WINDOW_DRAG );
    device->set_function( 2, AW_XOR );

}
static void ED_calc_ecoli_pos(AW_root *root){
    long apos = root->awar(AWAR_CURSOR_POSITION_LOCAL)->read_int();
    long rpos;
    long dummy;
    const char *error = edg.ref->abs_2_rel(apos,rpos,dummy);
    AWUSE(error);
    root->awar(AWAR_CURSER_POS_REF_ECOLI)->write_int( rpos);
}

static char *ED_create_sequences_for_gde(void *THIS, GBDATA **&the_species,
                                         uchar **&the_names, uchar **&the_sequences,
                                         long &numberspecies,long &maxalignlen)
{
    AED_window *aedw = (AED_window *)THIS;
    long top = aedw->root->aw_root->awar("gde/top_area")->read_int();
    long tops = aedw->root->aw_root->awar("gde/top_area_sai")->read_int();
    long toph = aedw->root->aw_root->awar("gde/top_area_helix")->read_int();
    long middle = aedw->root->aw_root->awar("gde/middle_area")->read_int();
    long middles = aedw->root->aw_root->awar("gde/middle_area_sai")->read_int();
    long middleh = aedw->root->aw_root->awar("gde/middle_area_helix")->read_int();
    long bottom = aedw->root->aw_root->awar("gde/bottom_area")->read_int();
    long bottoms = aedw->root->aw_root->awar("gde/bottom_area_sai")->read_int();
    long bottomh = aedw->root->aw_root->awar("gde/bottom_area_helix")->read_int();

    numberspecies = 0;
    AED_area_entry  *ae;
    if (aedw->area_top) for ( ae = aedw->area_top->first; ae ;ae=ae->next) {
        if ( !((!ae->ad_extended && top ) || (ae->ad_extended && tops)) ) continue;
        numberspecies++;
        if (toph)   numberspecies++;
    }
    if (aedw->area_middle) for ( ae = aedw->area_middle->first; ae ;ae=ae->next) {
        if ( !((!ae->ad_extended && middle ) || (ae->ad_extended && middles)) ) continue;
        numberspecies++;
        if (middleh) numberspecies++;
    }
    if (aedw->area_bottom) for ( ae = aedw->area_bottom->first; ae ;ae=ae->next) {
        if ( !((!ae->ad_extended && bottom ) || (ae->ad_extended && bottoms)) ) continue;
        numberspecies++;
        if (bottomh) numberspecies++;
    }
    the_species = (GBDATA **)GB_calloc(sizeof(void *),(size_t)numberspecies+1);
    the_names = 0;
    the_sequences = (uchar **)GB_calloc(sizeof(uchar *),(size_t)numberspecies+1);
    long i = 0;
    maxalignlen = 0;
    if (aedw->area_top) for ( ae = aedw->area_top->first; ae ;ae=ae->next) {
        if ( !((!ae->ad_extended && top ) || (ae->ad_extended && tops)) ) continue;
        the_sequences[i] = (uchar *)strdup(ae->adt_sequence->show_get());
        the_species[i] = ae->ad_species->get_GBDATA();
        maxalignlen = ae->adt_sequence->show_len();
        i++;
        if (toph) {
            the_sequences[i] = (uchar *)edg.helix->seq_2_helix(ae->adt_sequence->show_get(),'.');
            the_species[i] = ae->ad_species->get_GBDATA();
            i++;
        }
    }
    if (aedw->area_middle) for ( ae = aedw->area_middle->first; ae ;ae=ae->next) {
        if ( !((!ae->ad_extended && middle ) || (ae->ad_extended && middles)) ) continue;
        the_sequences[i] = (uchar *)strdup(ae->adt_sequence->show_get());
        the_species[i] = ae->ad_species->get_GBDATA();
        maxalignlen = ae->adt_sequence->show_len();
        i++;
        if (middleh) {
            the_sequences[i] = (uchar *)edg.helix->seq_2_helix(ae->adt_sequence->show_get(),'.');
            the_species[i] = ae->ad_species->get_GBDATA();
            i++;
        }
    }
    if (aedw->area_bottom) for ( ae = aedw->area_bottom->first; ae ;ae=ae->next) {
        if ( !((!ae->ad_extended && bottom ) || (ae->ad_extended && bottoms)) ) continue;
        the_sequences[i] = (uchar *)strdup(ae->adt_sequence->show_get());
        the_species[i] = ae->ad_species->get_GBDATA();
        maxalignlen = ae->adt_sequence->show_len();
        i++;
        if (bottomh) {
            the_sequences[i] = (uchar *)edg.helix->seq_2_helix(ae->adt_sequence->show_get(),'.');
            the_species[i] = ae->ad_species->get_GBDATA();
            i++;
        }
    }
    return 0;
}

static void create_edit_variables(AW_root *root, AW_default awr, AED_window *aedw){
    AWUSE(awr);
    root->awar_string( AWAR_SPECIES_DEST,"",        AW_ROOT_DEFAULT);
    root->awar_string( AWAR_SPECIES_NAME_LOCAL, "",     AW_ROOT_DEFAULT);
    root->awar_string( AWAR_SPECIES_NAME,   "",     gb_main);
    root->awar_int( AWAR_CURSOR_POSITION_LOCAL, 0,      AW_ROOT_DEFAULT);
    root->awar_int( AWAR_CURSOR_POSITION,   0,      gb_main);
    root->awar_int( AWAR_CURSER_POS_REF_ECOLI, 0,AW_ROOT_DEFAULT);
    root->awar_int( AWAR_LINE_SPACING,      7)  ->add_target_var(&aed_root.line_spacing);
    root->awar_int( AWAR_CENTER_SPACING,        -1) ->add_target_var(&aed_root.center_spacing);
    root->awar_int( AWAR_HELIX_AT_SAIS,         1)  ->add_target_var((long *)&aed_root.helix_at_extendeds);
    root->awar_int( AWAR_SECURITY_LEVEL, 0,AW_ROOT_DEFAULT);
    root->awar(AWAR_SECURITY_LEVEL)->add_callback(aed_changesecurity,(AW_CL)aedw);
    root->awar_int( AWAR_EDIT_MODE,         aedw->edit_modus)   ->add_target_var((long *)&aedw->edit_modus);
    root->awar_int( AWAR_EDIT_DIRECTION,        1)          ->add_target_var((long *)&aedw->edit_direction);
    root->awar_int( AWAR_EDIT_MULTI_SEQ, 0,AW_ROOT_DEFAULT);
    root->awar(AWAR_CURSOR_POSITION_LOCAL)->add_callback(ED_calc_ecoli_pos);

    root->awar(AWAR_SPECIES_NAME_LOCAL)->map(AWAR_SPECIES_NAME);

    create_gde_var(root,awr, ED_create_sequences_for_gde,CGSS_WT_EDIT,(void *)aedw);

    ARB_init_global_awars(root, awr, gb_main);
}

static AW_window *create_edit_preset_window(AW_root *awr){
    AW_window_simple *aws = new AW_window_simple;
    aws->init( awr, "EDIT_PROPS", "EDIT_PROPERTIES");
    aws->label_length( 25 );
    aws->button_length( 20 );

    aws->at           ( 10,10 );
    aws->auto_space(10,10);
    aws->callback     ( AW_POPDOWN );
    aws->create_button( "CLOSE", "CLOSE", "C" );
    aws->at_newline();

    aws->create_option_menu( AWAR_HELIX_AT_SAIS, "Show Helix at SAIs", "R" );
    aws->insert_option        ( "no",  "n", 0 );
    aws->insert_option        ( "yes",  "r", 1 );
    aws->update_option_menu();
    aws->at_newline();

    aws->label("Vert. Line Space");
    aws->create_input_field(AWAR_LINE_SPACING,4);
    aws->at_newline();

    aws->label("Seq. - Helix Space");
    aws->create_input_field(AWAR_CENTER_SPACING,4);
    aws->at_newline();

    aws->window_fit();
    return (AW_window *)aws;
}

static void ED_focus_cb(AW_root *,GBDATA *)
{
    GB_transaction dummy(gb_main);
}

void aed_create_window(AED_root *aedr) {

    AED_window              *aed_window;
    AW_device               *device;
    AW_device               *info_device;

    AW_window_menu  *awmm;
    char left_text[100];
    AED_left_side           *left_side;
    int is_client = (GB_read_clients(gb_main)<0);

    aed_window = new AED_window;
    aed_window->init(aedr);

    aed_window->aww = awmm = new AW_window_menu;
    awmm->init(aedr->aw_root,"ARB_EDIT", "ARB_EDIT",800,600);

    create_edit_variables(awmm->get_root(),aed_window->root->db,aed_window);

    AW_gc_manager preset_window =
        AW_manage_GC (awmm,awmm->get_device(AW_MIDDLE_AREA), AED_GC_NAME, AED_GC_NAME_DRAG, AW_GCM_DATA_AREA,
                      (AW_CB)aed_resize,(AW_CL)aed_window,0,
                      false,
                      "#CCF177",
                      "NAMES$#0000B9",
                      "SELECTED$#FF0000",
                      "#SEQUENCES$black",
                      "#HELIX$#FF5500",
                      "-CS_NORMAL$black",
                      "-CS_1$#eee",
                      "-CS_2$#ddd",
                      "-CS_3$#ccc",
                      "-CS_4$#bbb",
                      "-CS_5$#aaa",
                      "-CS_6$#999",
                      "-CS_7$#888",
                      "-CS_8$#777",
                      "-CS_9$#666",
                      0);

    char source[256];
    char dest[256];
    sprintf(source,AWP_FONTNAME_TEMPLATE,awmm->window_defaults_name,"HELIX");
    sprintf(dest,AWP_FONTNAME_TEMPLATE,awmm->window_defaults_name,"SEQUENCES");
    awmm->get_root()->awar(source)->map(dest);

    sprintf(source,AWP_FONTSIZE_TEMPLATE,awmm->window_defaults_name,"HELIX");
    sprintf(dest,AWP_FONTSIZE_TEMPLATE,awmm->window_defaults_name,"SEQUENCES");
    awmm->get_root()->awar(source)->map(dest);

    st_ml = new_ST_ML(gb_main);

    awmm->create_menu(       "1",   "File",     "F" );

    if (!is_client){
        awmm->insert_menu_topic("save_bin","Save BIN","B","save_bin.hlp",   AWM_ALL,aed_save_bin,(AW_CL)aed_window,0);
        awmm->insert_separator();
    }

    awmm->insert_separator();
    GDE_load_menu(awmm,"pretty_print");
    awmm->insert_separator();
    awmm->insert_menu_topic("quit","Quit","Q","quit.hlp",   AWM_ALL,        aed_quit,(AW_CL)aed_window->root,1);




    awmm->create_menu(0,"EDIT","E");
    awmm->insert_menu_topic("search",   "Search ...",               "S","ne_search.hlp",    AWM_ALL,    AW_POPUP, (AW_CL)create_tool_search, (AW_CL)aed_window);
    awmm->insert_menu_topic("replace",  "Replace ...",          "R","ne_replace.hlp",   AWM_ALL,    AW_POPUP, (AW_CL)create_tool_replace, (AW_CL)aed_window);
    awmm->insert_menu_topic("complement",   "Complement ...",           "C","ne_compl.hlp", AWM_ALL,    AW_POPUP, (AW_CL)create_tool_complement, (AW_CL)aed_window );
    awmm->insert_separator();
    awmm->insert_menu_topic("create_sequence","Create Sequence ...",        "e","ne_new_sequence.hlp",  AWM_ALL,AW_POPUP, (AW_CL)create_new_seq_window, (AW_CL)aed_window);
    awmm->insert_menu_topic("open_sequence","Open Existing Sequence ...",       "O","ne_new_sequence.hlp",  AWM_ALL,AW_POPUP, (AW_CL)create_old_seq_window, (AW_CL)aed_window);
    awmm->insert_menu_topic("copy_sequence","Copy Selected Sequence ...",       "p","ne_copy_sequence.hlp", AWM_ALL,AW_POPUP, (AW_CL)create_new_copy_window, (AW_CL)aed_window);
    awmm->insert_separator();
    //  awmm->insert_menu_topic(0,"Align Sequence (V1.0)...",           "A","ne_align_seq.hlp", AWM_ALL,AW_POPUP, (AW_CL)create_aligner_window, 0 );
    awmm->insert_menu_topic("align_sequence","Align Sequence (V2.0)...",            "2","ne_align_seq.hlp", AWM_ALL,AW_POPUP, (AW_CL)create_naligner_window, 0 );
    awmm->insert_separator();
    awmm->insert_menu_topic("s_prot_to_0",  "Set Protection Level of selected Sequence to 0","0","security.hlp",    AWM_ALL, (AW_CB)set_security, (AW_CL)aed_window, 0);
    awmm->insert_menu_topic("s_prot_to_1",  "Set Protection Level of selected Sequence to 1","1","security.hlp",    AWM_ALL, (AW_CB)set_security, (AW_CL)aed_window, 1);
    awmm->insert_menu_topic("s_prot_to_2",  "Set Protection Level of selected Sequence to 2","2","security.hlp",    AWM_ALL, (AW_CB)set_security, (AW_CL)aed_window, 2);
    awmm->insert_menu_topic("s_prot_to_3",  "Set Protection Level of selected Sequence to 3","3","security.hlp",    AWM_ALL, (AW_CB)set_security, (AW_CL)aed_window, 3);
    awmm->insert_menu_topic("s_prot_to_4",  "Set Protection Level of selected Sequence to 4","4","security.hlp",    AWM_ALL, (AW_CB)set_security, (AW_CL)aed_window, 4);
    awmm->insert_menu_topic("s_prot_to_5",  "Set Protection Level of selected Sequence to 5","5","security.hlp",    AWM_ALL, (AW_CB)set_security, (AW_CL)aed_window, 5);
    awmm->insert_separator();
    awmm->insert_menu_topic("mark_all_editor",  "Mark All Sequences in Editor",     "M","mark.hlp", AWM_ALL, (AW_CB)set_mark_cb, (AW_CL)aed_window, 1);
    awmm->insert_menu_topic("unmark_all_editor",    "Unmark All Sequences in Editor",       "U","mark.hlp", AWM_ALL, (AW_CB)set_mark_cb, (AW_CL)aed_window, 0);
    awmm->insert_menu_topic("mark_selected",    "Mark Selected Sequence",           "k","mark.hlp", AWM_ALL, (AW_CB)set_smark_cb, (AW_CL)aed_window, 1);
    awmm->insert_menu_topic("unmark_selected",  "Unmark Selected Sequence",         "n","mark.hlp", AWM_ALL, (AW_CB)set_smark_cb, (AW_CL)aed_window, 0);

    awmm->create_menu(0,"Props","P");
    awmm->insert_menu_topic("props_menu",   "Menu: Colors and Fonts ...",   "M","props_frame.hlp",  AWM_ALL,    AW_POPUP, (AW_CL)AW_preset_window,  0 );
    awmm->insert_menu_topic("props_seq",    "Sequences: Colors and Fonts ...",  "C","neprops_data.hlp", AWM_ALL,    AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)preset_window );
    awmm->insert_menu_topic("props_etc",    "ETC ...",          "E","neprops.hlp",  AWM_ALL,    AW_POPUP, (AW_CL)create_edit_preset_window, (AW_CL)0 );
    //  awmm->insert_menu_topic(0,"NDS ...",            "N","neprops_nds.hlp",  AWM_ALL, aed_popup_config_window, (AW_CL)aed_window, AW_TRUE );
    awmm->insert_menu_topic("props_key_map","Key Mappings ...",     "K","nekey_map.hlp",    AWM_ALL,    AW_POPUP, (AW_CL)create_key_map_window, 0 );
    awmm->insert_menu_topic("props_helix",  "Helix Symbols ...",        "H","helixsym.hlp", AWM_ALL,    AW_POPUP, (AW_CL)create_helix_props_window, (AW_CL)
                            new AW_cb_struct(awmm,(AW_CB)aed_resize,(AW_CL)aed_window,0) );
    awmm->insert_separator();
    awmm->insert_menu_topic("save_props",   "Save Properties (in ~/.arb_prop/edit.arb)","S","savedef.hlp",  AWM_ALL, (AW_CB) AW_save_defaults, 0, 0 );
    awmm->create_menu(0,"ETC","C");
    awmm->insert_menu_topic("synchronize",  "Synchronize Cursor Position",      "S","exportcursor.hlp", AWM_ALL,    aed_use_focus, (AW_CL)aed_window, AW_TRUE );
    awmm->insert_menu_topic("unsynchronize","Don't Synchronize Cursor",         "D","exportcursor.hlp", AWM_ALL,    aed_use_focus, (AW_CL)aed_window, AW_FALSE );
    awmm->insert_menu_topic("refresh_helix","Reload Helix (SAI 'HELIXNR/HELIX')",       "H","helix.hlp",    AWM_ALL,    (AW_CB)reload_helix, (AW_CL)aed_window, 0 );
    awmm->insert_menu_topic("refresh_ecoli","Reload Reference (SAI 'ECOLI')",       "R","ecoliref.hlp", AWM_ALL,    (AW_CB)reload_ref, (AW_CL)aed_window, 0 );
    awmm->insert_menu_topic("enable_col_stat","Enable Column Statistic",            "C","st_ml.hlp",    AWM_ALL,AW_POPUP,(AW_CL)st_create_main_window,(AW_CL)st_ml);
    //  awmm->insert_menu_topic("submission",   "Submission ...",               "S","submission.hlp",   AWM_TUM,    AW_POPUP, (AW_CL)create_submission_window, (AW_CL)aed_window );
    awmm->insert_separator();
    awmm->insert_sub_menu(0,"GDE Specials","G");
    GDE_load_menu(awmm,0,0);
    awmm->close_sub_menu();

    awmm->set_info_area_height( 250 );
    awmm->at(5,2);
    awmm->auto_space(5,-2);
    //  awmm->shadow_width(1);

    int db_pathx,db_pathy;
    awmm->get_at_position( &db_pathx,&db_pathy );
    awmm->callback( aed_quit,(AW_CL)aed_window->root,1);
    awmm->button_length(0);
    awmm->help_text("quit.hlp");
    awmm->create_button("QUIT", "QUIT");


    awmm->callback(AED_undo_cb,(AW_CL)aed_window,(AW_CL)GB_UNDO_UNDO);
    awmm->help_text("undo.hlp");
    awmm->create_button("UNDO", "#undo.bitmap",0);

    awmm->callback(AED_undo_cb,(AW_CL)aed_window,(AW_CL)GB_UNDO_REDO);
    awmm->help_text("undo.hlp");
    awmm->create_button("REDO", "#redo.bitmap",0);

    int db_treex,db_treey;
    awmm->get_at_position( &db_treex,&db_treey );
    awmm->button_length(16);
    //  awmm->help_text("nt_tree_select.hlp");
    awmm->label("Pos.");
    awmm->callback((AW_CB)jump_to_cursor_position,(AW_CL)aed_window,
                   (AW_CL)AWAR_CURSOR_POSITION_LOCAL);
    awmm->create_input_field(AWAR_CURSOR_POSITION_LOCAL,4);

    awmm->label("E.coli");
    awmm->callback((AW_CB)jump_to_cursor_position,(AW_CL)aed_window,
                   (AW_CL)AWAR_CURSER_POS_REF_ECOLI);
    awmm->create_input_field(AWAR_CURSER_POS_REF_ECOLI,4);

    awmm->label("Mode");
    awmm->create_option_menu(AWAR_EDIT_MODE,0,0);
    awmm->insert_option("align",0,(int)AED_ALIGN);
    awmm->insert_option("insert",0,(int)AED_INSERT);
    awmm->insert_default_option("replace",0,(int)AED_REPLACE);
    awmm->update_option_menu();

    awmm->label("Protection");
    awmm->create_option_menu(AWAR_SECURITY_LEVEL);
    awmm->insert_option("0",0,0);
    awmm->insert_option("1",0,1);
    awmm->insert_option("2",0,2);
    awmm->insert_option("3",0,3);
    awmm->insert_option("4",0,4);
    awmm->insert_option("5",0,5);
    awmm->insert_default_option("6",0,6);
    awmm->update_option_menu();

    awmm->create_toggle(AWAR_EDIT_DIRECTION,"edit/r2l.bitmap","edit/l2r.bitmap");

    awmm->label("MG");
    awmm->create_toggle(AWAR_EDIT_MULTI_SEQ);

    awmm->callback(AW_POPUP_HELP,(AW_CL)"arb_edit.hlp");
    awmm->button_length(0);
    awmm->help_text("help.hlp");
    awmm->create_button("HELP", "HELP","H");

    awmm->callback( AW_help_entry_pressed );
    awmm->create_button(0,"?");

    awmm->set_info_area_height( 30 );
    awmm->set_bottom_area_height( 0 );


    info_device = awmm->get_device (AW_INFO_AREA);
    aed_initialize_device(info_device);

    info_device->set_foreground_color( 2, AW_WINDOW_DRAG );

    left_side = new AED_left_side( aed_left_security, "Security" );
    left_side->in_side = aed_window->show_dlist_left_side;
    aed_window->show_dlist_left_side->append( left_side );

    left_side = new AED_left_side( aed_left_name, "Spezies Name" );
    left_side->in_side = aed_window->show_dlist_left_side;
    aed_window->show_dlist_left_side->append( left_side );

#if 0
    left_side = new AED_left_side( aed_left_CA, "Number of 'A's" );
    left_side->in_side = aed_window->hide_dlist_left_side;
    aed_window->hide_dlist_left_side->append( left_side );

    left_side = new AED_left_side( aed_left_CC, "Number of 'C's" );
    left_side->in_side = aed_window->hide_dlist_left_side;
    aed_window->hide_dlist_left_side->append( left_side );

    left_side = new AED_left_side( aed_left_CG, "Number of 'G's" );
    left_side->in_side = aed_window->hide_dlist_left_side;
    aed_window->hide_dlist_left_side->append( left_side );

    left_side = new AED_left_side( aed_left_CT, "Number of 'T's" );
    left_side->in_side = aed_window->hide_dlist_left_side;
    aed_window->hide_dlist_left_side->append( left_side );

    left_side = new AED_left_side( aed_left_ACGT, "Number of 'ACGT's" );
    left_side->in_side = aed_window->show_dlist_left_side;
    aed_window->show_dlist_left_side->append( left_side );

    left_side = new AED_left_side( aed_left_GC, "GC-quota" );
    left_side->in_side = aed_window->show_dlist_left_side;
    aed_window->show_dlist_left_side->append( left_side );
#endif
    device = awmm->get_device (AW_MIDDLE_AREA  );
    device->set_filter(AED_F_ALL);
    aed_window->make_left_text( left_text, NULL );
    awmm->set_horizontal_scrollbar_left_indent( device->get_string_size( AED_GC_NAME, left_text,0 ) + 20 );

    awmm->set_expose_callback (AW_MIDDLE_AREA,          aed_expose,(AW_CL)aed_window,0);
    awmm->set_resize_callback (AW_MIDDLE_AREA,          aed_resize,(AW_CL)aed_window,0);
    awmm->get_root()->set_focus_callback((AW_RCB)ED_focus_cb,(AW_CL)gb_main,0);

    awmm->set_vertical_change_callback(     aed_vertical,(AW_CL)aed_window, (AW_CL)AW_TRUE );
    awmm->set_horizontal_change_callback(       aed_horizontal,(AW_CL)aed_window, (AW_CL)AW_TRUE );

    awmm->set_input_callback (AW_MIDDLE_AREA,aed_input,(AW_CL)aed_window,0);
    awmm->set_motion_callback (AW_MIDDLE_AREA,aed_motion,(AW_CL)aed_window,0);
    awmm->set_double_click_callback (AW_MIDDLE_AREA,(AW_CB)aed_double_click,(AW_CL)aed_window,0);

    awmm->get_root()->add_timed_callback(2000,aed_timer,(AW_CL)aed_window,(AW_CL)awmm);

    awmm->get_root()->awar(AWAR_HELIX_AT_SAIS)->add_callback( (AW_RCB)aed_resize, (AW_CL)aed_window,0);
    awmm->get_root()->awar(AWAR_LINE_SPACING)->add_callback( (AW_RCB)aed_resize, (AW_CL)aed_window,0);
    awmm->get_root()->awar(AWAR_CENTER_SPACING)->add_callback( (AW_RCB)aed_resize, (AW_CL)aed_window,0);
    create_naligner_variables(awmm->get_root(),aed_window->root->db);
    create_tool_variables(awmm->get_root(),aed_window->root->db);
    //  create_submission_variables(awmm->get_root(),aed_window->root->db);
    edg.edk = new ed_key;
    edg.edk->create_awars(awmm->get_root());

    aed_window->load_data();
    awmm->reset_scrolled_picture_size();    // middle_area berechnen
    aed_window->calculate_size(awmm);

    awmm->show();

}


/****************************************************************************************************/
/****************************************************************************************************/
/****************************************************************************************************/

AED_root aed_root;


int main(int argc,char **argv) {
    const char *path;
    aw_initstatus();

    if (argc > 2 || (argc == 2 && strcmp(argv[1], "--help") == 0)) {
        fprintf(stderr,
                "\n"
                "Purpose: Start the old editor.\n"
                "Usage:   arb_edit [database]\n"
                "\n"
                );
        return EXIT_FAILURE;
    }

    if (argc == 1) path = ":";
    else path           = argv[1];

    if (aed_adopen(path,aed_root.ad_main)){ // Database is opened
        fprintf(stderr, "arb_edit: could not open database '%s'\n", path);
        return EXIT_FAILURE;
    }
    ad_main = aed_root.ad_main;
    gb_main = ad_main->get_GBDATA();

    aed_root.db = aed_root.aw_root->open_default( ".arb_prop/edit.arb" );

    aed_root.aw_root->init_variables( aed_root.db );
    aed_root.aw_root->init( "ARB_EDITOR" );     // window-system is initialized

    aed_create_window(&aed_root); // creates editor window and inserts callbacks
    aed_root.aw_root->main_loop(); // let's enter main-loop

    return EXIT_SUCCESS;
}


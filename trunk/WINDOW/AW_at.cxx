#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <arbdb.h>
#include "aw_root.hxx"
#include "aw_device.hxx"
#include "aw_at.hxx"
#include "aw_window.hxx"
#include "aw_xfig.hxx"

/*******************************************************************************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

AW_at::AW_at(void) {
    memset((char*)this,0,sizeof(AW_at));
    length_of_buttons = 10;
    shadow_thickness = 2;
}


void AW_window::shadow_width (int shadow_thickness ) { _at->shadow_thickness = shadow_thickness; }

void AW_window::label_length( int length ) { _at->length_of_label_for_inputfield = length; }

void AW_window::button_length( int length ) { _at->length_of_buttons = length; }
int  AW_window::get_button_length() const { return _at->length_of_buttons; }

void AW_window::highlight( void ) { _at->highlight = AW_TRUE; }

void AW_window::auto_increment( int x, int y ) {
    _at->auto_increment_x = x;
    _at->auto_increment_y = y;
    _at->x_for_newline = _at->x_for_next_button;
    _at->do_auto_space = AW_FALSE;
    _at->do_auto_increment = AW_TRUE;
    _at->biggest_height_of_buttons = 0;
}


void AW_window::auto_space( int x, int y ) {
    _at->auto_space_x = x;
    _at->auto_space_y = y;
    _at->x_for_newline = _at->x_for_next_button;
    _at->do_auto_space = AW_TRUE;
    _at->do_auto_increment = AW_FALSE;
    _at->biggest_height_of_buttons = 0;
}


void AW_window::auto_off( void ) {
    _at->do_auto_space = AW_FALSE;
    _at->do_auto_increment = AW_FALSE;
}

void AW_window::at_set_min_size(int xmin, int ymin) {
    if (xmin > _at->max_x_size) _at->max_x_size = xmin; // this looks wrong, but its right!
    if (ymin > _at->max_y_size) _at->max_y_size = ymin;

    if (recalc_size_at_show) {
        set_window_size(_at->max_x_size+1000, _at->max_y_size+1000);
    }
}


/*******************************************************************************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

void AW_window::id( const char *id ) {
    delete _at->id_for_next_button;
    _at->id_for_next_button     = strdup( id );
}


void AW_window::help_text(const char *id ) {
    delete _at->helptext_for_next_button;
    _at->helptext_for_next_button   = strdup( id );
}


void AW_window::mask( AW_active mask ) {
    _at->mask_for_next_button       = mask;
}


void AW_window::callback( void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2 ) {
    _callback = new AW_cb_struct(this,(AW_CB)f,cd1,cd2);
}

void AW_window::callback( void (*f)(AW_window*,AW_CL), AW_CL cd1 ) {
    _callback = new AW_cb_struct(this,(AW_CB)f,cd1);
}

void AW_window::callback( void (*f)(AW_window*) ) {
    _callback = new AW_cb_struct(this,(AW_CB)f);
}
void AW_window::callback( AW_cb_struct * /*owner*/awcbs ) {
    _callback = awcbs;
}

void AW_window::d_callback( void (*f)(AW_window*,AW_CL,AW_CL), AW_CL cd1, AW_CL cd2 ) {
    _d_callback = new AW_cb_struct(this,(AW_CB)f,cd1,cd2);
}

void AW_window::d_callback( void (*f)(AW_window*,AW_CL), AW_CL cd1 ) {
    _d_callback = new AW_cb_struct(this,(AW_CB)f,cd1);
}

void AW_window::d_callback( void (*f)(AW_window*) ) {
    _d_callback = new AW_cb_struct(this,(AW_CB)f);
}
void AW_window::d_callback( AW_cb_struct * /*owner*/awcbs ) {
    _d_callback = awcbs;
}

void AW_window::set_background(const char *colorname){
    _at->background_colorname = strdup(colorname);
}


void AW_window::label( const char *label ) {
    delete _at->label_for_inputfield;
    _at->label_for_inputfield = strdup( label );
}


void AW_window::at( int x, int y ) {
    at_x(x);
    at_y(y);
}

void AW_window::at_x( int x ) {
    if (_at->x_for_next_button > _at->max_x_size) _at->max_x_size = _at->x_for_next_button;
    _at->x_for_next_button = x;
    if (_at->x_for_next_button > _at->max_x_size) _at->max_x_size = _at->x_for_next_button;
}


void AW_window::at_y( int y ) {
    if (_at->y_for_next_button + _at->biggest_height_of_buttons > _at->max_y_size)
        _at->max_y_size = _at->y_for_next_button + _at->biggest_height_of_buttons;
    _at->biggest_height_of_buttons = _at->biggest_height_of_buttons + _at->y_for_next_button - y;
    if (_at->biggest_height_of_buttons<0){
        _at->biggest_height_of_buttons = 0;
        if (_at->max_y_size < y)    _at->max_y_size = y;
    }
    _at->y_for_next_button = y;
}


void AW_window::at_shift( int x, int y ) {
    at(x+_at->x_for_next_button,y+_at->y_for_next_button);
}

void AW_window::at_newline( void ) {

    if ( _at->do_auto_increment ) {
        at_y(_at->auto_increment_y + _at->y_for_next_button);
    }
    else {
        if ( _at->do_auto_space ) {
            at_y(_at->y_for_next_button + _at->auto_space_y + _at->biggest_height_of_buttons);
        }
        else {
            AW_ERROR("neither auto_space nor auto_increment activated while using at_newline");
        }
    }
    at_x(_at->x_for_newline);
}


void AW_window::at( const char *id ) {
    AWUSE(id);
    char to_position[100];memset(to_position,0,sizeof(to_position));
    _at->attach_y = _at->attach_x = AW_FALSE;
    _at->attach_ly = _at->attach_lx = AW_FALSE;
    _at->attach_any = AW_FALSE;
    _at->correct_for_at_string = AW_TRUE;

    if ( !xfig_data ) {
        AW_ERROR( "no xfig file loaded " );
        return;
    }
    AW_xfig *xfig = (AW_xfig *)xfig_data;
    AW_xfig_pos *pos;
    if (1){
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->hash,id);
    }
    if (!pos){
        sprintf( to_position, "X:%s", id ); pos = (AW_xfig_pos*)GBS_read_hash(xfig->hash,to_position);
        if (pos) _at->attach_any = _at->attach_lx = AW_TRUE;
    }
    if (!pos){
        sprintf( to_position, "Y:%s", id ); pos = (AW_xfig_pos*)GBS_read_hash(xfig->hash,to_position);
        if (pos) _at->attach_any = _at->attach_ly = AW_TRUE;
    }
    if (!pos){
        sprintf( to_position, "XY:%s", id );    pos = (AW_xfig_pos*)GBS_read_hash(xfig->hash,to_position);
        if (pos) _at->attach_any = _at->attach_lx = _at->attach_ly = AW_TRUE;
    }
    if( !pos ) {
        AW_ERROR(" ID '%s' does not exist in xfig file", id);
        return;
    }

    at( (pos->x - xfig->minx), (pos->y - xfig->miny - this->get_root()->font_height - 9));
    _at->correct_for_at_center = pos->center;

    if (1){
        sprintf( to_position, "to:%s", id );    pos = (AW_xfig_pos*)GBS_read_hash(xfig->hash,to_position);
    }
    if (!pos) {
        sprintf( to_position, "to:X:%s", id );  pos = (AW_xfig_pos*)GBS_read_hash(xfig->hash,to_position);
        if (pos) _at->attach_any = _at->attach_x = AW_TRUE;
    }
    if (!pos) {
        sprintf( to_position, "to:Y:%s", id );  pos = (AW_xfig_pos*)GBS_read_hash(xfig->hash,to_position);
        if (pos) _at->attach_any = _at->attach_y = AW_TRUE;
    }
    if (!pos) {
        sprintf( to_position, "to:XY:%s", id ); pos = (AW_xfig_pos*)GBS_read_hash(xfig->hash,to_position);
        if (pos) _at->attach_any = _at->attach_x = _at->attach_y = AW_TRUE;
    }

    if( pos ) {
        _at->to_position_exists = AW_TRUE;
        _at->to_position_x = (pos->x - xfig->minx);
        _at->to_position_y = (pos->y - xfig->miny);
        _at->correct_for_at_center = 0; // always justify left when a to-position exists
    }else {
        _at->to_position_exists = AW_FALSE;
    }
}


// set "$XY:id" manually

void AW_window::at_attach(AW_BOOL attach_x, AW_BOOL attach_y) {
    aw_assert(0); // this does not work
    _at->attach_lx  = attach_x;
    _at->attach_ly  = attach_y;
    _at->attach_any = attach_x || attach_y;
}

// set "$to:XY:id" manually
// use negative offsets to set offset from right/lower border to to-position

void AW_window::at_set_to(AW_BOOL attach_x, AW_BOOL attach_y, int xoff, int yoff) {
//     aw_assert(attach_x || attach_y); // use at_unset_to() to un-attach

    _at->attach_any = attach_x || attach_y;
    _at->attach_x   = attach_x;
    _at->attach_y   = attach_y;

    _at->to_position_exists = AW_TRUE;
    _at->to_position_x      = xoff >= 0 ? _at->x_for_next_button + xoff : _at->max_x_size+xoff;
    _at->to_position_y      = yoff >= 0 ? _at->y_for_next_button + yoff : _at->max_y_size+yoff;

    if (_at->to_position_x > _at->max_x_size) _at->max_x_size = _at->to_position_x;
    if (_at->to_position_y > _at->max_y_size) _at->max_y_size = _at->to_position_y;

    _at->correct_for_at_center = 0;

//     _at->correct_for_at_center = xoff <= 0 ? 0 : 2; // justify left (=0) or right (=2)
}

void AW_window::at_unset_to() {
    _at->attach_x   = _at->attach_y = _at->to_position_exists = AW_FALSE;
    _at->attach_any = _at->attach_lx || _at->attach_ly;
}

AW_BOOL AW_window::at_ifdef(const  char *id) {
    if (!xfig_data) return AW_FALSE;
    AW_xfig *xfig = (AW_xfig *)xfig_data;
    char     buffer[100];

#if defined(DEBUG)
    int printed =
#endif // DEBUG
        sprintf(buffer,"XY:%s",id);
#if defined(DEBUG)
    aw_assert(printed<100);
#endif // DEBUG

    if (GBS_read_hash(xfig->hash,buffer+3)) return AW_TRUE; // "tag"
    if (GBS_read_hash(xfig->hash,buffer+1)) return AW_TRUE; // "Y:tag"
    if (GBS_read_hash(xfig->hash,buffer)) return AW_TRUE; // "XY:tag"
    buffer[1] = 'X';
    if (GBS_read_hash(xfig->hash,buffer+1)) return AW_TRUE; // "X:tag"

    return AW_FALSE;
}

void AW_window::check_at_pos( void ) {
    if (_at->x_for_next_button<10){
        //  printf("X Position should be greater 10\n");
    }
}

void AW_window::get_at_position( int *x, int *y ) {
    *x = _at->x_for_next_button;
    *y = _at->y_for_next_button;
}


void AW_window::store_at_size_and_attach( AW_at_size *at_size ) {
    at_size->store(_at);
}

void AW_window::restore_at_size_and_attach( const AW_at_size *at_size ) {
    at_size->restore(_at);
}

void AW_window::unset_at_commands( void ) {

    delete _at->id_for_next_button;
    _at->id_for_next_button = NULL;

    delete _at->helptext_for_next_button;
    _at->helptext_for_next_button   = NULL;

    _at->mask_for_next_button           = AWM_ALL;

    _callback               = NULL;
    _d_callback             = NULL;

    free(_at->label_for_inputfield);
    _at->label_for_inputfield = NULL;

    _at->correct_for_at_string = AW_FALSE;
    _at->correct_for_at_center = 0;

    _at->to_position_exists = AW_FALSE;

    _at->highlight = AW_FALSE;
    free(_at->background_colorname);
    _at->background_colorname = 0;
}

void AW_window::increment_at_commands( int width, int height ) {

    at_shift(width,0);
    at_shift(-width,0);         // set bounding box

    if ( _at->do_auto_increment ) {
        at_shift(_at->auto_increment_x,0);
    }
    if ( _at->do_auto_space ) {
        at_shift(_at->auto_space_x + width,0);
    }

    if ( _at->biggest_height_of_buttons < height ) {
        _at->biggest_height_of_buttons = height;
    }

    if ( _at->max_y_size < (_at->y_for_next_button +_at->biggest_height_of_buttons +3.0)) {
        _at->max_y_size = _at->y_for_next_button +_at->biggest_height_of_buttons +3;
    }

    if ( _at->max_x_size < (_at->x_for_next_button + this->get_root()->font_width)) {
        _at->max_x_size = _at->x_for_next_button + this->get_root()->font_width;
    }
}

/*******************************************************************************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

int AW_window::calculate_string_width( int columns ) {
    if ( xfig_data ) {
        AW_xfig *xfig = (AW_xfig *)xfig_data;
        //return columns * this->get_root()->font_width;
        return (int)(columns * xfig->font_scale * 8);   /* stdfont 8x13 */
    }else {
        return columns * 8; /* stdfont 8x13 */
    }
}

int AW_window::calculate_label_length()
{
    if (_at->length_of_label_for_inputfield) return _at->length_of_label_for_inputfield;
    if (_at->label_for_inputfield) return strlen(_at->label_for_inputfield);
    return 0;
}

int AW_window::calculate_string_height( int rows , int offset ) {
    if ( xfig_data ) {
        AW_xfig *xfig = (AW_xfig *)xfig_data;
        return (int)((rows * 13 + offset ) * xfig->font_scale); /* stdfont 8x13 */
    }
    else {
        return (rows * 13 + offset );   /* stdfont 8x13 */
    }
}


char *AW_window::align_string( const char *string, int columns ) {
    char *help_label;

    help_label = new char[200];
    strcpy( help_label, string );
    strcat( help_label,"                                                                               " );
    help_label[columns] = '\0';

    return help_label;
}

/*******************************************************************************************************/
/*******************************************************************************************************/
/*******************************************************************************************************/

void AW_at_size::store(const AW_at *at) {
    to_position_exists = at->to_position_exists;
    if (to_position_exists) {
        to_offset_x = at->to_position_x - at->x_for_next_button;
        to_offset_y = at->to_position_y - at->y_for_next_button;
    }
    attach_x   = at->attach_x;
    attach_y   = at->attach_y;
    attach_lx  = at->attach_lx;
    attach_ly  = at->attach_ly;
    attach_any = at->attach_any;
}
void AW_at_size::restore(AW_at *at) const {
    at->to_position_exists = to_position_exists;
    if (to_position_exists) {
        at->to_position_x = at->x_for_next_button + to_offset_x;
        at->to_position_y = at->y_for_next_button + to_offset_y;
    }
    at->attach_x   = attach_x;
    at->attach_y   = attach_y;
    at->attach_lx  = attach_lx;
    at->attach_ly  = attach_ly;
    at->attach_any = attach_any;
}


void AW_at_maxsize::store(const AW_at *at) {
    maxx = at->max_x_size;
    maxy = at->max_y_size;
}
void AW_at_maxsize::restore(AW_at *at) const {
    at->max_x_size = maxx;
    at->max_y_size = maxy;
}




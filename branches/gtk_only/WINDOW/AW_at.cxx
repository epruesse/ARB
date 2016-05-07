// =============================================================== //
//                                                                 //
//   File      : AW_at.cxx                                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_at.hxx"
#include "aw_window.hxx"
#include "aw_xfig.hxx"
#include "aw_root.hxx"

#include <arbdbt.h>

AW_at::AW_at(AW_window* pWindow) {
    memset((char*)this, 0, sizeof(AW_at));

    aw_assert(NULL != pWindow);

    at_id = NULL;
    window = pWindow;
    xfig_data = NULL;
    
    window->get_font_size(font_width, font_height);

    widget_mask       = AWM_ALL;

    length_of_buttons = 10;
    height_of_buttons = 0;
    length_of_label_for_inputfield = 0;
    highlight = 0;

    helptext_for_next_button = NULL;
    background_color = 0;
    label_for_inputfield = NULL;
    x_for_next_button = 0;
    y_for_next_button = 0;
    max_x_size = 0;
    max_y_size = 0;

    to_position_x = 0;
    to_position_y = 0;
    to_position_exists = false;

    do_auto_space = false;
    auto_space_x = 0;
    auto_space_y = 0;

    do_auto_increment = false;
    auto_increment_x = 0;
    auto_increment_y = 0;

    biggest_height_of_buttons = 0;

    saved_xoff_for_label = 0;

    saved_x = 0;
    correct_for_at_center = 0;
    x_for_newline = 0;

    attach_x = false;           // attach right side to right form
    attach_y = false;
    attach_lx = false;          // attach left side to right form
    attach_ly = false;
    attach_any = false;
}

void AW_at::set_xfig(AW_xfig* xfig) {
    xfig_data = xfig;
    int xsize = xfig->maxx - xfig->minx;
    int ysize = xfig->maxy - xfig->miny;

    if (xsize > max_x_size) max_x_size = xsize;
    if (ysize > max_y_size) max_y_size = ysize;
}

void AW_at::set_mask(AW_active mask) {
    widget_mask = mask;
}

void AW_at::at(const char *at_id_) {
    char to_position[100];
    memset(to_position, 0, sizeof(to_position));

    freedup(at_id, at_id_);

    attach_y   = attach_x = false;
    attach_ly  = attach_lx = false;
    attach_any = false;

    if (!xfig_data) GBK_terminatef("no xfig-data loaded, can't position at(\"%s\")", at_id);

    AW_xfig     *xfig = xfig_data;
    AW_xfig_pos *pos;

    pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, at_id);

    if (!pos) {
        sprintf(to_position, "X:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) attach_any = attach_lx = true;
    }
    if (!pos) {
        sprintf(to_position, "Y:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) attach_any = attach_ly = true;
    }
    if (!pos) {
        sprintf(to_position, "XY:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) attach_any = attach_lx = attach_ly = true;
    }

    if (!pos) GBK_terminatef("ID '%s' does not exist in xfig file", at_id);

    at((pos->x - xfig->minx), (pos->y - xfig->miny - font_height));
    correct_for_at_center = pos->center;

    sprintf(to_position, "to:%s", at_id);
    pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);

    if (!pos) {
        sprintf(to_position, "to:X:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) attach_any = attach_x = true;
    }
    if (!pos) {
        sprintf(to_position, "to:Y:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) attach_any = attach_y = true;
    }
    if (!pos) {
        sprintf(to_position, "to:XY:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) attach_any = attach_x = attach_y = true;
    }

    if (pos) {
        to_position_exists = true;
        to_position_x = (pos->x - xfig->minx);
        to_position_y = (pos->y - xfig->miny);
        correct_for_at_center = 0; // always justify left when a to-position exists
    }
    else {
        to_position_exists = false;
    }
}

void AW_at::at(int x, int y) {
    at_x(x);
    at_y(y);
}

inline int force_valid_at_offset(int xy) {
    // Placing widgets at negative offsets is used in arb-motif.
    // In gtk doing so, lets widgets just disappear. Workaround: set offset to zero
    return xy<0 ? 0 : xy;
}

void AW_at::at_x(int x) {
    x = force_valid_at_offset(x);

    if (x_for_next_button > max_x_size) max_x_size = x_for_next_button;
    x_for_next_button                              = x;
    if (x_for_next_button > max_x_size) max_x_size = x_for_next_button;
}

void AW_at::at_y(int y) {
    y = force_valid_at_offset(y);

    if (y_for_next_button + biggest_height_of_buttons > max_y_size)
        max_y_size            = y_for_next_button + biggest_height_of_buttons;
    biggest_height_of_buttons = biggest_height_of_buttons + y_for_next_button - y;
    if (biggest_height_of_buttons<0) {
        biggest_height_of_buttons = 0;
        if (max_y_size < y)    max_y_size = y;
    }
    y_for_next_button = y;
}

void AW_at::at_shift(int x, int y) {
    at(x + x_for_next_button, y + y_for_next_button);
}

void AW_at::at_newline() {
  if (do_auto_increment) {
        at_y(auto_increment_y + y_for_next_button);
    }
    else {
        if (do_auto_space) {
            at_y(y_for_next_button + auto_space_y + biggest_height_of_buttons);
        }
        else {
            GBK_terminate("neither auto_space nor auto_increment activated while using at_newline");
        }
    }
    at_x(x_for_newline);
}

bool AW_at::at_ifdef(const char *at_id_) {
    AW_xfig *xfig = window->get_xfig_data();
    if (!xfig) return false;

    char buffer[100];

#if defined(DEBUG)
    int printed =
#endif // DEBUG
        sprintf(buffer, "XY:%s", at_id_);
#if defined(DEBUG)
    aw_assert(printed<100);
#endif // DEBUG

    if (GBS_read_hash(xfig->at_pos_hash, buffer+3)) return true; // "tag"
    if (GBS_read_hash(xfig->at_pos_hash, buffer+1)) return true; // "Y:tag"
    if (GBS_read_hash(xfig->at_pos_hash, buffer)) return true; // "XY:tag"
    buffer[1] = 'X';
    if (GBS_read_hash(xfig->at_pos_hash, buffer+1)) return true; // "X:tag"

    return false;
}

void AW_at::at_set_to(bool attach_x_i, bool attach_y_i, int xoff, int yoff) {
    // set "$to:XY:id" manually
    // use negative offsets to set offset from right/lower border to to-position

    attach_any = attach_x_i || attach_y_i;
    attach_x   = attach_x_i;
    attach_y   = attach_y_i;

    to_position_exists = true;
    to_position_x      = xoff >= 0 ? x_for_next_button + xoff : max_x_size+xoff;
    to_position_y      = yoff >= 0 ? y_for_next_button + yoff : max_y_size+yoff;

    if (to_position_x > max_x_size) max_x_size = to_position_x;
    if (to_position_y > max_y_size) max_y_size = to_position_y;

    correct_for_at_center = 0;
}

void AW_at::unset_at_commands() {
    correct_for_at_center = 0;
    to_position_exists    = false;
    highlight             = false;

    freenull(at_id);
    freenull(helptext_for_next_button);
    freenull(label_for_inputfield);

    background_color = 0;
}

void AW_at::increment_at_commands(int width, int height) {
    at_shift(width, 0);
    at_shift(-width, 0);        // set bounding box

    if (do_auto_increment) {
        at_shift(auto_increment_x, 0);
    }
    if (do_auto_space) {
        at_shift(auto_space_x + width, 0);
    }

    if (biggest_height_of_buttons < height) {
        biggest_height_of_buttons = height;
    }

    if (max_y_size < (y_for_next_button + biggest_height_of_buttons + 3.0)) {
        max_y_size = y_for_next_button + biggest_height_of_buttons + 3;
    }

    if (max_x_size < (x_for_next_button + font_width)) {
        max_x_size = x_for_next_button + font_width;
    }
}

void AW_at::at_unset_to(){
    attach_x   = attach_y = to_position_exists = false;
    attach_any = attach_lx || attach_ly;
}

void AW_at::auto_space(int x, int y) {
    do_auto_space = true;
    auto_space_x  = x;
    auto_space_y  = y;

    do_auto_increment = false;

    x_for_newline             = x_for_next_button;
    biggest_height_of_buttons = 0;
}

// -------------------------------------------------------------------------
//      force-diff-sync 9126478246 (remove after merging back to trunk)

void AW_at::auto_increment(int x, int y) {
    do_auto_increment = true;
    auto_increment_x  = x;
    auto_increment_y  = y;
    x_for_newline     = x_for_next_button;
    do_auto_space     = false;
    biggest_height_of_buttons = 0;
}

void AW_at::label_length(int length){
    length_of_label_for_inputfield = length;
}

void AW_at::button_length(int length) {
    length_of_buttons = length; 
}

int  AW_at::get_button_length() const {
    return length_of_buttons;
}

void AW_at::get_at_position(int *x, int *y) const {
    *x = x_for_next_button;
    *y = y_for_next_button;
}

int AW_at::get_at_xposition() const {
    return x_for_next_button; 
}

int AW_at::get_at_yposition() const {
    return y_for_next_button; 
}

// -------------------
//      AW_at_size

class AW_at_size : public AW_at_storage {
    int  to_offset_x;                               // here we use offsets (not positions like in AW_at)
    int  to_offset_y;
    bool to_position_exists;

    bool attach_x;           // attach right side to right form
    bool attach_y;
    bool attach_lx;          // attach left side to right form
    bool attach_ly;
    bool attach_any;

public:
    AW_at_size()
        : to_offset_x(0),
          to_offset_y(0),
          to_position_exists(false),
          attach_x(false),
          attach_y(false),
          attach_lx(false),
          attach_ly(false),
          attach_any(false)
    {}

    void store(const AW_at& at) OVERRIDE;
    void restore(AW_at& at) const OVERRIDE;
};

void AW_at_size::store(const AW_at& at) {
    to_position_exists = at.to_position_exists;
    if (to_position_exists) {
        to_offset_x = at.to_position_x - at.x_for_next_button;
        to_offset_y = at.to_position_y - at.y_for_next_button;
    }
    attach_x   = at.attach_x;
    attach_y   = at.attach_y;
    attach_lx  = at.attach_lx;
    attach_ly  = at.attach_ly;
    attach_any = at.attach_any;
}

void AW_at_size::restore(AW_at& at) const {
    at.to_position_exists = to_position_exists;
    if (to_position_exists) {
        at.to_position_x = at.x_for_next_button + to_offset_x;
        at.to_position_y = at.y_for_next_button + to_offset_y;
    }
    at.attach_x   = attach_x;
    at.attach_y   = attach_y;
    at.attach_lx  = attach_lx;
    at.attach_ly  = attach_ly;
    at.attach_any = attach_any;
}

// -------------------
//      AW_at_maxsize

class AW_at_maxsize : public AW_at_storage {
    int maxx;
    int maxy;

public:
    AW_at_maxsize()
        : maxx(0),
          maxy(0)
    {}

    void store(const AW_at &at) OVERRIDE;
    void restore(AW_at &at) const OVERRIDE;
};

void AW_at_maxsize::store(const AW_at &at) {
    maxx = at.max_x_size;
    maxy = at.max_y_size;
}

void AW_at_maxsize::restore(AW_at &at) const {
    at.max_x_size = maxx;
    at.max_y_size = maxy;
}

// -------------------
//      AW_at_auto

class AW_at_auto : public AW_at_storage {
    enum { INC, SPACE, OFF } type;
    int x, y;
    int xfn, xfnb, yfnb, bhob;
public:
    AW_at_auto() : type(OFF), x(0), y(0), xfn(0), xfnb(0), yfnb(0), bhob(0) {}

    void store(const AW_at &at) OVERRIDE;
    void restore(AW_at &at) const OVERRIDE;
};

void AW_at_auto::store(const AW_at &at) {
    if (at.do_auto_increment) {
        type = INC;
        x    = at.auto_increment_x;
        y    = at.auto_increment_y;
    }
    else if (at.do_auto_space)  {
        type = SPACE;
        x    = at.auto_space_x;
        y    = at.auto_space_y;
    }
    else {
        type = OFF;
    }

    xfn  = at.x_for_newline;
    xfnb = at.x_for_next_button;
    yfnb = at.y_for_next_button;
    bhob = at.biggest_height_of_buttons;
}

void AW_at_auto::restore(AW_at &at) const {
    at.do_auto_space     = (type == SPACE);
    at.do_auto_increment = (type == INC);

    if (at.do_auto_space) {
        at.auto_space_x = x;
        at.auto_space_y = y;
    }
    else if (at.do_auto_increment) {
        at.auto_increment_x = x;
        at.auto_increment_y = y;
    }

    at.x_for_newline             = xfn;
    at.x_for_next_button         = xfnb;
    at.y_for_next_button         = yfnb;
    at.biggest_height_of_buttons = bhob;
}

// -------------------------------
//      AW_at_storage factory

AW_at_storage *AW_at_storage::make(AW_window *aww, AW_at_storage_type type) {
    AW_at_storage *s = NULL;
    switch (type) {
        case AW_AT_SIZE_AND_ATTACH: s = new AW_at_size(); break;
        case AW_AT_AUTO:            s = new AW_at_auto(); break;
        case AW_AT_MAXSIZE:         s = new AW_at_maxsize(); break;
    }
    aww->store_at_to(*s);
    return s;
}

// -------------------------------------------------------------------------
//      force-diff-sync 6423462367 (remove after merging back to trunk)


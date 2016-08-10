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

AW_at::AW_at() {
    memset((char*)this, 0, sizeof(AW_at));

    length_of_buttons = 10;
    height_of_buttons = 0;
    shadow_thickness  = 2;
    widget_mask       = AWM_ALL;
}

void AW_window::at(const char *at_id) {
    char to_position[100];
    memset(to_position, 0, sizeof(to_position));

    _at->attach_y   = _at->attach_x = false;
    _at->attach_ly  = _at->attach_lx = false;
    _at->attach_any = false;

    if (!xfig_data) GBK_terminatef("no xfig-data loaded, can't position at(\"%s\")", at_id);

    AW_xfig     *xfig = (AW_xfig *)xfig_data;
    AW_xfig_pos *pos;

    pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, at_id);

    if (!pos) {
        sprintf(to_position, "X:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) _at->attach_any = _at->attach_lx = true;
    }
    if (!pos) {
        sprintf(to_position, "Y:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) _at->attach_any = _at->attach_ly = true;
    }
    if (!pos) {
        sprintf(to_position, "XY:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) _at->attach_any = _at->attach_lx = _at->attach_ly = true;
    }

    if (!pos) GBK_terminatef("ID '%s' does not exist in xfig file", at_id);

    at((pos->x - xfig->minx), (pos->y - xfig->miny - this->get_root()->font_height - 9));
    _at->correct_for_at_center = pos->center;

    sprintf(to_position, "to:%s", at_id);
    pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);

    if (!pos) {
        sprintf(to_position, "to:X:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) _at->attach_any = _at->attach_x = true;
    }
    if (!pos) {
        sprintf(to_position, "to:Y:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) _at->attach_any = _at->attach_y = true;
    }
    if (!pos) {
        sprintf(to_position, "to:XY:%s", at_id);
        pos = (AW_xfig_pos*)GBS_read_hash(xfig->at_pos_hash, to_position);
        if (pos) _at->attach_any = _at->attach_x = _at->attach_y = true;
    }

    if (pos) {
        _at->to_position_exists = true;
        _at->to_position_x = (pos->x - xfig->minx);
        _at->to_position_y = (pos->y - xfig->miny);
        _at->correct_for_at_center = 0; // always justify left when a to-position exists
    }
    else {
        _at->to_position_exists = false;
    }
}

void AW_window::at(int x, int y) {
    at_x(x);
    at_y(y);
}

void AW_window::at_x(int x) {
    if (_at->x_for_next_button > _at->max_x_size) _at->max_x_size = _at->x_for_next_button;
    _at->x_for_next_button = x;
    if (_at->x_for_next_button > _at->max_x_size) _at->max_x_size = _at->x_for_next_button;
}

void AW_window::at_y(int y) {
    if (_at->y_for_next_button + _at->biggest_height_of_buttons > _at->max_y_size)
        _at->max_y_size = _at->y_for_next_button + _at->biggest_height_of_buttons;
    _at->biggest_height_of_buttons = _at->biggest_height_of_buttons + _at->y_for_next_button - y;
    if (_at->biggest_height_of_buttons<0) {
        _at->biggest_height_of_buttons = 0;
        if (_at->max_y_size < y)    _at->max_y_size = y;
    }
    _at->y_for_next_button = y;
}

void AW_window::at_shift(int x, int y) {
    at(x+_at->x_for_next_button, y+_at->y_for_next_button);
}

void AW_window::at_newline() {
    if (_at->do_auto_increment) {
        at_y(_at->auto_increment_y + _at->y_for_next_button);
    }
    else {
        if (_at->do_auto_space) {
            at_y(_at->y_for_next_button + _at->auto_space_y + _at->biggest_height_of_buttons);
        }
        else {
            GBK_terminate("neither auto_space nor auto_increment activated while using at_newline");
        }
    }
    at_x(_at->x_for_newline);
}

bool AW_window::at_ifdef(const  char *at_id_) {
    AW_xfig *xfig = (AW_xfig *)xfig_data;
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

void AW_window::at_set_to(bool attach_x, bool attach_y, int xoff, int yoff) {
    // set "$to:XY:id" manually
    // use negative offsets to set offset from right/lower border to to-position

    _at->attach_any = attach_x || attach_y;
    _at->attach_x   = attach_x;
    _at->attach_y   = attach_y;

    _at->to_position_exists = true;
    _at->to_position_x      = xoff >= 0 ? _at->x_for_next_button + xoff : _at->max_x_size+xoff;
    _at->to_position_y      = yoff >= 0 ? _at->y_for_next_button + yoff : _at->max_y_size+yoff;

    if (_at->to_position_x > _at->max_x_size) _at->max_x_size = _at->to_position_x;
    if (_at->to_position_y > _at->max_y_size) _at->max_y_size = _at->to_position_y;

    _at->correct_for_at_center = 0;
}

void AW_window::unset_at_commands() {
    _callback   = NULL;
    _d_callback = NULL;

    _at->correct_for_at_center = 0;
    _at->to_position_exists    = false;
    _at->highlight             = false;

    freenull(_at->helptext_for_next_button);
    freenull(_at->label_for_inputfield);

    _at->background_color = 0;
}

void AW_window::increment_at_commands(int width, int height) {

    at_shift(width, 0);
    at_shift(-width, 0);        // set bounding box

    if (_at->do_auto_increment) {
        at_shift(_at->auto_increment_x, 0);
    }
    if (_at->do_auto_space) {
        at_shift(_at->auto_space_x + width, 0);
    }

    if (_at->biggest_height_of_buttons < height) {
        _at->biggest_height_of_buttons = height;
    }

    if (_at->max_y_size < (_at->y_for_next_button + _at->biggest_height_of_buttons + 3.0)) {
        _at->max_y_size = _at->y_for_next_button + _at->biggest_height_of_buttons + 3;
    }

    if (_at->max_x_size < (_at->x_for_next_button + this->get_root()->font_width)) {
        _at->max_x_size = _at->x_for_next_button + this->get_root()->font_width;
    }
}

void AW_window::at_unset_to() {
    _at->attach_x   = _at->attach_y = _at->to_position_exists = false;
    _at->attach_any = _at->attach_lx || _at->attach_ly;
}

void AW_window::auto_space(int x, int y) {
    _at->do_auto_space = true;
    _at->auto_space_x  = x;
    _at->auto_space_y  = y;

    _at->do_auto_increment = false;

    _at->x_for_newline             = _at->x_for_next_button;
    _at->biggest_height_of_buttons = 0;
}

// -------------------------------------------------------------------------
//      force-diff-sync 9126478246 (remove after merging back to trunk)

void AW_window::auto_increment(int x, int y) {
    _at->do_auto_increment         = true;
    _at->auto_increment_x          = x;
    _at->auto_increment_y          = y;
    _at->x_for_newline             = _at->x_for_next_button;
    _at->do_auto_space             = false;
    _at->biggest_height_of_buttons = 0;
}

void AW_window::label_length(int length) {
    _at->length_of_label_for_inputfield = length;
}

void AW_window::button_length(int length) {
    _at->length_of_buttons = length;
}

int  AW_window::get_button_length() const {
    return _at->length_of_buttons;
}

void AW_window::get_at_position(int *x, int *y) const {
    *x = _at->x_for_next_button;
    *y = _at->y_for_next_button;
}

int AW_window::get_at_xposition() const {
    return _at->x_for_next_button;
}

int AW_window::get_at_yposition() const {
    return _at->y_for_next_button;
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

int AW_window::calculate_string_width(int columns) const {
    if (xfig_data) {
        AW_xfig *xfig = (AW_xfig *)xfig_data;
        return (int)(columns * xfig->font_scale * XFIG_DEFAULT_FONT_WIDTH);   // stdfont 8x13
    }
    else {
        return columns * XFIG_DEFAULT_FONT_WIDTH; // stdfont 8x13
    }
}

int AW_window::calculate_string_height(int rows, int offset) const {
    if (xfig_data) {
        AW_xfig *xfig = (AW_xfig *)xfig_data;
        return (int)((rows * XFIG_DEFAULT_FONT_HEIGHT + offset) * xfig->font_scale); // stdfont 8x13
    }
    else {
        return (rows * XFIG_DEFAULT_FONT_HEIGHT + offset);    // stdfont 8x13
    }
}

char *AW_window::align_string(const char *label_text, int columns) {
    // shortens or expands 'label_text' to 'columns' columns
    // if label_text contains '\n', each "line" is handled separately

    const char *lf     = strchr(label_text, '\n');
    char       *result = 0;

    if (!lf) {
        ARB_alloc(result, columns+1);

        int len              = strlen(label_text);
        if (len>columns) len = columns;

        memcpy(result, label_text, len);
        if (len<columns) memset(result+len, ' ', columns-len);
        result[columns] = 0;
    }
    else {
        char *part1    = ARB_strpartdup(label_text, lf-1);
        char *aligned1 = align_string(part1, columns);
        char *aligned2 = align_string(lf+1, columns);

        result = GBS_global_string_copy("%s\n%s", aligned1, aligned2);

        free(aligned2);
        free(aligned1);
        free(part1);
    }
    return result;
}


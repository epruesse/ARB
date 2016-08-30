// ============================================================= //
//                                                               //
//   File      : aw_common.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 20, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_common.hxx"
#include <vector>

struct AW_common::Pimpl {
    std::vector<AW_GC*> gcmap;

    AW_screen_area screen;

    Pimpl() {}
};

AW_common::AW_common()
    : prvt(new Pimpl()),
      default_aa_setting(AW_AA_DEFAULT)
{

    // start out with a faked pretty big screen
    // the editor balks out if with/height is smaller 
    // than the respective "scroll indent"
    prvt->screen.t = 0;
    prvt->screen.b = 1000;
    prvt->screen.l = 0;
    prvt->screen.r = 1000;
    
    // let's start with a gcmap that's got a bit of space
    prvt->gcmap.resize(32, NULL);
}

AW_common::~AW_common() {
    delete prvt;
}


const AW_screen_area& AW_common::get_screen() const { 
    return prvt->screen; 
}

void AW_common::reset_style() {
    std::vector<AW_GC*>::iterator it;
    for(it = prvt->gcmap.begin(); it != prvt->gcmap.end(); it++) {
        if (*it) (*it)->reset();
    }
}

/**
 * allocates a new GC structure
 * @param gc the number of the new gc
 */
void AW_common::new_gc(int gc) {
    aw_assert(gc >= -1);
    gc++; // -1 is background
    // make sure gcmap is large enough
    if ((unsigned int)gc >= prvt->gcmap.size())
        prvt->gcmap.resize(gc+1, NULL);

    //remove old gc (if there is one)
    if (prvt->gcmap[gc])
        delete prvt->gcmap[gc];

    // have implementation create new gc
    prvt->gcmap[gc] = create_gc();
}

/**
 * Tests whether a GC with the given index exists.
 */
bool AW_common::gc_mapable(int gc) const {
    gc++; // -1 is background
    return gc >= 0 && (unsigned int)gc < prvt->gcmap.size() && prvt->gcmap[gc+1];
}

/**
 * Retrieves const GC pointer
 * pointer may change on call to new_gc!
 */
const AW_GC *AW_common::map_gc(int gc) const {
    return  prvt->gcmap[gc+1];
}

/**
 * Retrieves GC pointer
 * pointer may change on call to new_gc!
 */
AW_GC *AW_common::map_mod_gc(int gc) {
    return prvt->gcmap[gc+1];
}

/**
 * Same result as map_gc(gc)->get_font_limits(c). DEPRECATED
 */
const AW_font_limits& AW_common::get_font_limits(int gc, char c) const {
    // for one characters (c == 0 -> for all characters)
    return c
        ? map_gc(gc)->get_font_limits(c)
        : map_gc(gc)->get_font_limits();
}

/**
 * Sets the clip window width/height from AW_screen_area.
 * screen.t and screen.l have to be 0!
 */
void AW_common::set_screen(const AW_screen_area& screen_) {
    // set clipping coordinates
    prvt->screen = screen_;
    aw_assert(prvt->screen.t == 0 && prvt->screen.l == 0);
}

/**
 * Sets the clip window width and height
 */
void AW_common::set_screen_size(unsigned int width, unsigned int height) {
    prvt->screen.t = 0;
    prvt->screen.b = height;
    prvt->screen.l = 0;
    prvt->screen.r = width;
}

void AW_common::set_default_aa(AW_antialias aa) {
    default_aa_setting = aa;
}

AW_antialias AW_common::get_default_aa() const {
    return default_aa_setting;
}


struct AW_GC::Pimpl {
    AW_common    *common;

    // font
    AW_font_limits         font_limits;
    mutable AW_font_limits one_letter;

    short width_of_chars[256];
    short ascent_of_chars[256];
    short descent_of_chars[256];
    Pimpl() {
        memset(width_of_chars, 0, 
               ARRAY_ELEMS(width_of_chars)*sizeof(*width_of_chars));
        memset(ascent_of_chars, 0, 
               ARRAY_ELEMS(ascent_of_chars)*sizeof(*ascent_of_chars));
        memset(descent_of_chars, 0, 
               ARRAY_ELEMS(descent_of_chars)*sizeof(*descent_of_chars));
    }
};


AW_GC::AW_GC(AW_common *common)
    : prvt(new Pimpl)
{
    prvt->common = common;
}

AW_GC::~AW_GC() {  
    delete prvt;
}

AW_GC::config::config() 
    : function(AW_COPY),
      grey_level(0.5),
      line_width(GC_DEFAULT_LINE_WIDTH),
      style(AW_SOLID), 
      color() 
{}

bool AW_GC::config::operator==(const AW_GC::config& o) const {
    return
        function   == o.function   &&
        grey_level == o.grey_level &&
        line_width == o.line_width &&
        style      == o.style      ;
        //        &&
        //color      == o.color;
}

void AW_GC::set_char_size(int i, int ascent, int descent, int width) {
    prvt->ascent_of_chars[i]  = ascent;
    prvt->descent_of_chars[i] = descent;
    prvt->width_of_chars[i]   = width;
    prvt->font_limits.notify(ascent, descent, width);
}

void AW_GC::set_no_char_size(int i) {
    prvt->ascent_of_chars[i]  = 0;
    prvt->descent_of_chars[i] = 0;
    prvt->width_of_chars[i]   = 0;
}

/**
 * Sets the combination function for drawing
 */
void AW_GC::set_function(AW_function mode) {
    if (conf.function != mode) { // @@@ why is this checked?
        conf.function = mode;
    }
}

/**
 * Sets the value of the alpha channel. 
 * 0 = transparent, 1 = black
 *
 * This used to be <0 = don't fill, 0.0 = white, 1.0 = black
 * but alpha looks nicer. 
 */
void AW_GC::set_grey_level(AW_grey_level grey_level_) {
    if (conf.grey_level != grey_level_) { // @@@ why is this checked?
        conf.grey_level = grey_level_;
    }
}

/**
 * Sets the attributes for lines
 */
void AW_GC::set_line_attributes(short new_width, AW_linestyle new_style) {
    aw_assert(new_width >= 1);
    if (new_style != conf.style || new_width != conf.line_width) { // @@@ why is this checked?
        conf.line_width = new_width;
        conf.style      = new_style;
    }
}

/**
 * Sets the color to draw with
 */
void AW_GC::set_fg_color(AW_rgb col) {
    if (conf.color != col) { // @@@ why is this checked?
        conf.color = col;
    }
}

/**
 * Sets a font by name. The font is not part of the default
 * config. 
 */
void AW_GC::set_font(const char* fontname, bool force_monospace) {
    prvt->font_limits.reset();
    wm_set_font(fontname, force_monospace);
}

AW_common *AW_GC::get_common() const {
    return prvt->common; 
}

const AW_font_limits& AW_GC::get_font_limits() const {
    return prvt->font_limits; 
}

const AW_font_limits& AW_GC::get_font_limits(char c) const {
    aw_assert(c); // you want to use the version w/o parameter
    prvt->one_letter.ascent  = prvt->ascent_of_chars[safeCharIndex(c)];
    prvt->one_letter.descent = prvt->descent_of_chars[safeCharIndex(c)];
    prvt->one_letter.width   = prvt->one_letter.min_width   = prvt->width_of_chars[safeCharIndex(c)];
    return prvt->one_letter;
}


short AW_GC::get_width_of_char(char c) const { 
    return prvt->width_of_chars[safeCharIndex(c)]; 
}

short AW_GC::get_ascent_of_char(char c) const { 
    return prvt->ascent_of_chars[safeCharIndex(c)]; 
}

short AW_GC::get_descent_of_char(char c) const { 
    return prvt->descent_of_chars[safeCharIndex(c)]; 
}


/**
 * calculate display size of 'str'
 * 'str' and/or 'textlen' may be 0
 * 'str'     == 0 -> calculate max width of any text with length 'textlen'
 * 'textlen' == 0 -> calls strlen when needed
 *  both 0 -> return 0
 */
int AW_GC::get_string_size(const char *str, long textlen) const {
    // The easiest precise way to figure out how wide a string
    // will be when rendered given the currently selected
    // font on the currently used device is by having pango
    // do the layout and check the size. That's what
    // get_actual_string() does.

    return get_string_size_fast(str, textlen);
}

int AW_GC::get_string_size_fast(const char *str, long textlen) const {
    int width = 0;
    if (prvt->font_limits.is_monospaced() || !str) {
        if (!textlen && str) textlen = strlen(str);
        width = textlen * prvt->font_limits.width;
    }
    else {
        for (int c = *(str++); c; c = *(str++)) width += prvt->width_of_chars[c];
    }

    return width;
}



/**
 * memorizes the current settings as default
 */
void AW_GC::establish_default() {
    default_conf = conf;
}

/**
 * restores default settings
 */
void AW_GC::reset() { 
    set_function(default_conf.function);
    set_grey_level(default_conf.grey_level);
    set_line_attributes(default_conf.line_width, default_conf.style);
    //    set_fg_color(default_conf.color);
}

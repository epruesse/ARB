// =============================================================== //
//                                                                 //
//   File      : AW_xfig.cxx                                       //
//   Purpose   : functions for processing xfig graphics            //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "aw_xfig.hxx"
#include <aw_device.hxx>
#include "aw_msg.hxx"

#include <arbdb.h>
#include <climits>

inline int scaleAndRound(int unscaled, double scaleFactor) {
    double scaled = double(unscaled)*scaleFactor;
    return AW_INT(scaled);
}

inline void setMinMax(int value, int& min, int& max) {
    if (value<min) min = value;
    if (value>max) max = value;
}

class Xfig_Eater {
    char *buffer;
    const char *delim;

    char *p; // last return of strtok
    bool failed;

    void nextToken() {
        if (!failed) {
            p = strtok(buffer, delim);
            buffer = 0;
            if (!p) failed = true;
        }
    }

public:
    Xfig_Eater(char *buffer_, const char *delim_) : buffer(buffer_), delim(delim_) {
        p = 0;
        failed = false;
    }

    bool eat_int(int& what) {
        nextToken();
        if (failed) return false;
        what = atoi(p);
        return true;
    }

    bool ignore(unsigned count=1) {
        while (count-->0) {
            nextToken();
            if (failed) return false;
        }
        return true;
    }

    char *get_rest() {
        if (failed || !p) return 0;
        return p+strlen(p)+1;
    }
};

void AW_xfig::calc_scaling(int font_width, int font_height) {
    double font_x_scale = abs(font_width) / (double) XFIG_DEFAULT_FONT_WIDTH;
    double font_y_scale = abs(font_height) / (double) XFIG_DEFAULT_FONT_HEIGHT;

    font_scale = font_x_scale>font_y_scale ? font_x_scale : font_y_scale;
    dpi_scale  = font_scale;
}

AW_xfig::AW_xfig(int font_width, int font_height) {
    // creates the same as loading an empty xfig
    init(font_width, font_height);
}

AW_xfig::AW_xfig(const char *filename, int font_width, int font_height) {
    /* Arguments:   xfig file, fontsize (>0: abort on error - normal usage!
     *                                   <0: returns NULL on error)
     *
     * Returns:     graphical data or NULL or exit!
     *
     * Description: load xfig graphical data for construction of windows,
     */
    aw_assert(filename && filename[0]);
    init(font_width, font_height);

    // ----------------

    GB_ERROR  error  = 0;
    char     *ret;
    char     *buffer = ARB_calloc<char>(MAX_XFIG_LENGTH);
    FILE     *file   = 0;

    if (filename[0]=='/') { // absolute file ?
        strcpy(buffer, filename);
        file = fopen(buffer, "r");
    }
    else {
        const char *fileInLib = GB_path_in_ARBLIB("pictures", filename);

        strcpy(buffer, fileInLib);
        file = fopen(fileInLib, "r");

        // Note: before 12/2008 file was also searched in $ARBHOME and current dir
    }

    if (!file) {
        error = GBS_global_string("Can't locate '%s'", filename);
    }
    else {
        char *expanded_filename = strdup(buffer);
        int   lineNumber        = 0;

        ret = fgets(buffer, MAX_XFIG_LENGTH, file); ++lineNumber;
        if (!ret || strncmp("#FIG", ret, 4)) {
            error = "Expected XFIG format";
        }
        else {
            enum {
                XFIG_UNKNOWN,
                XFIG_OLD_FORMAT, // XFIG 2.1 saves old format
                XFIG_NEW_FORMAT, // XFIG 3.2 saves new format
                XFIG_UNSUPPORTED

            } version = XFIG_UNKNOWN;

            char *xfig_version = strchr(ret, ' ');
            
            if (!xfig_version) {
                error = "Missing version info";
            }
            else {
                int   mainVersion  = 0;
                int   subVersion   = 0;
                *xfig_version++ = 0;
                mainVersion = atoi(xfig_version);

                char *vers_point = strchr(xfig_version, '.');
                if (vers_point) {
                    *vers_point++ = 0;
                    subVersion = atoi(vers_point);
                }
                else {
                    subVersion = 0; // none
                }

                if (mainVersion>3 || (mainVersion==3 && subVersion>2)) {
                    version = XFIG_UNSUPPORTED; // unsupported (maybe only untested)
                    error = "Xfig-format above 3.2 not supported";
                }
                else {
                    if (mainVersion==3 && subVersion==2) {
                        version = XFIG_NEW_FORMAT;
                    }
                    else {
                        version = XFIG_OLD_FORMAT;
                    }
                }
            }
            if (!error) {
                ret             = fgets(buffer, MAX_XFIG_LENGTH, file); ++lineNumber;
                if (!ret) error = "Unexpected end of file";
            }

            if (!error) {
                at_pos_hash = GBS_create_hash(100, GB_MIND_CASE);

                maxx = maxy = 0;
                minx = miny = INT_MAX;

                if (version==XFIG_NEW_FORMAT) { // XFIG 3.2 format
                    // new xfig format has the following changes:
                    //
                    //  - header (see just below)
                    //  - lines do not end with 9999 9999
                    //  - ??? maybe more changes


                    // over-read xfig-header:
                    // Landscape
                    // Center
                    // Metric
                    // A4
                    // 100.00
                    // Single
                    // -2

                    int count;
                    for (count = 0;
                         ret && count<=6;
                         ret=fgets(buffer, MAX_XFIG_LENGTH, file), count++, ++lineNumber)
                    {
                        const char *awaited = 0;
                        switch (count) {
                            case 0: awaited = "Landscape"; break;
                            case 1: awaited = "Center"; break;
                            case 2: awaited = "Metric"; break;
                            case 3: awaited = "A4"; break;
                            case 4: awaited = ""; break; // accept any magnification (accepted only 100 before)
                            case 5: awaited = "Single"; break;
                            case 6: awaited = "-2"; break;
                            default: aw_assert(0);
                        }

                        if (strncmp(ret, awaited, strlen(awaited))!=0) {
                            error = GBS_global_string("'%s' expected", awaited);
                        }
                    }
                }
            }

            if (!error) {
                // read resolution
                int dpi = 80;
                int default_dpi = 80; // used in old version (before 3.2)
                if (ret) {
                    char *p = strtok(ret, "\t");
                    if (p) dpi = atoi(p);

                    ret = fgets(buffer, MAX_XFIG_LENGTH, file); ++lineNumber;

                    if (dpi!=default_dpi) dpi_scale = font_scale * (double(default_dpi)/double(dpi));
                }

                while (ret) {
                    bool  got_nextline = false;
                    char *p;
                    int   width        = 0;
                    int   color        = 0;
                    int   x, y;

                    if (ret[0]=='2') {  // lines
                        int oldx = 0, oldy = 0;

                        {
                            Xfig_Eater args(ret, " \t");

                            bool ok =
                                args.ignore(3) &&       // ignore '2', type, depth
                                args.eat_int(width) &&  // width
                                args.eat_int(color);    // color

                            if (!ok) break;
                        }

                        while (1) {
                            ret = fgets(buffer, MAX_XFIG_LENGTH, file); ++lineNumber;
                            if (!ret) break;
                            if (ret[0]!='\t') {
                                got_nextline = true;
                                break;
                            }

                            Xfig_Eater args(ret, " \t");
                            bool ok = true;
                            oldx = oldy = INT_MAX;

                            while (ok) {
                                ok = args.eat_int(x) && args.eat_int(y);
                                if (!ok) break;

                                // 9999/9999 is the end of line-points marker in old version
                                if (version==XFIG_OLD_FORMAT && x==9999 && y==9999) break;

                                x = scaleAndRound(x, dpi_scale);
                                y = scaleAndRound(y, dpi_scale);

                                setMinMax(x, minx, maxx);
                                setMinMax(y, miny, maxy);

                                aw_assert(x>=0 && y>=0);

                                if (oldx == INT_MAX && oldy == INT_MAX) {
                                    oldx = x;
                                    oldy = y;
                                    continue;
                                }

                                struct AW_xfig_line *xline = new AW_xfig_line;
                                if (width >= MAX_LINE_WIDTH) width = MAX_LINE_WIDTH - 1;
                                xline->next = line[width];
                                line[width] = xline;
                                xline->x0 = oldx;
                                xline->y0 = oldy;
                                xline->x1 = x;
                                xline->y1 = y;
                                oldx = x;
                                oldy = y;
                                xline->color = color;
                            }
                        }

                    }
                    else if (ret[0]=='4') { // text
                        int align;
                        int fontnr   = -1;
                        int fontsize = -1;

                        // old format: 4 align font  fontsize   depth   color ???       angle justi flags width x y text
                        // new format: 4 align color depth      ???     font  fontsize  angle justi flags width x y text
                        //                     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

                        Xfig_Eater args(ret, " \t");

                        bool ok =
                            args.ignore(1) &&       // the '4'
                            args.eat_int(align);    // align

                        if (ok) {
                            if (version==XFIG_OLD_FORMAT) {
                                ok =
                                    args.eat_int(fontnr) &&         // font
                                    args.eat_int(fontsize) &&       // fontsize
                                    args.ignore(1) &&               // depth
                                    args.eat_int(color) &&          // color
                                    args.ignore(1);                 // ???
                            }
                            else {
                                aw_assert(version==XFIG_NEW_FORMAT);

                                ok =
                                    args.eat_int(color) &&          // color
                                    args.ignore(1) &&               // depth
                                    args.ignore(1) &&               // ???
                                    args.eat_int(fontnr) &&         // font
                                    args.eat_int(fontsize);         // fontsize
                            }

                            if (ok) {
                                ok =
                                    args.ignore(3) &&           // angle, justi, flags
                                    args.eat_int(width) &&      // width
                                    args.eat_int(x) &&          // x
                                    args.eat_int(y);            // y

                            }
                        }

                        if (ok && (p=args.get_rest())!=0) {
                            x = scaleAndRound(x, dpi_scale);
                            y = scaleAndRound(y, dpi_scale);

                            while (*p==' ' || *p=='\t') ++p;

                            char *endf = strchr(p, 1); // search for ASCII-1 (new EOL-marker)
                            char *endf2 = GBS_find_string(p, "\\001", 0); // search for "\001" (Pseudo-ASCII-1)
                            if (endf || endf2) {
                                if (endf) *endf = 0;
                                if (endf2) *endf2 = 0;
                            }

                            if (*p=='$') {      // text starts with a '$'
                                // place a button
                                if (strcmp(p, "$$") == 0) {
                                    this->centerx = x;
                                    this->centery = y;
                                }
                                else {
                                    struct AW_xfig_pos *xpos = new AW_xfig_pos;

                                    xpos->center = align;
                                    xpos->x      = x;
                                    xpos->y      = y;

                                    GBS_write_hash(at_pos_hash, p+1, (long)xpos);
                                }
                            }
                            else {
                                struct AW_xfig_text *xtext = new AW_xfig_text;
                                xtext->x = x;
                                xtext->y = y;

                                if (x>maxx) maxx = x;
                                if (y>maxy) maxy = y;
                                if (x<minx) minx = x;
                                if (y<miny) miny = y;

                                xtext->text = strdup(p);
                                xtext->fontsize = fontsize;
                                xtext->color = color;
                                xtext->center = align;
                                xtext->font = fontnr;
                                xtext->next = text;
                                text = xtext;
                            }
                        }

                    }

                    if (!got_nextline) {
                        ret = fgets(buffer, MAX_XFIG_LENGTH, file); ++lineNumber;
                    }
                }

                this->size_x = maxx - minx;
                this->size_y = maxy - miny;
            }
        }

        if (error) { // append file-info to error
            aw_assert(expanded_filename);
            error = GBS_global_string("While reading %s:%i:\nError: %s", expanded_filename, lineNumber, error);
        }

        free(expanded_filename);
        fclose(file);
    }

    free(buffer);

    if (error) {
        error = GBS_global_string("Failed to read XFIG resource (defect installation?)\n"
                                  "Reason: %s", error);

        if (font_width>0 && font_height>0) { // react with fatal exit
            GBK_terminate(error);
        }

        // special case (used by aw_read_xfigfont())
        aw_message(error);
    }
}

static long aw_xfig_hash_free_loop(const char *, long val, void *) {
    free((void*)val);
    return 0;
}

AW_xfig::~AW_xfig()
{
    int i;

    if (at_pos_hash) {
        GBS_hash_do_loop(at_pos_hash, aw_xfig_hash_free_loop, NULL);
        GBS_free_hash(at_pos_hash);
    }
    struct AW_xfig_text *xtext;
    while (text) {
        xtext = text;
        text = text->next;
        delete xtext->text;
        delete xtext;
    }

    struct AW_xfig_line *xline;
    for (i=0; i<MAX_LINE_WIDTH; i++) {
        while (line[i]) {
            xline = line[i];
            line[i] = xline->next;
            delete xline;
        }
    }
}

void AW_xfig::print(AW_device *device) {
    const AW_screen_area&  window_size = device->get_area_size();
    device->clear(-1);
    struct AW_xfig_text   *xtext;
    for (xtext = text; xtext; xtext=xtext->next) {
        char *str = xtext->text;

        if (str[0]) {
            int   x   = xtext->x;
            int   y   = xtext->y;

            if (str[1]) {
                if (str[1] == ':') {
                    if (str[0] == 'Y') {
                        y   += (window_size.b - window_size.t)- size_y;
                        str += 2;
                    }
                    else if (str[0] == 'X') {
                        x   += (window_size.r - window_size.l)- size_x;
                        str += 2;
                    }
                }
                else if (str[2] == ':' && str[0] == 'X' && str[1] == 'Y') {
                    x   += (window_size.r - window_size.l)- size_x;
                    y   += (window_size.b - window_size.t)- size_y;
                    str += 3;
                }
            }

            device->text(xtext->gc, str, (AW_pos)x, (AW_pos)y, (AW_pos)xtext->center*.5, AW_ALL_DEVICES_UNSCALED);
        }
    }

    struct AW_xfig_line *xline;
    for (int i=0; i<MAX_LINE_WIDTH; i++) {
        int line_width = std::max(1, scaleAndRound(i, font_scale));
        device->set_line_attributes(0, line_width, AW_SOLID);
        for (xline = line[i]; xline; xline=xline->next) {
            device->line(0, (AW_pos)xline->x0, (AW_pos)xline->y0, (AW_pos)xline->x1, (AW_pos)xline->y1);
        }
    }
}

void AW_xfig::create_gcs(AW_device *device, int depth)
{
    GB_HASH *gchash;
    int gc;
    char fontstring[100];
    gchash = GBS_create_hash(100, GB_MIND_CASE);

    struct AW_xfig_text *xtext;
    gc = 0;
    device->new_gc(gc);   // create at least one gc ( 0 ) for the lines
    device->set_foreground_color(gc, AW_WINDOW_FG);
    if (depth<=1) device->set_function(gc, AW_XOR);
    device->set_line_attributes(gc, 1, AW_SOLID);
    gc = 1;         // create gc for texts
    for (xtext = text; xtext; xtext=xtext->next) {
        sprintf(fontstring, "%i-%i", xtext->font, scaleAndRound(xtext->fontsize, font_scale));
        if (!(xtext->gc = (int)GBS_read_hash(gchash, fontstring))) {
            device->new_gc(gc);
            device->set_line_attributes(gc, 1, AW_SOLID);
            device->set_font(gc, xtext->font, scaleAndRound(xtext->fontsize, font_scale), 0);
            device->set_foreground_color(gc, AW_WINDOW_FG);
            if (depth<=1) device->set_function(gc, AW_XOR);
            xtext->gc = gc;
            GBS_write_hash(gchash, fontstring, gc);
            gc++;
        }
    }
    GBS_free_hash(gchash);
}

void AW_xfig::add_line(int x1, int y1, int x2, int y2, int width) { // add a line to xfig
    struct AW_xfig_line *xline = new AW_xfig_line;

    x1 = scaleAndRound(x1, dpi_scale);
    x2 = scaleAndRound(x2, dpi_scale);
    y1 = scaleAndRound(y1, dpi_scale);
    y2 = scaleAndRound(y2, dpi_scale);

    setMinMax(x1, minx, maxx);
    setMinMax(y1, miny, maxy);
    setMinMax(x2, minx, maxx);
    setMinMax(y2, miny, maxy);

    xline->x0 = x1;
    xline->y0 = y1;
    xline->x1 = x2;
    xline->y1 = y2;

    xline->color = 1;

    if (width >= MAX_LINE_WIDTH) width = MAX_LINE_WIDTH - 1;

    xline->next = line[width];
    line[width] = xline;
}

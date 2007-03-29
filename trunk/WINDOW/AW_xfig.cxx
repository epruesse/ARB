/* -----------------------------------------------------------------
 * Module:                        WINDOW/AW_xfig.cxx
 *
 * Exported Classes:              xfig
 *
 * Description: functions for processing xfig graphics for requesters, ...
 *
 * Integration Notes:
 *
 * 1) cleanup of fopen
 *
 * -----------------------------------------------------------------
 */

/*
 * $Header$
 *
 * $Log$
 * Revision 1.8  2007/03/29 16:53:43  westram
 * - use both font dimensions to calculate scaling factor
 * - added function calc_scaling() and removed dup-code
 *
 * Revision 1.7  2005/03/04 23:14:24  westram
 * - fixed calls to AW_device::clear/clear_part
 *
 * Revision 1.6  2005/01/28 14:32:04  westram
 * - fixed set_font() call
 *
 * Revision 1.5  2003/08/13 11:24:35  westram
 * - Text elements may be X/Y attached
 *
 * Revision 1.4  2002/11/21 18:21:50  westram
 * memory bugfixes
 *
 * Revision 1.3  2001/08/17 19:57:45  westram
 * * AW_xfig.cxx: - implemented AW_xfig::add_line
 *            - implemented new constructor
 *
 * Revision 1.2  2000/11/24 17:23:46  westram
 * *** empty log message ***
 *
 * Revision 1.1.1.1  2000/11/23 09:41:17  westram
 * Erster Import
 *
 * Revision 1.8  1995/03/13  16:53:41  jakobi
 * *** empty log message ***
 *
 * Revision 1.7  1995/03/13  15:22:33  jakobi
 * *** empty log message ***
 *
 * Revision 1.7  1995/03/13  15:22:33  jakobi
 * *** empty log message ***
 *
 * Revision 1.6  1995/03/13  12:23:48  jakobi
 * *** empty log message ***
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <arbdb.h>              // hash functions
#include "aw_root.hxx"
#include "aw_device.hxx"
#include "aw_xfig.hxx"

/* -----------------------------------------------------------------
 * Function:                     AW_xfig::AW_xfig
 *
 * Arguments:                    xfig file, fontsize (>0: abort on error - use for requesters,
 *                                                    <0: returns NULL on error)
 *
 * Returns:                      graphical data or NULL or exit!
 *
 * Description:                  load xfig graphical data for construction of requesters,
 *                               is also used to load line-data of vectorfont
 *
 * NOTE:
 *
 * Dependencies:
 * -----------------------------------------------------------------
 */

// --------------------------------------------------------------------------------
//     inline int scaleAndRound(int unscaled, double scaleFactor)
// --------------------------------------------------------------------------------
inline int scaleAndRound(int unscaled, double scaleFactor) {
    double scaled = double(unscaled)*scaleFactor;
    return int(scaled);
    //return scaled>0 ? int(scaled+0.5) : -int(-scaled+0.5);
}

//  -------------------------------------------------------------
//      inline void setMinMax(int value, int& min, int& max)
//  -------------------------------------------------------------
inline void setMinMax(int value, int& min, int& max) {
    if (value<min) min = value;
    if (value>max) max = value;
}

// --------------------------------------------------------------------------------
//     class Xfig_Eater
// --------------------------------------------------------------------------------
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

    bool eat_int(int& what)  {
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
    memset(this,0,sizeof(AW_xfig));
    calc_scaling(font_width, font_height);
}

AW_xfig::AW_xfig(const char *filename, int font_width, int font_height)
{
    int bail_out=0;
    const char *error_notice = "unknown";
    char *ret;
    char *expanded_filename = 0;
    enum {
        XFIG_UNKNOWN,
        XFIG_OLD_FORMAT, // XFIG 2.1 saves old format
        XFIG_NEW_FORMAT, // XFIG 3.2 saves new format
        XFIG_UNSUPPORTED

    } version = XFIG_UNKNOWN;

    if(!filename || !strlen(filename)) return;
    memset(this,0,sizeof(AW_xfig));
    
    calc_scaling(font_width, font_height);

    const char *arbhome = GB_getenvARBHOME();
    char *buffer = (char *)calloc(sizeof(char),MAX_XFIG_LENGTH);

    FILE *file;
    if (filename[0]=='/') {
        // absolute file ?
        strcpy(buffer,filename);
        file=fopen(buffer,"r");
    } else {
        // in $ARBHOME/lib/pictures ?
        sprintf(buffer,"%s/lib/pictures/%s",arbhome,filename);
        file=fopen(buffer,"r");
        if (!file) {
            // in $ARBHOME ?
            sprintf(buffer,"%s/%s",arbhome,filename);
            file = fopen(buffer,"r");
            if (!file) {
                // relative to $CWD?
                sprintf(buffer,"%s",filename);
                file = fopen(buffer,"r");
            }
        }
    }
    if (!file) bail_out=1;

    expanded_filename = strdup(buffer);
    int mainVersion = 0;
    int subVersion = 0;
    int lineNumber = 0;

    if (!bail_out) {
        ret = fgets(buffer,MAX_XFIG_LENGTH,file); ++lineNumber;
        if (!ret || strncmp("#FIG",ret,4) ) {
            bail_out=1;
        }
        else {
            char *xfig_version = strchr(ret, ' ');
            if (!xfig_version) {
                bail_out = 1;
            }
            else {
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
                    error_notice = "Xfig-format above 3.2 not supported";
                    bail_out = 1;
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
        }
    }

    if (!bail_out) {
        ret = fgets(buffer,MAX_XFIG_LENGTH,file); ++lineNumber;
        if (!ret ) bail_out=1;
    }

    if (bail_out) {
    do_bail_out:
        int fatal_exit = 0;

        aw_assert(error_notice);
        aw_assert(expanded_filename);
        
        if (font_width>0 && font_height>0) { // react with fatal exit
            fprintf(stderr,"Error: Xfig Resource File %s not found or invalid file - programmer's error or defect installation (circumstances: %s)\n",
                    expanded_filename, error_notice);
            fatal_exit = 1;
        }
        else {
            sprintf(AW_ERROR_BUFFER,"Warning: Requested Xfig Resource File %s not found or invalid (circumstances: %s).\n",
                    expanded_filename, error_notice);
            aw_message();
        }

        free(expanded_filename);
        delete buffer;
        if (file) fclose(file);

        // exit or return
        if (fatal_exit) {
            fprintf(stderr, "Cannot continue - terminating.\n");
            exit(-1);
        }
        return;
    }


    // failure of the constructor is shown as missing hash/text/lines on return

    // if (font_size<0) font_size = abs(font_size);

    hash = GBS_create_hash(100,0);
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
             ret=fgets(buffer,MAX_XFIG_LENGTH,file), count++,  ++lineNumber)
        {
            const char *awaited = 0;
            switch (count) {
                case 0: awaited = "Landscape"; break;
                case 1: awaited = "Center"; break;
                case 2: awaited = "Metric"; break;
                case 3: awaited = "A4"; break;
                case 4: awaited = "100.00"; break;
                case 5: awaited = "Single"; break;
                case 6: awaited = "-2"; break;
                default: aw_assert(0);
            }

            if (strncmp(ret, awaited, strlen(awaited))!=0) {
                error_notice = GBS_global_string("'%s' awaited in line #%i", awaited, lineNumber);
                goto do_bail_out;
            }
        }
    }

    // read resolution
    int dpi = 80;
    int default_dpi = 80; // used in old version (before 3.2)
    if (ret) {
        char *p = strtok(ret, "\t");
        if (p) dpi = atoi(p);

        ret = fgets(buffer,MAX_XFIG_LENGTH,file); ++lineNumber;

        if (dpi!=default_dpi) dpi_scale = font_scale * (double(default_dpi)/double(dpi));
    }

    while (ret) {
        AW_BOOL got_nextline = AW_FALSE;
        char *p;
        int width =0;
        int color = 0;
        int x,y;

        if (ret[0]=='2')  { // lines
            int oldx =0,oldy =0;

            {
                Xfig_Eater args(ret, " \t");

                bool ok =
                    args.ignore(3) &&       // ignore '2', type, depth
                    args.eat_int(width) &&  // width
                    args.eat_int(color);    // color

                if (!ok) break;
            }

            while (1) {
                ret = fgets(buffer,MAX_XFIG_LENGTH,file);  ++lineNumber;
                if (!ret) break;
                if (ret[0]!='\t') {
                    got_nextline = AW_TRUE;
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
                    //                     x = (int)(x*scale);
                    //                     y = (int)(y*scale);

                    setMinMax(x, minx, maxx);
                    setMinMax(y, miny, maxy);

                    aw_assert(x>=0 && y>=0);

                    if (oldx == INT_MAX && oldy == INT_MAX) {
                        oldx = x;
                        oldy = y;
                        continue;
                    }

                    struct AW_xfig_line *xline = new AW_xfig_line;
                    if (width>= MAX_LINE_WIDTH) width = MAX_LINE_WIDTH - 1;
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

        } // end if ('2')

        else if (ret[0]=='4') { // text
            int align;
            int fontnr;
            int fontsize;

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
                    if (!strcmp(p,"$$")) {
                        this->centerx = x;
                        this->centery = y;
                    }else{
                        struct AW_xfig_pos *xpos = new AW_xfig_pos;
                        xpos->center = align;
                        xpos->x = x; xpos->y = y;
                        GBS_write_hash(hash,p+1,(long)xpos);
                    }
                }else{
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

        } // end if ('4')

        if (!got_nextline) {
            ret = fgets(buffer,MAX_XFIG_LENGTH,file);  ++lineNumber;
        }
    }

    this->size_x = maxx - minx;
    this->size_y = maxy - miny;
    free(buffer);
    free(expanded_filename);
}

long aw_xfig_hash_free_loop(const char *key,long val)
{
    AWUSE(key);
    if (val) free( (char *)val);
    return 0;
}

AW_xfig::~AW_xfig(void)
{
    int i;

    if (hash) {
        GBS_hash_do_loop(hash,(gb_hash_loop_type)aw_xfig_hash_free_loop);
        GBS_free_hash(hash);
    }
    struct AW_xfig_text *xtext;
    while(text) {
        xtext = text;
        text = text->next;
        delete xtext->text;
        delete xtext;
    }

    struct AW_xfig_line *xline;
    for (i=0;i<MAX_LINE_WIDTH; i++){
        while(line[i]){
            xline = line[i];
            line[i] = xline->next;
            delete xline;
        }
    }
}

void AW_xfig::print(AW_device *device)
{
    int i;
    AW_rectangle ws;    // window size
    device->get_area_size(&ws);
    device->clear(-1);
    struct AW_xfig_text *xtext;
    for (xtext = text; xtext; xtext=xtext->next){
        char *str = xtext->text;
        int   x   = xtext->x;
        int   y   = xtext->y;

        if (str[1] == ':') {
            if (str[0] == 'Y') {
                y   += (ws.b - ws.t)- size_y;
                str += 2;
            }
            else if (str[0] == 'X') {
                x   += (ws.r - ws.l)- size_x;
                str += 2;
            }
        }
        else if (str[2] == ':' && str[0] == 'X' && str[1] == 'Y') {
            x   += (ws.r - ws.l)- size_x;
            y   += (ws.b - ws.t)- size_y;
            str += 3;
        }

        device->text(xtext->gc,str,(AW_pos)x,(AW_pos)y,(AW_pos)xtext->center*.5,-1,0,0);        // filter
    }

    struct AW_xfig_line *xline;
    for (i=0;i<MAX_LINE_WIDTH; i++){
        device->set_line_attributes(0,(AW_pos)scaleAndRound(i, font_scale),AW_SOLID);
        //device->set_line_attributes(0,(AW_pos)i,AW_SOLID);
        for (xline = line[i]; xline; xline=xline->next){
            device->line(0, (AW_pos)xline->x0,(AW_pos)xline->y0,
                         (AW_pos)xline->x1,(AW_pos)xline->y1,
                         -1,0,0);               // filter
        }
    }
}

void AW_xfig::create_gcs(AW_device *device, int depth)
{
    GB_HASH *gchash;
    int gc;
    char fontstring[100];
    gchash = GBS_create_hash(100,0);

    struct AW_xfig_text *xtext;
    gc = 0;
    device->new_gc( gc ); // create at least one gc ( 0 ) for the lines
    device->set_foreground_color( gc, AW_WINDOW_FG );
    if (depth<=1) device->set_function( gc,AW_XOR);
    device->set_line_attributes( gc, 0.3, AW_SOLID );
    gc = 1;         // create gc for texts
    for (xtext = text; xtext; xtext=xtext->next){
        sprintf(fontstring,"%i-%i",xtext->font,scaleAndRound(xtext->fontsize, font_scale));
        //sprintf(fontstring,"%i-%i",xtext->font,(int)(dpi_scale * xtext->fontsize));
        if ( !(xtext->gc = (int)GBS_read_hash(gchash,fontstring)) ) {
            device->new_gc( gc );
            device->set_line_attributes( gc, 0.3, AW_SOLID );
            device->set_font( gc, xtext->font, scaleAndRound(xtext->fontsize, font_scale), 0);
            //device->set_font( gc, xtext->font, (int)(scale * xtext->fontsize) );
            device->set_foreground_color( gc, AW_WINDOW_FG );
            if (depth<=1) device->set_function( gc,AW_XOR);
            xtext->gc = gc;
            GBS_write_hash(gchash,fontstring,gc);
            gc++;
        }
    }
    GBS_free_hash(gchash);
}

//  --------------------------------------------------------------------------
//      void AW_xfig::add_line(int x1, int y1, int x2, int y2, int width)
//  --------------------------------------------------------------------------
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

    if (width>= MAX_LINE_WIDTH) width = MAX_LINE_WIDTH - 1;

    xline->next = line[width];
    line[width] = xline;
}

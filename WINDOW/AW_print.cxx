#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include "aw_root.hxx"
#include "aw_device.hxx"
#include "aw_commn.hxx"
#include "aw_print.hxx"

//*****************************************************************************************
//			device_print
//*****************************************************************************************

AW_device_print::AW_device_print(AW_common *commoni) : AW_device(commoni) {
    out = 0;
}

void AW_device_print::init() {
    ;
}

AW_DEVICE_TYPE AW_device_print::type(void) { return AW_DEVICE_PRINTER; }

/******************************************************************************************/
/* line  text  zoomtext  box *******************************************************************************************/
/******************************************************************************************/


int AW_device_print::line(int gc, AW_pos x0,AW_pos y0, AW_pos x1,AW_pos y1, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    register class AW_GC_Xm *gcm = AW_MAP_GC(gc);
    AW_pos X0,Y0,X1,Y1;	// Transformed pos
    AW_pos CX0,CY0,CX1,CY1;	// Clipped line
    int	drawflag = 0;

    if(filteri & filter) {
        this->transform(x0,y0,X0,Y0);
        this->transform(x1,y1,X1,Y1);
        drawflag = this->clip(X0,Y0,X1,Y1,CX0,CY0,CX1,CY1);
        if (drawflag) {
            int line_width                = gcm->line_width;
            if (line_width<=0) line_width = 1;
            AWUSE(cd1);
            AWUSE(cd2);

            aw_assert(out); // file has to be good!
            fprintf(out, "2 1 0 %d 0 0 0 0.000 0 0 0\n\t%d %d %d %d 9999 9999\n",
                    (int)line_width,(int)CX0,(int)CY0,(int)CX1,(int)CY1);
        }
    }
    return drawflag;
}

int AW_draw_string_on_printer(AW_device *devicei, int gc, const char *str, size_t /*opt_strlen*/,size_t start, size_t size,
                              AW_pos x,AW_pos y, AW_pos opt_ascent,AW_pos opt_descent,
                              AW_CL cduser, AW_CL cd1, AW_CL cd2)
{
    AW_pos X,Y;
    AW_device_print *device = (AW_device_print *)devicei;
    AW_common *common = device->common;
    register class AW_GC_Xm *gcm = AW_MAP_GC(gc);

    AWUSE(cd1);AWUSE(cd2);AWUSE(opt_ascent);AWUSE(opt_descent);AWUSE(cduser);
    device->transform(x,y,X,Y);
    char *pstr = strdup(str+start);
    if (size < strlen(pstr)) pstr[size] = 0;
    else size  = strlen(pstr);
    size_t i;
    for (i=0;i<size;i++) {
        if (pstr[i] < ' ') pstr[i] = '?';
    }
    int fontnr = common->root->font_2_xfig(gcm->fontnr);
    if (fontnr<0) fontnr = - fontnr;
    if (str[0]) {
        fprintf(device->get_FILE(), "4 0 %d %d 0 0 0 0.000 4 %d %d %d %d ",
                fontnr,
                gcm->fontsize,
                (int)gcm->fontinfo.max_letter_height,
                (int)device->get_string_size(gc,str,0),
                AW_INT(X),AW_INT(Y));
        char *p;
        for (p = pstr; *p; p++) {
            if (*p >= 32) putc(*p,device->get_FILE());
        }
        fputc(1,device->get_FILE());
        fputc('\n',device->get_FILE());
    }
    free(pstr);
    return 1;
}

const char *AW_device_print::open(const char *path)
{
    if (out) {
        aw_error("You cannot reopen a device",0);
        fclose (out);
    }
    out = fopen(path,"w");
    if (!out) return "Sorry, I cannot open the file";
    fprintf(out,"#FIG 2.1\n");
    fprintf(out,"80 2\n");	/* 80 dbi Version 2 */
    return 0;
}

void AW_device_print::close(void){
    if (out) fclose(out);
    out = 0;
}


int AW_device_print::text(int gc, const char *str,AW_pos x,AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2, long opt_strlen) {
    return text_overlay(gc,str,opt_strlen,x,y,alignment,filteri,(AW_CL)this, cd1,cd2,0.0,0.0,AW_draw_string_on_printer);
}

int AW_device_print::box(int gc, AW_pos x0,AW_pos y0,AW_pos width,AW_pos height, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    AW_pos q[8];
    q[0] = x0;		q[1] = y0;
    q[2] = x0 + width;	q[3] = y0;
    q[4] = x0 + width;	q[5] = y0 + height;
    q[6] = x0;		q[7] = y0 + height;
    return this->filled_area(gc,4,q,filteri,cd1,cd2);
}

int AW_device_print::circle(int gc, AW_BOOL /*filled*/, AW_pos x0,AW_pos y0,AW_pos width,AW_pos height, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    AWUSE(cd1);AWUSE(cd2);
    register class AW_GC_Xm *gcm = AW_MAP_GC(gc);
    AW_pos x1,y1;
    AW_pos X0,Y0,X1,Y1;	// Transformed pos
    AW_pos CX0,CY0,CX1,CY1;	// Clipped line

    if(filteri & filter) {
        width *= get_scale();
        height *= get_scale();

        x1 = x0 + width;
        y1 = y0 + height;
        this->transform(x0,y0,X0,Y0);
        this->transform(x1,y1,X1,Y1);
        int drawflag = this->box_clip(X0,Y0,X1,Y1,CX0,CY0,CX1,CY1);
        if (drawflag) {
            AWUSE(cd1);
            AWUSE(cd2);
            short greylevel = (short)(gcm->grey_level*22);
            if (greylevel>21) greylevel = 21;

            int line_width = gcm->line_width;
            if (line_width<=0) line_width = 1;
            fprintf(out,"1 3  0 %d -1 0 0 %d 0.0000 1 0.000 %d %d %d %d %d %d %d %d\n",line_width,greylevel,
                    (int)CX0,(int)CY0,
                    (int)width,(int)height,
                    (int)CX0,(int)CY0,
                    (int)(CX0+width),(int)CY0);
        }
    }
    return 0;
}

int AW_device_print::filled_area(int gc, int npoints, AW_pos *points, AW_bitset filteri, AW_CL cd1, AW_CL cd2){
    int erg = 0;
    int i;
    if (	!(filteri & this->filter) ) return 0;
    erg |= AW_device::filled_area(gc,npoints,points,filteri,cd1,cd2);
    if (!erg) return 0;				// no line visible -> no area fill

    register class AW_GC_Xm *gcm = AW_MAP_GC(gc);
    AW_pos x,y;
    AW_pos X,Y;	// Transformed pos
    AW_pos CX0,CY0,CX1,CY1;	// Clipped line
    short greylevel = (short)(gcm->grey_level*22);
    if (greylevel>21) greylevel = 21;

    int line_width = gcm->line_width;
    if (line_width<=0) line_width = 1;

    fprintf(out, "2 3 0 %d -1 0 0 %d 0.000 -1 0 0\n",line_width, greylevel);

    for (i=0; i < npoints; i++) {
        x = points[2*i];
        y = points[2*i+1];
        this->transform(x,y,X,Y);
        this->box_clip(X,Y,0,0,CX0,CY0,CX1,CY1);
        fprintf(out,"	%d %d\n",(int)CX0,(int)CY0);
    }
    x = points[0];
    y = points[1];
    this->transform(x,y,X,Y);
    this->box_clip(X,Y,0,0,CX0,CY0,CX1,CY1);
    fprintf(out,"	%d %d 	9999 9999\n",(int)CX0,(int)CY0);

    return 1;
}

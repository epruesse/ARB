#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
// #include <malloc.h>
#include <math.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include <aw_root.hxx>
#include "aw_device.hxx"
#include "aw_commn.hxx"
#include <aw_Xm.hxx>

//*****************************************************************************************
//          device_Xm
//*****************************************************************************************

AW_device_Xm::AW_device_Xm(AW_common *commoni) : AW_device(commoni) {
;
}

void AW_device_Xm::init() {
;
}


AW_DEVICE_TYPE AW_device_Xm::type(void) { return AW_DEVICE_SCREEN; }


/******************************************************************************************/
/* line  text  zoomtext  box *******************************************************************************************/
/******************************************************************************************/


int AW_device_Xm::line(int gc, AW_pos x0,AW_pos y0, AW_pos x1,AW_pos y1, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    register class AW_GC_Xm *gcm = AW_MAP_GC(gc);
    AW_pos X0,Y0,X1,Y1; // Transformed pos
    AW_pos CX0,CY0,CX1,CY1; // Clipped line
    int drawflag = 0;

    if(filteri & filter) {
        this->transform(x0,y0,X0,Y0);
        this->transform(x1,y1,X1,Y1);
        drawflag = this->clip(X0,Y0,X1,Y1,CX0,CY0,CX1,CY1);
        if (drawflag) {
            AWUSE(cd1);
            AWUSE(filter);
            AWUSE(cd2);

            XDrawLine(common->display, common->window_id,
                      gcm->gc,(int)CX0,(int)CY0,(int)CX1,(int)CY1);
        }
    }

    AUTO_FLUSH(this);

    return drawflag;
}

int AW_draw_string_on_screen(AW_device *device, int gc,const  char *str, size_t /*opt_str_len*/,size_t start, size_t size,
                             AW_pos x,AW_pos y, AW_pos opt_ascent,AW_pos opt_descent,
                             AW_CL cduser, AW_CL cd1, AW_CL cd2)
{
    AW_pos X,Y;
    AWUSE(cd1);AWUSE(cd2);AWUSE(opt_ascent);AWUSE(opt_descent);AWUSE(cduser);
    device->transform(x,y,X,Y);
    aw_assert(size <= strlen(str));
    XDrawString(device->common->display, device->common->window_id, device->common->gcs[gc]->gc,
                AW_INT(X), AW_INT(Y), str + start , (int)size);

    AUTO_FLUSH(device);

    return 1;
}


int AW_device_Xm::text(int gc, const char *str,AW_pos x,AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2,long opt_strlen) {
    return text_overlay(gc,str,opt_strlen,x,y,alignment,filteri,(AW_CL)this, cd1,cd2,0.0,0.0,AW_draw_string_on_screen);
}

int AW_device_Xm::box(int gc, AW_pos x0,AW_pos y0,AW_pos width,AW_pos height, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    AWUSE(cd1);AWUSE(cd2);
    register class AW_GC_Xm *gcm = AW_MAP_GC(gc);
    AW_pos x1,y1;
    AW_pos X0,Y0,X1,Y1; // Transformed pos
    AW_pos CX0,CY0,CX1,CY1; // Clipped line
    int drawflag = 0;
    short greylevel = (short)(gcm->grey_level*22);
    if (greylevel>21) greylevel = 21;

    if(filteri & filter) {
        x1 = x0 + width;
        y1 = y0 + height;
        this->transform(x0,y0,X0,Y0);
        this->transform(x1,y1,X1,Y1);
        drawflag = this->box_clip(X0,Y0,X1,Y1,CX0,CY0,CX1,CY1);
        if (drawflag) {
            AWUSE(cd1);
            AWUSE(cd2);

            XFillRectangle(common->display, common->window_id, gcm->gc,
                           (int)CX0, (int)CY0,
                           ((int)CX1)-((int)CX0), ((int)CY1)-((int)CY0) );
        }
    }

    AUTO_FLUSH(this);

    return 0;
}

int AW_device_Xm::circle(int gc, AW_BOOL filled, AW_pos x0,AW_pos y0,AW_pos width,AW_pos height, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    AWUSE(cd1);AWUSE(cd2);
    register class AW_GC_Xm *gcm = AW_MAP_GC(gc);
    //AW_pos x1,y1;
    AW_pos X0,Y0,X1,Y1; // Transformed pos
    AW_pos XL,YL;   // Left edge of circle pos
    AW_pos CX0,CY0,CX1,CY1; // Clipped line
    int drawflag = 0;
    short greylevel = (short)(gcm->grey_level*22);
    if (greylevel>21) greylevel = 21;

    if(filteri & filter) {

        this->transform(x0,y0,X0,Y0);   // center

        x0 -= width;
        y0 -= height;
        this->transform(x0,y0,XL,YL);

        X1= X0 + 2.0; Y1 = Y0 + 2.0;
        X0 -= 2.0; Y0 -= 2.0;
        drawflag = this->box_clip(X0,Y0,X1,Y1,CX0,CY0,CX1,CY1);
        if (drawflag) {
            AWUSE(cd1);
            AWUSE(cd2);
            width  *= 2.0 * this->get_scale();
            height *= 2.0 * this->get_scale();
            if (!filled) {
                XDrawArc(common->display, common->window_id, gcm->gc, (int)XL,(int)YL, (int)width, (int)height,0,64*360 );
            }
             else {
                XFillArc(common->display, common->window_id, gcm->gc, (int)XL,(int)YL, (int)width, (int)height,0,64*360 );
            }
        }
    }

    AUTO_FLUSH(this);

    return 0;
}

void AW_device_Xm::clear() {
    XClearWindow(common->display,common->window_id);

    AUTO_FLUSH(this);
}

void AW_device_Xm::clear_part( AW_pos x, AW_pos y,AW_pos width, AW_pos height ) {
AW_pos X,Y;                                 // Transformed pos

    this->transform( x, y, X, Y );
    if ( X > this->clip_rect.r ) return;
    if ( X < this->clip_rect.l ) {
        width = width + X - this->clip_rect.l;
        X = this->clip_rect.l;
    }
    if( X + width > this->clip_rect.r ) {
        width = this->clip_rect.r - X;
    }
    if ( Y < this->clip_rect.t ) {
        height = height - ( this->clip_rect.t - Y );
        Y = this->clip_rect.t;
    }
    if ( Y + height > this->clip_rect.b ) {
        height = this->clip_rect.b - Y;
    }
    if ( Y > this->clip_rect.b ) return;
    if ( width <= 0 || height <= 0 ) return;

    XClearArea(common->display,common->window_id,(int)X,(int)Y,(int)width,(int)height,False);

    AUTO_FLUSH(this);
}


void AW_device_Xm::clear_text(int gc, const char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2) {
    AWUSE(filteri);AWUSE(cd1);AWUSE(cd2);
    register class AW_GC_Xm *gcm = AW_MAP_GC(gc);
    register XFontStruct    *xfs = &gcm->curfont;
    AW_pos                      X,Y;            // Transformed pos
    AW_pos                      width, height;
    long textlen = strlen(string);
    this->transform(x,y,X,Y);
    width = get_string_size(gc,string,textlen);
    height = xfs->max_bounds.ascent + xfs->max_bounds.descent;
    X = common->x_alignment(X,width,alignment);

    if (X > this->clip_rect.r) return;
    if (X < this->clip_rect.l) {
        width = width + X - this->clip_rect.l;
        X = this->clip_rect.l;
    }

    if(X + width > this->clip_rect.r) {
        width = this->clip_rect.r - X;
    }

    if (Y < this->clip_rect.t) return;
    if (Y > this->clip_rect.b) return;
    if ( width <= 0 || height <= 0 ) return;

    XClearArea(common->display, common->window_id,
               (int)X,(int)Y-(int)xfs->max_bounds.ascent,(int)width,(int)height,False);

    AUTO_FLUSH(this);
}


void AW_device_Xm::fast() {
    fastflag = 1;
}


void AW_device_Xm::slow() {
    fastflag = 0;
}



void AW_device_Xm::flush(void) {
    XFlush(common->display);
}


void AW_device_Xm::move_region( AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y ) {
    int             gc  = 0;
    class AW_GC_Xm *gcm = AW_MAP_GC(gc);
    XCopyArea( common->display, common->window_id, common->window_id,    gcm->gc,
               (int)src_x, (int)src_y, (int)width, (int)height,
               (int)dest_x, (int)dest_y );
    AUTO_FLUSH(this);
}

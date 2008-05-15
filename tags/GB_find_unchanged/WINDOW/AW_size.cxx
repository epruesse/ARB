#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include <aw_root.hxx>
#include "aw_device.hxx"
#include "aw_commn.hxx"
#include <aw_size.hxx>

#include <algorithm>

using namespace std;

//*****************************************************************************************
  //                                    size_device
  //*****************************************************************************************

  AW_device_size::AW_device_size(AW_common *commoni): AW_device(commoni) {
      ;
  }

void AW_device_size::init() {
    drawn = AW_FALSE;

    size_information.t = 0;
    size_information.b = 0;
    size_information.l = 0;
    size_information.r = 0;
}

AW_DEVICE_TYPE AW_device_size::type(void) { return AW_DEVICE_SIZE; }

void AW_device_size::privat_reset(void){
    this->init();
}

inline void AW_device_size::dot_transformed(AW_pos X, AW_pos Y) {
    if (drawn) {
        size_information.l = min(size_information.l, X);
        size_information.r = max(size_information.r, X);
        size_information.t = min(size_information.t, Y);
        size_information.b = max(size_information.b, Y);
    }
    else {
        size_information.l = size_information.r = X;
        size_information.t = size_information.b = Y;
        drawn              = true;
    }
}

inline void AW_device_size::dot(AW_pos x, AW_pos y) {
    AW_pos X, Y;
    transform(x, y, X, Y);
    dot_transformed(X, Y);
}

/***********************************************************************************************************************/
/* line  text  zoomtext  box *******************************************************************************************/
/***********************************************************************************************************************/

bool AW_device_size::invisible(int gc, AW_pos x, AW_pos y, AW_bitset filteri, AW_CL clientdata1, AW_CL clientdata2) {
    if (filteri & filter) dot(x, y);
    return AW_device::invisible(gc,x,y,filteri,clientdata1,clientdata2);
}


int AW_device_size::line(int gc, AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_bitset filteri, AW_CL clientdata1, AW_CL clientdata2) {
    AWUSE(clientdata1);AWUSE(clientdata2);
    AWUSE(gc);

    if (filteri & filter) {
        dot(x0, y0);
        dot(x1, y1);
        return AW_TRUE;
    }
    return AW_FALSE;
}

int AW_device_size::text(int gc, const char *str, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL clientdata1, AW_CL clientdata2, long opt_strlen) {
    AWUSE(clientdata1);AWUSE(clientdata2);

    if(filteri & filter) {
        XFontStruct *xfs = &(common->gcs[gc]->curfont);
        
        AW_pos X0,Y0;           // Transformed pos
        this->transform(x,y,X0,Y0);

        AW_pos l_ascent  = xfs->max_bounds.ascent;
        AW_pos l_descent = xfs->max_bounds.descent;
        AW_pos l_width   = get_string_size(gc, str, opt_strlen);
        X0               = common->x_alignment(X0,l_width,alignment);

        dot_transformed(X0, Y0-l_ascent);
        dot_transformed(X0+l_width, Y0+l_descent);
        return 1;
    }
    return 0;
}



void AW_device_size::get_size_information(AW_world *ptr) {
    *ptr = size_information;
}


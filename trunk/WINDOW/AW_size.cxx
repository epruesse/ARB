#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include <aw_root.hxx>
#include "aw_device.hxx"
#include "aw_commn.hxx"
#include <aw_size.hxx>

//*****************************************************************************************
//					size_device
//*****************************************************************************************

AW_device_size::AW_device_size(AW_common *commoni): AW_device(commoni) {
	;
}

void AW_device_size::init() {
	this->drawn = AW_FALSE;
}

AW_DEVICE_TYPE AW_device_size::type(void) { return AW_DEVICE_SIZE; }
void	AW_device_size::_privat_reset(void){
	this->init();
}

#define XSET(X) if(!this->drawn || size_information.l > X)\
			size_information.l = X;\
		if(!this->drawn || size_information.r < X)\
			size_information.r = X;

#define YSET(Y) if(!this->drawn || size_information.t > Y)\
			size_information.t = Y;\
		if(!this->drawn || size_information.b < Y)\
			size_information.b = Y;

/***********************************************************************************************************************/
/* line  text  zoomtext  box *******************************************************************************************/
/***********************************************************************************************************************/

int AW_device_size::invisible(int gc, AW_pos x, AW_pos y, AW_bitset filteri, AW_CL clientdata1, AW_CL clientdata2) {
	AW_pos X,Y;							// Transformed pos
	if(filteri & filter) {
		this->transform(x,y,X,Y);
		XSET(X);
		YSET(Y);
		this->drawn = AW_TRUE;
	}
	return AW_device::invisible(gc,x,y,filteri,clientdata1,clientdata2);
}


int	AW_device_size::line(int gc, AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_bitset filteri, AW_CL clientdata1, AW_CL clientdata2) {
	AWUSE(clientdata1);AWUSE(clientdata2);
	AWUSE(gc);
	AW_pos X0,Y0,X1,Y1;							// Transformed pos

	if(filteri & filter) {
		this->transform(x0,y0,X0,Y0);
		this->transform(x1,y1,X1,Y1);
		XSET(X0);
		YSET(Y0);
		this->drawn = AW_TRUE;
		XSET(X1);
		YSET(Y1);
		return AW_TRUE;
	}else{
		return AW_FALSE;
	}
}


int AW_device_size::text(int gc, const char *str, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL clientdata1, AW_CL clientdata2, long opt_strlen) {
	AWUSE(clientdata1);AWUSE(clientdata2);
	register XFontStruct *xfs = &(common->gcs[gc]->curfont);
	AW_pos 			X0,Y0;	// Transformed pos
	AW_pos			l_width, l_ascent, l_descent;

	if(filteri & filter) {
		this->transform(x,y,X0,Y0);
		l_ascent  = xfs->max_bounds.ascent;
		l_descent = xfs->max_bounds.descent;
		l_width   = this->get_string_size(gc,str,opt_strlen);
		X0        = common->x_alignment(X0,l_width,alignment);

        if( !drawn || size_information.l > X0 )                 size_information.l = X0;
        if( !drawn || size_information.r < X0 + l_width )       size_information.r = X0 + l_width;
        if( !drawn || size_information.t > Y0 - l_ascent )      size_information.t = Y0 - l_ascent;
        if( !drawn || size_information.b < Y0 + l_descent )     size_information.b = Y0 + l_descent;

        drawn = AW_TRUE;
		return 1;
	}
    else {
		return 0;
	}
}



void AW_device_size::get_size_information(AW_world *ptr) {
	if(!this->drawn) {
		size_information.l = 0.0;
		size_information.r = 0.0;
		size_information.t = 0.0;
		size_information.b = 0.0;
	}
	*ptr = this->size_information;
}


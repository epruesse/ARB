#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <X11/X.h>
#include <X11/Xlib.h>


#define _AW_COMMON_INCLUDED
#include <aw_root.hxx>
#include "aw_device.hxx"
#include "aw_commn.hxx"
#include <aw_click.hxx>


//*****************************************************************************************
//			device_click
//*****************************************************************************************

AW_device_click::AW_device_click(AW_common *commoni):AW_device(commoni) {
}


void AW_device_click::init(AW_pos mousex,AW_pos mousey, AW_pos max_distance_linei, AW_pos max_distance_texti, AW_pos radi, AW_bitset filteri) {
AWUSE(radi);
	mouse_x = mousex;
	mouse_y = mousey;
	filter = filteri;
	max_distance_line = max_distance_linei*max_distance_linei;
	max_distance_text = max_distance_texti;
	memset((char *)&opt_line,0,sizeof(opt_line));
	memset((char *)&opt_text,0,sizeof(opt_text));
	opt_line.exists = AW_FALSE;
	opt_text.exists = AW_FALSE;
}


AW_DEVICE_TYPE AW_device_click::type(void) {
	return AW_DEVICE_CLICK;
}


/***********************************************************************************************************************/
/* line  text  zoomtext  box *******************************************************************************************/
/***********************************************************************************************************************/

int	AW_device_click::line(int gc, AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_bitset filteri, AW_CL clientdata1, AW_CL clientdata2) {
    AW_pos X0,Y0,X1,Y1;				// Transformed pos
    AW_pos CX0,CY0,CX1,CY1;				// Clipped line
    int	drawflag;				// is line visible on screen
    AW_pos lx, ly, dx, dy, h1, h2, distance, skalar=0;
    AW_BOOL best_line = AW_FALSE;			// is this line the best ?

	AWUSE(gc);
    if(!(filteri & filter)) return AW_FALSE;

    this->transform(x0,y0,X0,Y0);
    this->transform(x1,y1,X1,Y1);
    drawflag = this->clip(X0,Y0,X1,Y1,CX0,CY0,CX1,CY1);

    if (drawflag) {
	//stimmen die Kreise um die Punkte ?

		

	// distance to the second point of the line
	dx = mouse_x - X1;
	dy = mouse_y - Y1;
	distance = (dx*dx) + (dy*dy);
	if (distance < max_distance_line) {
	    best_line = AW_TRUE;
	    max_distance_line = distance;
	    //add more comments
	    skalar = 0.0;
	}

	// distance to the first point of the line
	dx = mouse_x - X0;
	dy = mouse_y - Y0;
	distance = (dx*dx) + (dy*dy);
	if (distance < max_distance_line) {
	    best_line = AW_TRUE;
	    max_distance_line = distance;
	    //add more comments
	    skalar = 1.0;
	}

	lx = X1 - X0;
	ly = Y1 - Y0;
	h2 = (lx*lx) + (ly*ly);

	// Punkt darf nicht in der Verlaengerung der Linie liegen
	if (h2 > 0.0000000001){
	    skalar = (dx*lx+dy*ly)/h2;
	    if ( 0.0 <= skalar && skalar <= 1.0 ) {
		// berechne Trefferpunkt auf Linie
		// distance to the line
		h1 = dx*ly - dy*lx;
		distance = (h1*h1) / h2;
		if (distance < max_distance_line) {
		    best_line = AW_TRUE;
		    max_distance_line = distance;
		    //add more comments
		}
	    }
	}

	if(best_line == AW_TRUE) {
	    opt_line.x0 = x0;
	    opt_line.y0 = y0;
	    opt_line.x1 = x1;
	    opt_line.y1 = y1;
	    opt_line.height = distance;
	    opt_line.length = skalar;
	    opt_line.client_data1 = clientdata1;
	    opt_line.client_data2 = clientdata2;
	    opt_line.exists = AW_TRUE;
	}
	return AW_TRUE;
    }
    return AW_FALSE;
}


int AW_device_click::text(int gc, const char *str, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL clientdata1, AW_CL clientdata2,long opt_strlen) {
	class AW_GC_Xm 	*gcm = AW_MAP_GC(gc);
	register XFontStruct *xfs = &common->gcs[gc]->curfont;
	register	int position;
	AW_pos		X0,Y0;	// Transformed pos
	short		letter_width;
	int		text_width;
	AW_BOOL		not_optimal = AW_FALSE;
	AW_pos		best_dist = 0;

	if(filteri & filter) {


		this->transform(x,y,X0,Y0);

		AW_pos Yb = Y0+(AW_pos)(xfs->max_bounds.descent);
		AW_pos Yt = Y0-(AW_pos)(xfs->max_bounds.ascent);

			/***************** Fast check text against top bottom clip ***************************/
		if(this->clip_rect.t == 0) {
			if (Yb < this->clip_rect.t) return 0;
		}else {
			if (Yt < this->clip_rect.t) return 0;
		}

		if(this->clip_rect.b == common->screen.b) { 
			if (Yt > this->clip_rect.b) return 0;
		}else {
			if (Yb > this->clip_rect.b) return 0;
		}

#if 0
			/***************** Fast check mouse against top bottom areas ***************************/

		if (mouse_x < this->clip_rect.l - xfs->max_bounds.width) return 0;
		if (mouse_x > this->clip_rect.r) return 0;
#endif

			/***************** vertical check mouse against textsurrounding  ***************************/

		if(mouse_y > Yb) {				// outside text
			if(mouse_y > Yb + max_distance_text ){
				return 0;			// too far away
			} else {
				not_optimal = AW_TRUE;
				best_dist = mouse_y - Yb;
			}
		}else	if (mouse_y < (Yt) ) {
			if(mouse_y < Yt - max_distance_text ){
				return 0;
			} else {				
				not_optimal = AW_TRUE;
				best_dist = Yt - mouse_y;
			} 
		}


			/***************** align text  ***************************/
		int len;
		if (opt_strlen) len = (int)opt_strlen;
		else len = strlen(str);
		text_width = (int)get_string_size(gc,str,len);
		X0 = common->x_alignment(X0,text_width,alignment);
		text_width += xfs->max_bounds.width*2;	

			/**************** check against left right clipping areas *********/
		if (X0 + text_width< this->clip_rect.l) return 0;
		if (X0 > this->clip_rect.r + xfs->max_bounds.width) return 0;
		
			/***************** left of text  ***************************/
		if(mouse_x < (X0 - (xfs->max_bounds.width)) ) {
			return 0;
		}

			/***************** right of text  ***************************/
		if (mouse_x > (X0 + text_width) ) {
			return 0;
		}

			/***************** hit !!!!  ***************************/
		max_distance_text = best_dist;						// exact hit -> distance = 0;

		if (xfs->max_bounds.width == xfs->min_bounds.width) {			// monospaced font
			letter_width = xfs->max_bounds.width;
			position = (int)((mouse_x-X0)/letter_width);
			if (position<0) position = 0;
			if (position>len) position = len;
		}else {									// non-monospaced font
			position = 0;
			text_width = 0;
			while( (position <= len)  ) {
				text_width += gcm->width_of_chars[str[position]];
				if ( mouse_x > (X0 + text_width) )	position++;
				else					break;
			}
		}
		opt_text.x0 = x;
		opt_text.y0 = y;
		opt_text.alignment = alignment;
		opt_text.rotation = 0;
		opt_text.distance = max_distance_text;
		opt_text.cursor = position;
		opt_text.client_data1 = clientdata1;
		opt_text.client_data2 = clientdata2;
		opt_text.exists = AW_TRUE;
	}
	return 1;
}


void AW_device_click::get_clicked_line(class AW_clicked_line *ptr) {
	*ptr		= opt_line;
}


void AW_device_click::get_clicked_text(class AW_clicked_text *ptr) {
	*ptr			= opt_text;
}

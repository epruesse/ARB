#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include "aw_root.hxx"
#include "aw_device.hxx"
#include "aw_window.hxx"
#include "aw_commn.hxx"



void AW_clip::set_cliprect(AW_rectangle *rect, AW_BOOL allow_oversize) {
    clip_rect = *rect;	// coordintes : (0,0) = top-left-corner
    if (!allow_oversize){
        if (clip_rect.t < common->screen.t) clip_rect.t = common->screen.t;
        if (clip_rect.b > common->screen.b) clip_rect.b = common->screen.b;
        if (clip_rect.l < common->screen.l) clip_rect.l = common->screen.l;
        if (clip_rect.r > common->screen.r) clip_rect.r = common->screen.r;
    }

    top_font_overlap = 0;
    bottom_font_overlap = 0;
    left_font_overlap = 0;
    right_font_overlap = 0;

    if (allow_oversize) { // added 21.6.02 --ralf
        if (clip_rect.t < common->screen.t) set_top_font_overlap(true);
        if (clip_rect.b > common->screen.b) set_bottom_font_overlap(true);
        if (clip_rect.l < common->screen.l) set_left_font_overlap(true);
        if (clip_rect.r > common->screen.r) set_right_font_overlap(true);
    }
}

void AW_clip::reduceClipBorders(int top, int bottom, int left, int right) {
    if (top    > clip_rect.t) 	clip_rect.t = top;
    if (bottom < clip_rect.b) 	clip_rect.b = bottom;
    if (left   > clip_rect.l) 	clip_rect.l = left;
    if (right  < clip_rect.r) 	clip_rect.r = right;
}

void AW_clip::reduce_top_clip_border(int top){
    if (top > clip_rect.t) clip_rect.t = top;
}

void AW_clip::set_top_clip_border(int top, AW_BOOL allow_oversize) {
    clip_rect.t = top;
    if (!allow_oversize){
        if (clip_rect.t < common->screen.t) clip_rect.t = common->screen.t;
    }
    else {
        set_top_font_overlap(true); // added 21.6.02 --ralf
    }
}

void AW_clip::reduce_bottom_clip_border(int bottom) {
    if ( bottom < clip_rect.b)    clip_rect.b = bottom;
}

void AW_clip::set_bottom_clip_border(int bottom, AW_BOOL allow_oversize) {
    clip_rect.b = bottom;
    if (!allow_oversize){
        if (clip_rect.b > common->screen.b) clip_rect.b = common->screen.b;
    }
    else {
        set_bottom_font_overlap(true); // added 21.6.02 --ralf
    }
}

void AW_clip::set_bottom_clip_margin(int bottom,AW_BOOL allow_oversize) {
    clip_rect.b -= bottom;
    if (!allow_oversize){
        if (clip_rect.b > common->screen.b) clip_rect.b = common->screen.b;
    }
    else {
        set_bottom_font_overlap(true); // added 21.6.02 --ralf
    }
}
void AW_clip::reduce_left_clip_border(int left) {
    if (left > clip_rect.l)clip_rect.l = left;
}
void AW_clip::set_left_clip_border(int left, AW_BOOL allow_oversize) {
    clip_rect.l = left;
    if (!allow_oversize){
        if (clip_rect.l < common->screen.l) clip_rect.l = common->screen.l;
    }
    else {
        set_left_font_overlap(true); // added 21.6.02 --ralf
    }
}

void AW_clip::reduce_right_clip_border(int right) {
    if (right < clip_rect.r)	clip_rect.r = right;
}

void AW_clip::set_right_clip_border(int right, AW_BOOL allow_oversize) {
    clip_rect.r = right;
    if (!allow_oversize){
        if (clip_rect.r > common->screen.r) clip_rect.r = common->screen.r;
    }
    else {
        set_right_font_overlap(true); // added to correct problem with last char skipped (added 21.6.02 --ralf)
    }
}

void AW_clip::set_top_font_overlap(int val){
    top_font_overlap = val;
}
void AW_clip::set_bottom_font_overlap(int val){
    bottom_font_overlap = val;
}
void AW_clip::set_left_font_overlap(int val){
    left_font_overlap = val;
}
void AW_clip::set_right_font_overlap(int val){
    right_font_overlap = val;
}

AW_clip::AW_clip(){
    memset((char *)this,0,sizeof(*this));
}

/**********************************************************************/

int AW_clip::box_clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out)
{
    if (x0 < clip_rect.r) x0out = x0; else x0out = clip_rect.r;
    if (y0 < clip_rect.b) y0out = y0; else y0out = clip_rect.b;

    if (x0 < clip_rect.l) x0out = clip_rect.l;
    if (y0 < clip_rect.t) y0out = clip_rect.t;

    if (x1 < clip_rect.r) x1out = x1; else x1out = clip_rect.r;
    if (y1 < clip_rect.b) y1out = y1; else y1out = clip_rect.b;

    if (x1 < clip_rect.l) x1out = clip_rect.l;
    if (y1 < clip_rect.t) y1out = clip_rect.t;

    if (x0out >= x1out || y0out >= y1out) return 0;
    return 1;
}
/**********************************************************************/

int AW_clip::clip(AW_pos x0, AW_pos y0, AW_pos x1, AW_pos y1, AW_pos& x0out, AW_pos& y0out, AW_pos& x1out, AW_pos& y1out)
{
    int outcode0, outcode1, outcodeout;
    int done,success;
    AW_pos x =0, y =0;

    success=0;                              // indiactes wether part of line is visible
    done = 0;

    while (done == 0)
    {       outcode0 = compoutcode(x0,y0);
    outcode1 = compoutcode(x1,y1);

    if ((outcode0 | outcode1) == 0)
    {                       // line is inside the rectangle

        x0out=x0;       y0out=y0;               // clipped coordinates of line
        x1out=x1;       y1out=y1;

        done = 1;
        success=1;
    }

    if ((outcode0 & outcode1) != 0)
        //line is outside the rectangle
        done = 1;
    if (done == 0)                //;
    {
        if (outcode0 > 0)
            outcodeout = outcode0;
        else
            outcodeout = outcode1;
        if ((outcodeout & 8) != 0)
        {
            x = x0+(x1-x0)*(clip_rect.t-y0)/(y1-y0);
            y = clip_rect.t;
        }
        else
        {
            if ((outcodeout & 4) != 0)
            {
                x = x0+(x1-x0)*(clip_rect.b-y0)/(y1-y0);
                y = clip_rect.b;
            }
            else
            {
                if ((outcodeout & 2) != 0)
                {
                    y = y0+(y1-y0)*(clip_rect.r-x0)/(x1-x0);
                    x = clip_rect.r;
                }
                else
                { if ((outcodeout & 1) != 0)
                {

                    y = y0+(y1-y0)*(clip_rect.l-x0)/(x1-x0);
                    x = clip_rect.l;
                }
                }
            }
        }
        if (outcode0 > 0)
        {
            x0 = x;
            y0 = y;
        }
        else
        {
            x1 = x;
            y1 = y;
        }
    }
    }
    return success;
}



void	AW_matrix::shift_x(AW_pos xoff) {
    xoffset = xoff*scale;
}


void	AW_matrix::shift_y(AW_pos yoff) {
    yoffset = yoff*scale;
}


void	AW_matrix::shift_dx(AW_pos xoff) {
    xoffset += xoff*scale;
}


void	AW_matrix::shift_dy(AW_pos yoff) {
    yoffset += yoff*scale;
}


void	AW_matrix::zoom(AW_pos val) {
    scale *= val;
}


void	AW_matrix::reset(void) {
    scale = 1.0;
    xoffset = yoffset = 0.0;
}

/**********************************************************************************************
						GC_XM
**********************************************************************************************/
AW_GC_Xm::AW_GC_Xm(class AW_common	*commoni) {
    common = commoni;
    XGCValues	val;
    unsigned long value_mask;
    val.line_width = 1;
    value_mask = GCLineWidth;
    line_width = 1;
    style = AW_SOLID;
    function = AW_COPY;
    color = 0;
    grey_level = 0;
    gc 	= XCreateGC(common->display,common->window_id,value_mask,&val);
}


AW_GC_Xm::~AW_GC_Xm(void) {
    if (gc) XFreeGC(common->display,gc);
}


void AW_GC_Xm::set_fill(AW_grey_level grey_leveli) {	// <0 dont fill	 0.0 white 1.0 black
    grey_level = grey_leveli;
}


void AW_GC_Xm::set_lineattributes(AW_pos width,AW_linestyle stylei) {
    int lwidth = (int)(width*AW_PIXELS_PER_MM);
    if (stylei == style && line_width == lwidth) return;

    switch (style){
        case AW_SOLID:
            XSetLineAttributes(common->display, gc, lwidth, LineSolid, CapButt, JoinBevel);
            break;
        case AW_DOTTED:
            XSetLineAttributes(common->display, gc, lwidth, LineOnOffDash, CapButt, JoinBevel);
            break;
        default:
            break;
    };
    line_width = lwidth;
    style = style;
}


void AW_GC_Xm::set_function(AW_function mode)
{
    if (function != mode) {
        switch(mode) {
            case AW_XOR:
                XSetFunction(common->display,gc,GXxor);
                break;
            case AW_COPY:
                XSetFunction(common->display,gc,GXcopy);
                break;
        }
        function = mode;
        set_foreground_color(color);
    }
}

void AW_GC_Xm::set_foreground_color(unsigned long col) {
    color = (short)col;
    if (function == AW_XOR) {
        if (common->data_colors[0]) {
            col ^= common->data_colors[0][AW_DATA_BG];
        }else{
            col ^= common->frame_colors[AW_WINDOW_BG];
        }
    }
    XSetForeground(common->display,gc, col );
}

void AW_GC_Xm::set_background_color(unsigned long colori) {
    XSetBackground(common->display,gc, colori );
}

const AW_font_information *AW_gc::get_font_information(int gc, unsigned char c) {
    AW_GC_Xm            *gcm = (common->gcs[gc]);
    AW_font_information *ptr = &common->gcs[gc]->fontinfo;
    ptr->this_letter_ascent  = gcm->ascent_of_chars[c];
    ptr->this_letter_descent = gcm->descent_of_chars[c];
    ptr->this_letter_width   = gcm->width_of_chars[c];
    ptr->this_letter_height  = ptr->this_letter_ascent + ptr->this_letter_descent;
    return ptr;
}


/**********************************************************************************************
						GC
**********************************************************************************************/

int	AW_gc::get_string_size(int gc, const char *str, long textlen)
    // get the size of the string
{
    register XFontStruct *xfs = &common->gcs[gc]->curfont;
    register short *size_per_char = common->gcs[gc]->width_of_chars;
    if (!textlen)
    {
        if (!str) return 0;
        textlen = strlen(str);
    }
    register int c;
    register long l_width;

    if (xfs->max_bounds.width == xfs->min_bounds.width || !str) {
        // monospaced font
        l_width = textlen * xfs->max_bounds.width;
    }else {		// non-monospaced font
        l_width = 0;
        for (c = *(str++); c; c = *(str++)) {
            l_width += size_per_char[c];
        }
    }
    return (int)l_width;
}
void AW_gc::new_gc(int gc) {
    if (gc>= common->ngcs) {
        common->gcs = (AW_GC_Xm **)realloc((char *)common->gcs,sizeof(void *)*(gc+10));
        memset( &common->gcs[common->ngcs],0,sizeof(void *) * (gc-common->ngcs+10));
        common->ngcs = gc+10;
    }
    if (common->gcs[gc])delete (common->gcs[gc]);
    common->gcs[gc] = new AW_GC_Xm(common);
}


void AW_gc::set_fill(int gc,AW_grey_level grey_level){
    aw_assert(common->gcs[gc]);
    common->gcs[gc]->set_fill(grey_level);}


void AW_gc::set_font(int gc,AW_font font_nr, int size, int *found_size) {
    // if found_size != 0 -> return value for used font size 
    aw_assert(common->gcs[gc]);
    common->gcs[gc]->set_font(font_nr, size, found_size);
}

int AW_gc::get_available_fontsizes(int gc, AW_font font_nr, int *available_sizes) {
    aw_assert(common->gcs[gc]);
    return common->gcs[gc]->get_available_fontsizes(font_nr, available_sizes);
}


void AW_gc::set_line_attributes(int gc,AW_pos width,AW_linestyle style){
    aw_assert(common->gcs[gc]);
    common->gcs[gc]->set_lineattributes(width,style);
}


void AW_gc::set_function(int gc,AW_function function){
    aw_assert(common->gcs[gc]);
    common->gcs[gc]->set_function(function);
}


void AW_gc::set_foreground_color(int gc, AW_color color) {
    unsigned long col;
    if (color>=AW_DATA_BG) {
        col = common->data_colors[0][color];
    }else{
        col = common->frame_colors[color];
    }
    common->gcs[gc]->set_foreground_color(col);
}


void AW_gc::set_background_color(int gc, AW_color color) {
    unsigned long col;
    if (color>=AW_DATA_BG) {
        col = common->data_colors[0][color];
    }else{
        col = common->frame_colors[color];
    }
    common->gcs[gc]->set_background_color(col);
}



/**********************************************************************************************
						COMMON
**********************************************************************************************/
void AW_get_common_extends_cb(AW_window *aww,AW_common *common) {
    AWUSE(aww);
    Window	root;
    unsigned int width,height;
    unsigned int depth, borderwidth;
    XGetGeometry(common->display,common->window_id,
                 &root,
                 &common->screen_x_offset,	// xoffset
                 &common->screen_y_offset,	// yoffset
                 &width,
                 &height,
                 &borderwidth,		// border width
                 &depth);		// depth of display

    common->screen.t = 0;		// set clipping coordinates
    common->screen.b = height;
    common->screen.l = 0;
    common->screen.r = width;
}

AW_common::AW_common(AW_window *aww, AW_area area,	Display *display_in,XID window_id_in,unsigned long *fcolors,unsigned int **dcolors)
{
    memset((char *)this,0,sizeof(AW_common));
    root = aww->get_root();
    window_id = window_id_in;
    display = display_in;
    frame_colors = fcolors;
    data_colors = (unsigned long **)dcolors;
    ngcs = 8;
    gcs = (AW_GC_Xm **)malloc(sizeof(void *)*ngcs);
    memset((char *)gcs,0,sizeof(void *)*ngcs);
    aww->set_resize_callback(area,(AW_CB2)AW_get_common_extends_cb,(AW_CL)this,0);
    AW_get_common_extends_cb(aww,this);
}

/**********************************************************************************************
						DEVICE and GCS
**********************************************************************************************/


void		AW_device::push_clip_scale(void)
{
    AW_clip_scale_stack *stack = new AW_clip_scale_stack;
    stack->next = clip_scale_stack;
    clip_scale_stack = stack;
    stack->scale = scale;
    stack->xoffset = xoffset;
    stack->yoffset = yoffset;
    stack->top_font_overlap = top_font_overlap;
    stack->bottom_font_overlap = bottom_font_overlap;
    stack->left_font_overlap = left_font_overlap;
    stack->right_font_overlap = right_font_overlap;
    stack->clip_rect = clip_rect;
}
void		AW_device::pop_clip_scale(void){
    if (!clip_scale_stack) {
        AW_ERROR("To many pop_clip_scale on that device");
        return;
    }
    scale = clip_scale_stack->scale;
    xoffset = clip_scale_stack->xoffset;
    yoffset = clip_scale_stack->yoffset;
    clip_rect = clip_scale_stack->clip_rect;
    top_font_overlap = clip_scale_stack->top_font_overlap;
    bottom_font_overlap = clip_scale_stack->bottom_font_overlap;
    left_font_overlap = clip_scale_stack->left_font_overlap;
    right_font_overlap = clip_scale_stack->right_font_overlap;
    AW_clip_scale_stack *oldstack = clip_scale_stack;
    clip_scale_stack = clip_scale_stack->next;
    delete oldstack;
}

void AW_device::get_area_size(AW_rectangle *rect) {	//get the extends from the class AW_device
    *rect = common->screen;
}

void AW_device::get_area_size(AW_world *rect) {	//get the extends from the class AW_device
    rect->t = common->screen.t;
    rect->b = common->screen.b;
    rect->l = common->screen.l;
    rect->r = common->screen.r;
}

void AW_device::_privat_reset()
{
    ;
}

void AW_device::reset(){
    while (clip_scale_stack){
        pop_clip_scale();
    }
    get_area_size(&clip_rect);
    AW_matrix::reset();
    _privat_reset();
}

AW_device::AW_device(class AW_common *commoni) : AW_gc(){
    common = commoni;
    clip_scale_stack = 0;
    filter = (AW_bitset)-1;
}

AW_gc::AW_gc() : AW_clip(){
    ;
}
/**********************************************************************************************
						DEVICE and OUTPUT
**********************************************************************************************/

int AW_device::invisible(int gc, AW_pos x, AW_pos y, AW_bitset filteri, AW_CL clientdata1, AW_CL clientdata2) {
    AWUSE(clientdata1);AWUSE(clientdata2);
    AWUSE(gc);
    AW_pos X,Y;							// Transformed pos
    if(filteri & filter) {
        transform(x,y,X,Y);
        if ( X > clip_rect.r) return 0;
        if ( X < clip_rect.l) return 0;
        if ( Y > clip_rect.b) return 0;
        if ( Y < clip_rect.t) return 0;
    }
    return 1;
}

bool AW_device::ready_to_draw(int gc) {
    return AW_GC_MAPABLE(common, gc);
}

// PJ: ::zoomtext is defined in AW_xfigfont.cxx

int AW_device::box(int gc, AW_pos x0,AW_pos y0,AW_pos width,AW_pos height, AW_bitset filteri, AW_CL cd1, AW_CL cd2)
{
    int erg = 0;
    if (	!(filteri & filter) ) return 0;
    erg |= line(gc,x0,y0,x0+width,y0,filteri,cd1,cd2);
    erg |= line(gc,x0,y0,x0,y0+height,filteri,cd1,cd2);
    erg |= line(gc,x0+width,y0+height,x0+width,y0+height,filteri,cd1,cd2);
    erg |= line(gc,x0+width,y0+height,x0+width,y0+height,filteri,cd1,cd2);
    return erg;
}
int AW_device::circle(int gc, AW_BOOL /*filled has no effect here*/, AW_pos x0,AW_pos y0,AW_pos width,AW_pos height, AW_bitset filteri, AW_CL cd1, AW_CL cd2)
{
    int erg = 0;
    if (	!(filteri & filter) ) return 0;
    erg |= line(gc,x0,y0+height,x0+width,y0,filteri,cd1,cd2);
    erg |= line(gc,x0,y0+height,x0-width,y0,filteri,cd1,cd2);
    erg |= line(gc,x0,y0-height,x0+width,y0,filteri,cd1,cd2);
    erg |= line(gc,x0,y0-height,x0-width,y0,filteri,cd1,cd2);
    return erg;
}
int AW_device::filled_area(int gc, int npoints, AW_pos *points, AW_bitset filteri, AW_CL cd1, AW_CL cd2){
    int erg = 0;
    if (	!(filteri & filter) ) return 0;
    npoints--;
    erg |= line(gc,points[0],points[1],points[npoints*2],points[npoints*2+1],filteri,cd1,cd2);
    while (	npoints>0) {
        AW_pos x = *(points++);
        AW_pos y = *(points++);
        erg |= line(gc,x,y,points[0],points[1],filteri,cd1,cd2);
        npoints--;
    }
    return erg;
}


void AW_device::clear(void){;}

void AW_device::clear_part(AW_pos x, AW_pos y, AW_pos width, AW_pos height)
{ AWUSE(x);AWUSE(y);AWUSE(width);AWUSE(height);}
void AW_device::clear_text(int gc, const char *string, AW_pos x, AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cd1, AW_CL cd2)
{
    AWUSE(gc);AWUSE(string);AWUSE(x);AWUSE(y);AWUSE(filteri);AWUSE(cd1);AWUSE(cd2);AWUSE(alignment);
}
void AW_device::move_region( AW_pos src_x, AW_pos src_y, AW_pos width, AW_pos height, AW_pos dest_x, AW_pos dest_y )
{ AWUSE(src_x);AWUSE(src_y);AWUSE(width);AWUSE(height);AWUSE(dest_x);AWUSE(dest_y);}

void AW_device::fast(void){;}
void AW_device::slow(void){;}
void AW_device::flush(void){;}



const char * AW_device::open(const char *path)
{
    AW_ERROR ("This device dont allow 'open'");
    AWUSE(path);
    return 0;
}

void AW_device::close(void)
{
    AW_ERROR ("This device dont allow 'close'");
}

void AW_device::get_clicked_line(AW_clicked_line *ptr)
{
    AW_ERROR ("This device dont allow 'get_clicked_line'");
    AWUSE(ptr);
}
void AW_device::get_clicked_text(AW_clicked_text *ptr){
    AW_ERROR ("This device dont allow 'get_clicked_text'");
    AWUSE(ptr);
}

void AW_device::get_size_information(AW_world *ptr){
    AW_ERROR ("This device dont allow 'get_size_information'");
    AWUSE(ptr);
}




int AW_device::cursor(int gc, AW_pos x0,AW_pos y0, AW_cursor_type type, AW_bitset filteri, AW_CL clientdata1, AW_CL clientdata2) {
    register class AW_GC_Xm *gcm = AW_MAP_GC(gc);
    register XFontStruct *xfs = &gcm->curfont;
    AW_pos x1,x2,y1,y2;
    AW_pos X0,Y0;				// Transformed pos

    //  cursor insert         cursor overwrite
    //     (X0,Y0)
    //       /\                       .
    //      /  \                      .
    //      ----
    // (X1,Y1)(X2,Y2)

    if(filteri & filter) {
        if( type == AW_cursor_insert ) {
            transform(x0,y0,X0,Y0);

            if (X0 > clip_rect.r) return 0;
            if (X0 < clip_rect.l) return 0;
            if (Y0+(AW_pos)(xfs->max_bounds.descent) < clip_rect.t) return 0;
            if (Y0-(AW_pos)(xfs->max_bounds.ascent) > clip_rect.b) return 0;

            x1 = x0-4;
            y1 = y0+4;
            x2 = x0+4;
            y2 = y0+4;

            line(gc,x1,y1,x0,y0,filteri,clientdata1, clientdata2);
            line(gc,x2,y2,x0,y0,filteri,clientdata1, clientdata2);
            line(gc,x1,y1,x2,y2,filteri,clientdata1, clientdata2);
        }
    }
    return 1;
}

int AW_device::text_overlay( int gc, const char *opt_str, long opt_len,	// either string or strlen != 0
                             AW_pos x,AW_pos y, AW_pos alignment, AW_bitset filteri, AW_CL cduser, AW_CL cd1, AW_CL cd2,
                             AW_pos opt_ascent,AW_pos opt_descent, 		// optional height (if == 0 take font height)
                             int (*f)(AW_device *device, int gc, const char *opt_string, size_t opt_string_len,size_t start, size_t size,
                                      AW_pos x,AW_pos y, AW_pos opt_ascent,AW_pos opt_descent,
                                      AW_CL cduser, AW_CL cd1, AW_CL cd2))
{
    long	textlen;
    class AW_GC_Xm *gcm = AW_MAP_GC(gc);
    register XFontStruct *xfs = &gcm->curfont;
    register short *size_per_char = common->gcs[gc]->width_of_chars;
    register int 	xi, yi;
    register int	h;
    int		start;
    int		l;
    int		c =0;
    AW_pos		X0,Y0;					// Transformed pos
    AW_BOOL		inside_clipping_left = AW_TRUE;		// clipping at the left edge of the screen is different
    //	from clipping right of the left edge.
    AW_BOOL		inside_clipping_right = AW_TRUE;

    // es gibt 4 clipping Moeglichkeiten:
    // 1. man will fuer den Fall clippen, dass man vom linken display-Rand aus druckt	=> clipping rechts vom 1. Buchstaben
    // 2. man will fuer den Fall clippen, dass man mitten im Bildschirm ist			=> clipping links vom 1. Buchstaben
    // 3. man will fuer den Fall clippen, dass man mitten im Bildschirm ist			=> clipping links vom letzten Buchstaben
    // 4. man will fuer den Fall clippen, dass man bis zum rechten display-Rand druckt	=> clipping rechts vom letzten Buchstaben

    if (!(filter & filteri)) return 0;

    if (left_font_overlap || common->screen.l == clip_rect.l) { // was : clip_rect.l == 0
        inside_clipping_left = AW_FALSE;
    }

    if (right_font_overlap || clip_rect.r == common->screen.r) { // was : clip_rect.r == common->screen.r

        inside_clipping_right = AW_FALSE;
    }

    transform(x,y,X0,Y0);


    if (top_font_overlap || clip_rect.t == 0) { 						// check clip border inside screen
        if (Y0+(AW_pos)(xfs->max_bounds.descent) < clip_rect.t) return 0; // draw outside screen
    }else {
        if (Y0-(AW_pos)(xfs->max_bounds.ascent) < clip_rect.t) return 0; // dont cross the clip border
    }

    if (bottom_font_overlap || clip_rect.b == common->screen.b) { 				// check clip border inside screen drucken
        if (Y0-(AW_pos)(xfs->max_bounds.ascent) > clip_rect.b) return 0;	     // draw outside screen
    }else {
        if (Y0+(AW_pos)(xfs->max_bounds.descent)> clip_rect.b) return 0;	     // dont cross the clip border
    }

    if (!opt_len) {
        opt_len = textlen = strlen(opt_str);
    }else{
        textlen = opt_len;
    }

    aw_assert(opt_len == textlen);
    aw_assert(int(strlen(opt_str)) >= textlen);

    if (alignment){
        AW_pos width = get_string_size(gc,opt_str,textlen);
        X0 = X0-alignment*width;
    }
    xi = AW_INT(X0);
    yi = AW_INT(Y0);
    if (X0 > clip_rect.r) return 0;			      // right of screen

    l = (int)clip_rect.l;
    if (xi + textlen*xfs->max_bounds.width < l) return 0;		// left of screen

    start = 0;
    if (xi < l) {								// now clip left side
        if (xfs->max_bounds.width == xfs->min_bounds.width) {		//  monospaced font
            h = (l - xi)/xfs->max_bounds.width;
            if (inside_clipping_left) {
                if ( (l-xi)%xfs->max_bounds.width  >0 )	h += 1;
            }
            if (h >= textlen) return 0;
            start    = h;
            xi      += h*xfs->max_bounds.width;
            textlen -= h;

            if (textlen < 0) return 0;
            aw_assert(int(strlen(opt_str)) >= textlen);
        }else {								// non-monospaced font
            for (h=0; xi < l; h++) {
                if (!(c = opt_str[h])) return 0;
                xi += size_per_char[c];
            }
            if (!inside_clipping_left) {
                h-=1;
                xi -= size_per_char[c];
            }
            start    = h;
            textlen -= h;

            if (textlen < 0) return 0;
            aw_assert(int(strlen(opt_str)) >= textlen);
        }
    }

    // now clipp right side
    if (xfs->max_bounds.width == xfs->min_bounds.width) {			// monospaced font
        h = ((int)clip_rect.r - xi) / xfs->max_bounds.width;
        if (h < textlen) {
            if (inside_clipping_right) {
                textlen = h;
            }else{
                textlen = h+1;
            }
        }

        if (textlen < 0) return 0;
        aw_assert(int(strlen(opt_str)) >= textlen);
    }
    else {									// non-monospaced font
        l = (int)clip_rect.r - xi;
        for (h = start; l >= 0 && textlen > 0 ; h++, textlen--) { // was textlen >= 0
            l -= size_per_char[opt_str[h]];
        }
        textlen = h - start;
        if (l <= 0 && inside_clipping_right && textlen  > 0 ) {
            textlen -= 1;
        }

        if (textlen < 0) return 0;
        aw_assert(int(strlen(opt_str)) >= textlen);
    }
    X0 = (AW_pos)xi;
    rtransform(X0,Y0,x,y);

    aw_assert(opt_len >= textlen);
    aw_assert(textlen >= 0 && int(strlen(opt_str)) >= textlen);

    return f(this,gc,opt_str,opt_len, start ,(size_t)textlen, x,y, opt_ascent, opt_descent, cduser, cd1, cd2);
}

/**********************************************************************************************
						DEVICE and ETC
**********************************************************************************************/

void AW_device::set_filter(AW_bitset filteri) { filter = filteri; }

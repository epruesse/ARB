/* -----------------------------------------------------------------
 * Global Variables changed:      root object: vectorfont information
 *                                vectorfont/ * awars
 *
 * Dependencies:		  -> AW_preset: user interface for above info
 *                                -> AW_xfig:   loading of line data
 *
 * Description:                   implements scalable vector font based
 *                                on xfig font resource file 
 *                                [use width 1 for font and grid]
 *				  
 *				  aw_read_xfigfont (reading vectorfont)
 *                                AW_device::zoomtext* output routines (load on demand)
 *				  aw_xfig_font_create_filerequest
 *				  
 * 
 * Notes:             
 * 1) xfig output of font: put dots on line junctions - always -> printer ?
 * 5) font cleanup (lib/pictures/ *.vfont)
 * 6) consider font length for scrollbars -> 0size should show all text
 *    ---> size device already implements this - but fixed pixel size 
 *         standard text cannot be sent to the size device !!!   
 *         (currently size device only sees two bottom and top line
 *         of the bounding box of the string)
 * (12) click device: don't put vectorfonts into click device, for editing
 *      vectorfont must be turned of. click device should perhaps know
 *      certain line-groups as chars, so we can use vectorfonts for editing
 *      too...
 * 13) *** undefined symbol *** and *** delete vectorfont ***
 *     the undefined symbol exists only once and then is referenced multiple times,
 *     every other character is really unique
 *    
 * -----------------------------------------------------------------
 */

/*  
 * $Header$
 *
 * $Log$
 * Revision 1.2  2005/01/05 11:45:48  westram
 * - removed _AW_COMMON_INCLUDED define
 *
 * Revision 1.1.1.1  2000/11/23 09:41:17  westram
 * Erster Import
 *
 * Revision 1.10  1995/03/13  15:22:48  jakobi
 * *** empty log message ***
 *
 * Revision 1.10  1995/03/13  15:22:48  jakobi
 * *** empty log message ***
 *
 * Revision 1.9  1995/03/13  12:36:22  jakobi
 * *** empty log message ***
 *
 * Revision 1.7  1995/02/03  17:08:38  jakobi
 * *** empty log message ***
 *
 * Revision 1.6  1995/02/01  13:24:37  jakobi
 * *** empty log message ***
 *
 * Revision 1.5  1995/01/27  17:32:08  jakobi
 * *** empty log message ***
 *
 * Revision 1.4  1995/01/19  17:03:59  jakobi
 * *** empty log message ***
 *
 * Revision 1.3  1995/01/13  20:00:29  jakobi
 * *** empty log message ***
 *
 * Revision 1.2  1995/01/09  19:17:28  jakobi
 * *** empty log message ***
 *
 * Revision 1.1  1994/12/21  11:21:33  jakobi
 * Initial revision
 *
 */

/* includes */

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include "aw_root.hxx"
#include "aw_device.hxx"
#include "aw_commn.hxx"
#include "aw_xfig.hxx"
#include "aw_xfigfont.hxx"
#include "aw_window.hxx"
#include <arbdb.h>		// hash functions
#include "awt.hxx"	

/* -----------------------------------------------------------------
 * Function:                     aw_read_xfigfont
 *
 * Arguments:                    xfigfont filename
 *
 * Returns:                      an array of lists of lines + grid sizes
 *
 * Description:                  read xfig font file and compute lines 
 *                               for the characters
 *
 * NOTE:                         the leftmost and the topmost line define
 *                               the gridsizes (fontsize). the uppermost leftmost
 *                               point not (only) part of these lines
 *                               defines the grid (simply a help for the
 *                               font designer). Currently only width 1 is used.
 *
 * Dependencies:                 (xfig)  
 * -----------------------------------------------------------------
 */
AW_vectorfont *aw_read_xfigfont(char *filename)
{
   int i,j;
   AW_xfig_line *l,*ll,*gridxl,*gridyl,*default_symbol;
   AW_xfig_text *t;

   // data concerning the grid defining the fields containing the character art
   short x,y,gridxx,gridxy,gridyx,gridyy,rx,ly,maxx,maxy;
   short xfig_offy         = 9999; // offset of grid from 0/0
   short xfig_offx         = 9999;
   short xfig_gridx        = 9999; // grid cell size
   short xfig_gridy        = 9999;
   short xfig_maxx         = 0;    // max size of fontmap (used for grid tmp array)
   short xfig_maxy         = 0;

   // arrays for sorting the lines
   AW_xfig_line ***lines;
   char         **chars;

   // final vector font array 
   AW_xfig_line **aw_vectorchar;
   AW_xfig_vectorfont *aw_xfig_vectorfont;	

   // 0.    read the font resource file - use negative fontsize for non-fatal return
   AW_xfig *xfig = new  AW_xfig(filename, -XFIG_DEFAULT_FONT_SIZE);
   // just in case it's really zero ... 
   if (!xfig || !xfig->text || !xfig->line){
      xfig->~AW_xfig();
      sprintf(AW_ERROR_BUFFER,"Error: Cannot load xfig font resource %s,\nplease choose a valid one in the font selection menu entry.\n",filename); aw_message();
      return(NULL);
   }
   

   // 1.    sort lines to characters:
   // 1.1.1 find the grid (width must ALWAYS be 1 for the grid):
   //       get the horizontal line with the lowest y coordinates 
   //       get the vertical line with the lowest x coordinates
   //       (giving the base length values of the grid)
  
   l = xfig->line[1];
   gridxx=gridxy=gridyx=gridyy=9999; gridxl=gridyl=NULL;
   while (l) {
      // (horizontal (uppermost (leftmost)))  line: length is gridx size
      if (l->y0==l->y1) {
         ly=y=l->y0; if (l->x0 > l->x1) { x = l->x1; rx = l->x0;} else { x=l->x0; rx=l->x1;} 
         if (y<gridxy || (y==gridxy && x<gridxx)) {
            gridxx=x;
            gridxy=y;
            gridxl=l;
            xfig_gridx=rx-x;
         }
      } else if (l->x0==l->x1) {
      // (vertical (leftmost (uppermost)))  line: length is gridy size
         if (l->x0==l->x1) { 
            rx=x=l->x0; if (l->y0 > l->y1) {  y= l->y1; ly = l->y0;} else { y=l->y0; ly=l->y1;}
            if (x<gridyx || (x==gridyx && y<gridyy)) { 
               gridyx=x;  
               gridyy=y; 
               gridyl=l; 
               xfig_gridy=ly-y;
            } 
         } 
      }
      l=l->next;
   }
   
   // 1.1.2 get the origin of the grid: (horizontal (uppermost (leftmost))) line != gridxl
   //       and get the rightmost and the lowest point of all lines
   l = xfig->line[1];
   while (l) {
      if (l->y0 > l->y1) { y= l->y1; ly = l->y0;} else { y=l->y0; ly=l->y1;}
      if (l->x0 > l->x1) { x= l->x1; rx = l->x0;} else { x=l->x0; rx=l->x1;}
      if (y==ly) {
         if ( gridxl!=l && (y<xfig_offy || (y==xfig_offy && x<xfig_offx))) {
            xfig_offx=x;
            xfig_offy=y;
         } 
      } 
      if (ly>xfig_maxy) xfig_maxy=ly;
      if (rx>xfig_maxx) xfig_maxx=rx;
      l = l->next; 
   }

//fprintf(stderr, "%d %d\n%d %d\n%d %d\n",xfig_gridx, xfig_gridy, xfig_offx, xfig_offy, xfig_maxx, xfig_maxy);
   
   if ((9999==xfig_gridx) || (9999==xfig_gridy) || \
       (9999==xfig_offx) || (9999==xfig_offy) || \
       (!xfig_maxx) || (!xfig_maxy) || (!xfig_gridx) || (!xfig_gridy)){
      sprintf(AW_ERROR_BUFFER,"Error: Cannot determine font grid size for xfig font resource %s,\nplease choose a valid vectorfont in the font selection menu entry\n",filename); aw_message();
      return(NULL);
   }

// fprintf(stderr, "Processing %d lines (grid size %d %d offset %d %d).\n",i,xfig_gridx,xfig_gridy,xfig_offx,xfig_offy);

   // 1.1.3 set framed box as default character for *all* fields
   l = new AW_xfig_line; ll=l;
      ll->x0=0;
      ll->y0=0;
      ll->y1=0;
      ll->x1=xfig_gridx; 
   ll->next=new AW_xfig_line; ll=ll->next;
      ll->x0=xfig_gridx;
      ll->y0=0; 
      ll->x1=xfig_gridx; 
      ll->y1=xfig_gridy; 
   ll->next=new AW_xfig_line; ll=ll->next;
      ll->x0=xfig_gridx;
      ll->y0=xfig_gridy;
      ll->x1=0;
      ll->y1=xfig_gridy; 
   ll->next=new AW_xfig_line; ll=ll->next;
      ll->x0=0;
      ll->y0=xfig_gridy;
      ll->x1=0;
      ll->y1=0;
   ll->next=(struct AW_xfig_line *) NULL; 
   
   aw_vectorchar=(AW_xfig_line**) malloc((unsigned) sizeof(AW_xfig_line *)*XFIG_FONT_ELEMS);
   for(i=0;i<XFIG_FONT_ELEMS;i++) aw_vectorchar[i]=l;
   default_symbol=l;


   // 1.2   create array[maxx-offx div grid + 1][maxy-offy div grid +1]
   //       link lines and chars into grid array structure
   maxx = (xfig_maxx - xfig_offx) / xfig_gridx + 2;
   maxy = (xfig_maxy - xfig_offy) / xfig_gridy + 2;

//   fprintf(stderr,"Grid elements prepared: %d,%d.\n",maxx,maxy);

   lines= (AW_xfig_line***) malloc((unsigned) sizeof(AW_xfig_line **)*maxx);
   for(i=0;i<maxx;i++)
     lines[i]=(AW_xfig_line**) malloc((unsigned) sizeof(AW_xfig_line *)*maxy);
   chars= (char**) malloc((unsigned) sizeof(char *)*maxx);
   for(i=0;i<maxx;i++)
     chars[i]=new char[maxy];
   for(i=0;i<maxx;i++)
      for(j=0;j<maxy;j++) {
         lines[i][j]=NULL;
         chars[i][j]='\0';
      }
   
   // 1.2.1 enter active character's line into field's list of lines
   //       note: crossing of the field's walls is forbidden
   l=xfig->line[1];
   while(l) {
      if ((l->x0 - xfig_offx) % xfig_gridx) { // no line of the grid
         x  = (l->x0 - xfig_offx) / xfig_gridx;
         y  = (l->y0 - xfig_offy) / xfig_gridy;
         if ((x>=maxx) || (y>=maxy) || (x<0) || (y<0) || ((l->x0-xfig_offx)<0) || ((l->y0-xfig_offy)<0)) 
            { l = l->next; continue; }
         ll = lines[x][y]; 
         lines[x][y]=new AW_xfig_line;
         lines[x][y]->next = ll; 
         lines[x][y]->x0   = (l->x0 - xfig_offx) % xfig_gridx;
         lines[x][y]->x1   = (l->x1 - xfig_offx) % xfig_gridx;
         lines[x][y]->y0   = (l->y0 - xfig_offy) % xfig_gridy;
         lines[x][y]->y1   = (l->y1 - xfig_offy) % xfig_gridy;
//fprintf(stderr, "Valid Line %d,%d to %d,%d from %d,%d , grid %d,%d\n",lines[x][y]->x0,lines[x][y]->y0,lines[x][y]->x1,lines[x][y]->y1,l->x0,l->y0,x,y);
      }
      l = l->next;
   }

   // 1.2.2 enter the one identifying character of this field into field's identifying char
   t=xfig->text;
   while(t) {
      if ((t->x - xfig_offx) % xfig_gridx) { // no line of the grid
         x  = (t->x - xfig_offx) / xfig_gridx;
         y  = (t->y - xfig_offy - 1) / xfig_gridy; // move char above baseline (might be the grid's position!
         if ((x>=maxx) || (y>=maxy) || (x<0) || (y<0) || ((t->y - xfig_offy)<0) || ((t->x - xfig_offx)<0)) 
            { t = t->next; continue; }
	 chars[x][y]=t->text[0];
//fprintf(stderr, "Valid Char /%s/ from %d,%d , grid %d,%d (%d,%d)\n",t->text,t->x,t->y,x,y,t->x-xfig_offx,t->y-xfig_offy);
      }
      t = t->next;
   }

   
   // 1.3   link field graphical data to the correct character of the vectorfont
   //       NOTE: char<XFIG_FONT_ELEMS may be always true ...; XFIG_FONT_ELEMS > ' ' !
   for(i=0;i<maxx;i++)
      for(j=0;j<maxy;j++) 
	  /*if (chars[i][j]<XFIG_FONT_ELEMS)*/ /* this is always TRUE */
            if ((chars[i][j]!='\0') && (0<=chars[i][j]) && lines[i][j]) 
               aw_vectorchar[chars[i][j]]=lines[i][j];
   if (aw_vectorchar[' ']==default_symbol)
      aw_vectorchar[' ']=NULL;


/* display data on characters
   for(i=0;i<XFIG_FONT_ELEMS;i++) {
      l=aw_vectorchar[i];
      if (l!=default_symbol) {
         fprintf(stderr, "Character %d defined",i);
         if (((i&0x7f)>31)&&(i!=0x7f)) fprintf(stderr,"[(%c)] at %8x",i&0x7f,l);
         if(l) {
            fprintf(stderr, ":\n");
            for (j=1;l;j++,l=l->next) 
               fprintf(stderr, "Line %d from (%d,%d) to (%d,%d)\n",j,l->x0,l->y0,l->x1,l->y1);
         }        
      }
   }
*/


   // 1.4   free some storage (ATTN -> we used shallow copy for our undefined symbol): 
   //       it's only the linkage above the line data
   xfig->~AW_xfig();
   for(i=0;i<maxx;i++) {
         delete lines[i];
	 delete chars[i];
   }
   delete chars;
   delete lines;
   
   
   // 1.5   link data into return object
   aw_xfig_vectorfont = new AW_xfig_vectorfont;
   if (aw_xfig_vectorfont) {
      aw_xfig_vectorfont->lines=aw_vectorchar;
      aw_xfig_vectorfont->gridsizex =xfig_gridx;
      aw_xfig_vectorfont->gridsizey =xfig_gridy;
      aw_xfig_vectorfont->default_symbol=default_symbol;
   }
   fprintf(stderr,"Done loading vector font.\n");   
   return aw_xfig_vectorfont;
} 



// delete old vectorfont: normal characters, default symbol and finally the head itself
void aw_xfig_font_deletefont(AW_root *aw_root) {
      int i;
      AW_xfig_line *l,*ll;
      if (aw_root->vectorfont_lines->lines) {
         for(i=0;i<XFIG_FONT_ELEMS;i++) {
            l=aw_root->vectorfont_lines->lines[i];
            if (l!=aw_root->vectorfont_lines->default_symbol)
               while(l) {
                  ll=l; l=l->next; delete ll;
               }
         }
      }
      l=aw_root->vectorfont_lines->default_symbol;
      while(l) {
         ll=l; l=l->next; delete ll;
      }     
      delete aw_root->vectorfont_lines;
      aw_root->vectorfont_lines=NULL;
}



/* -----------------------------------------------------------------
 * Function:                    AW_device::zoomtext      (angle given)
 *                              AW_device::zoomtext4line (lower borderline given)
 *
 * Arguments:                   string, position data, admin data
 *
 * Returns:                     !=0 if something was drawn on screen 
 *
 * Description:                 standard vectorfont output - frontend
 *                              to compute the scaling - ALL sizes are 
 *                              given in world coordinates!
 *
 * NOTE:                        use "rotation=atan2(y1-y0,x1-x0);" to convert
 *                              coordinates to angles
 *
 * Global Variables modified:   aw_root: zoomtext may be deactivated; 
 *                                        vectorfont may be loaded 
 *                              (may change AWAR vectorfont/active)
 * -----------------------------------------------------------------
 */

int    AW_device::zoomtext(int gc, const char *string, AW_pos x,AW_pos y, AW_pos  
				height, AW_pos alignment,AW_pos rotation,
                                AW_bitset filteri,AW_CL cd1,AW_CL cd2) {
   int g;
   AW_pos scale_2;
   AW_root *aw_root=common->root;

   //fprintf(stderr,"\n Type=%x Filter=%x\n",type(),filteri);
   // device not in filter
   if (!(type()&filteri)) return(0);

   // 1     use standardfont? load vector font?
   if (!aw_root->vectorfont_zoomtext)
      return(text(gc, string, x, y, alignment, filteri&(~AW_DEVICE_SIZE), cd1, cd2));   

   // currently click device would only see single lines instead of chars
   // --> nonsense --> so make vtext not clickable...
   if (this->type()==AW_DEVICE_CLICK) return(0);

   if (!height) return(0);

   if (!aw_root->vectorfont_lines) {
      aw_root->vectorfont_lines=aw_read_xfigfont(aw_root->vectorfont_name);
   }
   if (!aw_root->vectorfont_lines) { 
      aw_root->awar("vectorfont/active")->write_int(0);
      return(text(gc, string, x, y, alignment, filteri&(~AW_SIZE), cd1, cd2));
   }

   g=aw_root->vectorfont_lines->gridsizey; // get native height of the vectorfont (in xfig pixels)

   // get scaling ratio: height > 0 use zoom dependent size    - height world coords
   //                           < 0 use zoom independent size  - height in pixels 
   //                             note that this may be unsupported - 
   //                             DON'T EVER SEND THIS TO THE SIZE DEVICE: filteri & ~AW_DEVICE_SIZE!
   // height is given in world coordinates: example: world -0.5 .. +0.5 with height 1e-3
   // userscale is a user setting, usually 0.7 .. 1
   if (height>0) {
      scale_2=(AW_pos)height/g*aw_root->vectorfont_userscale; 
   } else {
      filteri=filteri&(~AW_DEVICE_SIZE);
      // old zoom independent scaling, assuming height as font point size
      // scale_2=(AW_pos)height/g*(float)aw_root->vectorfont_userscale/get_scale();
      scale_2=-(AW_pos)height/g*aw_root->vectorfont_userscale/get_scale();
   }



   // another interesting mode would be this one:
   // a) acc. to the no of named species explicitely displayed -> scale font rel. to height>0
   // b) acc. to angle in PH_dtree -> scale font rel. to height.
   // however, this is an exercise for zoomtext's caller.
   
   return(AW_device::zoomtext1(gc, string, x, y,  
				scale_2, alignment, rotation,
                                filteri,cd1,cd2));
}


int     AW_device::zoomtext4line(int gc, const char *string, AW_pos height, 
				AW_pos lx0, AW_pos ly0, 
				AW_pos lx1, AW_pos ly1,  
				AW_pos alignmentx, AW_pos alignmenty, 
				AW_bitset filteri,AW_CL cd1,AW_CL cd2)
{
   AW_pos x=0, y=0, rx0=0, ry0=0;
   AW_pos linelen;
   AW_pos scale_2=0;
   AW_pos rotation;
   AW_pos cosrot, sinrot;
   int gx,gy,rc;
   AW_root *aw_root=common->root;

   // exchange point 1 and 2 of the given line:
   // the string box is mirrored at this line
   // for fixed height, string always starts at point 1

   // device not in filter?
   if (!(type()&filteri)) return(0);

line(gc, lx0, ly0, lx1, ly1, filteri, cd1, cd2);

   if (!aw_root->vectorfont_zoomtext)
      return(text(gc, string, x, y, alignmentx, filteri&(~AW_SIZE), cd1, cd2));   

   if (type()==AW_DEVICE_CLICK) return(0);

   if (!aw_root->vectorfont_lines) {
      aw_root->vectorfont_lines=aw_read_xfigfont(aw_root->vectorfont_name);
   }
   if (!aw_root->vectorfont_lines) {
       aw_root->awar("vectorfont/active")->write_int(0);
     return(text(gc, string, x, y, alignmentx, filteri&(~AW_SIZE), cd1, cd2));
   }    

   gy=aw_root->vectorfont_lines->gridsizey;
   gx=aw_root->vectorfont_lines->gridsizex;

   rotation=atan2(ly1-ly0,lx1-lx0); 
   linelen=sqrt((lx1-lx0)*(lx1-lx0) + (ly1-ly0)*(ly1-ly0));
 
    if (height>=0) {
       if (height>0) 
          // font height given in world coordinates (line specifies only angle of text)
          scale_2=(AW_pos)height/gy*aw_root->vectorfont_userscale; 
       // * use this instead to implement zoom-independent font height 
       // * scale=-(AW_pos)height/gy*aw_root->vectorfont_userscale/get_scale();
       else 
          // given string width relative to length of line
          scale_2 = (AW_pos)linelen/gx/strlen(string);
    } else {
       filteri=filteri&(~AW_DEVICE_SIZE);
       // old zoom independent scaling, assuming height as font point size
       scale_2=-(AW_pos)height/gy*aw_root->vectorfont_userscale/get_scale();
    }

   // xfig coord * scale       = relative world coord
   // world      * get_scale() = display coord

   if (alignmentx || alignmenty) {
      // y: start text at x/-(alignmenty scaled character height) 

      if (alignmenty)
         y=-gy*scale_2*alignmenty;

      // x: start text at -(alignmentx scaled linelength)/y, this is similar to text
      // ===> POSITIVE VALUES MOVE BACK TO TOP/LEFT
      if (alignmentx)
         x=-linelen*alignmentx;

      cosrot = cos(rotation);
      sinrot = sin(rotation);
      rx0=cosrot*x - sinrot*y;
      ry0=sinrot*x + cosrot*y;
   }
   rc=zoomtext1(gc, string, lx0+rx0, ly0+ry0, scale_2, 0, rotation, (AW_bitset) filteri, cd1, cd2); 
   return rc;
}

// sub for zoomtext and zoomtext4line
int    AW_device::zoomtext1(int gc, const char *string, AW_pos x,AW_pos y, AW_pos  
				scale_2, AW_pos alignment,AW_pos rotation,
                                AW_bitset filteri,AW_CL cd1,AW_CL cd2) {
   const char *pstring=string;
   AW_xfig_line *pline;
   AW_pos x0, x1, y0, y1;
   AW_pos rx0, rx1, ry0, ry1; 
   AW_pos offset=0;
   int rc=0,rrc=0;
   int gx,gy, swap=0,l;
   AW_pos tx0,ty0,tx1,ty1;
   AW_pos cosrot, sinrot; 
   AW_root *aw_root=common->root;

   if(!aw_root->vectorfont_lines || !aw_root->vectorfont_lines->lines) return(0);

   // scaling and transformation factor
   cosrot = cos(rotation)*scale_2;
   sinrot = sin(rotation)*scale_2;
 
   // swap up/down in quadrant 2 and 3
   if (cosrot>0.0) swap=1;
   else pstring+=strlen(pstring)-1;
   
   // font sizes
   gx=aw_root->vectorfont_lines->gridsizex;
   gy=aw_root->vectorfont_lines->gridsizey;
   l=gx*strlen(string);

   // eg. 1: move string one length to the left, 0 stay, -1 move string one string
   //         length to the right
   if(alignment) {
   //   x-=(cosrot*l-sinrot*gy)*alignment;      
   //   y-=(sinrot*l+cosrot*gy)*alignment;
      x-=(cosrot*l*alignment);
      y-=(sinrot*l*alignment);                  
   }

   //fprintf(stderr,"AW %d!=type %d",AW_DEVICE_SIZE,type());

   // clipping: bounding box is g in every direction
   // clip by upper and lower line of a string box, skip if both are offscreen
   x0=0; x1=l; 
   y0=y1=0;

   rx0=cosrot*x0-sinrot*y0;
   ry0=sinrot*x0+cosrot*y0;
   rx1=cosrot*x1-sinrot*y1;
   ry1=sinrot*x1+cosrot*y1;

   transform(x+rx0,y+ry0,tx0,ty0); transform(x+rx1,y+ry1,tx1,ty1);
   rc=compoutcode(tx0,ty0) & compoutcode(tx1,ty1);
   if (type()==AW_DEVICE_SIZE)
      rrc=line(gc, x+rx0, y+ry0, x+rx1, y+ry1, filteri, cd1, cd2);

//line(gc, x+rx0, y+ry0, x+rx1, y+ry1, filteri, cd1, cd2);

   y0=y1=-gy;
   rx0=cosrot*x0-sinrot*y0; 
   ry0=sinrot*x0+cosrot*y0;
   rx1=cosrot*x1-sinrot*y1;
   ry1=sinrot*x1+cosrot*y1;

   transform(x+rx0,y+ry0,tx0,ty0); transform(x+rx1,y+ry1,tx1,ty1);
   rc=rc  & compoutcode(tx0,ty0) & compoutcode(tx1,ty1);
   if (type()==AW_DEVICE_SIZE)
      rrc+=line(gc, x+rx0, y+ry0, x+rx1, y+ry1, filteri, cd1, cd2);

//line(gc, x+rx0, y+ry0, x+rx1, y+ry1, filteri, cd1, cd2);

   // if every point is on the same side outside the cliprect then don't draw
   if(rc) {
   //   fprintf(stderr, "CLIPPED\n");
      return(0);
   }
   rc=0;

   // we've substituted the string by two lines when computing the size
   // currently this code is never reached, as we're already CLIPPED
   if(type()==AW_DEVICE_SIZE){
      return(rrc);
   }

   // skip strings with height smaller than 4 pixels
   if (gy*scale_2*get_scale()<XFIG_FONT_VISIBLE_HEIGHT) {
      rx1=cosrot*l; ry1=sinrot*l;
      rc=line(gc,x,y,x+rx1,y+ry1,filteri,cd1,cd2);
      return(rc);
   }

//rx1=cosrot*l; ry1=sinrot*l;
//transform(x,y,tx0,ty0); transform(x+rx1,y+ry1,tx1,ty1);
//fprintf(stderr,"\nNotClipped %g %g %g %g %g %d %d %d %d\n",get_scale(),tx0,ty0,tx1,ty1,clip_rect.t,clip_rect.b,clip_rect.l,clip_rect.r);

// get line_width (calculate: width = real gy/20; change if linewidth is smaller)
   
   // now print the string
   while(*pstring) {
      if (/*(*pstring < XFIG_FONT_ELEMS) && */ /* this is always TRUE */
	  aw_root->vectorfont_lines->lines[*pstring]){
         // process lines of the active character
         pline=aw_root->vectorfont_lines->lines[*pstring];
         while(pline) {
	    if (swap) {
	       x0=pline->x0+offset;
               y0=pline->y0-gy;
               x1=pline->x1+offset;
               y1=pline->y1-gy;
	    } 
	    else {
	       x0=gx-pline->x0+offset;
               y0=-pline->y0;
               x1=gx-pline->x1+offset;
               y1=-pline->y1; 
	    }
            // rotation to screen system               
            rx0=cosrot*x0-sinrot*y0;
            ry0=sinrot*x0+cosrot*y0;
            rx1=cosrot*x1-sinrot*y1;
            ry1=sinrot*x1+cosrot*y1; 
	    rc += line(gc, x+rx0, y+ry0, x+rx1, y+ry1, filteri, cd1, cd2);
            pline=pline->next;
         }
      }
      if (!swap) pstring--;
      else pstring++;
      offset+=gx;
   } 
   
   return rc;
}


// ************** callback to change the vectorfont  ***********
void aw_xfig_font_changefont_cb(AW_root *aw_root){
   char *file = aw_root->awar("vectorfont/file_name")->read_string();
   AW_vectorfont *tmp;

// fprintf(stderr,"Changed to: %s\n", file);

   // irrelevant call?
   if ((file[0]=='\0') || (!GB_is_regularfile(file))) { delete file; return; }

   // this might be a file, so try to read it as a .vfont file
   tmp = aw_read_xfigfont(file);
   if (tmp&&aw_root->vectorfont_lines) {
      aw_xfig_font_deletefont(aw_root);
   }
   if (tmp) {
      aw_root->vectorfont_lines=tmp;
      aw_root->awar("vectorfont/name")->write_string(file);
   }
   delete file;
}

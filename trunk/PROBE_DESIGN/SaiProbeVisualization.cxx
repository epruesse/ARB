#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <ctype.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_preset.hxx>
#include <awt.hxx>

#include <awt_canvas.hxx>
#include <awt_tree.hxx>
#include <awt_dtree.hxx>
#include <awt_tree_cb.hxx>

#include "SaiProbeVisualization.hxx"

extern GBDATA *gb_main;   
saiProbeData *g_pbdata = 0;
AW_root *g_root = 0;
char *startPosStr = 0;
const char *g_SaiColorString;

AW_gc_manager SAI_graphic::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw, AW_CL cd2) {
    AW_gc_manager preset_window =
        AW_manage_GC (aww,
                      device,
                      SAI_GC_FOREGROUND, 
                      SAI_GC_MAX, 
                      AW_GCM_DATA_AREA,
                      (AW_CB)AWT_resize_cb, 
                      (AW_CL)ntw,
                      cd2,
                      false,
                      "#FFFFFF",
                      "Foreground$#ffffff",
                      "Probe$#55b1ff",
                      0 );
    
    return preset_window;
}

SAI_graphic::SAI_graphic(AW_root *aw_rooti, GBDATA *gb_maini):AWT_graphic() {
    exports.dont_fit_x      = 1;
    exports.dont_fit_y      = 1;
    exports.dont_fit_larger = 0;
    exports.left_offset     = 20;
    exports.right_offset    = 200;
    exports.top_offset      = 30;
    exports.bottom_offset   = 30;
    exports.dont_scroll     = 0;

    this->aw_root = aw_rooti;
    this->gb_main = gb_maini;
}

SAI_graphic::~SAI_graphic(void) {}

void SAI_graphic::show(AW_device *device){
    paint(device);
}

void SAI_graphic::info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct)
{
    aw_message("INFO MESSAGE");
    AWUSE(device);
    AWUSE(x);
    AWUSE(y);
    AWUSE(cl);
    AWUSE(ct);
}

static inline void paintBackground (AW_device *device, char *probeRegion, AW_pos pbRgX1, AW_pos pbY, AW_pos pbMaxAscent, AW_pos pbMaxHeight) {
    for (unsigned int j = 0; j<strlen(probeRegion);j++){
        if (probeRegion[j]=='A') 
            device->box(SAI_GC_FOREGROUND,(pbRgX1+(j*pbMaxAscent)),((pbY-pbMaxHeight)+4),pbMaxAscent, pbMaxHeight-2, -1, 0,0);
    }
}

// --------------  painting routine --------------------//
void SAI_graphic::paint(AW_device *device) {

    AW_font_information *fgFontInfo = device->get_font_information(SAI_GC_FOREGROUND_FONT, 'A');
    AW_font_information *pbFontInfo = device->get_font_information(SAI_GC_PROBE_FONT, 'A');
    double fgMaxHeight = fgFontInfo->max_letter_height; 
    double fgMaxAscent = fgFontInfo->this_letter_ascent; 
    double pbMaxHeight = pbFontInfo->max_letter_height; 
    double pbMaxAscent = pbFontInfo->this_letter_ascent; 

    AW_pos fgX,fgY,pbRgX1,pbRgX2,pbY,pbX, lineXpos; 
    fgX = 0; fgY = fgMaxHeight + 10; 
    pbX = 0; pbY = pbMaxHeight + 10; 
    pbRgX1 = pbRgX2 = lineXpos = 0;

    int tmpStrLen, strLen = 0;
    char *saiSelected = g_root->awar(AWAR_SAI_2_PROBE)->read_string();

    int startPos, endPos;
    const char *saiCols = 0;

    char buf[1024];
    if (strcmp(saiSelected,"")==0)  sprintf(buf,"Selected SAI = Not Selected!");
    else                            sprintf(buf,"Selected SAI = %s",saiSelected);
    device->text(SAI_GC_FOREGROUND, buf, 100, -30, 0, 1, 0, 0, 0); 

    if(g_pbdata){
        device->text(SAI_GC_FOREGROUND,  "Species",0,10, 0, 1, 0, 0, 0); 
        list<const char*>::iterator i; char *tmp;
        if (!g_pbdata->probeSpecies.empty()) {
            for(i=g_pbdata->probeSpecies.begin();i!=g_pbdata->probeSpecies.end();++i){
                tmp = strdup((const char*)*i); 
                device->text(SAI_GC_FOREGROUND, tmp,fgX,fgY, 0, 1, 0, 0, 0); fgY+=fgMaxHeight;
                tmpStrLen = strlen(tmp);
                if (tmpStrLen > strLen) strLen = tmpStrLen;
            }
        }
        pbRgX1 = (strLen * fgMaxAscent) + 15;
        pbX    = pbRgX1 + (9 * pbMaxAscent);
        pbRgX2 = pbX + ((strlen(g_pbdata->probeTarget)) * pbMaxAscent);
        device->text(SAI_GC_PROBE, g_pbdata->probeTarget, pbX, 10, 0, 1, 0, 0, 0); 
        device->set_line_attributes(SAI_GC_FOREGROUND,2, AW_SOLID); 

        char *tmp1,*tmp2, *probeRegion, *pbRgTmp, baseBuf[2]; baseBuf[1] = 0;
        char *saiSelected = g_root->awar(AWAR_SAI_2_PROBE)->read_string();
        char buf[1024];
        if (saiSelected) sprintf(buf,"Selected SAI = %s",saiSelected);
        
        if (!g_pbdata->probeSeq.empty()) {
            for(i=g_pbdata->probeSeq.begin();i!=g_pbdata->probeSeq.end();++i){
                tmp1 = strdup((const char*)*i);
                tmp2 = strtok(tmp1," "); int tag = 0;
                while (tmp2 = strtok(0," ")) {
                    if(tag==5) startPosStr = strdup((const char*)tmp2);
                    probeRegion = strdup((const char*)tmp2); //getting only probe region
                    tag++;
                }
                probeRegion = strtok(probeRegion,"-"); 

                startPos = atoi(startPosStr) - 9; endPos = startPos + 36;
                //                saiCols  = getSaiColorString(aw_root, startPos, endPos); cout<<"saiCols"<<saiCols<<endl;

                paintBackground(device, probeRegion, pbRgX1, pbY, pbMaxAscent, pbMaxHeight);
                device->text(SAI_GC_PROBE, probeRegion, pbRgX1, pbY, 0, 1, 0, 0,0);

                bool bProbeReg = true;
                while (probeRegion = strtok(0,"-")) {
                    pbRgTmp = strdup((const char*)probeRegion);
                    if (bProbeReg) {
                     for (unsigned int j = 0; j<strlen(pbRgTmp);j++) {
                         if (pbRgTmp[j]=='=') { 
                             device->line(SAI_GC_PROBE,(pbX+(j*pbMaxAscent)),(pbY-pbMaxHeight/3),(pbX+((j+1)*pbMaxAscent)),(pbY-pbMaxHeight/3));
                         } else {
                             baseBuf[0] = pbRgTmp[j];
                             device->box(SAI_GC_FOREGROUND,(pbX+(j*pbMaxAscent)),(pbY-pbMaxHeight),pbMaxAscent, pbMaxHeight, -1, 0,0);
                             device->text(SAI_GC_PROBE,baseBuf, (pbX+(j*pbMaxAscent)), pbY, 0, 1, 0, 0, 0); 
                         }
                     }
                     bProbeReg = false;
                    }
                }
                paintBackground(device, pbRgTmp, pbRgX2, pbY, pbMaxAscent, pbMaxHeight);
                device->text(SAI_GC_PROBE, pbRgTmp, pbRgX2, pbY, 0, 1, 0, 0, 0); 
                pbY+=pbMaxHeight; 
            }
        }
        lineXpos = pbRgX2 + (9 * pbMaxAscent);
        device->set_line_attributes(SAI_GC_FOREGROUND,1, AW_SOLID); 
        device->line(SAI_GC_FOREGROUND,0,-20,lineXpos,-20);
        device->line(SAI_GC_FOREGROUND,0,pbY,lineXpos,pbY);
    }
}

void transferProbeData(AW_root *root, struct saiProbeData *spd) {
    g_pbdata = (struct saiProbeData *)spd;//copying the probe data to gloabal struct g_pbdata
}

void refreshCanvas (void *dummy, AW_CL *cl_ntw) {
    AWUSE(dummy);
    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;

    ntw->refresh();
}

static AW_window *createSelectSAI_window(AW_root *aw_root){
    static AW_window_simple *aws = 0;
    if(aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( aw_root, "SELECT_SAI", "SELECT SAI");
    aws->load_xfig("selectSAI.fig");

    aws->at("selection");
    aws->callback((AW_CB0)AW_POPDOWN);
    awt_create_selection_list_on_extendeds(gb_main,(AW_window *)aws,AWAR_SAI_2_PROBE);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->window_fit();
    return (AW_window *)aws;
}

AW_window *createSaiProbeMatchWindow(AW_root *awr){ 
    GB_transaction dummy(gb_main);
    g_root = awr;

    AW_window_menu *awm = new AW_window_menu();
    awm->init(awr,"MATCH_SAI", "PROBE AND SAI",200,300);
    AW_gc_manager aw_gc_manager;
    SAI_graphic *saiProbeGraphic = new SAI_graphic(awr,gb_main);
    
    AWT_canvas *ntw = new AWT_canvas(gb_main,awm, saiProbeGraphic, aw_gc_manager,AWAR_TARGET_STRING);
    ntw->recalc_size();

    awm->create_menu( 0, "File", "F", "secedit_file.hlp",  AWM_ALL );
    awm->insert_menu_topic( "selectSAI", "Select SAI", "S","selectSai.hlp", AWM_ALL,AW_POPUP, (AW_CL)createSelectSAI_window, (AW_CL)0);
     awm->insert_menu_topic( "SetColors", "Set Colors", "t","setColors.hlp", AWM_ALL,AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager);
    awm->insert_menu_topic( "close", "Close", "C","quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 1,0);
    
    return awm;
}


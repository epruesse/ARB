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
saiProbeData  *g_pbdata    = 0;
AW_root       *g_root      = 0;
char          *startPosStr = 0;
const char    *g_SaiColorString;
int            skipPos;
int            probeLen = 0;
static bool    in_colorDefChanged_callback = false; // used to avoid colorDef correction

#define BUFSIZE 100
static const char *getAwarName(int awarNo) {
    static char buf[BUFSIZE];

    strcpy(buf, AWAR_SAI_COLOR);
    (strchr(buf, 0)-1)[0] = '0'+awarNo;

    return buf;
}

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
                      "+-COLOR 0$#FFFFFF", "-COLOR 1$#E0E0E0", 
                      "+-COLOR 2$#C0C0C0", "-COLOR 3$#A0A0A0",
                      "+-COLOR 4$#909090", "-COLOR 5$#808080",
                      "+-COLOR 6$#707070", "-COLOR 7$#505050", 
                      "+-COLOR 8$#404040", "-COLOR 9$#303030",
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

static void colorDefChanged_callback(AW_root *awr, AW_CL cl_awarNo) {
    if (!in_colorDefChanged_callback) { 
        in_colorDefChanged_callback = true;
        unsigned char charUsed[256]; memset(charUsed,255,256);
        {
            for (int i=0; i<10 ; i++){
                char *awarString_next = awr->awar_string(getAwarName(i))->read_string();
                for(int c=0; awarString_next[c]; ++c){
                    charUsed[awarString_next[c]] = i;
                }
                free(awarString_next);
            }
            int   awarNo     = (int)cl_awarNo;
            char *awarString = awr->awar_string(getAwarName(awarNo))->read_string();

            for (int c = 0; awarString[c]; ++c) {
                charUsed[awarString[c]] = awarNo;
            }
            free(awarString);

            typedef unsigned char mystr[256];
            mystr s[10];
            for (int i=0; i<10; i++)  s[i][0]=0; //initializing the strings

            for (int i=0; i<256; i++) {
                int table = charUsed[i];
                if (table != 255) {
                    char *eos = strchr((char *)s[table],0); // get pointer to end of string
                    eos[0] = char(i);
                    eos[1] = 0;
                }
            }

            for (int i=0; i<10; i++) {
                awr->awar_string(getAwarName(i))->write_string((char *)s[i]);
            }
        }
        in_colorDefChanged_callback = false;
    }
}

void createSaiProbeAwars(AW_root *aw_root) {
    for (int i = 0; i < 10; i++){   // initialising 10 color definition string AWARS
       AW_awar *def_awar = aw_root->awar_string(getAwarName(i),"", AW_ROOT_DEFAULT);
       def_awar->add_callback(colorDefChanged_callback, (AW_CL)i);
    }
}

const char *translateSAItoColors(AW_root *awr, int start, int end, int mode) {
    static int   seqBufferSize = 0;
    static char *saiColors     = 0;

    if (start >= end) return 0;

    int seqSize = (end - start) + 1;

    if(seqSize > seqBufferSize){
        free(saiColors);
        seqBufferSize = seqSize;
        saiColors =  (char*)GB_calloc(seqBufferSize, sizeof(char));
    }
    else memset(saiColors,0,sizeof(char)*seqSize);

    char *saiSelected = awr->awar(AWAR_SAI_2_PROBE)->read_string();

    GB_push_transaction(gb_main);
    char   *alignment_name = GBT_get_default_alignment(gb_main);
    GBDATA *gb_extended    = GBT_find_SAI(gb_main, saiSelected);

    if (gb_extended) {
        GBDATA *gb_ali = GB_find(gb_extended, alignment_name, 0, down_level);
        if (gb_ali) {
            const char *saiData      = 0;
            bool        free_saiData = false;

            {
                GBDATA *saiSequence = GB_find(gb_ali, "data", 0, down_level); // search "data" entry (normal SAI)
                if (saiSequence) {
                    saiData = GB_read_char_pntr(saiSequence); // not allocated
                }
                else {
                    saiSequence = GB_find(gb_ali, "bits", 0, down_level); // search "bits" entry (binary SAI)
                    if (saiSequence) {
                        saiData      = GB_read_as_string(saiSequence); // allocated
                        free_saiData = true; // free saiData below
                    }
                }
            }

            if (saiData) {
                char trans_table[256];
                {
                    // build the translation table:
                    memset(trans_table, 0, 256);
                    for (int i = 0; i < SAI_CLR_COUNT; ++i) {
                        char *def      = awr->awar(getAwarName(i))->read_string(); 
                        int   clrRange = SAI_GC_0 + i;

                        for (char *d = def; *d; ++d) {
                            trans_table[*d] = clrRange;
                        }
                        free(def);
                    }
                }

                // translate SAI to colors
                int i, j, k, pfxCnt; i = j = k = pfxCnt = 0; 
                switch (mode) {
                case PROBE:
                    for ( i = start, j = i; i < end; ++i) {
                        if ((saiData[i] == '.') || (saiData[i] == '=') || (saiData[i] == '-')) {
                            skipPos++;
                        }
                        else {
                            if ((j-start) > probeLen) break;
                            saiColors[j-start] = trans_table[saiData[i]];
                            j++;
                        }
                    }
                    break;
                case PROBE_PREFIX:
                    for ( k = start ; ; k--) {  
                        if ((saiData[k] != '.') && (saiData[k] != '=') && (saiData[k] != '-')) 
                            pfxCnt ++;
                        if (pfxCnt > 9) { 
                            start = k;
                            break;
                        }
                    }
                    for ( i = start, j = i; i < end; ++i) {
                        if ((saiData[i] != '.') && (saiData[i] != '=') && (saiData[i] != '-')) {
                            saiColors[j-start] = trans_table[saiData[i]];
                            j++;
                        }
                    }
                    break;
                case PROBE_SUFFIX:
                    for ( i = start, j = i; i < end; ++i) {
                        if ((saiData[i] != '.') && (saiData[i] != '=') && (saiData[i] != '-')) {
                            saiColors[j-start] = trans_table[saiData[i]];
                            j++;
                        }
                    }
                    break;
                }
            }

            if (free_saiData) {
                free(const_cast<char*>(saiData)); // in this case saiData is allocated (see above)
            }
        }
    }
    free(alignment_name);
    free(saiSelected);
    GB_pop_transaction(gb_main);

    if (saiColors) return saiColors;
    else           return 0; 
}

static inline void paintBackground (AW_device *device, char *probeRegion, AW_pos pbRgX1, AW_pos pbY, AW_pos pbMaxAscent, AW_pos pbMaxHeight, const char *saiCols) {
    for (unsigned int j = 0; j<strlen(probeRegion);j++) {
        if (saiCols[j] != 0)
            device->box(saiCols[j],(pbRgX1+(j*pbMaxAscent)),((pbY-pbMaxHeight)+4),pbMaxAscent, pbMaxHeight-2, -1, 0,0);
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
            for ( i = g_pbdata->probeSpecies.begin(); i != g_pbdata->probeSpecies.end(); ++i ) {
                tmp = strdup((const char*) * i); 
                device->text(SAI_GC_FOREGROUND, tmp,fgX,fgY, 0, 1, 0, 0, 0); fgY += fgMaxHeight;
                tmpStrLen = strlen(tmp);
                if (tmpStrLen > strLen) strLen = tmpStrLen;
            }
        }
        pbRgX1 = (strLen * fgMaxAscent) + 15;
        pbX    = pbRgX1 + (9 * pbMaxAscent);
        pbRgX2 = pbX + ((strlen(g_pbdata->probeTarget)) * pbMaxAscent);
        device->text(SAI_GC_PROBE, g_pbdata->probeTarget, pbX, 10, 0, 1, 0, 0, 0); 
        device->set_line_attributes(SAI_GC_FOREGROUND,2, AW_SOLID); 

        probeLen = strlen(g_pbdata->probeTarget);

        char *tmp1, *tmp2, *probeRegion, *pbRgTmp, baseBuf[2]; baseBuf[1] = 0;
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



                startPos = atoi(startPosStr); endPos = startPos + 50;
                // pre-probe region - 9 bases
                saiCols  = translateSAItoColors(aw_root, startPos, endPos, PROBE_PREFIX); 

                if (saiCols) paintBackground(device, probeRegion, pbRgX1, pbY, pbMaxAscent, pbMaxHeight, saiCols);
                device->text(SAI_GC_PROBE, probeRegion, pbRgX1, pbY, 0, 1, 0, 0,0);

                skipPos = 0;
                // probe region 
                saiCols  = translateSAItoColors(aw_root, startPos, endPos, PROBE); 
                bool bProbeReg = true;
                while (probeRegion = strtok(0,"-")) {
                    pbRgTmp = strdup((const char*)probeRegion);
                    if (bProbeReg) {
                     for (unsigned int j = 0; j<strlen(pbRgTmp);j++) {
                         if (saiCols[j] != 0) {
                             device->box(saiCols[j],(pbX+(j*pbMaxAscent)), ((pbY-pbMaxHeight)+4), pbMaxAscent, pbMaxHeight-2, -1, 0,0);
                         }
                         if (pbRgTmp[j]=='=') { 
                             device->line(SAI_GC_PROBE,(pbX+(j*pbMaxAscent)),(pbY-pbMaxHeight/3),(pbX+((j+1)*pbMaxAscent)),(pbY-pbMaxHeight/3));
                         } else {
                             baseBuf[0] = pbRgTmp[j];
                             device->text(SAI_GC_PROBE,baseBuf, (pbX+(j*pbMaxAscent)), pbY, 0, 1, 0, 0, 0); 
                         }
                     }
                     bProbeReg = false;
                    }
                }
                //post-probe region - 9 bases
                saiCols  = translateSAItoColors(aw_root, (startPos + probeLen + skipPos), endPos, PROBE_SUFFIX); 
                if (saiCols) paintBackground(device, pbRgTmp, pbRgX2, pbY, pbMaxAscent, pbMaxHeight, saiCols);
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
    AWUSE(root);
    g_pbdata = (struct saiProbeData *)spd;//copying the probe data to gloabal struct g_pbdata
}

void refreshCanvas (void *dummy, AW_CL *cl_ntw) {
    AWUSE(dummy);
    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;

    ntw->refresh();
}

static AW_window *create_colorTranslationTable_window(AW_root *aw_root){  // creates color tranlation table window
    static AW_window_simple *aws = 0;
    if(aws) return (AW_window *)aws;

    aws = new AW_window_simple;
    aws->init( aw_root, "SAI_CTT", "Color Translation Table");
    aws->load_xfig("probeSaiColors.fig");

    char at_name[] = "rangex";
    char *dig      = strchr(at_name, 0)-1;

    for (int i = 0; i<SAI_CLR_COUNT; ++i) {
        dig[0] = '0'+i;
        aws->at(at_name);
        aws->create_input_field(getAwarName(i), 15);
    }

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    return (AW_window *)aws;
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

    createSaiProbeAwars(awr); // creating awars for colors ( 0 to 9)

    AW_window_menu *awm = new AW_window_menu();
    awm->init(awr,"MATCH_SAI", "PROBE AND SAI",200,300);
    AW_gc_manager aw_gc_manager;
    SAI_graphic *saiProbeGraphic = new SAI_graphic(awr,gb_main);
    
    AWT_canvas *ntw = new AWT_canvas(gb_main,awm, saiProbeGraphic, aw_gc_manager,AWAR_TARGET_STRING);
    ntw->recalc_size();

    awm->insert_help_topic("saiProbe_help_how", "How to Visualize SAI`s ?", "H", "saiProbeHelp.hlp", AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"saiProbeHelp.hlp", 0);

    awm->create_menu( 0, "File", "F", "secedit_file.hlp",  AWM_ALL );
    awm->insert_menu_topic( "selectSAI", "Select SAI", "S","selectSai.hlp", AWM_ALL,AW_POPUP, (AW_CL)createSelectSAI_window, (AW_CL)0);
    awm->insert_menu_topic( "clrTransTable", "Define Color Translations", "C","selectSai.hlp", AWM_ALL,AW_POPUP, (AW_CL)create_colorTranslationTable_window, (AW_CL)0);
    awm->insert_menu_topic( "SetColors", "Set Colors and Fonts", "t","setColors.hlp", AWM_ALL,AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager);
    awm->insert_menu_topic( "close", "Close", "C","quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 1,0);
    
    return awm;
}


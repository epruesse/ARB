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
#include <awt_config_manager.hxx>

#include "SaiProbeVisualization.hxx"
#include "probe_match_parser.hxx"

#define PROBE_PREFIX_LENGTH 9
#define PROBE_SUFFIX_LENGTH 9

extern GBDATA *gb_main;
saiProbeData  *g_pbdata                    = 0;
static char   *saiValues                   = 0;
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
                      "#00AA00",
                      "Foreground$#FF0000",
                      "Probe$#FFFF00",
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

void SAI_graphic::command(AW_device */*device*/, AWT_COMMAND_MODE /*cmd*/, int button, AW_key_mod /*key_modifier*/, char /*key_char*/,
                          AW_event_type type, AW_pos /*x*/, AW_pos /*y*/, AW_clicked_line *cl, AW_clicked_text *ct)
{
    if (type == AW_Mouse_Press && (cl->exists || ct->exists) && button != AWT_M_MIDDLE ) {
        int clicked_idx = 0;
        if(ct->exists) {
            clicked_idx = (int)ct->client_data1;
            const char *species_name = g_pbdata->probeSpecies[clicked_idx];
            aw_root->awar(AWAR_SPECIES_NAME)->write_string(species_name);
        }
    }
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
    awr->awar(AWAR_DISP_SAI)->touch(); // refreshes the display
}

static void refreshCanvas(AW_root *awr, AW_CL cl_ntw) { 
    // repaints the canvas
    AWUSE(awr);
    AWT_canvas *ntw = (AWT_canvas*)cl_ntw;
    ntw->refresh();
}

static void createSaiProbeAwars(AW_root *aw_root) {
    // creating awars specific for the painting routine
    aw_root->awar_int(AWAR_DISP_SAI, 0, AW_ROOT_DEFAULT); // to display SAI values
   
    for (int i = 0; i < 10; i++){   // initialising 10 color definition string AWARS
       AW_awar *def_awar = aw_root->awar_string(getAwarName(i),"", AW_ROOT_DEFAULT);
       def_awar->add_callback(colorDefChanged_callback, (AW_CL)i);
    }
}

static void addCallBacks(AW_root *awr, AWT_canvas *ntw) {
    // adding callbacks to the awars to refresh the display if recieved any changes 
    awr->awar(AWAR_DISP_SAI)   ->add_callback(refreshCanvas, (AW_CL)ntw);
    awr->awar(AWAR_SAI_2_PROBE)->add_callback(refreshCanvas, (AW_CL)ntw);
    awr->awar(AWAR_PROBE_LIST) ->add_callback(refreshCanvas, (AW_CL)ntw);
}

static const char *translateSAItoColors(AW_root *awr, int start, int end, int speciesNo) {
    // translating SAIs to color strings
    static int   seqBufferSize = 0;
    static char *saiColors     = 0;

    if (start >= end) return 0;

    int seqSize = (end - start) + 1;

    if(seqSize > seqBufferSize){
        free(saiColors);
        seqBufferSize = seqSize;
        saiColors = (char*)malloc(seqBufferSize);
        saiValues = (char*)malloc(seqBufferSize);
    }
    
    memset(saiColors, '0'-1, seqSize);
    memset(saiValues, '0'-1, seqSize);

    char *saiSelected = awr->awar(AWAR_SAI_2_PROBE)->read_string();

    GB_push_transaction(gb_main);
    char   *alignment_name = GBT_get_default_alignment(gb_main);
    GBDATA *gb_extended    = GBT_find_SAI(gb_main, saiSelected);
    int     positions      = 0;

    if (gb_extended) {
        GBDATA *gb_ali = GB_find(gb_extended, alignment_name, 0, down_level);
        if (gb_ali) {
            const char *saiData      = 0;
            const char *seqData      = 0;
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

                const char *species_name = g_pbdata->probeSpecies[speciesNo];
                GBDATA *gb_species       = GBT_find_species(gb_main, species_name);
                GBDATA *gb_seq_data      = GBT_read_sequence(gb_species,alignment_name);
                if (gb_seq_data) seqData = GB_read_char_pntr(gb_seq_data);
            }

            if (saiData) {
                char trans_table[256];
                {

                    // @@@ FIXME: build trans_table ONCE for each refresh only (not for each translation)
                    
                    // build the translation table
                    memset(trans_table, '0'-1, 256);
                    for (int i = 0; i < SAI_CLR_COUNT; ++i) {
                        char *def      = awr->awar(getAwarName(i))->read_string();
                        int   clrRange = i + '0'; // configured values use '0' to '9' (unconfigured use '0'-1)

                        for (const char *d = def; *d; ++d) {
                            trans_table[*d] = clrRange;
                        }
                        free(def);
                    }
                }

                // translate SAI to colors
                int i, j;
                for ( i = start, j = 0 ; i <= end; ++i) {
                    if ((seqData[i] != '.') && (seqData[i] != '-')) {
                        saiColors[j] = trans_table[saiData[i]];
                        saiValues[j] = saiData[i];
                        ++j;
                    }
                }
                positions = j;
            }

            if (free_saiData) {
                free(const_cast<char*>(saiData)); // in this case saiData is allocated (see above)
            }
        }
    }

    saiColors[positions] = 0;
    saiValues[positions] = 0;

    free(alignment_name);
    free(saiSelected);
    GB_pop_transaction(gb_main);

    return saiColors;
}

static int calculateEndPosition(int startPos, int speciesNo, int mode, int probeLen, GB_ERROR *err) {
    // returns -2 in case of error
    // Note: if mode == 'PROBE_PREFIX' the result is 1 in front of last base (and so may be -1)

    int i, endPos, baseCntr;
    i      = baseCntr = 0;
    endPos = -2;
    *err   = 0;

    GB_push_transaction(gb_main);
    char       *alignment_name = GBT_get_default_alignment(gb_main);
    const char *species_name   = g_pbdata->probeSpecies[speciesNo];
    GBDATA     *gb_species     = GBT_find_species(gb_main, species_name);
    if (gb_species) {
        GBDATA *gb_seq_data      = GBT_read_sequence(gb_species,alignment_name);
        if (gb_seq_data) {
            const char *seqData = GB_read_char_pntr(gb_seq_data);

            if (seqData) {
                switch (mode) {
                    case PROBE:
                        for ( i = startPos; baseCntr < probeLen; ++i) {
                            if ((seqData[i] != '-') && (seqData[i] != '.'))
                                baseCntr++;
                        }
                        break;
                    case PROBE_PREFIX:
                        for ( i = startPos; baseCntr < PROBE_PREFIX_LENGTH && i >= 0; --i) {
                            if ((seqData[i] != '-') && (seqData[i] != '.'))
                                baseCntr++;
                        }
                        break;
                    case PROBE_SUFFIX:
                        for ( i = startPos; baseCntr < PROBE_SUFFIX_LENGTH && seqData[i]; ++i) {
                            if ((seqData[i] != '-') && (seqData[i] != '.'))
                                baseCntr++;
                        }
                        break;
                }
                endPos = i;
            }
            else {
                *err = "can't read data";
            }
        }
        else {
            *err = "no data";
        }
    }
    else {
        *err = "species not found";
    }
    free(alignment_name);
    GB_pop_transaction(gb_main);

    return endPos;
}

// --------------------------------------------------------------------------------
// painting routine

static void paintBackgroundAndSAI (AW_device *device, size_t probeRegionLen, AW_pos pbRgX1, AW_pos pbY, AW_pos pbMaxWidth, AW_pos pbMaxHeight,
                             const char *saiCols, int dispSai)
{
    // painting background in translated colors from the chosen SAI values
    // and also printing the values based on the options set by user
    for (size_t j = 0; j<probeRegionLen; j++) {
        if (saiCols[j] >= '0') {
            device->box(saiCols[j]-'0'+SAI_GC_0, pbRgX1+j*pbMaxWidth, pbY-pbMaxHeight+1, pbMaxWidth, pbMaxHeight, -1, 0,0);
        }
        if (dispSai && saiValues[j]) {
            char saiVal[2];
            saiVal[0] = saiValues[j];
            saiVal[1] = 0;
            device->text(SAI_GC_FOREGROUND, saiVal, (pbRgX1+(j*pbMaxWidth)), pbY+pbMaxHeight, 0, 1, 0, 0, 0);
        }
    }
}

static void paintProbeInfo(AW_device *device, const char *probe_info, AW_pos x, AW_pos y, AW_pos xoff, AW_pos yoff, AW_pos maxDescent, AW_CL clientdata) {
    char probeChar[2];
    probeChar[1] = 0;

    for (size_t j = 0; probe_info[j]; ++j) {
        if (probe_info[j] == '=') {
            AW_pos yl = y-maxDescent-(yoff-maxDescent)/3;
            AW_pos xl = x+xoff*j;
            device->line(SAI_GC_PROBE, xl, yl, xl+xoff-1, yl);
        }
        else {
            probeChar[0] = probe_info[j];
            device->text(SAI_GC_PROBE, probeChar, x+j*xoff, y-maxDescent, 0, AW_SCREEN|AW_CLICK, clientdata, 0);
        }
    }
}

void SAI_graphic::paint(AW_device *device) {
    // Painting routine of the canvas based on the probe match results

    double xStep      = 0;
    double yStep      = 0;
    double maxDescent = 0;
    // detect x/y step to use
    {
        AW_font_information *fgFontInfo = device->get_font_information(SAI_GC_FOREGROUND_FONT, 0);
        AW_font_information *pbFontInfo = device->get_font_information(SAI_GC_PROBE_FONT, 0);

        xStep      = fgFontInfo->max_letter_width;
        yStep      = fgFontInfo->max_letter_height;
        maxDescent = fgFontInfo->max_letter_descent;

        if (pbFontInfo->max_letter_width  > xStep)       xStep      = pbFontInfo->max_letter_width;
        if (pbFontInfo->max_letter_height > yStep)       yStep      = pbFontInfo->max_letter_height;
        if (pbFontInfo->max_letter_descent > maxDescent) maxDescent = pbFontInfo->max_letter_descent;
    }

    AW_pos fgX,fgY,pbRgX1,pbRgX2,pbY,pbX,lineXpos;
    fgX = 0; fgY = yStep + 10;
    pbX = 0; pbY = yStep + 10;
    pbRgX1 = pbRgX2 = lineXpos = 0;

    int tmpStrLen, strLen = 0;
    char *saiSelected = aw_root->awar(AWAR_SAI_2_PROBE)->read_string();
    int dispSai       = aw_root->awar(AWAR_DISP_SAI)->read_int(); // to display SAI below probe targets

    int endPos, startPos = 0;
    const char *saiCols = 0;

    char buf[1024];
    if (strcmp(saiSelected,"")==0)  sprintf(buf,"Selected SAI = Not Selected!");
    else sprintf(buf,"Selected SAI = %s",saiSelected);
    device->text(SAI_GC_FOREGROUND, buf, 100, -30, 0, 1, 0, 0);

    double yLineStep = dispSai ? yStep*2 : yStep;

    if (g_pbdata) {
        device->text(SAI_GC_FOREGROUND,  "Species",0,10, 0, 1, 0, 0, 0);
        if (!g_pbdata->probeSpecies.empty()) {
            for ( size_t j = 0; j < g_pbdata->probeSpecies.size(); ++j ) {
                const char *name = g_pbdata->probeSpecies[j];
                device->text(SAI_GC_FOREGROUND, name, fgX, fgY, 0, AW_SCREEN|AW_CLICK, (AW_CL)j, 0, 0);
                fgY       += yLineStep;
                tmpStrLen  = strlen(name);
                if (tmpStrLen > strLen) strLen = tmpStrLen;
            }
        }

        double spacer = 4.0;

        pbRgX1 = (strLen * xStep) + 15;
        pbX    = pbRgX1 + (9 * xStep) + spacer;
        pbRgX2 = pbX + (g_pbdata->getProbeTargetLen() * xStep) + spacer;

        paintProbeInfo(device, g_pbdata->getProbeTarget(), pbX, 10, xStep, yStep, maxDescent, (AW_CL)0);
        device->set_line_attributes(SAI_GC_FOREGROUND,2, AW_SOLID);

        int              probeLen = g_pbdata->getProbeTargetLen();
        ProbeMatchParser parser(g_pbdata->getProbeTarget(), g_pbdata->getHeadline());
        if (parser.get_error()) {
            device->text(SAI_GC_PROBE, GBS_global_string("Error: %s", parser.get_error()), pbRgX2, pbY, 0, AW_SCREEN, (AW_CL)0, 0, 0);
        }
        else {
            for (size_t i = 0;  i < g_pbdata->probeSeq.size(); ++i) { // loop over all matches
                GB_ERROR    error;

                ParsedProbeMatch parsed(g_pbdata->probeSeq[i], parser);

                if ((error = parsed.get_error())) {
                    device->text(SAI_GC_PROBE, GBS_global_string("Error: %s", error), pbRgX2, pbY, 0, AW_SCREEN, (AW_CL)i, 0, 0);
                }
                else {
                    const char *probeRegion      = parsed.get_probe_region();
                    sai_assert(probeRegion);
                    char       *probeRegion_copy = strdup(probeRegion);

                    const char *tok_prefix = strtok(probeRegion_copy, "-");
                    const char *tok_infix  = tok_prefix ? strtok(0, "-") : 0;
                    const char *tok_suffix = tok_infix ? strtok(0, "-") : 0;

                    if (!tok_suffix) {
                        // handle special case where no suffix exists
                        char *end = strchr(probeRegion, 0);
                        if (end>probeRegion && end[-1] == '-') tok_suffix = "";
                    }

                    const char *err = 0;
                    if (tok_suffix) {
                        // --------------------
                        // pre-probe region - 9 bases
                        startPos = parsed.get_position();
                        if (parsed.get_error()) {
                            err = GBS_global_string("Could not parse match position (Reason: %s).", parsed.get_error());
                        }
                        else {
                            const char *endErr;
                            endPos = calculateEndPosition(startPos-1, i, PROBE_PREFIX, probeLen, &endErr);
                            if (endPos == -2) {
                                err = GBS_global_string("Can't handle '%s' (%s)", g_pbdata->probeSpecies[i], endErr);
                            }
                            else {
                                sai_assert(!endErr);
                                sai_assert(endPos >= -1); // note: -1 gets fixed in the next line
                                endPos++; // calculateEndPosition returns 'one position in front of start'
                                saiCols = translateSAItoColors(aw_root, endPos, startPos-1, i);
                                if (saiCols)  {
                                    int positions = strlen(saiCols);
                                    int skipLeft  = PROBE_PREFIX_LENGTH-positions;
                                    sai_assert(skipLeft >= 0);
                                    paintBackgroundAndSAI(device, positions, pbRgX1+skipLeft*xStep, pbY, xStep, yStep, saiCols, dispSai);
                                }
                                paintProbeInfo(device, tok_prefix, pbRgX1, pbY, xStep, yStep, maxDescent, (AW_CL)i);

                                // --------------------
                                // probe region
                                endPos  = calculateEndPosition(startPos, i, PROBE, probeLen, &endErr);
                                sai_assert(endPos >= startPos);
                                sai_assert(!endErr);
                                saiCols = translateSAItoColors(aw_root, startPos, endPos, i);

                                paintBackgroundAndSAI(device, strlen(tok_infix), pbX, pbY, xStep, yStep, saiCols, dispSai);
                                paintProbeInfo(device, tok_infix, pbX, pbY, xStep, yStep, maxDescent, (AW_CL)i);

                                //post-probe region - 9 bases
                                size_t post_start_pos = endPos;

                                endPos = calculateEndPosition(post_start_pos, i, PROBE_SUFFIX, probeLen, &endErr);
                                sai_assert(endPos >= int(post_start_pos));
                                sai_assert(!endErr);

                                saiCols = translateSAItoColors(aw_root, post_start_pos, endPos, i);
                                if (saiCols) paintBackgroundAndSAI(device, strlen(tok_suffix), pbRgX2, pbY, xStep, yStep, saiCols, dispSai);
                                paintProbeInfo(device, tok_suffix, pbRgX2, pbY, xStep, yStep, maxDescent, (AW_CL)i);
                            }
                        }
                    }
                    else {
                        err = GBS_global_string("probe-region '%s' has invalid format", probeRegion);
                    }

                    if (err) device->text(SAI_GC_PROBE, err, pbRgX2, pbY, 0, AW_SCREEN, (AW_CL)i, 0, 0);
                    free(probeRegion_copy);
                }
                pbY += yLineStep;
            }
        }
        lineXpos = pbRgX2 + (9 * xStep);
        device->set_line_attributes(SAI_GC_FOREGROUND,1, AW_SOLID);

        device->line(SAI_GC_FOREGROUND,0,-20,lineXpos,-20);
        device->line(SAI_GC_FOREGROUND,0,pbY,lineXpos,pbY);

        {
            double vert_x1 = pbX-spacer/2;
            double vert_x2 = pbRgX2-spacer/2;
            device->line(SAI_GC_FOREGROUND, vert_x1, -20, vert_x1, pbY);
            device->line(SAI_GC_FOREGROUND, vert_x2, -20, vert_x2, pbY);
        }
    }
}

void transferProbeData(struct saiProbeData *spd) {
    // store pointer to currently used probe data
    g_pbdata = spd;

}

/* ---------------------------------- Creating WINDOWS ------------------------------------------------ */

static void saiColorDefs_init_config(AWT_config_definition& cdef) {
    for (int i = 0; i < 10; i++) {
        const char *awarDef = getAwarName(i);
        cdef.add(awarDef, "" , i);
    }
}

static char *saiColorDefs_store_config(AW_window *aww, AW_CL, AW_CL ) {
    AWT_config_definition cdef(aww->get_root());
    saiColorDefs_init_config(cdef);
    return cdef.read();
}

static void saiColorDefs_restore_config(AW_window *aww, const char *stored_string, AW_CL, AW_CL) {
    AWT_config_definition cdef(aww->get_root());
    saiColorDefs_init_config(cdef);
    cdef.write(stored_string);
}

static AW_window *create_colorTranslationTable_window(AW_root *aw_root){  // creates color tranlation table window
    //window to define color translations of selected SAI
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

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "saveSaiColorDefs", saiColorDefs_store_config, saiColorDefs_restore_config, 0, 0);

    aws->at("dispSai");
    aws->create_toggle(AWAR_DISP_SAI);

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    return (AW_window *)aws;
}

static AW_window *createSelectSAI_window(AW_root *aw_root){
    // window to select SAI from the existing SAIs in the database
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
    // Main Window - Canvas on which the actual painting is done
    GB_transaction dummy(gb_main);

    createSaiProbeAwars(awr); // creating awars for colors ( 0 to 9)

    AW_window_menu *awm = new AW_window_menu();
    awm->init(awr,"MATCH_SAI", "PROBE AND SAI",200,300);
    AW_gc_manager aw_gc_manager;
    SAI_graphic *saiProbeGraphic = new SAI_graphic(awr,gb_main);

    AWT_canvas *ntw = new AWT_canvas(gb_main,awm, saiProbeGraphic, aw_gc_manager,AWAR_TARGET_STRING);
    ntw->recalc_size();

    awm->insert_help_topic("saiProbe_help_how", "How to Visualize SAI`s ?", "H", "saiProbeHelp.hlp", AWM_ALL, (AW_CB)AW_POPUP_HELP, (AW_CL)"saiProbeHelp.hlp", 0);

    awm->create_menu( 0, "File", "F", 0,  AWM_ALL );
    awm->insert_menu_topic( "close", "Close", "C","quit.hlp", AWM_ALL, (AW_CB)AW_POPDOWN, 1,0);

    awm->create_menu( 0, "Properties", "P", 0,  AWM_ALL );
    awm->insert_menu_topic( "selectSAI", "Select SAI", "S","selectSai.hlp", AWM_ALL,AW_POPUP, (AW_CL)createSelectSAI_window, (AW_CL)0);
    awm->insert_menu_topic( "clrTransTable", "Define Color Translations", "D","selectSai.hlp", AWM_ALL,AW_POPUP, (AW_CL)create_colorTranslationTable_window, (AW_CL)0);
    awm->insert_menu_topic( "SetColors", "Set Colors and Fonts", "t","setColors.hlp", AWM_ALL,AW_POPUP, (AW_CL)AW_create_gc_window, (AW_CL)aw_gc_manager);

    addCallBacks(awr, ntw);

    return awm;
}


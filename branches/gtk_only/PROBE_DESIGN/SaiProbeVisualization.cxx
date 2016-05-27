// =============================================================== //
//                                                                 //
//   File      : SaiProbeVisualization.cxx                         //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "SaiProbeVisualization.hxx"
#include "probe_match_parser.hxx"

#include <nds.h>
#include <items.h>
#include <awt_sel_boxes.hxx>
#include <awt_config_manager.hxx>
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <aw_preset.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>

#include <iostream>
#include <arb_global_defs.h>
#include <item_sel_list.h>


using namespace std;

#define PROBE_PREFIX_LENGTH 9
#define PROBE_SUFFIX_LENGTH 9

static saiProbeData *g_pbdata                    = 0;
static char         *saiValues                   = 0;
static bool          in_colorDefChanged_callback = false; // used to avoid colorDef correction

#define BUFSIZE 100
static const char *getAwarName(int awarNo) {
    static char buf[BUFSIZE];

    strcpy(buf, AWAR_SPV_SAI_COLOR);
    (strchr(buf, 0)-1)[0] = '0'+awarNo;

    return buf;
}

AW_gc_manager *SAI_graphic::init_devices(AW_window *aww, AW_device *device, AWT_canvas *scr) {
    AW_gc_manager *gc_manager =
        AW_manage_GC(aww,
                     scr->get_gc_base_name(),
                     device,
                     SAI_GC_HIGHLIGHT,
                     SAI_GC_MAX,
                     AW_GCM_DATA_AREA,
                     makeGcChangedCallback(AWT_GC_changed_cb, scr),
                     false,
                     "#005500",
                     "Selected Probe$#FF0000",
                     "Foreground$#FFAA00",
                     "Probe$#FFFF00",
                     "+-COLOR 0$#FFFFFF", "-COLOR 1$#E0E0E0",
                     "+-COLOR 2$#C0C0C0", "-COLOR 3$#A0A0A0",
                     "+-COLOR 4$#909090", "-COLOR 5$#808080",
                     "+-COLOR 6$#707070", "-COLOR 7$#505050",
                     "+-COLOR 8$#404040", "-COLOR 9$#303030",
                     NULL);

    return gc_manager;
}

SAI_graphic::SAI_graphic(AW_root *aw_rooti, GBDATA *gb_maini) {
    exports.zoom_mode = AWT_ZOOM_NEVER;
    exports.fit_mode  = AWT_FIT_NEVER;

    exports.set_standard_default_padding();

    this->aw_root = aw_rooti;
    this->gb_main = gb_maini;
}

void SAI_graphic::handle_command(AW_device *, AWT_graphic_event& event) {
    if (event.type() == AW_Mouse_Press && event.button() != AW_BUTTON_MIDDLE) {
        const AW_clicked_element *clicked = event.best_click();
        if (clicked->is_text()) {
            int         clicked_idx  = (int)clicked->cd1();
            const char *species_name = g_pbdata->probeSpecies[clicked_idx].c_str();

            aw_root->awar(AWAR_SPECIES_NAME)->write_string(species_name);
            aw_root->awar(AWAR_SPV_SELECTED_PROBE)->write_string(species_name);
        }
    }
}

SAI_graphic::~SAI_graphic() {}

void SAI_graphic::show(AW_device *device) {
    paint(device);
}

static void colorDefChanged_callback(AW_root *awr, int awarNo) {
    if (!in_colorDefChanged_callback) {
        LocallyModify<bool> flag(in_colorDefChanged_callback, true);
        unsigned char charUsed[256]; memset(charUsed, 255, 256);
        {
            for (int i=0; i<10;  i++) {
                char *awarString_next = awr->awar_string(getAwarName(i))->read_string();
                for (int c=0; awarString_next[c]; ++c) {
                    charUsed[(unsigned char)awarString_next[c]] = i;
                }
                free(awarString_next);
            }
            char *awarString = awr->awar_string(getAwarName(awarNo))->read_string();

            for (int c = 0; awarString[c]; ++c) {
                charUsed[(unsigned char)awarString[c]] = awarNo;
            }
            free(awarString);

            typedef unsigned char mystr[256];
            mystr s[10];
            for (int i=0; i<10; i++)  s[i][0]=0; // initializing the strings

            for (int i=0; i<256; i++) {
                int table = charUsed[i];
                if (table != 255) {
                    char *eos = strchr((char *)s[table], 0); // get pointer to end of string
                    eos[0] = char(i);
                    eos[1] = 0;
                }
            }

            for (int i=0; i<10; i++) {
                awr->awar_string(getAwarName(i))->write_string((char *)s[i]);
            }
        }
    }
    awr->awar(AWAR_SPV_DISP_SAI)->touch(); // refreshes the display
}

static void refreshCanvas(AW_root*, AWT_canvas *scr) {
    // repaints the canvas
    scr->refresh();
}

static void createSaiProbeAwars(AW_root *aw_root) {
    // creating awars specific for the painting routine
    aw_root->awar_int(AWAR_SPV_DISP_SAI, 0, AW_ROOT_DEFAULT); // to display SAI values

    for (int i = 0; i < 10; i++) {  // initializing 10 color definition string AWARS
       AW_awar *def_awar = aw_root->awar_string(getAwarName(i), "", AW_ROOT_DEFAULT);
       def_awar->add_callback(makeRootCallback(colorDefChanged_callback, i));
    }
}

static void addCallBacks(AW_root *awr, AWT_canvas *scr) {
    // add callbacks to the awars (refresh display on change)
    RootCallback refresh_cb = makeRootCallback(refreshCanvas, scr);

    awr->awar(AWAR_SPV_DISP_SAI)      ->add_callback(refresh_cb);
    awr->awar(AWAR_SPV_SAI_2_PROBE)   ->add_callback(refresh_cb);
    awr->awar(AWAR_SPV_DB_FIELD_NAME) ->add_callback(refresh_cb);
    awr->awar(AWAR_SPV_DB_FIELD_WIDTH)->add_callback(refresh_cb);
    awr->awar(AWAR_SPV_SELECTED_PROBE)->add_callback(refresh_cb);
    awr->awar(AWAR_SPV_ACI_COMMAND)   ->add_callback(refresh_cb);
}

static const char *translateSAItoColors(AW_root *awr, GBDATA *gb_main, int start, int end, int speciesNo) {
    // translating SAIs to color strings
    static int   seqBufferSize = 0;
    static char *saiColors     = 0;

    if (start >= end) return 0;

    int seqSize = (end - start) + 1;

    if (seqSize > seqBufferSize) {
        free(saiColors);
        seqBufferSize = seqSize;
        saiColors = (char*)malloc(seqBufferSize);
        saiValues = (char*)malloc(seqBufferSize);
    }

    memset(saiColors, '0'-1, seqSize);
    memset(saiValues, '0'-1, seqSize);

    char *saiSelected = awr->awar(AWAR_SPV_SAI_2_PROBE)->read_string();

    GB_push_transaction(gb_main);
    char   *alignment_name = GBT_get_default_alignment(gb_main);
    GBDATA *gb_extended    = GBT_find_SAI(gb_main, saiSelected);
    int     positions      = 0;

    if (gb_extended) {
        GBDATA *gb_ali = GB_entry(gb_extended, alignment_name);
        if (gb_ali) {
            const char *saiData      = 0;
            const char *seqData      = 0;
            bool        free_saiData = false;

            {
                GBDATA *saiSequence = GB_entry(gb_ali, "data"); // search "data" entry (normal SAI)
                if (saiSequence) {
                    saiData = GB_read_char_pntr(saiSequence); // not allocated
                }
                else {
                    saiSequence = GB_entry(gb_ali, "bits"); // search "bits" entry (binary SAI)
                    if (saiSequence) {
                        saiData      = GB_read_as_string(saiSequence); // allocated
                        free_saiData = true; // free saiData below
                    }
                }

                const char *species_name = g_pbdata->probeSpecies[speciesNo].c_str();
                GBDATA *gb_species       = GBT_find_species(gb_main, species_name);
                GBDATA *gb_seq_data      = GBT_find_sequence(gb_species, alignment_name);
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
                            trans_table[(unsigned char)*d] = clrRange;
                        }
                        free(def);
                    }
                }

                // translate SAI to colors
                int i, j;
                for (i = start, j = 0;   i <= end; ++i) {
                    if ((seqData[i] != '.') && (seqData[i] != '-')) {
                        saiColors[j] = trans_table[(unsigned char)saiData[i]];
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

static int calculateEndPosition(GBDATA *gb_main, int startPos, int speciesNo, int mode, int probeLen, GB_ERROR *err) {
    // returns -2 in case of error
    // Note: if mode == 'PROBE_PREFIX' the result is 1 in front of last base (and so may be -1)

    int i, endPos, baseCntr;
    i      = baseCntr = 0;
    endPos = -2;
    *err   = 0;

    GB_push_transaction(gb_main);
    char       *alignment_name = GBT_get_default_alignment(gb_main);
    const char *species_name   = g_pbdata->probeSpecies[speciesNo].c_str();
    GBDATA     *gb_species     = GBT_find_species(gb_main, species_name);
    if (gb_species) {
        GBDATA *gb_seq_data      = GBT_find_sequence(gb_species, alignment_name);
        if (gb_seq_data) {
            const char *seqData = GB_read_char_pntr(gb_seq_data);

            if (seqData) {
                switch (mode) {
                    case PROBE:
                        for (i = startPos; baseCntr < probeLen; ++i) {
                            if ((seqData[i] != '-') && (seqData[i] != '.'))
                                baseCntr++;
                        }
                        break;
                    case PROBE_PREFIX:
                        for (i = startPos; baseCntr < PROBE_PREFIX_LENGTH && i >= 0; --i) {
                            if ((seqData[i] != '-') && (seqData[i] != '.'))
                                baseCntr++;
                        }
                        break;
                    case PROBE_SUFFIX:
                        for (i = startPos; baseCntr < PROBE_SUFFIX_LENGTH && seqData[i]; ++i) {
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
            device->box(saiCols[j]-'0'+SAI_GC_0, AW::FillStyle::SOLID, pbRgX1+j*pbMaxWidth, pbY-pbMaxHeight+1, pbMaxWidth, pbMaxHeight);
        }
        if (dispSai && saiValues[j]) {
            char saiVal[2];
            saiVal[0] = saiValues[j];
            saiVal[1] = 0;
            device->text(SAI_GC_FOREGROUND, saiVal, (pbRgX1+(j*pbMaxWidth)), pbY+pbMaxHeight, 0, AW_SCREEN, 0);
        }
    }
}

// static void paintProbeInfo(AW_device *device, const char *probe_info, AW_pos x, AW_pos y, AW_pos xoff, AW_pos yoff, AW_pos maxDescent, AW_CL clientdata, int textCOLOR) {
static void paintProbeInfo(AW_device *device, const char *probe_info, AW_pos x, AW_pos y, AW_pos xoff, AW_pos yoff, AW_pos maxDescent, int textCOLOR) {
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
            // device->text(textCOLOR, probeChar, x+j*xoff, y-maxDescent, 0, AW_SCREEN|AW_CLICK, clientdata, 0);
            device->text(textCOLOR, probeChar, x+j*xoff, y-maxDescent, 0, AW_SCREEN|AW_CLICK);
        }
    }
}

static char *GetDisplayInfo(AW_root *root, GBDATA *gb_main, const char *speciesName, size_t displayWidth, const char *default_tree_name)
{
    GB_ERROR        error       = 0;
    char           *displayInfo = 0;
    GB_transaction  ta(gb_main);
    GBDATA         *gb_Species  = GBT_expect_species(gb_main, speciesName);

    if (!gb_Species) error = GB_await_error();
    else {
        char *field_content = 0;
        {
            const char *dbFieldName = root->awar_string(AWAR_SPV_DB_FIELD_NAME)->read_char_pntr();
            if (strcmp(dbFieldName, NO_FIELD_SELECTED) == 0) {
                field_content = strdup("no field, no content");
            }
            else {
                GBDATA *gb_field = GB_search(gb_Species, dbFieldName, GB_FIND);
                if (gb_field) {
                    field_content             = GB_read_as_string(gb_field);
                    if (!field_content) error = GB_await_error();
                }
                else {
                    if (GB_have_error()) {
                        error = GBS_global_string("Failed to retrieve field '%s' (Reason: %s)", dbFieldName, GB_await_error());
                    }
                    else {
                        error = GBS_global_string("No entry '%s' in species '%s'", dbFieldName, speciesName);
                    }
                }
            }
        }

        if (!error) {
            char *aciCommand        = root->awar_string(AWAR_SPV_ACI_COMMAND)->read_string();
            displayInfo             = GB_command_interpreter(gb_main, field_content, aciCommand, gb_Species, default_tree_name);
            if (!displayInfo) error = GB_await_error();
            free(aciCommand);
        }

        if (displayInfo && strlen(displayInfo)>displayWidth) displayInfo[displayWidth] = 0; // shorten result
        free(field_content);
    }

    if (error) freedup(displayInfo, error);                // display the error

    sai_assert(displayInfo);
    return displayInfo;
}

void SAI_graphic::paint(AW_device *device) {
    // Painting routine of the canvas based on the probe match results

    double xStep_info   = 0;
    double xStep_border = 0;
    double xStep_target = 0;
    double yStep        = 0;
    double maxDescent   = 0;
    // detect x/y step to use
    {
        const AW_font_limits& fgFontLim = device->get_font_limits(SAI_GC_FOREGROUND_FONT, 0);
        const AW_font_limits& pbFontLim = device->get_font_limits(SAI_GC_PROBE_FONT, 0);
        const AW_font_limits& hlFontLim = device->get_font_limits(SAI_GC_HIGHLIGHT_FONT, 0);

        AW_font_limits target_font_limits(pbFontLim, hlFontLim);
        AW_font_limits all_font_limits(fgFontLim, target_font_limits);

        xStep_info   = fgFontLim.width;
        xStep_border = pbFontLim.width;
        xStep_target = target_font_limits.width;

        yStep      = all_font_limits.height;
        maxDescent = all_font_limits.descent;
    }

    AW_pos fgY = yStep + 10;
    AW_pos pbY = yStep + 10;

    char *saiSelected  = aw_root->awar(AWAR_SPV_SAI_2_PROBE)->read_string();
    int   dispSai      = aw_root->awar(AWAR_SPV_DISP_SAI)->read_int();       // to display SAI below probe targets
    int   displayWidth = aw_root->awar(AWAR_SPV_DB_FIELD_WIDTH)->read_int(); // display column width of the selected database field

    {
        char buf[1024];
        if (strcmp(saiSelected, "")==0) sprintf(buf, "Selected SAI = Not Selected!");
        else sprintf(buf, "Selected SAI = %s", saiSelected);
        device->text(SAI_GC_PROBE, buf, 100, -30, 0.0, AW_SCREEN);
    }

    double yLineStep = dispSai ? yStep*2 : yStep;

    if (g_pbdata) {
        device->text(SAI_GC_PROBE,  "Species INFO", 0, 8, 0.0, AW_SCREEN);
        if (!g_pbdata->probeSpecies.empty()) {
            char *default_tree  = aw_root->awar(AWAR_TREE)->read_string();
            char *selectedProbe = aw_root->awar(AWAR_SPV_SELECTED_PROBE)->read_string();

            for (size_t j = 0; j < g_pbdata->probeSpecies.size(); ++j) {
                const char *name        = g_pbdata->probeSpecies[j].c_str();
                char       *displayInfo = GetDisplayInfo(aw_root, gb_main, name, displayWidth, default_tree);

                AW_pos fgX = 0;

                AW_click_cd cd(device, j);
                if (strcmp(selectedProbe, name) == 0) {
                    device->box(SAI_GC_FOREGROUND, AW::FillStyle::SOLID, fgX, (fgY - (yStep * 0.9)), (displayWidth * xStep_info), yStep);
                    device->text(SAI_GC_HIGHLIGHT, displayInfo, fgX, fgY-1, 0, AW_SCREEN|AW_CLICK, 0);
                }
                else {
                    device->text(SAI_GC_FOREGROUND, displayInfo, fgX, fgY, 0, AW_SCREEN|AW_CLICK, 0);
                }
                fgY += yLineStep;

                free(displayInfo);
            }

            free(selectedProbe);
            free(default_tree);
        }

        double spacer   = 4.0;
        AW_pos lineXpos = 0;

        AW_pos pbRgX1 = ((displayWidth+1) * xStep_info);
        AW_pos pbX    = pbRgX1 + (9 * xStep_border) + spacer;
        AW_pos pbRgX2 = pbX + (g_pbdata->getProbeTargetLen() * xStep_target) + spacer;

        int  probeLen = g_pbdata->getProbeTargetLen();

        device->box(SAI_GC_FOREGROUND, AW::FillStyle::SOLID, pbX, 10-yStep, (probeLen * xStep_target), yStep);
        paintProbeInfo(device, g_pbdata->getProbeTarget(), pbX, 10, xStep_target, yStep, maxDescent, SAI_GC_HIGHLIGHT);
        device->set_line_attributes(SAI_GC_FOREGROUND, 2, AW_SOLID);


        ProbeMatchParser parser(g_pbdata->getProbeTarget(), g_pbdata->getHeadline());
        if (parser.get_error()) {
            device->text(SAI_GC_PROBE, GBS_global_string("Error: %s", parser.get_error()), pbRgX2, pbY, 0, AW_SCREEN, 0);
        }
        else {
            for (size_t i = 0;  i < g_pbdata->probeSeq.size(); ++i) { // loop over all matches
                GB_ERROR         error;
                ParsedProbeMatch parsed(g_pbdata->probeSeq[i].c_str(), parser);
                AW_click_cd      cd(device, i);

                if ((error = parsed.get_error())) {
                    device->text(SAI_GC_PROBE, GBS_global_string("Error: %s", error), pbRgX2, pbY, 0, AW_SCREEN, 0);
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
                        const char *end = strchr(probeRegion, 0);
                        if (end>probeRegion && end[-1] == '-') tok_suffix = "";
                    }

                    const char *err = 0;
                    if (tok_suffix) {
                        // --------------------
                        // pre-probe region - 9 bases
                        int startPos = parsed.get_position();
                        if (parsed.get_error()) {
                            err = GBS_global_string("Could not parse match position (Reason: %s).", parsed.get_error());
                        }
                        else {
                            const char *endErr;
                            int         endPos = calculateEndPosition(gb_main, startPos-1, i, PROBE_PREFIX, probeLen, &endErr);
                            if (endPos == -2) {
                                err = GBS_global_string("Can't handle '%s' (%s)", g_pbdata->probeSpecies[i].c_str(), endErr);
                            }
                            else {
                                sai_assert(!endErr);
                                sai_assert(endPos >= -1); // note: -1 gets fixed in the next line
                                endPos++; // calculateEndPosition returns 'one position in front of start'
                                const char *saiCols = translateSAItoColors(aw_root, gb_main, endPos, startPos-1, i);
                                if (saiCols) {
                                    int positions = strlen(saiCols);
                                    int skipLeft  = PROBE_PREFIX_LENGTH-positions;
                                    sai_assert(skipLeft >= 0);
                                    paintBackgroundAndSAI(device, positions, pbRgX1+skipLeft*xStep_border, pbY, xStep_border, yStep, saiCols, dispSai);
                                }
                                paintProbeInfo(device, tok_prefix, pbRgX1, pbY, xStep_border, yStep, maxDescent, SAI_GC_PROBE);

                                // --------------------
                                // probe region
                                endPos  = calculateEndPosition(gb_main, startPos, i, PROBE, probeLen, &endErr);
                                sai_assert(endPos >= startPos);
                                sai_assert(!endErr);
                                saiCols = translateSAItoColors(aw_root, gb_main, startPos, endPos, i);

                                paintBackgroundAndSAI(device, strlen(tok_infix), pbX, pbY, xStep_target, yStep, saiCols, dispSai);
                                paintProbeInfo(device, tok_infix, pbX, pbY, xStep_target, yStep, maxDescent, SAI_GC_PROBE);

                                // post-probe region - 9 bases
                                size_t post_start_pos = endPos;

                                endPos = calculateEndPosition(gb_main, post_start_pos, i, PROBE_SUFFIX, probeLen, &endErr);
                                sai_assert(endPos >= int(post_start_pos));
                                sai_assert(!endErr);

                                saiCols = translateSAItoColors(aw_root, gb_main, post_start_pos, endPos, i);
                                if (saiCols) paintBackgroundAndSAI(device, strlen(tok_suffix), pbRgX2, pbY, xStep_border, yStep, saiCols, dispSai);
                                paintProbeInfo(device, tok_suffix, pbRgX2, pbY, xStep_border, yStep, maxDescent, SAI_GC_PROBE);
                            }
                        }
                    }
                    else {
                        err = GBS_global_string("probe-region '%s' has invalid format", probeRegion);
                    }

                    if (err) device->text(SAI_GC_PROBE, err, pbRgX2, pbY, 0, AW_SCREEN, 0);
                    free(probeRegion_copy);
                }
                pbY += yLineStep;
            }
        }
        lineXpos = pbRgX2 + (9 * xStep_border);
        device->set_line_attributes(SAI_GC_FOREGROUND, 1, AW_SOLID);

        device->line(SAI_GC_FOREGROUND, 0, -20, lineXpos, -20);
        device->line(SAI_GC_FOREGROUND, 0, pbY, lineXpos, pbY);

        {
            double vert_x1 = pbX-spacer/2;
            double vert_x2 = pbRgX2-spacer/2;
            device->line(SAI_GC_FOREGROUND, vert_x1, -20, vert_x1, pbY);
            device->line(SAI_GC_FOREGROUND, vert_x2, -20, vert_x2, pbY);
        }
    }
}

void transferProbeData(saiProbeData *spd) {
    // store pointer to currently used probe data
    g_pbdata = spd;

}

// ---------------------------------- Creating WINDOWS ------------------------------------------------

static AWT_predefined_config predefined_saiColorDefinitions[] = {
    {
        "*binary",
        "Use with SAIs containing binary columns\ne.g. \'markerline\'",
        "0='-.0=';1='';2='';3='';4='';5='';6='';7='';8='';9='1x'"
    },
    {
        "*column_weights_09",
        "Use with SAIs containing column weights (0-9)\ne.g. MAX_FREQUENCY",
        "0='0';1='1';2='2';3='3';4='4';5='5';6='6';7='7';8='8';9='9'"
    },
    {
        "*column_weights_0Z_posvar",
        "Use with SAIs containing column weights (0-9,A-Z)\ne.g. POS_VAR_BY_PARSIMONY",
        "0='012';1='345';2='678';3='9AB';4='CDE';5='FGH';6='IJK';7='LMN';8='OPQ';9='RST'"
    },
    {
        "*sequence_data",
        "Use with SAIs containing nucleotide sequence data",
        "0='-.';1='';2='';3='A';4='';5='C';6='';7='G';8='';9='TU'"
    },
    {
        "*helix",
        "Use with SAI:HELIX",
        "0='';1='';2='';3='<[';4='';5='';6='>]';7='';8='';9=''"
    },
    { 0, 0, 0 }
};

static void setup_saiColorDefs_config(AWT_config_definition& cdef) {
    for (int i = 0; i < 10; i++) {
        const char *awarDef = getAwarName(i);
        cdef.add(awarDef, "",  i);
    }
}

static AW_window *create_colorTranslationTable_window(AW_root *aw_root) { // creates color translation table window
    // window to define color translations of selected SAI
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(aw_root, "SAI_CTT", "Color Translation Table");
        aws->load_xfig("probeSaiColors.fig");

        char at_name[] = "rangex";
        char *dig      = strchr(at_name, 0)-1;

        for (int i = 0; i<SAI_CLR_COUNT; ++i) {
            dig[0] = '0'+i;
            aws->at(at_name);
            aws->create_input_field(getAwarName(i), 15);
        }

        aws->at("config");
        AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "saveSaiColorDefs", makeConfigSetupCallback(setup_saiColorDefs_config), NULL, predefined_saiColorDefinitions);

        aws->at("dispSai");
        aws->create_toggle(AWAR_SPV_DISP_SAI);

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");
    }
    return aws;
}

static AW_window *createDisplayField_window(AW_root *aw_root, GBDATA *gb_main) {
    // window to select existing species field (simple NDS setup for PROBE/SAI-viewer)
    static AW_window_simple *aws = 0;
    if (!aws) {
        aws = new AW_window_simple;
        aws->init(aw_root, "SELECT_DISPLAY_FIELD", "Select display field");
        aws->load_xfig("displayField.fig");

        aws->button_length(10);

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE", "C");

        aws->at("help");
        aws->callback(makeHelpCallback("displayField.hlp"));
        aws->create_button("HELP", "HELP", "H");

        create_itemfield_selection_button(aws, FieldSelDef(AWAR_SPV_DB_FIELD_NAME, gb_main, SPECIES_get_selector(), FIELD_FILTER_NDS, "display-field"), "dbField");

        aws->at("aciSelect");
        aws->button_length(12);
        aws->callback(makeWindowCallback(AWT_popup_select_srtaci_window, AWAR_SPV_ACI_COMMAND));
        aws->create_button("SELECT_ACI", "Select ACI");

        aws->at("aciCmd");
        aws->create_input_field(AWAR_SPV_ACI_COMMAND, 40);

        aws->at("width");
        aws->create_input_field(AWAR_SPV_DB_FIELD_WIDTH, 4);

        aws->window_fit();
    }
    return aws;
}

static AW_window *createSaiColorWindow(AW_root *aw_root, AW_gc_manager *gc_manager) {
    return AW_create_gc_window_named(aw_root, gc_manager, "SAI_COLOR_DEF", "Probe/SAI Colors and Fonts");
}

AW_window *createSaiProbeMatchWindow(AW_root *awr, GBDATA *gb_main) {
    // Main Window - Canvas on which the actual painting is done
    GB_transaction ta(gb_main);

    createSaiProbeAwars(awr); // creating awars for colors ( 0 to 9)

    AW_window_menu *awm = new AW_window_menu;
    awm->init(awr, "MATCH_SAI", "PROBE AND SAI", 200, 300);

    SAI_graphic *saiProbeGraphic = new SAI_graphic(awr, gb_main);
    AWT_canvas  *scr             = new AWT_canvas(gb_main, awm, awm->get_window_id(), saiProbeGraphic, AWAR_TARGET_STRING);

    scr->recalc_size();

    awm->insert_help_topic("How to Visualize SAI`s ?", "V", "saiProbe.hlp", AWM_ALL, makeHelpCallback("saiProbe.hlp"));

    awm->create_menu("File", "F", AWM_ALL);
    awm->insert_menu_topic("close", "Close", "C", "quit.hlp", AWM_ALL, AW_POPDOWN);

    awm->create_menu("Properties", "P", AWM_ALL);
    awm->insert_menu_topic("selectDispField", "Select display field",      "F", "displayField.hlp", AWM_ALL, makeCreateWindowCallback(createDisplayField_window, gb_main));
    awm->insert_menu_topic("selectSAI",       "Select SAI",                "S", "saiProbe.hlp",     AWM_ALL, makeWindowCallback(awt_popup_SAI_selection_list, AWAR_SPV_SAI_2_PROBE, gb_main));
    awm->insert_menu_topic("clrTransTable",   "Define Color Translations", "D", "saiProbe.hlp",     AWM_ALL, create_colorTranslationTable_window);
    awm->insert_menu_topic("SetColors",       "Set Colors and Fonts",      "t", "color_props.hlp",  AWM_ALL, makeCreateWindowCallback(createSaiColorWindow, scr->gc_manager));

    addCallBacks(awr, scr);

    return awm;
}

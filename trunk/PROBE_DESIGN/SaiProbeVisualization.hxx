#include <vector.h>

#define sai_assert(cond) arb_assert(cond)

#define AWAR_PROBE_LIST "tmp/probe_design/probe_list"

#define AWAR_SAI_2_PROBE "sai_visualize/sai_2_probe"
#define AWAR_DISP_SAI    "sai_visualize/disp_sai"
#define AWAR_SAI_COLOR   "sai_visualize/probeSai/color_0"

#define AWAR_DB_FIELD_NAME   "sai_visualize/db_field_name"
#define AWAR_DB_FIELD_WIDTH  "sai_visualize/db_field_width"
#define AWAR_ACI_COMMAND     "sai_visualize/aci_command"
#define AWAR_SELECTED_PROBE  "sai_visualize/selected_probe"

#define SAI_CLR_COUNT 10

enum {
    PROBE,
    PROBE_PREFIX,
    PROBE_SUFFIX
};

enum {
    SAI_GC_HIGHLIGHT,  SAI_GC_HIGHLIGHT_FONT  = SAI_GC_HIGHLIGHT,
    SAI_GC_FOREGROUND, SAI_GC_FOREGROUND_FONT = SAI_GC_FOREGROUND,
    SAI_GC_PROBE,      SAI_GC_PROBE_FONT      = SAI_GC_PROBE,

    SAI_GC_0,  SAI_GC_1,  SAI_GC_2,  SAI_GC_3,  SAI_GC_4,
    SAI_GC_5,  SAI_GC_6,  SAI_GC_7,  SAI_GC_8,  SAI_GC_9,

    SAI_GC_MAX
};

// global data for interaction with probe match result list:

class saiProbeData {
    char   *probeTarget;
    size_t  probeTargetLen;
    char   *headline;           // needed for ProbeMatchParser

public:

    std::vector<const char*> probeSpecies;
    std::vector<const char*> probeSeq;

    saiProbeData() : probeTarget(strdup("<notarget>")), probeTargetLen(0), headline(0) {}
    ~saiProbeData() {
        free(probeTarget);
        free(headline);
    }

    const char *getProbeTarget() const {
        // sai_assert(probeTarget); // always need a target
        return probeTarget;
    }
    size_t getProbeTargetLen() const {
        return probeTargetLen;
    }
    const char *getHeadline() const { return headline; }

    void setProbeTarget(const char *target) {
        sai_assert(target);
        free(probeTarget);
        unsigned int len = strlen(target);
        char temp[len];temp[len] = '\0';
        for( unsigned int i = 0; i < len; i++) {
            temp[i] = toupper(target[i]);  // converting the Bases to Upper case
        }
        probeTarget    = strdup(temp);
        probeTargetLen = strlen(probeTarget);
    }
    void setHeadline(const char *hline) {
        sai_assert(hline);
        free(headline);
        headline = strdup(hline);
    }
};

class SAI_graphic: public AWT_graphic {
public:
    GBDATA     *gb_main;
    AW_root    *aw_root;

    SAI_graphic(AW_root *aw_root, GBDATA *gb_main);
    ~SAI_graphic(void);

    AW_gc_manager init_devices(AW_window *,AW_device *,AWT_canvas *ntw,AW_CL);

    void show(AW_device *device);
    void info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);
    void command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, char key_char,
                 AW_event_type type, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);
    void paint(AW_device *device);

};

AW_window *createSaiProbeMatchWindow(AW_root *awr);
void transferProbeData(struct saiProbeData *spd);

#ifndef SAIPROBEVISUALIZATION_HXX
#define SAIPROBEVISUALIZATION_HXX

#ifndef _GLIBCXX_CCTYPE
#include <cctype>
#endif
#ifndef _GLIBCXX_VECTOR
#include <vector>
#endif
#ifndef AWT_CANVAS_HXX
#include <awt_canvas.hxx>
#endif

#include <string>

#define sai_assert(cond) arb_assert(cond)

#define AWAR_SPV_SAI_2_PROBE "sai_visualize/sai_2_probe"
#define AWAR_SPV_DISP_SAI    "sai_visualize/disp_sai"
#define AWAR_SPV_SAI_COLOR   "sai_visualize/probeSai/color_0"

#define AWAR_SPV_DB_FIELD_NAME   "sai_visualize/db_field_name"
#define AWAR_SPV_DB_FIELD_WIDTH  "sai_visualize/db_field_width"
#define AWAR_SPV_ACI_COMMAND     "sai_visualize/aci_command"
#define AWAR_SPV_SELECTED_PROBE  "sai_visualize/selected_probe"

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

class saiProbeData : virtual Noncopyable { // Note: also used for ProbeCollection!
    std::string probeTarget;
    std::string headline;           // needed for ProbeMatchParser

public:

    std::vector<std::string> probeSpecies;
    std::vector<std::string> probeSeq;

    saiProbeData() : probeTarget("<notarget>"), headline() {}
    ~saiProbeData() {
        probeSpecies.clear();
        probeSeq.clear();
    }

    const char *getProbeTarget() const {
        // sai_assert(probeTarget.length() > 0); // always need a target
        return probeTarget.c_str();
    }
    size_t getProbeTargetLen() const {
        return probeTarget.length();
    }
    const char *getHeadline() const { return headline.c_str(); }

    void setProbeTarget(const char *target) {
        sai_assert(target);

        unsigned int len = strlen(target);
        char temp[len]; temp[len] = '\0';
        for (unsigned int i = 0; i < len; i++) {
            temp[i] = toupper(target[i]);  // converting the Bases to Upper case
        }
        probeTarget = temp;
    }
    void setHeadline(const char *hline) {
        sai_assert(hline);
        headline = hline;
    }
};

struct SAI_graphic : public AWT_nonDB_graphic, virtual Noncopyable {
    GBDATA     *gb_main;
    AW_root    *aw_root;

    SAI_graphic(AW_root *aw_root, GBDATA *gb_main);
    ~SAI_graphic() OVERRIDE;

    AW_gc_manager *init_devices(AW_window *, AW_device *, AWT_canvas *scr) OVERRIDE;

    void show(AW_device *device) OVERRIDE;
    void handle_command(AW_device *device, AWT_graphic_event& event) OVERRIDE;
    void paint(AW_device *device);

};

AW_window *createSaiProbeMatchWindow(AW_root *awr, GBDATA *gb_main);
void transferProbeData(saiProbeData *spd);

#else
#error SaiProbeVisualization.hxx included twice
#endif

#include <vector.h>

// to use to display SAI in the probe match section
#define AWAR_PROBE_SAI_MATCH  "tmp/probe_design/probe_sai_match"
#define AWAR_PROBE_LIST       "tmp/probe_design/probe_list"
#define AWAR_SAI_2_PROBE      "tmp/probe_design/sai_2_probe"
#define AWAR_DISP_SAI         "tmp/probe_design/disp_sai"

#define AWAR_SAI_COLOR        "tmp/probeSai/color_0"
#define SAI_CLR_COUNT 10

enum {
    PROBE,
    PROBE_PREFIX,
    PROBE_SUFFIX
};

enum {
    SAI_GC_FOREGROUND, SAI_GC_FOREGROUND_FONT = SAI_GC_FOREGROUND,
    SAI_GC_PROBE,      SAI_GC_PROBE_FONT      = SAI_GC_PROBE,

    SAI_GC_0,  SAI_GC_1,  SAI_GC_2,  SAI_GC_3,  SAI_GC_4,
    SAI_GC_5,  SAI_GC_6,  SAI_GC_7,  SAI_GC_8,  SAI_GC_9,

    SAI_GC_MAX
};

struct saiProbeData {
    const char *probeTarget;
    std::vector<const char*> probeSpecies;
    std::vector<const char*> probeSeq;
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

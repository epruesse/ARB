#include <list.h>

// to use to display SAI in the probe match section
#define AWAR_PROBE_SAI_MATCH "tmp/probe_design/probe_sai_match"
#define AWAR_PROBE_LIST "tmp/probe_design/probe_list"
#define AWAR_SAI_2_PROBE "tmp/probe_design/sai_2_probe"

enum {
    SAI_GC_FOREGROUND, SAI_GC_FOREGROUND_FONT = SAI_GC_FOREGROUND,
    SAI_GC_PROBE,      SAI_GC_PROBE_FONT      = SAI_GC_PROBE,
    SAI_GC_MAX
};

struct saiProbeData {
    const char *probeTarget;
    list<const char*> probeSpecies;
    list<const char*> probeSeq;
};

class SAI_graphic: public AWT_graphic {
public:
    GBDATA *gb_main;
    AW_root *aw_root;

    SAI_graphic(AW_root *aw_root, GBDATA *gb_main);
    ~SAI_graphic(void);

    AW_gc_manager init_devices(AW_window *,AW_device *,AWT_canvas *ntw,AW_CL);

    void show(AW_device *device);
    void info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);
    void paint(AW_device *device);
};

AW_window *createSaiProbeMatchWindow(AW_root *awr);
void transferProbeData(AW_root *root, struct saiProbeData *spd);

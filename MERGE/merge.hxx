#define AWAR_MERGE_DB "tmp/merge1/db"
#define AWAR_MAIN_DB  "tmp/merge2/db"

AW_window * MG_merge_alignment_cb(AW_root *awr);
AW_window * MG_merge_names_cb(AW_root *awr);
AW_window * MG_merge_species_cb(AW_root *awr);
AW_window * MG_merge_extendeds_cb(AW_root *awr);
AW_window * MG_merge_trees_cb(AW_root *awr);
AW_window *create_mg_check_fields(AW_root *aw_root);

void MG_create_trees_awar(AW_root *aw_root, AW_default aw_def);
void MG_create_extendeds_var(AW_root *aw_root, AW_default aw_def);
void MG_create_alignment_vars(AW_root *aw_root,AW_default aw_def);
void MG_create_species_var(AW_root *aw_root, AW_default aw_def);

int MG_check_alignment(AW_window *aww, int fast = 0);

#define AWAR_REMAP_SPECIES_LIST "merge/remap_species_list"
#define AWAR_REMAP_ENABLE "merge/remap_enable"

class MG_remap {
    int in_length;
    int out_length;
    int *remap_tab;
    int *soft_remap_tab;
    int compiled;
public:
    MG_remap();
    ~MG_remap();
    GB_ERROR set(const char *in_reference, const char *out_reference); // returns only warnings
    GB_ERROR compile();		// after last set
    char *remap(const char *sequence); // returns 0 on error, else copy of sequence
};

class AW_root;

class MG_remaps {
public:
    int n_remaps;
    char **alignment_names;
    MG_remap **remaps;
    MG_remaps(GBDATA *gb_left,GBDATA *gb_right,AW_root *awr);
    ~MG_remaps();
};

extern GBDATA *gb_merge;
extern GBDATA *gb_dest;
extern GBDATA *gb_main;






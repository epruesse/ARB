
#ifndef sec_graphic_hxx_included
#define sec_graphic_hxx_included

#ifndef _STRING_H
#include "string.h"
#endif

#ifndef _CTYPE_H
#include "ctype.h"
#endif

#define AWAR_SECEDIT_BASELINEWIDTH  "secedit/baselinewidth"
#define AWAR_SECEDIT_IMEXPORT_BASE  "secedit/imexportbase"

#define AWAR_SECEDIT_DIST_BETW_STRANDS  "secedit/layout/dist_betw_strands"
#define AWAR_SECEDIT_SKELETON_THICKNESS "secedit/layout/skelton_thickness"
#define AWAR_SECEDIT_SHOW_DEBUG     "secedit/layout/show_debug_info"
#define AWAR_SECEDIT_SHOW_HELIX_NRS     "secedit/layout/show_helix_numbers"
#define AWAR_SECEDIT_SHOW_STR_SKELETON  "secedit/layout/show_structure_skeleton"
#define AWAR_SECEDIT_HIDE_BASES     "secedit/layout/hide_bases"
#define AWAR_SECEDIT_HIDE_BONDS     "secedit/layout/hide_bonds"

#define AWAR_SECEDIT_STRONG_PAIRS   "secedit/layout/pairs/strong"       // Bonds
#define AWAR_SECEDIT_NORMAL_PAIRS   "secedit/layout/pairs/normal"
#define AWAR_SECEDIT_WEAK_PAIRS     "secedit/layout/pairs/weak"
#define AWAR_SECEDIT_NO_PAIRS       "secedit/layout/pairs/no"
#define AWAR_SECEDIT_USER_PAIRS     "secedit/layout/pairs/user"

#define AWAR_SECEDIT_STRONG_PAIR_CHAR   "secedit/layout/pairs/strong_char"
#define AWAR_SECEDIT_NORMAL_PAIR_CHAR   "secedit/layout/pairs/normal_char"
#define AWAR_SECEDIT_WEAK_PAIR_CHAR     "secedit/layout/pairs/weak_char"
#define AWAR_SECEDIT_NO_PAIR_CHAR   "secedit/layout/pairs/no_char"
#define AWAR_SECEDIT_USER_PAIR_CHAR     "secedit/layout/pairs/user_char"

// AWAR to display Sequence associated Information in the secondary structure editor
#define AWAR_SECEDIT_DISPLAY_SAI    "tmp/secedit/display_sai"

// names for database:
#define NAME_OF_STRUCT_SEQ      "_STRUCT"
#define NAME_OF_REF_SEQ         "REF"

enum {
    SEC_GC_LOOP=0,      SEC_GC_FIRST_FONT = SEC_GC_LOOP,
    SEC_GC_HELIX,
    SEC_GC_NHELIX,
    SEC_GC_DEFAULT,
    SEC_GC_BONDS,
    SEC_GC_ECOLI,       SEC_GC_LAST_FONT = SEC_GC_ECOLI,
    SEC_GC_HELIX_NO,

    SEC_GC_CBACK_0,	// Ranges for SAI visualization
    SEC_GC_CBACK_1,
    SEC_GC_CBACK_2,
    SEC_GC_CBACK_3,
    SEC_GC_CBACK_4,
    SEC_GC_CBACK_5,
    SEC_GC_CBACK_6,
    SEC_GC_CBACK_7,
    SEC_GC_CBACK_8,
    SEC_GC_CBACK_9,

    SEC_GC_CURSOR,
    SEC_GC_MBACK,      //mismatches

    SEC_GC_SBACK_0, // User 1  // Background for search 
    SEC_GC_SBACK_1,  // User 2
    SEC_GC_SBACK_2,  // Probe
    SEC_GC_SBACK_3,  // Primer (local)
    SEC_GC_SBACK_4,  // Primer (region)
    SEC_GC_SBACK_5,  // Primer (global)
    SEC_GC_SBACK_6,  // Signature (local)
    SEC_GC_SBACK_7,  // Signature (region)
    SEC_GC_SBACK_8,  // Signature (global)

    SEC_SKELE_HELIX,  //skeleton helix color
    SEC_SKELE_LOOP,  //skeleton loop color
    SEC_SKELE_NHELIX,  //skeleton non-pairing helix color

    SEC_GC_MAX
}; // AW_gc

class SEC_root;

enum {
    SEC_UPDATE_OK = 0,
    SEC_UPDATE_RELOADED =1
};

#define SEC_BOND_BASE_CHARS 5
#define SEC_BOND_BASE_CHAR  "ACGTU"
#define SEC_BOND_PAIR_CHARS 8
#define SEC_BOND_PAIR_CHAR  "-.o~+=# "

class SEC_bond_def {
    char bond[SEC_BOND_BASE_CHARS][SEC_BOND_BASE_CHARS];

    int get_index(char c) const;
    void clear();
    int insert(const char *pairs, char character);
    char get_bond(char base1, char base2) const;

public:
    SEC_bond_def() { clear(); }
    int update(AW_root *awr);

    void paint(AW_device *device, SEC_root *root, char base1, char base2, double x1, double y1, double x2, double y2, double base_dist, double char_size) const;
};

class SEC_graphic: public AWT_graphic {
protected:

    // variables - tree compatibility
    AW_clicked_line rot_cl;
    AW_clicked_text rot_ct;
    AW_clicked_line old_rot_cl;

    AW_device *disp_device; // device for  rekursiv Funktions

public:

    GBDATA *gb_main;
    AW_root *aw_root;
    SEC_root *sec_root;
    SEC_bond_def bond;

    int change_flag;    // used to indicate resize ....
    GBDATA *gb_struct;  // used to save the structure
    GBDATA *gb_struct_ref; // used to save reference numbers
    long    last_saved; // the transaction serial id when we last saved everything

    double x_cursor,y_cursor;
    // *********** public section
    SEC_graphic(AW_root *aw_root, GBDATA *gb_main);
    virtual ~SEC_graphic(void);

    virtual AW_gc_manager init_devices(AW_window *,AW_device *,AWT_canvas *ntw,AW_CL);

    virtual void show(AW_device *device);
    virtual void info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);
    virtual void command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, char key_char, AW_event_type type,
             AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);

    GB_ERROR load(GBDATA *gb_main, const char *name,AW_CL link_to_database, AW_CL insert_delete_cbs);
    GB_ERROR save(GBDATA *gb_main, const char *name,AW_CL cd1, AW_CL cd2);

    GB_ERROR write_data_to_db(const char *data, const char *x_string);

    int check_update(GBDATA *gb_main);  // reload tree if needed
    //  void update(GBDATA *gb_main);

};

#warning replace SEC_font_info by AW_font_group
class SEC_font_info
{
    int width,  // maximum letter sizes
        height,
        ascent,
        descent;

    static int max_width;
    static int max_height;
    static int max_ascent;
    static int max_descent;

public:

    SEC_font_info() {}

    static void reset_maximas()
    {
        max_width = 0;
        max_ascent = 0;
        max_height = 0;
        max_descent = 0;
    }

    void update(const AW_font_information *font_info)
    {
        width = font_info->max_letter_width;        if (width>max_width) max_width = width;
        height = font_info->max_letter_height;      if (height>max_height) max_height = height;
        ascent = font_info->max_letter_ascent;      if (ascent>max_ascent) max_ascent = ascent;
        descent = font_info->max_letter_descent;    if (descent>max_descent) max_descent = descent;
    }

    int get_width() const { return width; } // this font
    int get_height() const { return height; }
    int get_ascent() const { return ascent; }
    int get_descent() const { return descent; }

    static int get_max_width() { return max_width; }    // all update'd fonts
    static int get_max_height() { return max_height; }
    static int get_max_ascent() { return max_ascent; }
    static int get_max_descent() { return max_descent; }
};

extern SEC_font_info font_info[];
extern SEC_graphic *SEC_GRAPHIC;


#endif

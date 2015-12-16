// =============================================================== //
//                                                                 //
//   File      : aw_preset.hxx                                     //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AW_PRESET_HXX
#define AW_PRESET_HXX

#ifndef AW_WINDOW_HXX
#include "aw_window.hxx"
#endif
#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef CB_H
#include <cb.h>
#endif

#define AWP_COLORNAME_TEMPLATE "GCS/%s/MANAGE_GCS/%s/colorname"
#define AWP_FONTNAME_TEMPLATE "GCS/%s/MANAGE_GCS/%s/font"
#define AWP_FONTSIZE_TEMPLATE "GCS/%s/MANAGE_GCS/%s/size"

void AW_save_properties(AW_window *aw);   // use this if you're unsure
void AW_save_specific_properties(AW_window *aw, const char *filename);

AW_window *AW_preset_window(AW_root *root);

void AW_insert_common_property_menu_entries(AW_window_menu_modes *awmm);
void AW_insert_common_property_menu_entries(AW_window_simple_menu *awsm);

enum AW_GCM_AREA {
    AW_GCM_DATA_AREA,
    AW_GCM_WINDOW_AREA
};

DECLARE_CBTYPE_FVV_AND_BUILDERS(GcChangedCallback, void, GcChange); // generates makeGcChangedCallback

/* creates some GC pairs: one for normal operation,
                    the other for drag mode
        eg.
        AW_manage_GC(aww,"ARB_NT",device,10,20,AW_GCM_DATA_AREA, my_expose_cb, cd1 ,cd2, "name","#sequence",NULL);

                (see implementation for more details on parameter strings)

        will create 4 GCs:
            GC 10 (normal) and 20 (drag)
            GC 11 (normal and monospaced (indicated by '#')
            21 drag and monospaced
            don't forget the 0 at the end of the fontname field

            When the GCs are modified the 'changecb' is called
*/

AW_gc_manager AW_manage_GC(AW_window                *aww,
                           const char               *gc_base_name,
                           AW_device                *device, int base_gc, int base_drag, AW_GCM_AREA area,
                           const GcChangedCallback&  changecb,
                           bool                      define_color_groups,
                           const char               *default_background_color,
                           ...) __ATTR__SENTINEL;




AW_window *AW_create_gc_window(AW_root *aw_root, AW_gc_manager id); // opens the properties Window

// same as AW_create_gc_window, but uses different window id and name
// (use if if there are two or more color def windows in one application,
// otherwise they save the same window properties)
AW_window *AW_create_gc_window_named(AW_root * aw_root, AW_gc_manager id_par, const char *wid, const char *windowname);

void AW_copy_GCs(AW_root *aw_root, const char *source_window, const char *dest_window, bool has_font_info, const char *id0, ...) __ATTR__SENTINEL;

#else
#error aw_preset.hxx included twice
#endif // AW_PRESET_HXX

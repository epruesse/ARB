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

void AW_save_properties(AW_window *aw);   // use this if you're unsure
void AW_save_specific_properties(AW_window *aw, const char *filename);

void AW_insert_common_property_menu_entries(AW_window_menu_modes *awmm);
void AW_insert_common_property_menu_entries(AW_window_simple_menu *awsm);

enum AW_GCM_AREA {
    AW_GCM_DATA_AREA,
    AW_GCM_WINDOW_AREA
};

DECLARE_CBTYPE_FVV_AND_BUILDERS(GcChangedCallback, void, GcChange); // generates makeGcChangedCallback

AW_gc_manager *AW_manage_GC(AW_window                *aww,
                            const char               *gc_base_name,
                            AW_device                *device, int base_gc, int base_drag, AW_GCM_AREA area,
                            const GcChangedCallback&  changecb,
                            bool                      define_color_groups,
                            const char               *default_background_color,
                            ...) __ATTR__SENTINEL;




AW_window *AW_create_gc_window(AW_root *aw_root, AW_gc_manager *gcman); // opens the properties Window

AW_window *AW_create_gc_window_named(AW_root * aw_root, AW_gc_manager *gcman_par, const char *wid, const char *windowname);

void AW_copy_GCs(AW_root *aw_root, const char *source_window, const char *dest_window, bool has_font_info, const char *id0, ...) __ATTR__SENTINEL;

#else
#error aw_preset.hxx included twice
#endif // AW_PRESET_HXX

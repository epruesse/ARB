#ifndef AW_PRESET_HXX
#define AW_PRESET_HXX

#define AWP_COLORNAME_TEMPLATE "GCS/%s/MANAGE_GCS/%s/colorname"
#define AWP_FONTNAME_TEMPLATE "GCS/%s/MANAGE_GCS/%s/font"
#define AWP_FONTSIZE_TEMPLATE "GCS/%s/MANAGE_GCS/%s/size"

void AW_save_defaults( AW_window *aw ); // use this if you're unsure
void AW_save_specific_defaults( AW_window *aw, const char *filename);

AW_window *AW_preset_window( AW_root *root );

typedef enum {
    AW_GCM_DATA_AREA,
    AW_GCM_WINDOW_AREA
} AW_GCM_AREA;

AW_gc_manager AW_manage_GC(AW_window                                       *aww,
                           AW_device                                       *device, int base_gc, int base_drag, AW_GCM_AREA area,
                           void (*changecb)(AW_window*,AW_CL,AW_CL), AW_CL  cd1, AW_CL cd2,
                           bool                                             define_color_groups,
                           const char                                      *default_background_color,
                           ...);
/* creates some GC pairs: one for normal operation,
                    the other for drag mode
        eg.
        AW_manage_GC(aww,device,10,20,AW_GCM_DATA_AREA, my_expose_cb, cd1 ,cd2, "name","#sequence",0);

                (see implementation for more details on parameter strings)

        will create 4 GCs:
            GC 10 (normal) and 20 (drag)
            GC 11 (normal and monospaced (indicated by '#')
               21 drag and monospaced
            don't forget the 0 at the end of the fontname field

            When the GCs are modified the 'changecb' is called
        */

AW_window *AW_create_gc_window(AW_root *aw_root, AW_gc_manager id);
        /* opens the properties Window */


void AW_preset_create_font_chooser(AW_window *aws, const char *awar, const char *label,bool message_reload = false);
void AW_preset_create_scale_chooser(AW_window *aws, const char *awar, const char *label);
void AW_preset_create_color_chooser(AW_window *aws, const char *awar, const char *label,bool message_reload = false, bool show_label = false);

void AW_copy_GCs(AW_root *aw_root, const char *source_window, const char *dest_window, AW_BOOL has_font_info, const char *id0, ...);

#else
#error aw_preset.hxx included twice
#endif

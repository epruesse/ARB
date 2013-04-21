
#include <stack>

#include "aw_drawing_area.hxx"
#include "aw_at_layout.hxx"


/**
 * This class hides all private or gtk dependent attributes.
 * This is done to avoid gtk includes in the header file.
 */
class AW_window::AW_window_gtk : virtual Noncopyable {
public:
    
    GtkWindow *window; /**< The gtk window instance managed by this aw_window */
    
    /**
     *  A fixed size widget spanning the whole window or a part of the window. Everything is positioned on this widget using absolut coordinates.
     * @note This area might not be present in every window.
     * @note This area is the same as the INFO_AREA
     */
    AwAtLayout *fixed_size_area; 
    
    /**
     * A menu bar at the top of the window.
     * This may not be present in all windows. Check for NULL before use
     */
    GtkMenuBar *menu_bar;
    
    /**
     * Used to keep track of the menu structure while creating menus.
     * Empty if menu_bar is NULL.
     */
    std::stack<GtkMenuShell*> menus;
    
    /**
     * The help menu. Might not exist in some windows. Check for NULL before use.
     */
    GtkMenuShell *help_menu;
    
    /**
     * The mode menu. Might not exist in some windows. Check for NULL before use.
     */
    GtkToolbar *mode_menu;
 
    /**
     * The last created radio button. NULL if no open group.
     */
    GtkWidget *radio_last;

    /*
     * The box for the current toggle field (radio group). NULL if no open field.
     */
    GtkWidget *toggle_field;
    
    /**
     * Name of the awar of the current toggle field (radio group). NULL if no open field.
     */
    const char *toggle_field_awar_name;

    /*
     * The current ComboBox, NULL if none under construction
     */
    GtkWidget *combo_box;

    /**The accelerator group */
    GtkAccelGroup *accel_group;

    /**The last created widget */
    GtkWidget *last_widget;
    
    /**
     * A window consists of several areas.
     * Some of those areas are named, some are unnamed.
     * The unnamed areas are instantiated once and never changed, therefore no references to unnamed areas exist.
     * Named areas are instantiated depending on the window type.
     *
     * This vector contains references to the named areas.
     * The AW_Area enum is used to index this vector.
     */
    std::vector<AW_area_management *> areas;
    
    /**
     * Callback struct for the currently open popup
     */
    AW_cb_struct  *popup_cb;
    
    /**
     TODO comment
     */
    AW_cb_struct *focus_cb;
    
    /** Contains the last callback struct created by AW_window::callback(). */
    AW_cb_struct *callback; 

    /** Contains the last callback struct created by AW_window::callback(). */
    AW_cb_struct *d_callback; 
    
    /**The drawing area of this window. Might be NULL. */
    AW_drawing_area *drawing_area;
    /**ID of the delete event handler for this window. -1 if no handler present.*/
    int delete_event_handler_id;
    
       
    /**
     * default constructor
     */
    AW_window_gtk();

    /**
     * show window
     */
    void show();

    /**
     * Set window title.
     */
    void set_title(const char*); 

    /**
     * Set window width/height
     */
    void set_size(int width, int height);

    /**
     * Get window width/height
     */
    void get_size(int& width, int& height);

    /**
     * Make window resizable (or not)
     */
    void set_resizable(bool resizable);

    /**
     * Get approximate character width/height for current window.
     */
    void get_font_size(int& with, int& height);
};


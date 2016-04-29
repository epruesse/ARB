
#include <stack>

#include "aw_drawing_area.hxx"
#include "aw_action.hxx"


/**
 * This class hides all private or gtk dependent attributes.
 * This is done to avoid gtk includes in the header file.
 */
class AW_window_gtk  {
    AW_window_gtk(const AW_window_gtk&);
    AW_window_gtk& operator=(const AW_window_gtk&);
public:
    
    GtkWindow *window; /**< The gtk window instance managed by this aw_window */
    
    /**
     *  A fixed size widget spanning the whole window or a part of the window. Everything is positioned on this widget using absolut coordinates.
     * @note This area might not be present in every window.
     * @note This area is the same as the INFO_AREA
     */
    AwDrawingArea *fixed_size_area; 
    
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
    GtkRadioButton *radio_last;

    /**
     * The current selection list 
     */
    AW_selection_list *selection_list;

    /*
     * The box for the current toggle field (radio group). NULL if no open field.
     */
    GtkWidget *toggle_field;
    
    /**
     * Name of the awar of the current toggle field (radio group). NULL if no open field.
     */
    char *toggle_field_awar_name;

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
    AW_signal popup_cb;
    
    /**
     TODO comment
    */
    AW_signal focus_cb;

    /** Template for action to be created next (for setting help, cbs, etc prior to making button) */
    AW_action action_template;

    /**The drawing area of this window. Might be NULL. */
    AwDrawingArea *drawing_area;

    /**The bottom area of this window. Might be NULL. */
    AwDrawingArea *bottom_area;

    /**true if this window should be hidden instead of destroyed on close*/
    bool hide_on_close;
    
    /** Will be triggered when the window close button is clicked */
    AW_action *close_action;
       
    /**
     * default constructor
     */
    AW_window_gtk();

    /**
     * destructor
     */
    ~AW_window_gtk();

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


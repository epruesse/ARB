
#include <gtk/gtklabel.h>
#include <gtk/gtkfixed.h>
#include <gtk/gtkbutton.h>
#include <gtk/gtkcheckbutton.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkmenubar.h>
#include <gtk/gtkhbox.h>

#include <vector>
#include <stack>
#include <gtk-2.0/gtk/gtkmenuitem.h>
#include <gtk-2.0/gtk/gtkseparatormenuitem.h>
#include <gtk-2.0/gtk/gtktoolbar.h>


/**
 * This class hides all private or gtk dependent attributes.
 * This is done to avoid gtk includes in the header file.
 */
class AW_window::AW_window_gtk : private Noncopyable {
public:
    
    GtkWindow *window; /**< The gtk window instance managed by this aw_window */
    
    /**
     *  A fixed size widget spanning the whole window or a part of the window. Everything is positioned on this widget using absolut coordinates.
     * @note This area might not be present in every window.
     * @note This area is the same as the INFO_AREA
     */
    GtkFixed *fixed_size_area; 
    
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
     * The box for the current radio group. NULL if no open group.
     */
    GtkWidget *radio_box;

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
     * This is a counter for the number of items in the mode_menu.
     * It only exists to satisfy the old interface of create_mode()
     */
    int number_of_modes;
    
    /**
     * Callback struct for the currently open popup
     */
    AW_cb_struct  *popup_cb;
    
    /**
     TODO comment
     */
    AW_cb_struct *focus_cb;
    
    /*
     * List of callbacks for the different mode buttons
     */
    AW_cb_struct **modes_f_callbacks;
    
    /** Contains the last callback struct created by AW_window::callback(). */
    AW_cb_struct *callback; 
    
    
    /**
     * Adjustment of the horizontal scrollbar.
     * @note might not be present in every window. Check for NULL before use.
     */
    GtkAdjustment *hAdjustment;
    
     /**
     * Adjustment of the vertical scrollbar.
     * @note might not be present in every window. Check for NULL before use.
     */   
    GtkAdjustment *vAdjustment;
    
    AW_window_gtk();
};


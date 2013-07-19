#include "aw_action.hxx"
#include "aw_assert.hxx"
#include "aw_root.hxx"
#include "aw_window.hxx"
#include "aw_window_gtk.hxx"
#include "aw_msg.hxx"

/**
 * Set the ID naming this action
 * INTERNAL USE BY AW_window AND AW_root only!
 */
void AW_action::set_id(const char* _id) {
    if (id) {
        free(id);
        id = NULL;
    }
    if (_id) {
        id = strdup(_id);
    }
}

/** 
 * Get the ID naming this action
 */
const char* AW_action::get_id() const {
    return id;
}

/**
 * Set the label shown for widgets associated with this action.
 * @param _label C string containing the label or NULL for no label.
 */
void AW_action::set_label(const char* _label) {
    if (label) {
        free(label);
        label = NULL;
    }
    if (_label) {
        label = strdup(_label);
    }
}

/**
 * Get the label for this action (see set_label)
 */
const char* AW_action::get_label() const {
    return label;
}

/**
 * Set the filename of the icon shown for widgets associated with this action.
 * @param _icon C string containing the icon name or NULL for no icon.
 */
void AW_action::set_icon(const char* _icon) {
    if (icon) {
        free(icon);
        icon = NULL;
    }
    if (_icon) {
        icon = strdup(_icon);
    }
}

/**
 * Get the icon for this action (see set_icon) 
 */
const char* AW_action::get_icon() const {
    return icon;
}

/**
 * Set the tooltip shown for widgets associated with this action.
 * @param _tooltip C string containing the tooltip or NULL for no tooltip.
 */
void AW_action::set_tooltip(const char* _tooltip) {
    if (tooltip) {
        free(tooltip);
        tooltip = NULL;
    } 
    if (_tooltip) {
        tooltip = strdup(_tooltip);
    }
}

/**
 * Get the tooltip for this action (see set_tooltip) 
 */
const char* AW_action::get_tooltip() const {
    return tooltip;
}

/**
 * Set the help system entry (filename) associated with this action.
 * @param _icon C string containing the help entry or NULL for no help.
 */
void AW_action::set_help(const char* _help_entry) {
    if (help_entry) {
        free(help_entry);
        help_entry = NULL;
    }
    if (_help_entry) {
        help_entry = strdup(_help_entry);
    }
}

/**
 * Get the help for this action (see set_help) 
 */
const char* AW_action::get_help() const {
    return help_entry;
}

/**
 * Set the "active mask" (bitmap to determine in which states this
 * action should be active/sensitive
 */
void AW_action::set_active_mask(AW_active mask) {
    active_mask = mask;
}

/**
 * Get active mask (see set_active_mask)
 */
AW_active AW_action::get_active_mask() const {
    return active_mask;
}

/**
 * Constructs an empty, unnamed action.
 */
AW_action::AW_action() 
    : AW_signal(),
      id(NULL),
      label(NULL),
      icon(NULL),
      tooltip(NULL),
      help_entry(NULL),
      active_mask(AWM_ALL)
{
}

/**
 * Constructs action from another action. 
 * ID is *NOT* copied!
 * INTERNAL USE BY AW_window AND AW_root only!
 */
AW_action::AW_action(const AW_action& o) 
    : AW_signal(o),
      id(NULL),
      label(NULL),
      icon(NULL),
      tooltip(NULL),
      help_entry(NULL),
      active_mask(AWM_ALL)
{
    *this = o;
}

/**
 * Copies data from other action into this one.
 * ID is *NOT* copied!
 * INTERNAL USE BY AW_window AND AW_root only!
 */
AW_action& AW_action::operator=(const AW_action& o) {
    AW_signal::operator=(o);
    set_label(o.label);
    set_icon(o.icon);
    set_tooltip(o.tooltip);
    set_help(o.help_entry);
    set_active_mask(o.active_mask);
    dclick = o.dclick;

    return *this;
}


/**
 * Just the destructor
 */
AW_action::~AW_action() {
    if (id) free(id);
    if (label) free(label);
    if (icon) free(icon);
    if (tooltip) free(tooltip);
    if (help_entry) free(help_entry);
}


/**
 * Pre-emission hook for things that have to be done before each action cb
 * - help
 * - macro recording
 */
bool AW_action::pre_emit() {
    AW_root *root = AW_root::SINGLETON;

    if (root->is_help_active()) {
        root->set_help_active(false);
        root->set_cursor(NORMAL_CURSOR);

        if (help_entry) {
            AW_POPUP_HELP(NULL, (AW_CL)help_entry);
        }
        return false; // action is done, showing the help
    }

    if (root->is_tracking()) { 
        root->track_action(id);
    }

    root->set_cursor(WAIT_CURSOR);
    return true;
}

void AW_action::post_emit() {
    AW_root *root = AW_root::SINGLETON;
    if (! root->is_help_active()) {
        AW_root::SINGLETON->set_cursor(NORMAL_CURSOR);
    }
}

void AW_action::enable_by_mask(AW_active mask) {
    set_enabled( (mask & active_mask) != 0);
}

#ifdef UNIT_TESTS
#include <test_unit.h>

#endif

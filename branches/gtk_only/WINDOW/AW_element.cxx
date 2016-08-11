#include "aw_element.hxx"
#include "aw_window.hxx"
#include <arb_string.h>

/**
 * Constructs an empty, unnamed action.
 */
AW_element::AW_element()
    : id(NULL),
      label(NULL),
      icon(NULL),
      tooltip(NULL),
      help_entry(NULL),
      active_mask(AWM_ALL),
      active(true),
      accel_key(0)
{
}


/**
 * Constructs action from another action. 
 * ID is *NOT* copied!
 * INTERNAL USE BY AW_window AND AW_root only!
 */
AW_element::AW_element(const AW_element& o) 
    : id(NULL),
      label(NULL),
      icon(NULL),
      tooltip(NULL),
      help_entry(NULL),
      active_mask(AWM_ALL),
      active(true),
      accel_key(0)
{
    *this = o;
}

/**
 * Copies data from other action into this one.
 * ID is *NOT* copied!
 * INTERNAL USE BY AW_window AND AW_root only!
 */
AW_element& AW_element::operator=(const AW_element& o) {
    set_label(o.label);
    set_icon(o.icon);
    set_tooltip(o.tooltip);
    set_help(o.help_entry);
    set_active_mask(o.active_mask);
    active=o.active;
    set_accel(o.get_accel());

    return *this;
}

/**
 * Just the destructor
 */
AW_element::~AW_element() {
    if (id) free(id);
    if (label) free(label);
    if (icon) free(icon);
    if (tooltip) free(tooltip);
    if (help_entry) free(help_entry);
}

static bool equal(const char* a, const char* b) {
    return a == b || (a != NULL && b != NULL && strcmp(a,b) == 0);
}

#if defined(DEBUG)
static void show_equal(const char* n, const char* a, const char* b) {
    if (!equal(a,b)) printf(" %s %s != %s\n", n, a, b);
}
#endif

bool AW_element::operator==(const AW_element& o) const {
    bool res = 
           equal(id,o.id)
        && equal(label,o.label)
        && equal(icon,o.icon)
        && equal(tooltip,o.tooltip)
        && equal(help_entry,o.help_entry)
        && active_mask == o.active_mask
        && active      == o.active
        && accel_key   == o.accel_key;
#ifdef DEBUG
    if (!res) {
        printf("element %s == %s?\n", id, o.id);
        show_equal("label", label, o.label);
        show_equal("icon", icon, o.icon);
        show_equal("tooltip", tooltip, o.tooltip);
        show_equal("help_entry", help_entry, o.help_entry);
        if (active_mask != o.active_mask) printf(" active_mask %li != %li\n", active_mask, o.active_mask);
        if (active != o.active) printf(" active %i != %i\n", active, o.active);
        if (accel_key != o.accel_key) printf(" accel_key %i != %i\n", accel_key, o.accel_key);
    }
#endif

    return res;
}

//////////    property getter/setters   ////////////

/**
 * Set the ID naming this action
 * INTERNAL USE BY AW_window AND AW_root only!
 */
void AW_element::set_id(const char* _id) {
    if (id) {
        free(id);
        id = NULL;
    }
    if (_id) {
        id = ARB_strdup(_id);
    }
}

/** 
 * Get the ID naming this action
 */
const char* AW_element::get_id() const {
    return id;
}

/**
 * Set the label shown for widgets associated with this action.
 * @param _label C string containing the label or NULL for no label.
 */
void AW_element::set_label(const char* _label) {
    if (label) {
        free(label);
        label = NULL;
    }
    if (_label) {
        label = ARB_strdup(_label);
    }
}

/**
 * Get the label for this action (see set_label)
 */
const char* AW_element::get_label() const {
    return label;
}

/**
 * Set the filename of the icon shown for widgets associated with this action.
 * @param _icon C string containing the icon name or NULL for no icon.
 */
void AW_element::set_icon(const char* _icon) {
    if (icon) {
        free(icon);
        icon = NULL;
    }
    if (_icon) {
        icon = ARB_strdup(_icon);
    }
}

/**
 * Get the icon for this action (see set_icon) 
 */
const char* AW_element::get_icon() const {
    return icon;
}

/**
 * Set the tooltip shown for widgets associated with this action.
 * @param _tooltip C string containing the tooltip or NULL for no tooltip.
 */
void AW_element::set_tooltip(const char* _tooltip) {
    if (tooltip) {
        free(tooltip);
        tooltip = NULL;
    } 
    if (_tooltip) {
        tooltip = ARB_strdup(_tooltip);
    }
}

/**
 * Get the tooltip for this action (see set_tooltip) 
 */
const char* AW_element::get_tooltip() const {
    return tooltip;
}

/**
 * Set the help system entry (filename) associated with this action.
 * @param _icon C string containing the help entry or NULL for no help.
 */
void AW_element::set_help(const char* _help_entry) {
    if (help_entry) {
        free(help_entry);
        help_entry = NULL;
    }
    if (_help_entry) {
        help_entry = ARB_strdup(_help_entry);
    }
}

/**
 * Get the help for this action (see set_help) 
 */
const char* AW_element::get_help() const {
    return help_entry;
}

/**
 * Set the "active mask" (bitmap to determine in which states this
 * action should be active/sensitive
 */
void AW_element::set_active_mask(AW_active mask) {
    active_mask = mask;
}

/**
 * Get active mask (see set_active_mask)
 */
AW_active AW_element::get_active_mask() const {
    return active_mask;
}

/**
 * Set the accellerator key (e.g. ctrl-x)
 */
void AW_element::set_accel(guint a) {
    accel_key = a;
}

/** 
 * Get the accellerator key
 */
guint AW_element::get_accel() const {
    return accel_key;
}

void AW_element::set_enabled(bool enabled) {
    active = enabled;
}

bool AW_element::get_enabled() const {
    return active;
}

void AW_element::enable_by_mask(AW_active mask) {
    set_enabled( (mask & active_mask) != 0);
}

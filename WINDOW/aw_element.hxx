#pragma once

#include "aw_base.hxx"

/**
 * Base class for AW_action and AW_awar
 * Stores general attributes of GUI elements
 */
class AW_element {
private:
    friend class AW_window;
    friend class AW_root;

    char      *id;          // unique action name
    char      *label;       // description for user
    char      *icon;        // filename of icon
    char      *tooltip;
    char      *help_entry;  // filename of help entry
    AW_active  active_mask; // sensitivity mask
    bool       active;      // sensitivity
    guint      accel_key;   // accellerator key

    void set_id(const char*);
protected:

public:
    AW_element();
    AW_element(const AW_element&);
    AW_element& operator=(const AW_element&);
    virtual ~AW_element();
    bool operator==(const AW_element& o) const;

    void set_label(const char*);
    void set_icon(const char*);
    void set_tooltip(const char*);
    void set_help(const char*);
    void set_active_mask(AW_active);
    void set_accel(guint);
    virtual void set_enabled(bool);
    void enable_by_mask(AW_active);

    const char* get_id() const;
    const char* get_label() const;
    const char* get_icon() const;
    const char* get_tooltip() const;
    const char* get_help() const;
    AW_active   get_active_mask() const;
    bool        get_enabled() const;
    guint       get_accel() const;


};

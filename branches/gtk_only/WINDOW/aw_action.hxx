#pragma once

#include "aw_signal.hxx"
#include "aw_base.hxx"

class AW_root;
class AW_window;

class AW_action : public AW_signal {
private:
    char      *id;         // unique action name
    char      *label;      // description for user
    char      *icon;       // filename of icon
    char      *tooltip;
    char      *help_entry; // filename of help entry
    AW_active  active_mask;  // sensitivity mask

    friend class AW_window;
    friend class AW_root;
    
    AW_action();
    ~AW_action();
    AW_action(const AW_action&);
    AW_action& operator=(const AW_action&);

    void set_id(const char*);
protected:
    virtual bool pre_emit();
    virtual void post_emit();

public:
    AW_signal  dclick;  // triggered on double click

    void set_label(const char*);
    void set_icon(const char*);
    void set_tooltip(const char*);
    void set_help(const char*);
    void set_active_mask(AW_active);
    const char* get_id() const;
    const char* get_label() const;
    const char* get_icon() const;
    const char* get_tooltip() const;
    const char* get_help() const;
    AW_active   get_active_mask() const;

    void enable_by_mask(AW_active);
};

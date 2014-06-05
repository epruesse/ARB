#pragma once

#include "aw_element.hxx"
#include "aw_signal.hxx"

typedef void (*AW_postcb_cb)(AW_window *);

class AW_root;
class AW_window;

class AW_action : public AW_element {
private:
    struct Pimpl;
    Pimpl *prvt;
 
    friend class AW_root;
    friend class AW_window;
    friend class AW_window_gtk;
    friend class AW_window_menu_modes;
protected:
    AW_action();
    ~AW_action();
    AW_action(const AW_action&);
    AW_action& operator=(const AW_action&);

public:
    AW_signal clicked;
    AW_signal dclicked;

    bool equal_nobound(const AW_action&) const;

    virtual void user_clicked(GtkWidget*);
    void set_enabled(bool) OVERRIDE;
  
    void bind(GtkWidget*, const char*);
    void unbind(GtkWidget*, const char*);
    void bound_set(const char*, ...) __ATTR__SENTINEL;

    static void set_AW_postcb_cb(AW_postcb_cb);
};

#pragma once

#include "aw_element.hxx"
#include "aw_signal.hxx"


class AW_root;
class AW_window;

class AW_action : public AW_element {
private:
    struct Pimpl;
    Pimpl *prvt;
   
    AW_action();
    ~AW_action();
    AW_action(const AW_action&);
    AW_action& operator=(const AW_action&);

    friend class AW_root;
    friend class AW_window;
public:
    AW_signal clicked;
    AW_signal dclicked;

    void user_clicked();
    void set_enabled(bool) OVERRIDE;
  
    void bind(GtkWidget*, const char*);
    void unbind(GtkWidget*, const char*);
  
};

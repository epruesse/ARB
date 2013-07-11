#pragma once
#include "aw_gtk_forward_declarations.hxx"

class AW_awar;
class AW_selection_list;

class AW_dialog : virtual Noncopyable {
private:
    struct Pimpl;
    Pimpl *prvt;
    
    /**Disables the default selection of the selection list of both the selection list
       and the input field use the same awar.*/
    void disable_default_selection_if_same_awars();
    
public:
    AW_dialog();
    ~AW_dialog();

    void run();
    void set_title(const char*);
    void set_message(const char*);
    void create_buttons(const char* buttons);
    void create_input_field(AW_awar*);
    void create_toggle(AW_awar*, const char *label);
    AW_selection_list* create_selection_list(AW_awar*);

    int get_result();
};

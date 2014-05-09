#pragma once

class AW_awar;
class AW_selection_list;

class AW_dialog : virtual Noncopyable {
private:
    struct Pimpl;
    Pimpl *prvt;
    
public:
    AW_dialog();
    ~AW_dialog();

    void run();
    void set_title(const char*);
    void set_message(const char*);
    void create_buttons(const char* buttons);
    void create_input_field(AW_awar*);
    void create_toggle(AW_awar*, const char *label);
    AW_selection_list* create_selection_list(AW_awar *awar, bool fallback2default);

    int get_result();
};

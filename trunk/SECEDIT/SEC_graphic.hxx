// =============================================================== //
//                                                                 //
//   File      : sec_graphic.hxx                                   //
//   Purpose   : interface to structure GUI                        //
//   Time-stamp: <Fri Sep/14/2007 09:47 MET Coder@ReallySoft.de>   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //


#ifndef SEC_GRAPHIC_HXX
#define SEC_GRAPHIC_HXX

#ifndef _STRING_H
#include "string.h"
#endif
#ifndef _CTYPE_H
#include "ctype.h"
#endif

#ifndef AWT_CANVAS_HXX
#include <awt_canvas.hxx>
#endif
#ifndef AW_POSITION_HXX
#include <aw_position.hxx>
#endif

using namespace AW;

// names for database:
#define NAME_OF_STRUCT_SEQ      "_STRUCT"
#define NAME_OF_REF_SEQ         "REF"

class SEC_root;

enum SEC_update_request {
    SEC_UPDATE_OK              = 0, // no update needed
    SEC_UPDATE_ZOOM_RESET      = 1, // zoom reset needed
    SEC_UPDATE_SHOWN_POSITIONS = 2, // recalculation of shown positions needed
    SEC_UPDATE_RECOUNT         = 4, // complete relayout needed
    SEC_UPDATE_RELOAD          = 8, // reload structure from DB
};

class SEC_base;

class SEC_graphic: public AWT_graphic {
    SEC_update_request  update_requested;
    char               *load_error; // error occurred during load()

protected:

    // variables - tree compatibility
    AW_clicked_line rot_cl;
    AW_clicked_text rot_ct;
    AW_clicked_line old_rot_cl;

    AW_device *disp_device; // device for recursive functions

    GB_ERROR handleKey(AW_event_type event, AW_key_mod key_modifier, AW_key_code key_code, char key_char);
    GB_ERROR handleMouse(AW_device *device, AW_event_type event, int button, AWT_COMMAND_MODE cmd, const Position& world, SEC_base *elem, int abspos);

public:

    GBDATA *gb_main;
    AW_root *aw_root;
    SEC_root *sec_root;
    // SEC_bond_def bond;

    GBDATA *gb_struct;          // used to save the structure
    GBDATA *gb_struct_ref;      // used to save reference numbers
    
    mutable long last_saved;    // the transaction serial id when we last saved everything

    // *********** public section
    SEC_graphic(AW_root *aw_root, GBDATA *gb_main);
    virtual ~SEC_graphic(void);

    virtual AW_gc_manager init_devices(AW_window *,AW_device *,AWT_canvas *ntw,AW_CL);

    virtual void show(AW_device *device);
    virtual void info(AW_device *device, AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);
    virtual void command(AW_device *device, AWT_COMMAND_MODE cmd, int button, AW_key_mod key_modifier, AW_key_code key_code, char key_char, AW_event_type type,
             AW_pos x, AW_pos y, AW_clicked_line *cl, AW_clicked_text *ct);

    GB_ERROR load(GBDATA *gb_main, const char *name, AW_CL cd1, AW_CL cd2); // load structure from DB
    GB_ERROR save(GBDATA *gb_main, const char *name, AW_CL cd1, AW_CL cd2); // save structure to DB
    int check_update(GBDATA *gb_main);  // perform requested updates
    void update(GBDATA *gb_main);

    GB_ERROR write_data_to_db(const char *data, const char *x_string) const;
    GB_ERROR read_data_from_db(char **data, char **x_string) const;

    void request_update(SEC_update_request req) { update_requested = static_cast<SEC_update_request>(update_requested|req); }
};

// extern SEC_graphic *SEC_GRAPHIC;


#endif

// =========================================================== //
//                                                             //
//   File      : awt.hxx                                       //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWT_HXX
#define AWT_HXX

#ifndef AW_BASE_HXX
#include <aw_base.hxx>
#endif
#ifndef ARBDB_BASE_H
#include <arbdb_base.h>
#endif


#define awt_assert(cond) arb_assert(cond)

// ------------------------------------------------------------

// holds all stuff needed to make ad_.. functions work with species _and_ genes (and more..)


// ------------------------------------------------------------

void AWT_create_ascii_print_window(AW_root *awr, const char *text_to_print, const char *title=0);
void AWT_show_file(AW_root *awr, const char *filename);

// open database viewer using input-mask-file
class awt_item_type_selector;
GB_ERROR AWT_initialize_input_mask(AW_root *root, GBDATA *gb_main, const awt_item_type_selector *sel, const char* mask_name, bool localMask);


class awt_input_mask_descriptor {
private:
    char *title;                // title of the input mask
    char *internal_maskname;    // starts with 0 for local mask and with 1 for global mask
    // followed by file name w/o path
    char *itemtypename;         // name of the itemtype
    bool  local_mask;           // true if mask file was found in "$ARB_PROP/inputMasks"
    bool  hidden;               // if true, mask is NOT shown in Menus

public:
    awt_input_mask_descriptor(const char *title_, const char *maskname_, const char *itemtypename_, bool local, bool hidden_);
    awt_input_mask_descriptor(const awt_input_mask_descriptor& other);
    virtual ~awt_input_mask_descriptor();

    awt_input_mask_descriptor& operator = (const awt_input_mask_descriptor& other);

    const char *get_title() const { return title; }
    const char *get_maskname() const { return internal_maskname+1; }
    const char *get_internal_maskname() const { return internal_maskname; }
    const char *get_itemtypename() const { return itemtypename; }

    bool is_local_mask() const { return local_mask; }
    bool is_hidden() const { return hidden; }
};

const awt_input_mask_descriptor *AWT_look_input_mask(int id); // id starts with 0; returns 0 if no more masks

#if defined(DEBUG)
// database browser :
void AWT_create_db_browser_awars(AW_root *aw_root, AW_default aw_def);
void AWT_announce_db_to_browser(GBDATA *gb_main, const char *description);
void AWT_browser_forget_db(GBDATA *gb_main);

void AWT_create_debug_menu(AW_window *awmm);
void AWT_check_action_ids(AW_root *aw_root, const char *suffix);
#endif // DEBUG

class UserActionTracker;
AW_root *AWT_create_root(const char *properties, const char *program, UserActionTracker *user_tracker, int* argc, char*** argv);

void AWT_install_cb_guards();
void AWT_install_postcb_cb(AW_postcb_cb postcb_cb);

#else
#error awt.hxx included twice
#endif // AWT_HXX

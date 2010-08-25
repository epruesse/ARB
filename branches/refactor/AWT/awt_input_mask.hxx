//  ==================================================================== //
//                                                                       //
//    File      : AWT_input_mask.h                                       //
//    Purpose   : General input masks                                    //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in August 2001           //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef AWT_INPUT_MASK_HXX
#define AWT_INPUT_MASK_HXX

#ifndef _CPP_STRING
#include <string>
#endif
#ifndef AWT_HXX
#include <awt.hxx>
#endif


typedef enum {
    AWT_IT_UNKNOWN,
    AWT_IT_SPECIES,
    AWT_IT_ORGANISM,
    AWT_IT_GENE,
    AWT_IT_EXPERIMENT,

    AWT_IT_TYPES
} awt_item_type;

//  -------------------------------------
//      class awt_item_type_selector
//  -------------------------------------
// awt_item_type_selector is an interface for specific item-types
//
// derive from awt_item_type_selector to get the functionality for
// other item types.
//
// (implemented for Species in nt_item_type_species_selector (see NTREE/NT_extern.cxx) )
//

class awt_item_type_selector {
private:
    awt_item_type my_type;
public:
    awt_item_type_selector(awt_item_type for_type) : my_type(for_type) {}
    virtual ~awt_item_type_selector() {}

    awt_item_type get_item_type() const { return my_type; }

    // add/remove callbacks to awars (i.e. to AWAR_SPECIES_NAME)
    virtual void add_awar_callbacks(AW_root *root, void (*f)(AW_root*, AW_CL), AW_CL cl_mask) const = 0;
    virtual void remove_awar_callbacks(AW_root *root, void (*f)(AW_root*, AW_CL), AW_CL cl_mask) const = 0;

    // returns the current item
    virtual GBDATA *current(AW_root *root, GBDATA *gb_main) const = 0;

    // returns the keypath for items
    virtual const char *getKeyPath() const = 0;

    // returns the name of an awar containing the name of the current item
    virtual const char *get_self_awar() const = 0;

    // returns the maximum length of the name of the current item
    virtual size_t get_self_awar_content_length() const = 0;
};

typedef void (*AWT_OpenMaskWindowCallback)(AW_window* aww, AW_CL cl_id, AW_CL cl_user);

awt_item_type AWT_getItemType(const std::string& itemtype_name);
void          AWT_create_mask_submenu(AW_window_menu_modes *awm, awt_item_type wanted_item_type, AWT_OpenMaskWindowCallback open_mask_window_cb, AW_CL cl_user);
void          AWT_destroy_input_masks();

#else
#error AWT_input_mask.hxx included twice
#endif // AWT_INPUT_MASK_HXX


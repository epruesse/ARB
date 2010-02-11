// ================================================================ //
//                                                                  //
//   File      : AW_select.cxx                                      //
//   Purpose   :                                                    //
//                                                                  //
//   Coded by Ralf Westram (coder@reallysoft.de) in February 2010   //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "aw_select.hxx"

// ---------------------
//      AW_selection

void AW_selection::refresh() {
    get_win()->clear_selection_list(get_sellist());
    fill();
    get_win()->update_selection_list(get_sellist());
}


// -------------------------
//      AW_DB_selection

void AW_DB_selection_refresh_cb(GBDATA *, int *cl_selection, GB_CB_TYPE) {
    AW_DB_selection *selection = (AW_DB_selection*)cl_selection;;
    selection->refresh();
}

AW_DB_selection::AW_DB_selection(AW_window *win_, AW_selection_list *sellist_, GBDATA *gbd_)
    : AW_selection(win_, sellist_)
    , gbd(gbd_)
{
    GB_transaction ta(gbd);
    GB_add_callback(gbd, GB_CB_CHANGED, AW_DB_selection_refresh_cb, (int*)this);
}

AW_DB_selection::~AW_DB_selection() {
    GB_transaction ta(gbd);
    GB_remove_callback(gbd, GB_CB_CHANGED, AW_DB_selection_refresh_cb, (int*)this);
}




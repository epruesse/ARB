#include "aw_window.hxx"
#include "aw_root.hxx"
#include "aw_window_gtk.hxx"

void AW_clock_cursor(AW_root *awr) {
  awr->set_cursor(WAIT_CURSOR);
}

void AW_normal_cursor(AW_root *awr) {
  awr->set_cursor(NORMAL_CURSOR);
}

void AW_help_entry_pressed(AW_window *window) {
    aw_assert(NULL != window);
    //Help mode is global. It is managed by aw_root.
    //This method should be part of aw_root, not aw_window.
    FIXME("This method should be moved to aw_root.");
    AW_root* pRoot = window->get_root();
    aw_assert(NULL != pRoot);
    
    pRoot->set_help_active(true);
    
    pRoot->set_cursor(HELP_CURSOR);
}

void AW_POPDOWN(AW_window *window){
    window->hide();
}


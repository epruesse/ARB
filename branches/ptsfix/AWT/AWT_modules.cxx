// ============================================================= //
//                                                               //
//   File      : AWT_modules.cxx                                 //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in April 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "awt_modules.hxx"
#include <aw_window.hxx>

#define awt_assert(cond) arb_assert(cond)

void awt_create_order_buttons(AW_window *aws, awt_orderfun reorder_cb, AW_CL cl) {
    // generates order buttons at current pos and binds them to 'reorder_cb'
    // 'cl' is forwarded to reorder_cb

    AW_at_auto auto_at;
    auto_at.store(aws->_at);

    aws->auto_space(1, 1);
    aws->at_newline();

    int x, y; aws->get_at_position(&x, &y);

    aws->callback((AW_CB)reorder_cb, ARM_TOP, cl); aws->create_button("moveTop", "#moveTop.xpm", 0);

    aws->at_newline();
    int yoff = aws->get_at_yposition()-y;
    awt_assert(yoff>0);

    aws->at(x, y+1*yoff); aws->callback((AW_CB)reorder_cb, ARM_UP,     cl); aws->create_button("moveUp",     "#moveUp.xpm",     0);
    aws->at(x, y+2*yoff); aws->callback((AW_CB)reorder_cb, ARM_DOWN,   cl); aws->create_button("moveDown",   "#moveDown.xpm",   0);
    aws->at(x, y+3*yoff); aws->callback((AW_CB)reorder_cb, ARM_BOTTOM, cl); aws->create_button("moveBottom", "#moveBottom.xpm", 0);

    auto_at.restore(aws->_at);
}

inline const char *bitmap_name(bool rightwards, bool all) {
    if (all) return rightwards ? "#moveAllRight.xpm" : "#moveAllLeft.xpm"; // uses_pix_res("moveAllRight.xpm", "moveAllLeft.xpm");
    return rightwards ? "#moveRight.xpm" : "#moveLeft.xpm";                // uses_pix_res("moveRight.xpm", "moveLeft.xpm"); see ../SOURCE_TOOLS/check_ressources.pl@uses_pix_res
}

void awt_create_collect_buttons(AW_window *aws, bool collect_rightwards, awt_collectfun collect_cb, AW_CL cl) {
    // generates collect buttons at current pos and binds them to 'collect_cb'
    // 'cl' is forwarded to 'collect_cb'
    // 'collect_rightwards' affects the direction of the buttons 

    AW_at_auto auto_at;
    auto_at.store(aws->_at);

    aws->auto_space(1, 1);
    aws->button_length(0);
    aws->at_newline();

    int x, y; aws->get_at_position(&x, &y);

    bool       collect = collect_rightwards;
    const bool all     = true;

    aws->callback((AW_CB)collect_cb, ACM_FILL, cl); aws->create_button("ADDALL", bitmap_name(collect, all), 0);

    aws->at_newline();
    int yoff = aws->get_at_yposition()-y;
    awt_assert(yoff>0);

    aws->at(x, y+1*yoff); aws->callback((AW_CB)collect_cb, ACM_ADD,    cl); aws->create_button("ADD",    bitmap_name(collect,  !all), 0);
    aws->at(x, y+2*yoff); aws->callback((AW_CB)collect_cb, ARM_DOWN,   cl); aws->create_button("REMOVE", bitmap_name(!collect, !all), 0);
    aws->at(x, y+3*yoff); aws->callback((AW_CB)collect_cb, ARM_BOTTOM, cl); aws->create_button("CLEAR",  bitmap_name(!collect, all),  0);

    auto_at.restore(aws->_at);
}

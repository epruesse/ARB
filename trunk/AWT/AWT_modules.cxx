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
    // generates order buttons at 'atpos' and binds them to 'reorder_cb'
    // 'cl' is forwarded to reorder_cb

    AW_at_auto auto_at;
    auto_at.store(aws->_at);

    aws->auto_space(1, 1);
    aws->at_newline();

    int x, y; aws->get_at_position(&x, &y);

    aws->at(x-1, y); // workaround button displacement (1 pixel rightwards)
    aws->callback((AW_CB)reorder_cb, ARM_TOP, cl); aws->create_button("moveTop", "#moveTop.bitmap", 0);

    aws->at_newline();
    int yoff = aws->get_at_yposition()-y;
    awt_assert(yoff>0);

    aws->at(x, y+1*yoff); aws->callback((AW_CB)reorder_cb, ARM_UP,     cl); aws->create_button("moveUp",     "#moveUp.bitmap",     0);
    aws->at(x, y+2*yoff); aws->callback((AW_CB)reorder_cb, ARM_DOWN,   cl); aws->create_button("moveDown",   "#moveDown.bitmap",   0);
    aws->at(x, y+3*yoff); aws->callback((AW_CB)reorder_cb, ARM_BOTTOM, cl); aws->create_button("moveBottom", "#moveBottom.bitmap", 0);

    auto_at.restore(aws->_at);
}



// ============================================================ //
//                                                              //
//   File      : aw_scalar.cxx                                  //
//   Purpose   : Scalar variables (similar to perl scalars)     //
//                                                              //
//   Coded by Ralf Westram (coder@reallysoft.de) in June 2011   //
//   Institute of Microbiology (Technical University Munich)    //
//   http://www.arb-home.de/                                    //
//                                                              //
// ============================================================ //

#include "aw_scalar.hxx"
#include "aw_awar.hxx"
#include <arb_msg.h>

AW_scalar::AW_scalar(AW_awar *awar) {
    switch (awar->get_type()) {
        case AW_INT:     type = INT;   value.i = awar->read_int(); break;
        case AW_FLOAT:   type = FLOAT; value.f = awar->read_float(); break;
        case AW_STRING:  type = STR;   value.s = awar->read_string(); break;
        case AW_POINTER: type = PNTR;  value.p = awar->read_pointer(); break;
        default : GBK_terminatef("AWAR type %i unhandled", awar->get_type()); break;
    }
}


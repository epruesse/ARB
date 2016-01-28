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

AW_scalar::AW_scalar(AW_awar *awar) {
    type = awar->get_type();
    switch (type) {
        case GB_INT:     value.i = awar->read_int(); break;
        case GB_FLOAT:   value.f = awar->read_float(); break;
        case GB_STRING:  value.s = awar->read_string(); break;
        case GB_POINTER: value.p = awar->read_pointer(); break;
        default : GBK_terminatef("AWAR type %i unhandled", awar->get_type()); break;
    }
}

GB_ERROR AW_scalar::write_to(class AW_awar *awar) {
    GB_ERROR error = NULL;
    switch (awar->get_type()) {
        case GB_INT:     error = awar->write_int(get_int()); break;
        case GB_FLOAT:   error = awar->write_float(get_float()); break;
        case GB_STRING:  error = awar->write_string(get_string()); break;
        case GB_POINTER: error = awar->write_pointer(get_pointer()); break;
        default : GBK_terminatef("AWAR type %i unhandled", awar->get_type()); break;
    }
    return error;
}

bool AW_scalar::operator == (const AW_awar*const& awar) const {
    // sync with aw_scalar.hxx@op_equal
    switch (type) {
        case GB_INT:     return value.i == awar->read_int();
        case GB_FLOAT:   return fabs(value.f - awar->read_float()) < 0.000001;
        case GB_STRING:  return strcmp(value.s, awar->read_char_pntr()) == 0;
        case GB_POINTER: return value.p == awar->read_pointer();
        default: aw_assert(false); return false;
    }
}

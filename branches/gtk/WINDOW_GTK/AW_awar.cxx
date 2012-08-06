// ============================================================= //
//                                                               //
//   File      : AW_awar.cxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Arne Boeckmann aboeckma@mpi-bremen.de on Aug 1, 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "aw_gtk_migration_helpers.hxx"
#include "aw_awar.hxx"

char *AW_awar::read_as_string() {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

const char * AW_awar::read_char_pntr() {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

double AW_awar::read_float() {
    GTK_NOT_IMPLEMENTED;
    return 0;

}

long AW_awar::read_int() {
    GTK_NOT_IMPLEMENTED;
    return 0;

}

GBDATA *AW_awar::read_pointer() {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

char *AW_awar::read_string() {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

void AW_awar::touch() {
    GTK_NOT_IMPLEMENTED;
}

void AW_awar::untie_all_widgets() {
    GTK_NOT_IMPLEMENTED;
}

GB_ERROR AW_awar::write_as_string(char const*) {
    GTK_NOT_IMPLEMENTED;
    return 0;

}

GB_ERROR AW_awar::write_float(double) {
    GTK_NOT_IMPLEMENTED;
    return 0;

}

GB_ERROR AW_awar::write_int(long) {
    GTK_NOT_IMPLEMENTED;
    return 0;

}

GB_ERROR AW_awar::write_pointer(GBDATA*) {
    GTK_NOT_IMPLEMENTED;
    return 0;

}


GB_ERROR AW_awar::write_string(char const*) {
    GTK_NOT_IMPLEMENTED;
    return 0;

}


AW_awar *AW_awar::add_callback(Awar_CB2 /*f*/, AW_CL /*cd1*/, AW_CL /*cd2*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;

}

AW_awar *AW_awar::add_callback(Awar_CB1 /*f*/, AW_CL /*cd1*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;

}

AW_awar *AW_awar::add_callback(Awar_CB0 /*f*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;

}

AW_awar *AW_awar::add_target_var(char **/*ppchr*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;

}

AW_awar *AW_awar::add_target_var(long */*pint*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;

}

AW_awar *AW_awar::add_target_var(float */*pfloat*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_VARIABLE_TYPE AW_awar::get_type() const {
    GTK_NOT_IMPLEMENTED;
    return (AW_VARIABLE_TYPE)0;
}


AW_awar *AW_awar::map(const char */*awarn*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_awar *AW_awar::map(AW_default /*dest*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_awar *AW_awar::map(AW_awar */*dest*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_awar *AW_awar::remove_callback(Awar_CB2 /*f*/, AW_CL /*cd1*/, AW_CL /*cd2*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_awar *AW_awar::remove_callback(Awar_CB1 /*f*/, AW_CL /*cd1*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_awar *AW_awar::remove_callback(Awar_CB0 /*f*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_awar *AW_awar::set_minmax(float /*min*/, float /*max*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_awar *AW_awar::set_srt(const char */*srt*/) {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

GB_ERROR AW_awar::toggle_toggle() {
    GTK_NOT_IMPLEMENTED;
    return 0;
}

AW_awar *AW_awar::unmap() {
    GTK_NOT_IMPLEMENTED;
    return 0;
}


/* 
 * File:   aw_type_checking.hxx
 * Author: aboeckma
 *
 * Created on January 22, 2013, 2:54 PM
 */

#pragma once
#include "aw_base.hxx"
#include <string>
__ATTR__NORETURN inline void type_mismatch(const char *triedType, const char *intoWhat) {
    GBK_terminatef("Cannot insert %s into %s which uses a non-%s AWAR", triedType, intoWhat, triedType);
}

__ATTR__NORETURN inline void selection_type_mismatch(const char *triedType) { type_mismatch(triedType, "selection-list"); }
__ATTR__NORETURN inline void option_type_mismatch(const char *triedType) { type_mismatch(triedType, "option-menu"); }
__ATTR__NORETURN inline void toggle_type_mismatch(const char *triedType) { type_mismatch(triedType, "toggle"); }

class AW_TypeCheck {
    
public:
    static GB_TYPES getVarType(int) {return GB_INT;}
    static GB_TYPES getVarType(float) {return GB_FLOAT;}
    static GB_TYPES getVarType(char*) {return GB_STRING;}
    static GB_TYPES getVarType(const char*) {return GB_STRING;} 
    static GB_TYPES getVarType(std::string) {return GB_STRING;}
    static GB_TYPES getVarType(char) {return GB_BYTE;}  
    static GB_TYPES getVarType(unsigned char) {return GB_BYTE;}
};

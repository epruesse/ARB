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
    static AW_VARIABLE_TYPE getVarType(int) {return AW_INT;}
    static AW_VARIABLE_TYPE getVarType(float) {return AW_FLOAT;}
    static AW_VARIABLE_TYPE getVarType(char*) {return AW_STRING;}
    static AW_VARIABLE_TYPE getVarType(const char*) {return AW_STRING;} 
    static AW_VARIABLE_TYPE getVarType(std::string) {return AW_STRING;}
    static AW_VARIABLE_TYPE getVarType(char) {return AW_BYTE;}  
    static AW_VARIABLE_TYPE getVarType(unsigned char) {return AW_BYTE;}
};

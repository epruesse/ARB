/* 
 * File:   AW_motif_gtk_conversion.cxx
 * Author: aboeckma
 * 
 * Created on October 23, 2012, 11:01 AM
 */

#include "aw_motif_gtk_conversion.hxx"

std::string AW_motif_gtk_conversion::convert_mnemonic(const char* text, const char* mnemonic) {
    std::string strText(text);
    std::string strMnemonic(mnemonic);
    if(NULL != mnemonic) {
        size_t mnemonicPos = strText.find(strMnemonic);
        if (strText.npos == mnemonicPos) {
            //mnemonic string is not part of the name, append it
            strText = strText + "(_" + strMnemonic + ")";
        }
        else {
            //mnemonic string is part of the name
            //insert _ infront of the mnemonic string to indicate the mnemonic char
            strText.insert(mnemonicPos, "_");
        }
    }
    return strText;
}
    

/* 
 * File:   AW_motif_gtk_conversion.hxx
 * Author: aboeckma
 *
 * Created on October 23, 2012, 11:01 AM
 */

#pragma once
#include <string>
/**
 * This class provides methods needed to convert motif internal data structures
 * to gtk internal structures.
 */
class AW_motif_gtk_conversion {
public:

    /**
     * Converts a pair of label name and mnemonic into a gtk mnemonic string.
     * @param text Text that should contain the mnemonic
     * @param mnemonic the mnemonic char.
     * @return 
     */
    static std::string convert_mnemonic(const char* text, const char* mnemonic);
    
    
    
private:

};

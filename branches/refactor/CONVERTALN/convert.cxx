// ================================================================ 
// 
// File      : convert.c                                          
// Purpose   : some helpers for global data handling              
// 
// ================================================================

#include "embl.h"
#include "genbank.h"
#include "macke.h"

SmartPtr<InputFormat> InputFormat::create(char informat) {
    switch (informat) {
        case GENBANK: return new GenBank;
        case SWISSPROT:
        case EMBL: return new Embl;
        case MACKE: return new Macke;
        default: ca_assert(0);
    }
    return NULL;
}

SeqPtr GenBank::get_data(FILE_BUFFER ifp) {
    SeqPtr seq = new Seq;
    char eof = genbank_in_locus(*this, *seq, ifp);
    if (eof == EOF) {
        seq.SetNull();
    }
    else {
        char temp[TOKENSIZE];
        genbank_key_word(locus, 0, temp, TOKENSIZE);
        seq->set_id(temp);
    }
    return seq;
}

SeqPtr Embl::get_data(FILE_BUFFER ifp) {
    SeqPtr seq = new Seq;
    char            eof      = embl_in_id(*this, *seq, ifp);

    if (eof == EOF) {
        seq.SetNull();
    }
    else {
        char temp[TOKENSIZE];
        embl_key_word(id, 0, temp, TOKENSIZE);
        seq->set_id(temp);
    }
    return seq;
}

SeqPtr Macke::get_data(FILE_BUFFER ifp) {
    SeqPtr seq = new Seq;
    char eof = macke_in_name_and_data(*this, *seq, ifp);
    if (eof == EOF) {
        seq.SetNull();
    }
    else {
        seq->set_id(seqabbr);
    }
    return seq;
}

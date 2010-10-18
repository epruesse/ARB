// ================================================================
//
// File      : convert.c
// Purpose   : some helpers for global data handling
//
// ================================================================

#include "embl.h"
#include "genbank.h"
#include "macke.h"

InputFormatPtr InputFormat::create(char informat) {
    switch (informat) {
        case GENBANK: return new GenBank;
        case SWISSPROT:
        case EMBL: return new Embl;
        case MACKE: return new Macke;
        default: ca_assert(0);
    }
    return NULL;
}

SeqPtr GenBank::read_data(Reader& reader) {
    SeqPtr seq = new Seq;
    genbank_in_simple(*this, *seq, reader);
    if (reader.failed() || seq->is_empty()) {
        seq.SetNull();
    }
    else {
        char temp[TOKENSIZE];
        genbank_key_word(locus, 0, temp, TOKENSIZE);
        seq->set_id(temp);
    }
    return seq;
}

SeqPtr Embl::read_data(Reader& reader) {
    SeqPtr seq = new Seq;
    embl_in_simple(*this, *seq, reader);

    if (reader.failed() || seq->is_empty()) {
        seq.SetNull();
    }
    else {
        char temp[TOKENSIZE];
        embl_key_word(id, 0, temp, TOKENSIZE);
        seq->set_id(temp);
    }
    return seq;
}

SeqPtr Macke::read_data(Reader& reader) {
    SeqPtr seq = new Seq;
    macke_in_simple(*this, *seq, reader);
    if (reader.failed() || seq->is_empty()) {
        seq.SetNull();
    }
    else {
        seq->set_id(seqabbr);
    }
    return seq;
}

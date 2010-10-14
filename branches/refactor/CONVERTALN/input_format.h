#ifndef INPUT_FORMAT_H
#define INPUT_FORMAT_H

#ifndef SEQ_H
#include "seq.h"
#endif
#ifndef DEFS_H
#include "defs.h"
#endif

struct InputFormat {
    virtual ~InputFormat() {}
    virtual SeqPtr get_data(FILE_BUFFER ifp) = 0;
    virtual void reinit() = 0;

    static SmartPtr<InputFormat> create(char informat);
    static bool is_known(char informat) {
        return informat == GENBANK || informat == EMBL || informat == SWISSPROT || informat == MACKE;
    }
};

#else
#error input_format.h included twice
#endif // INPUT_FORMAT_H

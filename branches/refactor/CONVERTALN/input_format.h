#ifndef INPUT_FORMAT_H
#define INPUT_FORMAT_H

#ifndef SEQ_H
#include "seq.h"
#endif
#ifndef DEFS_H
#include "defs.h"
#endif

class InputFormat {
    mutable char *id; // id of entry (=short-name)

public:
    InputFormat() : id(NULL) {}
    virtual ~InputFormat() { freenull(id); }

    virtual SeqPtr read_data(Reader& reader) = 0;
    virtual void reinit()                    = 0;
    virtual char *create_id() const    = 0;

    const char *get_id() const {
        if (!id) id = create_id();
        return id;
    }

    static SmartPtr<InputFormat> create(Format inType);
};

typedef SmartPtr<InputFormat> InputFormatPtr;

#else
#error input_format.h included twice
#endif // INPUT_FORMAT_H

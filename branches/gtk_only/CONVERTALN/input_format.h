#ifndef INPUT_FORMAT_H
#define INPUT_FORMAT_H

#ifndef SEQ_H
#include "seq.h"
#endif
#ifndef DEFS_H
#include "defs.h"
#endif

class FormatReader;

struct OutputFormat {
    virtual ~OutputFormat() { }
    virtual Format format() const = 0;
};

// all input formats have to be output formats:
class InputFormat : public OutputFormat, virtual Noncopyable {
    mutable char *id; // id of entry (=short-name)

    virtual char *create_id() const = 0;
public:
    InputFormat() : id(NULL) {}
    ~InputFormat() OVERRIDE { freenull(id); }

    virtual void reinit()                  = 0;
    Format format() const OVERRIDE = 0;

    const char *get_id() const {
        if (!id) id = create_id();
        return id;
    }
};

typedef SmartPtr<InputFormat> InputFormatPtr;
typedef SmartPtr<OutputFormat> OutputFormatPtr;

#else
#error input_format.h included twice
#endif // INPUT_FORMAT_H

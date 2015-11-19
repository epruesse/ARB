#ifndef FUN_H
#define FUN_H

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif

// forward decls for prototypes

enum Format {
    // input/output formats
    EMBL,
    GENBANK,
    MACKE,
    SWISSPROT,
    LAST_INPUT_FORMAT = SWISSPROT,

    // output-only formats
    GCG,
    NEXUS,
    PHYLIP,
    FASTDNAML,
    PRINTABLE,

    UNKNOWN,
};

inline bool is_input_format(Format inType) { return inType <= LAST_INPUT_FORMAT; }

class Reader;
class Writer;

struct Embl;
struct Emblref;
struct EmblSwissprotReader;
struct GenBank;
struct GenbankRef;
struct GenbankReader;
struct Macke;
struct MackeReader;
struct Paup;
struct Seq;
struct Alignment;

struct RDP_comments;
struct OrgInfo;
struct SeqInfo;

typedef void (*RDP_comment_parser)(char*& datastring, int start_index, Reader& reader);

class FormattedFile : virtual Noncopyable {
    char   *name_;
    Format  type_;
public:
    FormattedFile() : name_(0), type_(UNKNOWN) {}
    FormattedFile(const char *Name, Format Type);
    ~FormattedFile();

    void init(const char *Name, Format Type);

    const char *name() const { return name_; }
    Format type() const { return type_; }
};

#ifndef BUFFEREDFILEREADER_H
#include <BufferedFileReader.h>
#endif
#ifndef PROTOTYPES_H
#include "prototypes.h"
#endif

#else
#error fun.h included twice
#endif // FUN_H


// =============================================================== //
//                                                                 //
//   File      : parser.h                                          //
//   Purpose   :                                                   //
//                                                                 //
// =============================================================== //

#ifndef PARSER_H
#define PARSER_H

#ifndef READER_H
#include "reader.h"
#endif


class Parser : virtual Noncopyable {
protected:
    EntryState state;
    Seq&       seq;
    Reader&    reader;

public:
    Parser(Seq& seq_, Reader& reader_)
        : state(ENTRY_NONE),
          seq(seq_),
          reader(reader_)
    {}
    virtual ~Parser() {}

    bool parse_entry() __ATTR__USERESULT {
        state = ENTRY_NONE;

        for (; reader.line() && state != ENTRY_COMPLETED;) {
            if (!has_content(reader.line())) { ++reader; continue; } // skip empty lines

            parse_section();
        }

        if (state == ENTRY_STARTED) throw_incomplete_entry();
        ++reader;

        seq.set_id(get_data().get_id());
        return state == ENTRY_COMPLETED;
    }

    virtual void parse_section() = 0;
    virtual const InputFormat& get_data() const = 0;
};


#else
#error parser.h included twice
#endif // PARSER_H

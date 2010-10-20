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


class Parser : Noncopyable {
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

    void parse_entry() {
        state = ENTRY_NONE;

        for (; reader.line() && state != ENTRY_COMPLETED;) {
            if (!has_content(reader.line())) { ++reader; continue; } // skip empty lines

            parse_section();
        }

        if (state == ENTRY_STARTED) throw_incomplete_entry();
        ++reader;
    }

    virtual void parse_section() = 0;
};


#else
#error parser.h included twice
#endif // PARSER_H

// ============================================================= //
//                                                               //
//   File      : MultiFileReader.cxx                             //
//   Purpose   : Read multiple files like one concatenated file  //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2014   //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#include "MultiFileReader.h"
#include "arb_msg.h"

const string& MultiFileReader::getFilename() const {
    if (!reader) {
        if (last_reader) return last_reader->getFilename();
        arb_assert(0);
        static string nf = "unknown-source";
        return nf;
    }
    return reader->getFilename();
}
bool MultiFileReader::getLine_intern(string& line) {
    bool gotLine = false;
    if (reader) {
        gotLine = reader->getLine(line);
        if (!gotLine) {
            nextReader();
            gotLine = getLine_intern(line);
            if (gotLine) setLineNumber(reader->getLineNumber());
        }
    }
    return gotLine;
}

FILE *MultiFileReader::open(int i) {
    FILE *in       = fopen(files[i], "rt");
    if (!in) error = new string(GB_IO_error("reading", files[i]));
    return in;
}

void MultiFileReader::nextReader() {
    if (reader) {
        if (last_reader) {
            delete last_reader;
            last_reader = NULL;
        }
        last_reader = reader;
        reader      = NULL;
    }
    ++at;
    if (at<files.size()) {
        FILE *in = open(at);
        if (in) {
            reader = new BufferedFileReader(files[at], in);
        }
    }
}

MultiFileReader::MultiFileReader(const CharPtrArray& files_)
    : files(files_),
      reader(NULL),
      last_reader(NULL),
      error(NULL),
      at(-1)
{
    for (size_t i = 0; i<files.size() && !error; ++i) {
        // test open all files
        FILE *in = open(i);
        if (in) fclose(in);
    }
    if (!error) nextReader();
}

MultiFileReader::~MultiFileReader() {
    delete reader;
    delete last_reader;
    delete error;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif

void TEST_MultiFileReader() {
    ConstStrArray files;
    files.put("general/broken_LF.input"); // 6 lines
    files.put("general/dos.input");       // 6 lines
    files.put("general/mac.input");       // 6 lines
    files.put("general/empty.input");     // 1 line
    files.put("general/text.input");      // 6 lines

    {
        MultiFileReader reader(files);

        TEST_EXPECT_NO_ERROR(reader.get_error());

        string line;
        string emptyLineError;
        string db_found_msgs;
        int    lineCount      = 0;
        int    emptyLineCount = 0;

        while (reader.getLine(line)) {
            lineCount++;
            if (line.empty()) {
                emptyLineCount++;
                emptyLineError = reader.lineError("seen empty line");
            }
            else if (strstr(line.c_str(), "database") != 0) {
                db_found_msgs += reader.lineError("DB") + "\n";
            }
        }

        TEST_EXPECT_NO_ERROR(reader.get_error());
        TEST_EXPECT_EQUAL(lineCount, 25);
        TEST_EXPECT_EQUAL(emptyLineCount, 1);
        TEST_EXPECT_EQUAL(emptyLineError, "general/empty.input:1: seen empty line");
        TEST_EXPECT_EQUAL(db_found_msgs,
                          "general/broken_LF.input:2: DB\n"
                          "general/dos.input:2: DB\n"
                          "general/mac.input:2: DB\n"
                          "general/text.input:2: DB\n");
    }

    files.put("general/nosuch.input");
    {
        MultiFileReader reader(files);
        TEST_EXPECT_EQUAL(reader.get_error(), "While reading 'general/nosuch.input': No such file or directory");
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------


//  ==================================================================== //
//                                                                       //
//    File      : arb_help2xml.cxx                                       //
//    Purpose   : Converts old ARB help format to XML                    //
//    Time-stamp: <Fri Dec/03/2004 12:36 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in October 2001          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

// #include "arb_help2xml.h"

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif
#define h2x_assert(bed) arb_assert(bed)

#include <stdio.h>
#include <stdarg.h>
#include <list>
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <sys/stat.h>

#include <xml.hxx>

using namespace std;

/* hide GNU extensions for non-gnu compilers: */
#ifndef GNU
# ifndef __attribute__
#  define __attribute__(x)
# endif
#endif

string strf(const char *format, ...) __attribute__((format(printf, 1, 2)));


// #define DUMP_DATA // use this to see internal data (class Helpfile)
#define MAX_LINE_LENGTH 500     // maximum length of lines in input stream
#define TABSIZE         8


static const char *knownSections[] = { "OCCURRENCE", "DESCRIPTION", "NOTES", "EXAMPLES", "WARNINGS", "BUGS",
                                       "QUESTION", "ANSWER", "SECTION",
                                       0 };

using namespace std;

// #define X() do { printf("Line=%i\n", __LINE__); } while(0)
// #define D(x) do { printf("%s='%s'\n", #x, x); } while(0)

    // --------------------------------------------------------------------------------
//     string vstrf(const char *format, va_list argPtr)
// --------------------------------------------------------------------------------
string vstrf(const char *format, va_list argPtr) {
    static size_t  buf_size = 256;
    static char   *buffer   = new char[buf_size];

    size_t length;
    while (1) {
        if (!buffer) {
            h2x_assert(buffer);     // to stop when debugging
            throw string("out of memory");
        }

        length = vsnprintf(buffer, buf_size, format, argPtr);
        if (length < buf_size) break; // string fits into current buffer

        // otherwise resize buffer :
        buf_size += buf_size/2;
        //                 cerr << "Reallocate vstrf-buffer to size=" << buf_size << endl;
        delete [] buffer;
        buffer    = new char[buf_size];
    }

    return string(buffer, length);
}

//  ---------------------------------------------
//      string strf(const char *format, ...)
//  ---------------------------------------------
string strf(const char *format, ...) {
    va_list argPtr;
    va_start(argPtr, format);
    string result = vstrf(format, argPtr);
    va_end(argPtr);

    return result;
}

//  ---------------------
//      class Reader
//  ---------------------
class Reader {
private:
    istream& in;
    char     lineBuffer[MAX_LINE_LENGTH];
    char     lineBuffer2[MAX_LINE_LENGTH];
    bool     readAgain;
    bool     eof;
    int      lineNo;

    void getline() {
        if (!eof) {
            if (!in.good()) eof = true;
            in.getline(lineBuffer, MAX_LINE_LENGTH);
            lineNo++;

            if (strchr(lineBuffer, '\t')) {
                int o2 = 0;

                for (int o = 0; lineBuffer[o]; ++o) {
                    if (lineBuffer[o] == '\t') {
                        int spaces = TABSIZE - (o2 % TABSIZE);
                        while (spaces--) lineBuffer2[o2++] = ' ';
                    }
                    else {
                        lineBuffer2[o2++] = lineBuffer[o];
                    }
                }
                lineBuffer2[o2] = 0;
                strcpy(lineBuffer, lineBuffer2);
            }
        }
    }

public:
    Reader(istream& in_) : in(in_) , readAgain(true) , eof(false), lineNo(0) { getline(); }
    virtual ~Reader() {}

    const char *getNext() {
        if (readAgain) readAgain = false;
        else getline();
        return eof ? 0 : lineBuffer;
    }

    void back() {
        h2x_assert(!readAgain);
        readAgain = true;
    }

    int getLineNo() const { return lineNo; }
};


typedef list<string> Strings;
typedef list<string> Section;

//  ---------------------------
//      class NamedSection
//  ---------------------------
class NamedSection {
private:
    string  name;
    Section section;

public:
    NamedSection(const string& name_, const Section& section_)
        : name(name_)
        , section(section_)
    {}
    virtual ~NamedSection() {}

    const Section& getSection() const { return section; }
    const string& getName() const { return name; }
};

typedef list<NamedSection> NamedSections;

//  -----------------------
//      class Helpfile
//  -----------------------
class Helpfile {
private:
    Strings       uplinks;
    Strings       references;
    Section       title;
    NamedSections sections;

public:
    Helpfile() {}
    virtual ~Helpfile() {}

    void readHelp(istream& in, const string& filename);
    void writeXML(FILE *out, const string& page_name, const string& path1, const string& path2);
    void extractInternalLinks();
};

inline bool isWhite(char c) { return c == ' '; }

#if defined(DUMP_DATA)
//  ------------------------------------------------------------------------------------
//      static void display(const Strings& strings, const string& title, FILE *out)
//  ------------------------------------------------------------------------------------
static void display(const Strings& strings, const string& title, FILE *out) {
    fprintf(out, "  %s:\n", title.c_str());
    for (Strings::const_iterator s = strings.begin(); s != strings.end(); ++s) {
        fprintf(out, "    '%s'\n", s->c_str());
    }
}
//  --------------------------------------------------------------------------------------
//      static void display(const Sections& sections, const string& title, FILE *out)
//  --------------------------------------------------------------------------------------
static void display(const Sections& sections, const string& title, FILE *out) {
    fprintf(out, "%s:\n", title.c_str());
    for (Sections::const_iterator s = sections.begin(); s != sections.end(); ++s) {
        display(s->second, s->first, out);
    }
}
#endif // DUMP_DATA

//  ----------------------------------------------------
//      inline bool isEmptyOrComment(const char *s)
//  ----------------------------------------------------
inline bool isEmptyOrComment(const char *s) {
    if (s[0] == '#') return true;
    for (int off = 0; ; ++off) {
        if (s[off] == 0) return true;
        if (!isWhite(s[off])) break;
    }

    return false;
}

//  -----------------------------------------------------------------------------
//      inline const char *extractKeyword(const char *line, string& keyword)
//  -----------------------------------------------------------------------------
inline const char *extractKeyword(const char *line, string& keyword) {
    // returns NULL if no keyword was found
    // otherwise returns position behind keyword and sets value of 'keyword'

    char *space = strchr(line, ' ');
    if (space && space>line) {
        keyword = string(line, 0, space-line);
        return space;
    }
    else if (!space) { // test for keyword w/o content behind
        if (line[0]) { // not empty
            keyword = line;
//             cout << "keyword='" << keyword << "'\n";
            return strchr(line, 0);
        }
    }
    return 0;
}

//  ------------------------------------------------------
//      inline const char *eatWhite(const char *line)
//  ------------------------------------------------------
inline const char *eatWhite(const char *line) {
    // skips whitespace
    while (isWhite(*line)) ++line;
    return line;
}

//  -------------------------------------------------------------------
//      inline void pushParagraph(Section& sec, string& paragraph)
//  -------------------------------------------------------------------
inline void pushParagraph(Section& sec, string& paragraph) {
    if (paragraph.length()) {
        sec.push_back(paragraph);
        paragraph = "";
    }
}

//  ----------------------------------------------------
//      inline const char *firstChar(const char *s)
//  ----------------------------------------------------
inline const char *firstChar(const char *s) {
    while (isWhite(s[0])) ++s;
    return s;
}

//  --------------------------------------------------------------------------------------------------
//      static void parseSection(Section& sec, const char *line, int indentation, Reader& reader)
//  --------------------------------------------------------------------------------------------------
static void parseSection(Section& sec, const char *line, int indentation, Reader& reader) {
    string paragraph          = line;
    int    lines_in_paragraph = 1;

    while (1) {
        line = reader.getNext();
        if (!line) break;
        if (isEmptyOrComment(line)) {
            pushParagraph(sec, paragraph); lines_in_paragraph = 0;
        }
        else {
            string      keyword;
            const char *rest = extractKeyword(line, keyword);

            if (rest) { // a new keyword
                reader.back();
                break;
            }

            string Line = line;
            const char *first = firstChar(line);
            if (first[0] == '-') {
                pushParagraph(sec, paragraph); lines_in_paragraph = 0;
                Line[first-line] = ' ';
            }

            if (paragraph.length()) {
                paragraph = paragraph+"\n"+Line;
            }
            else {
                paragraph = string("\n")+Line;
            }
            lines_in_paragraph++;
        }
    }

    pushParagraph(sec, paragraph);

    if (sec.size()>0) {
        string spaces;
        spaces.reserve(indentation);
        spaces.append(indentation, ' ');

        Section::iterator p = sec.begin();

        *p = string("\n")+spaces+*p;
//         advance(p, sec.size()-1);
//         *p = *p+string("\n");
    }
}
//  ------------------------------------------------------------
//      inline string cutoff_hlp_extension(const string& s)
//  ------------------------------------------------------------
inline string cutoff_hlp_extension(const string& s) {
    // cuts off the '.hlp'
    size_t pos   = s.find(".hlp");
    if ((pos+4) != s.length()) {
        pos          = s.find(".ps");
        if ((pos+3) == s.length()) return s; // accept .ps
        throw string("Expected extension .hlp");
    }
    return string(s, 0, s.length()-4);
}
// -----------------------------------------------------------------------------------------------
//      static void check_duplicates(const string& link, const char *where, Strings existing)
// -----------------------------------------------------------------------------------------------
inline void check_duplicates(const string& link, const char *where, Strings existing) {
    for (Strings::const_iterator ex = existing.begin(); ex != existing.end(); ++ex) {
        if (*ex == link) throw strf("%s-Link used again (duplicated link)", where);
    }
}
// -----------------------------------------------------------------------------------------------
//      static void check_duplicates(const string& link, Strings uplinks, Strings references)
// -----------------------------------------------------------------------------------------------
inline void check_duplicates(const string& link, Strings uplinks, Strings references) {
    check_duplicates(link, "UP", uplinks);
    check_duplicates(link, "SUB", references);
}

//  ---------------------------------------------
//      void Helpfile::readHelp(istream& in)
//  ---------------------------------------------
void Helpfile::readHelp(istream& in, const string& filename) {
    Reader      read(in);
    const char *line;
    const char *name_only = strrchr(filename.c_str(), '/');

    h2x_assert(name_only);
    ++name_only;

    try {
        while (1) {
            line = read.getNext();
            if (!line) break;

//             X(); D(line);

            if (isEmptyOrComment(line)) {
                continue;
            }

//             cout << "line='" << line << "'\n";

            string      keyword;
            const char *rest = extractKeyword(line, keyword);

//             X(); D(rest); D(keyword.c_str());

            if (rest) {         // found a keyword
                if (keyword == "UP") {
                    rest = eatWhite(rest);
                    if (strlen(rest)) {
                        string rest_noext = cutoff_hlp_extension(rest);

                        check_duplicates(rest_noext, uplinks, references);
                        if (strcmp(name_only, rest) == 0) throw "UP link to self";

                        uplinks.push_back(rest_noext);
                    }
                }
                else if (keyword == "SUB") {
                    rest = eatWhite(rest);
                    if (strlen(rest)) {
                        string rest_noext = cutoff_hlp_extension(rest);

                        check_duplicates(rest_noext, uplinks, references);
                        if (strcmp(name_only, rest) == 0) throw "SUB link to self";

                        references.push_back(rest_noext);
                    }
                }
                else if (keyword == "TITLE") {
                    rest = eatWhite(rest);
                    parseSection(title, rest, rest-line, read);

                    if (title.empty()) throw "empty TITLE not allowed";

                    const char *t = title.front().c_str();

                    if (strstr(t, "Standard help file form") != 0) {
                        throw strf("Illegal title for help file: '%s'", t);
                    }
                }
                else {
                    if (keyword == "NOTE") keyword      = "NOTES";
                    if (keyword == "EXAMPLE") keyword   = "EXAMPLES";
                    if (keyword == "OCCURENCE") keyword = "OCCURRENCE";
                    if (keyword == "WARNING") keyword   = "WARNINGS";

                    int idx;
                    for (idx = 0; knownSections[idx]; ++idx) {
                        if (knownSections[idx] == keyword) {
                            break;
                        }
                    }

                    if (knownSections[idx]) {
                        if (keyword == "SECTION") {
                            string      section_name = eatWhite(rest);
                            Section     sec;
                            parseSection(sec, "", 0, read);
                            sections.push_back(NamedSection(section_name, sec));
                        }
                        else {
                            Section sec;
                            rest = eatWhite(rest);
                            parseSection(sec, rest, rest-line, read);
                            sections.push_back(NamedSection(keyword, sec));
                        }
                    }
                    else {
                        throw strf("unknown keyword '%s'", keyword.c_str());
                    }
                }
            }
            else {
                throw strf("Unhandled line");
            }
        }
    }
    catch (string& err) { throw strf("%s:%i: %s", filename.c_str(), read.getLineNo(), err.c_str()); }
    catch (const char *err) { throw strf("%s:%i: %s", filename.c_str(), read.getLineNo(), err); }
}
//  -------------------------------------------------------------------------
//      static bool shouldReflow(const string& s, int& foundIndentation)
//  -------------------------------------------------------------------------
static bool shouldReflow(const string& s, int& foundIndentation) {
    // foundIndentation is only valid if shouldReflow() returns true
    enum { START, CHAR, SPACE, MULTIPLE, DOT, DOTSPACE } state = START;
    bool equal_indent = true;
    int  lastIndent   = -1;
    int  thisIndent   = 0;

    for (string::const_iterator c = s.begin(); c != s.end(); ++c, ++thisIndent) {
        if (*c == '\n') {
            state      = START;
            thisIndent = 0;
        }
        else if (isWhite(*c)) {
            if (state == DOT || state == DOTSPACE)  state = DOTSPACE; // multiple spaces after DOT are allowed
            else if (state == SPACE)                state = MULTIPLE; // now seen multiple spaces
            else if (state == CHAR)                 state = SPACE; // now seen 1 space
        }
        else {
            if (state == MULTIPLE) return false; // character after multiple spaces
            if (state == START) {
                if (lastIndent == -1) lastIndent                = thisIndent;
                else if (lastIndent != thisIndent) equal_indent = false;
            }
            if (*c == '.' || *c == ',')  state = DOT;
            else state                      = CHAR;
        }
    }

    if (equal_indent) {
        foundIndentation = lastIndent-1;
        h2x_assert(lastIndent >= 0);
    }
    return equal_indent;
}

//  -----------------------------------------------------------------------------------
//      static bool startsWithNumber(string& s, int& number, bool do_erase = true)
//  -----------------------------------------------------------------------------------
static bool startsWithNumber(string& s, int& number, bool do_erase = true) {
    // tests if first line starts with 'number.'
    // if true then the number is removed

    size_t off = s.find_first_not_of(" \n");
    if (off == string::npos) return false;
    if (!isdigit(s[off])) return false;

    size_t num_start = off;
    number           = 0;

    for (; isdigit(s[off]); ++off) {
        number = number*10 + (s[off]-'0');
    }

    if (s[off] != '.') return false;
    if (s[off+1] != ' ') return false;

    if (do_erase) {
        // remove 'number.' from string :
        for (size_t erase = num_start; erase <= off; ++erase) {
            s[erase] = ' ';
        }
    }

    return true;
}

//  ----------------------------------------------------------
//      static int get_first_indentation(const string& s)
//  ----------------------------------------------------------
static int get_first_indentation(const string& s) {
    // returns the indentation of the first line containing other than spaces
    size_t text_start   = s.find_first_not_of(" \n");
    size_t prev_lineend = s.find_last_of('\n', text_start);

    if (prev_lineend == string::npos) return text_start;
    return text_start-prev_lineend-1;
}

//  --------------------------------------------------------------------
//      static string correctSpaces(const string& text, int change)
//  --------------------------------------------------------------------
static string correctSpaces(const string& text, int change) {
    h2x_assert(text.find('\n') == string::npos);

    if (!change) return text;

    size_t first = text.find_first_not_of(' ');
    if (first == string::npos) return ""; // empty line

    if (change<0) {
        int remove = -change;
        h2x_assert(remove <= int(first));
        return text.substr(remove);
    }

    h2x_assert(change>0);           // add spaces
    return string(change, ' ')+text;
}

//  -------------------------------------------------------------------------
//      static string correctIndentation(const string& text, int change)
//  -------------------------------------------------------------------------
static string correctIndentation(const string& text, int change) {
    // removes 'remove' spaces from evry line

    size_t this_lineend = text.find('\n');
    string result;

    if (this_lineend == string::npos) {
        result = correctSpaces(text, change);
    }
    else {
        result = correctSpaces(text.substr(0, this_lineend), change);

        while (this_lineend != string::npos) {
            size_t next_lineend = text.find('\n', this_lineend+1);
            if (next_lineend == string::npos) { // last line
                result = result+"\n"+correctSpaces(text.substr(this_lineend+1), change);
            }
            else {
                result = result+"\n"+correctSpaces(text.substr(this_lineend+1, next_lineend-this_lineend-1), change);
            }
            this_lineend = next_lineend;
        }
    }
    return result;
}

//  ------------------------------------------------------
//      inline size_t countSpaces(const string& text)
//  ------------------------------------------------------
inline size_t countSpaces(const string& text) {
    size_t first = text.find_first_not_of(' ');
    if (first == string::npos) return INT_MAX; // empty line
    return first;
}

//  -------------------------------------------------------------
//      static size_t scanMinIndentation(const string& text)
//  -------------------------------------------------------------
static size_t scanMinIndentation(const string& text) {
    size_t this_lineend = text.find('\n');
    size_t min_indent   = INT_MAX;

    if (this_lineend == string::npos) {
        min_indent = countSpaces(text);
    }
    else {
        while (this_lineend != string::npos) {
            size_t next_lineend = text.find('\n', this_lineend+1);
            if (next_lineend == string::npos) {
                min_indent = min(min_indent, countSpaces(text.substr(this_lineend+1)));
            }
            else {
                min_indent = min(min_indent, countSpaces(text.substr(this_lineend+1, next_lineend-this_lineend-1)));
            }
            this_lineend = next_lineend;
        }
    }

    if (min_indent == INT_MAX) min_indent = 0; // only empty lines
    return min_indent;
}

//  ----------------------------
//      class ParagraphTree
//  ----------------------------
class ParagraphTree {
private:
    ParagraphTree *brother;     // has same indentation as this
    ParagraphTree *son;         // indentation + 1

    bool is_enumerated;         // 1., 2.,  usw.
    int  enumeration;           // the value of the enumeration (undefined if !is_enumerated)

    bool reflow;                // should the paragraph be reflown ? (true if indentation is equal for all lines of text)
    int  indentation;           // the real indentation of the black (after enumeration was removed)

    string text;                // text of the Section (containing linefeeds)

    friend ParagraphTree* buildParagraphTree(Section::const_iterator begin, const Section::const_iterator end);

    ParagraphTree(Section::const_iterator begin, const Section::const_iterator end) {
        h2x_assert(begin != end);

        text = *begin;
        son  = 0;

        enumeration   = 0;
        is_enumerated = startsWithNumber(text, enumeration);

        if (is_enumerated) {
            size_t text_start     = text.find_first_not_of(" \n");
            size_t next_linestart = text.find('\n', text_start);

            if (next_linestart != string::npos) {
                // more than one line -> set indent of 1st line to indent of 2nd line :
                ++next_linestart; // point behind \n
                size_t ind = get_first_indentation(text.substr(next_linestart));
                text       = string("\n")+string(ind, ' ') + text.substr(text_start);
            }
        }

        indentation = 0;
        reflow      = shouldReflow(text, indentation);

        if (!reflow) {
            size_t reststart = text.find('\n', 1);
            if (reststart != string::npos) {
                int    rest_indent;
                string rest        = text.substr(reststart);
                bool   rest_reflow = shouldReflow(rest, rest_indent);

                if (rest_reflow) {
                    int    first_indent = countSpaces(text.substr(1));
                    size_t last         = text.find_last_not_of(' ', reststart-1);
                    bool   is_header    = last != string::npos && text[last] == ':';

                    if (!is_header && rest_indent == (first_indent+8)) {
#if defined(DEBUG)
                        size_t textstart = text.find_first_not_of(" \n");
                        h2x_assert(textstart != string::npos);
#endif // DEBUG

                        // text = string("\n")+string(rest_indent, ' ')
                        // + text.substr(textstart, reststart-textstart)+rest;

                        text   = text.substr(0, reststart)+correctIndentation(rest, -8);
                        reflow = shouldReflow(text, indentation);
                    }
//                     else {
//                         char buffer[100];
//                         sprintf(buffer, "rest_indent=%i first_indent=%i ", rest_indent, first_indent);
//                         text = string("\n")+string(first_indent, ' ')+buffer+text;
//                     }
                }
            }
        }

        if (!reflow) {
            indentation = scanMinIndentation(text);
        }

        text    = correctIndentation(text, -indentation);
        brother = buildParagraphTree(++begin, end);
    }

public:
    virtual ~ParagraphTree() {
        delete brother;
        delete son;
    }

    size_t countTextNodes() {
        size_t nodes        = 1; // this
        if (son) nodes     += son->countTextNodes();
        if (brother) nodes += brother->countTextNodes();
        return nodes;
    }

    void dump(ostream& out) {
        out << "text='" << text << "'\n";
        out << "is_enumerated='" << is_enumerated << "'";
        out << "enumeration='" << enumeration << "'";
        out << "reflow='" << reflow << "'";
        out << "indentation='" << indentation << "'\n";

        if (brother) { cout << "\nbrother:\n"; brother->dump(out); cout << "\n"; }
        if (son) { cout << "\nson:\n"; son->dump(out); cout << "\n"; }
    }

    static ParagraphTree* buildParagraphTree(Section::const_iterator begin, const Section::const_iterator end) {
        if (begin == end) return 0;
        return new ParagraphTree(begin, end);
    }
    static ParagraphTree* buildParagraphTree(const NamedSection& N) {
        ParagraphTree  *ptree = 0;
        const Section&  S     = N.getSection();
        if (S.empty()) throw string("Tried to build an empty ParagraphTree (Section=")+N.getName()+")";
        else ptree = buildParagraphTree(S.begin(), S.end());
        return ptree;
    }

    bool contains(ParagraphTree *that) {
        return
            this == that ||
            (son && son->contains(that)) ||
            (brother && brother->contains(that));
    }

    ParagraphTree *predeccessor(ParagraphTree *before_this) {
        if (brother == before_this) return this;
        if (!brother) return 0;
        return brother->predeccessor(before_this);
    }

    void append(ParagraphTree *new_brother) {
        if (!brother) brother = new_brother;
        else brother->append(new_brother);
    }

    ParagraphTree* removeTill(ParagraphTree *after) {
        ParagraphTree *removed    = this;
        ParagraphTree *after_pred = this;

        while (1) {
            h2x_assert(after_pred);
            h2x_assert(after_pred->brother); // removeTill called with non-existing 'after'

            if (after_pred->brother == after) { // found after
                after_pred->brother = 0; // unlink
                break;
            }
            after_pred = after_pred->brother;
        }

        return removed;
    }

    ParagraphTree *firstEnumerated() {
        if (is_enumerated) return this;
        if (brother) return brother->firstEnumerated();
        return 0;
    }
    ParagraphTree *nextEnumerated() {
        h2x_assert(is_enumerated);
        if (brother) {
            ParagraphTree *next = brother->firstEnumerated();
            if (next && next->enumeration == (enumeration+1)) {
                return next;
            }
        }
        return 0;
    }

    ParagraphTree* firstWithSameOrLowerIndent(int wanted_indentation) {
        if (indentation <= wanted_indentation) return this;
        if (!brother) return 0;
        return brother->firstWithSameOrLowerIndent(wanted_indentation);
    }

    ParagraphTree* format_indentations();
    ParagraphTree* format_enums();
private:
    static ParagraphTree* buildNewParagraph(const string& Text) {
        Section S;
        S.push_back(Text);
        return new ParagraphTree(S.begin(), S.end());
    }
    ParagraphTree *extractEmbeddedEnum(int lookfor) {
        size_t this_lineend = text.find('\n');

        while (this_lineend != string::npos) {
            int    number;
            string embedded = text.substr(this_lineend);
            if (startsWithNumber(embedded, number, false)) {
                if (number == lookfor) {
                    text.erase(this_lineend);
                    return buildNewParagraph(embedded);
                }
                break;
            }
            this_lineend = text.find('\n', this_lineend+1);
        }

        return 0;
    }
public:
    static size_t embeddedCounter;

    void xml_write(bool ignore_enumerated = false, bool write_as_entry = false);
};

size_t ParagraphTree::embeddedCounter = 0;

//  -----------------------------------------------------
//      ParagraphTree* ParagraphTree::format_enums()
//  -----------------------------------------------------
ParagraphTree* ParagraphTree::format_enums() {
    // reformats tree such that all enums are brothers
    ParagraphTree *enum_this = firstEnumerated();

    if (enum_this) {        // we have enumeration
        ParagraphTree *before_enum = predeccessor(enum_this);

        if (before_enum) {
            h2x_assert(before_enum->son == 0);
            before_enum->son     = before_enum->brother;
            before_enum->brother = 0;
        }

        for (ParagraphTree *enum_next = enum_this->nextEnumerated();
             enum_next;
             enum_this = enum_next, enum_next = enum_this->nextEnumerated())
        {
            if (enum_next != enum_this->brother) {
                h2x_assert(enum_this->son == 0);
                enum_this->son     = enum_this->brother->removeTill(enum_next);
                enum_this->brother = enum_next;
            }
        }

        // enum_this is the last enumeration
        h2x_assert(!enum_this->son);

        if (enum_this->brother) { // there are more sections behind enum
            ParagraphTree *after_enum = enum_this->firstWithSameOrLowerIndent(enum_this->indentation-1);

            if (after_enum) { // indent should go back after enum
                h2x_assert(!enum_this->son);

                if (after_enum != enum_this->brother) {
                    enum_this->son = enum_this->brother->removeTill(after_enum);
                }
                enum_this->brother = 0;

                h2x_assert(before_enum);
                h2x_assert(before_enum->brother == 0);
                before_enum->brother = after_enum->format_enums();
            }
            else { // nothing after enum -> take all as children
                h2x_assert(enum_this->son == 0);
                enum_this->son     = enum_this->brother;
                enum_this->brother = 0;
            }
            if (enum_this->son) enum_this->son = enum_this->son->format_enums();
        }
        else {
            if (before_enum) {
                if (before_enum->son) before_enum->son->append(before_enum->brother);
                before_enum->brother = 0;
            }
        }

        if (enum_this->enumeration == 1) { // oops - only '1.' search for enum inside block
            ParagraphTree *lookin = enum_this;
            int            lookfor;

            for ( lookfor = 2; ; ++lookfor) {
                ParagraphTree *next_enum = lookin->extractEmbeddedEnum(lookfor);
                if (!next_enum) break;

                embeddedCounter++;
                next_enum->brother = lookin->brother;
                lookin->brother    = next_enum;
                lookin             = next_enum;
            }
        }

        return this;
    }

    // no enumerations found
    return this;
}

//  ------------------------------------------------------------
//      ParagraphTree* ParagraphTree::format_indentations()
//  ------------------------------------------------------------
ParagraphTree* ParagraphTree::format_indentations() {
    if (brother) {
        if (is_enumerated)  {
            if (brother) brother = brother->format_indentations();
            if (son) son = son->format_indentations();
        }
        else {
            ParagraphTree *same_indent = brother->firstWithSameOrLowerIndent(indentation);
            if (same_indent) {
                if (same_indent == brother) {
                    brother     = brother->format_indentations();
                    if(son) son = son->format_indentations();
                }
                else {
                    ParagraphTree *children = brother->removeTill(same_indent);
                    brother                 = same_indent->format_indentations();
                    h2x_assert(!son);
                    son                     = children->format_indentations();
                }
            }
            else { // none with same indent
                h2x_assert(!son);
                son     = brother->format_indentations();
                brother = 0;
            }
        }
    }
    else { // no brother
        if (son) son = son->format_indentations();
    }
    return this;
}

static void print_XML_Text_expanding_links(const string& text) {
    size_t found = text.find("LINK{", 0);
    if (found != string::npos) {
        size_t inside_link = found+5;
        size_t close = text.find('}', inside_link);

        if (close != string::npos) {
            string link_target = text.substr(inside_link, close-inside_link);
            string type        = "unknown";
            string dest        = link_target;

            if (link_target.find("http://") != string::npos) { type = "www"; }
            else if (link_target.find("ftp://") != string::npos) { type = "www"; }
            else if (link_target.find("file://") != string::npos) { type = "www"; }
            else if (link_target.find('@') != string::npos) { type = "email"; }
            else {
                type = "help";
                dest = cutoff_hlp_extension(link_target);
            }

            {
                XML_Text t(text.substr(0, found));
            }
            {
                XML_Tag link("LINK");
                link.set_on_extra_line(false);
                link.add_attribute("type", type);
                link.add_attribute("dest", dest);
            }

            return print_XML_Text_expanding_links(text.substr(close+1));
        }
    }

    XML_Text t(text);
}

//  -----------------------------------------------------------------------------------
//      void ParagraphTree::xml_write(bool ignore_enumerated, bool write_as_entry)
//  -----------------------------------------------------------------------------------
void ParagraphTree::xml_write(bool ignore_enumerated, bool write_as_entry) {
    if (is_enumerated && !ignore_enumerated) {
        XML_Tag e("ENUM");
        xml_write(true);
    }
    else {
        {
            XML_Tag *para = 0;
//             char     buffer[100];

            if (is_enumerated || write_as_entry) {
                para = new XML_Tag("ENTRY");
            }
            else {
                para = new XML_Tag("P");
            }

//             sprintf(buffer, "%i", indentation);
//             para->add_attribute("indentation", buffer);

//             if (is_enumerated) {
//                 sprintf(buffer, "%i", enumeration);
//                 para->add_attribute("enumerated", buffer);
//             }

            {
                XML_Tag textblock("T");
                textblock.add_attribute("reflow", reflow ? "1" : "0");
                {
                    string usedText;
                    if (reflow)  {
                        usedText = correctIndentation(text, (textblock.Indent()+1) * the_XML_Document->indentation_per_level);
                    }
                    else {
                        usedText = text;
                    }
//                     XML_Text t(usedText.substr(1)); // skip first char (\n)

                    print_XML_Text_expanding_links(usedText);

//                     XML_Text t(usedText);
                }
            }
            if (son) {
                if (!son->is_enumerated && son->brother) {
                    XML_Tag sontag("LIST");
                    son->xml_write(false, true);
                }
                else {
                    son->xml_write(false);
                }
            }

            delete para;
        }
        if (brother) brother->xml_write(ignore_enumerated, write_as_entry);
    }
}

// --------------------------------------------------------------------------------------------------------------------------
//      static void addMissingAttribute(XML_Tag& link, const string& basename, const string& path1, const string& path2)
// --------------------------------------------------------------------------------------------------------------------------
static void addMissingAttribute(XML_Tag& link, const string& basename, const string& path1, const string& path2) {
    struct stat st;
    string      fullname1 = path1+basename+".hlp";
    string      fullname2 = path2+basename+".hlp";

    if (stat(fullname1.c_str(), &st) == -1 &&
        stat(fullname2.c_str(), &st) == -1)
    {
        link.add_attribute("missing", "1");
    }
}

// ---------------------------------------------------------------------------------------------------------------
//      void Helpfile::writeXML(FILE *out, const string& page_name, const string& path1, const string& path2)
// ---------------------------------------------------------------------------------------------------------------
void Helpfile::writeXML(FILE *out, const string& page_name, const string& path1, const string& path2) {
#if defined(DUMP_DATA)
    display(uplinks, "Uplinks", stdout);
    display(references, "References", stdout);
    display(title, "Title", stdout);
    display(sections, "Sections", stdout);
#endif // DUMP_DATA

    XML_Document xml("PAGE", "arb_help.dtd", out);
    xml.skip_empty_tags       = true;
    xml.indentation_per_level = 2;
    xml.getRoot().add_attribute("name", page_name);
#if defined(DEBUG)
    xml.getRoot().add_attribute("edit_warning", "devel"); // inserts a edit warning into development version
#else
    xml.getRoot().add_attribute("edit_warning", "release"); // inserts a different edit warning into release version
#endif // DEBUG

    for (Strings::const_iterator s = uplinks.begin(); s != uplinks.end(); ++s) {
        XML_Tag uplink("UP");
        uplink.add_attribute("dest", *s);
        addMissingAttribute(uplink, *s, path1, path2);
    }
    for (Strings::const_iterator s = references.begin(); s != references.end(); ++s) {
        XML_Tag sublink("SUB");
        sublink.add_attribute("dest", *s);
        addMissingAttribute(sublink, *s, path1, path2);
    }
    {
        XML_Tag title_tag("TITLE");
        for (Section::const_iterator s = title.begin(); s != title.end(); ++s) {
            if (s != title.begin()) { XML_Text text("\n"); }
            XML_Text text(*s);
        }
    }

    for (NamedSections::const_iterator named_sec = sections.begin(); named_sec != sections.end(); ++named_sec) {
        XML_Tag section_tag("SECTION");
        section_tag.add_attribute("name", named_sec->getName());

        ParagraphTree *ptree = ParagraphTree::buildParagraphTree(*named_sec);

        ParagraphTree::embeddedCounter = 0;

#if defined(DEBUG)
        size_t textnodes = ptree->countTextNodes();
#endif // DEBUG

        ptree = ptree->format_enums();

#if defined(DEBUG)
        size_t textnodes2 = ptree->countTextNodes();
        h2x_assert(textnodes2 == (textnodes+ParagraphTree::embeddedCounter)); // if this occurs format_enums has an error
#endif // DEBUG

        ptree = ptree->format_indentations();

#if defined(DEBUG)
        size_t textnodes3 = ptree->countTextNodes();
        h2x_assert(textnodes3 == textnodes2); // if this occurs format_indentations has an error
#endif // DEBUG

        ptree->xml_write();

        delete ptree;
    }
}
//  ------------------------------------------
//      void Helpfile::extractInternalLinks()
//  ------------------------------------------
void Helpfile::extractInternalLinks() {
    for (NamedSections::const_iterator named_sec = sections.begin(); named_sec != sections.end(); ++named_sec) {
        try {
            const Section& s = named_sec->getSection();
            for (Section::const_iterator li = s.begin(); li != s.end(); ++li) {
                const string& line = *li;
                size_t        start = 0;

                while (1) {
                    size_t found = line.find("LINK{", start);
                    if (found == string::npos) break;
                    found += 5;
                    size_t close = line.find('}', found);
                    if (close == string::npos) break;

                    string link_target = line.substr(found, close-found);

                    if (link_target.find("http://") == string::npos &&
                        link_target.find("ftp://")  == string::npos &&
                        link_target.find("file://") == string::npos &&
                        link_target.find('@')       == string::npos)
                    {
                        string rest_noext = cutoff_hlp_extension(link_target);

                        try {
                            check_duplicates(rest_noext, uplinks, references);
                            references.push_back(rest_noext);
                        }
                        catch (string& err) {
                            cout << "Duplicated reference ingnored (" << err << ")\n";
                        }
                    }
                    // printf("link_target='%s'\n", link_target.c_str());

                    start = close+1;
                }
            }
        }
        catch (string& err) {
            throw string("'"+err+"' while scanning LINK{} in SECTION '"+named_sec->getName()+'\'');
        }
    }
}
// -------------------------------------------------------------------
//      static void show_err(string err, const string& helpfile )
// -------------------------------------------------------------------
static void show_err(const string& err, const string& helpfile ) {
    if (err.find(helpfile+':') != string::npos) {
        cerr << err;
    }
    else {
        cerr << helpfile << ":1: " << err;
    }
    cerr << '\n';
}



//  -----------------------------------------
//      int main(int argc, char *argv[])
//  -----------------------------------------
int main(int argc, char *argv[]) {
    Helpfile help;
    string   arb_help;

    try {
        if (argc != 3) {
            cerr << "Usage: arb_help2xml <ARB helpfile> <XML output>\n";
            return EXIT_FAILURE;
        }

        arb_help          = argv[1];
        string xml_output = argv[2];

        {
            ifstream in(arb_help.c_str());
            help.readHelp(in, arb_help);
        }

        help.extractInternalLinks();

        {
            FILE *out = std::fopen(xml_output.c_str(), "wt");
            if (!out) throw string("Can't open '")+xml_output+'\'';

            try {
                help.writeXML(out, cutoff_hlp_extension(string(arb_help, 8)), "oldhelp/", "genhelp/"); // cut off 'oldhelp/' and '.hlp'
                fclose(out);
            }
            catch (...) {
                fclose(out);
                remove(xml_output.c_str());
                throw;
            }
        }

        return EXIT_SUCCESS;
    }
    catch (string& err)         { show_err(err, arb_help); }
    catch (const char * err)    { show_err(err, arb_help); }
    catch (...)                 { cerr << "unknown exception (arb_help2xml)\n"; }

    return EXIT_FAILURE;
}



//  ==================================================================== //
//                                                                       //
//    File      : arb_help2xml.cxx                                       //
//    Purpose   : Converts old ARB help format to XML                    //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in October 2001          //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//  ==================================================================== //

#include <xml.hxx>
#include <arb_defs.h>
#include <arb_diff.h>

#include <list>
#include <set>
#include <iostream>
#include <fstream>

#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <climits>

#include <unistd.h>
#include <sys/stat.h>

using namespace std;

#define h2x_assert(bed) arb_assert(bed)

#if defined(DEBUG)
#define WARN_FORMATTING_PROBLEMS
// #define WARN_MISSING_HELP // @@@ reenable later
// #define DUMP_PARAGRAPHS
#endif // DEBUG


#if defined(WARN_FORMATTING_PROBLEMS)
// #define WARN_POSSIBLY_WRONG_INDENTATION_CORRECTION
#define WARN_FIXED_LAYOUT_LIST_ELEMENTS
// #define WARN_LONESOME_LIST_ELEMENTS
#endif


#define MAX_LINE_LENGTH 200     // maximum length of lines in input stream
#define TABSIZE         8

static const char *knownSections[] = { "OCCURRENCE", "DESCRIPTION", "NOTES", "EXAMPLES", "WARNINGS", "BUGS",
                                       "QUESTION", "ANSWER", "SECTION",
                                       0 };

enum SectionType {
    SEC_OCCURRENCE,
    SEC_DESCRIPTION,
    SEC_NOTES,
    SEC_EXAMPLES,
    SEC_WARNINGS,
    SEC_BUGS,
    SEC_QUESTION,
    SEC_ANSWER,
    SEC_SECTION,

    SEC_NONE,

    SEC_FAKE, 
};


__ATTR__VFORMAT(1) static string vstrf(const char *format, va_list argPtr) {
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
        delete [] buffer;
        buffer    = new char[buf_size];
    }

    return string(buffer, length);
}

__ATTR__FORMAT(1) static string strf(const char *format, ...) {
    va_list argPtr;
    va_start(argPtr, format);
    string result = vstrf(format, argPtr);
    va_end(argPtr);

    return result;
}

// -----------------------------
//      warnings and errors

class LineAttachedMessage {
    string message;
    size_t lineno;

public:
    LineAttachedMessage(const string& message_, size_t lineno_)
        : message(message_)
        , lineno(lineno_)
    {}

    const string& Message() const { return message; }
    size_t Lineno() const { return lineno; }
};

const size_t NO_LINENUMBER_INFO = -1U;

LineAttachedMessage unattached_message(const string& message) { return LineAttachedMessage(message, NO_LINENUMBER_INFO); }


static list<LineAttachedMessage> warnings;
inline void add_warning(const LineAttachedMessage& laMsg) {
    warnings.push_back(laMsg);
}
inline void add_warning(const string& warning, size_t lineno) {
    add_warning(LineAttachedMessage(warning, lineno));
}

struct MessageAttachable {
    virtual ~MessageAttachable() {}

    virtual string location_description() const = 0; // may return empty string
    virtual size_t line_number() const          = 0;

    LineAttachedMessage attached_message(const string& message) const {
        string where = location_description();
        if (where.empty()) return LineAttachedMessage(message, line_number());
        return LineAttachedMessage(message+" ["+where+"]", line_number());
    }
    void attach_warning(const string& message) const {
        add_warning(attached_message(message));
    }
};


// ----------------------
//      class Reader

class Reader : public MessageAttachable {
private:
    istream& in;
    char     lineBuffer[MAX_LINE_LENGTH];
    char     lineBuffer2[MAX_LINE_LENGTH];
    bool     readAgain;
    bool     eof;
    int      lineNo;

    string location_description() const OVERRIDE { return ""; }
    size_t line_number() const OVERRIDE { return lineNo; }

    void getline() {
        if (!eof) {
            if (in.eof()) eof = true;
            else {
                h2x_assert(in.good());

                in.getline(lineBuffer, MAX_LINE_LENGTH);
                lineNo++;

                if (in.eof()) eof = true;
                else if (in.fail()) throw "line too long";

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

                char *eol = strchr(lineBuffer, 0)-1;
                while (eol >= lineBuffer && isspace(eol[0])) {
                    eol[0] = 0; // trim trailing whitespace
                    eol--;
                }
                if (eol > lineBuffer) {
                    // now eol points to last character
                    if (eol[0] == '-' && isalnum(eol[-1])) {
                        attach_warning("manual hyphenation detected");
                    }
                }
            }
        }
    }

public:
    Reader(istream& in_) : in(in_),  readAgain(true),  eof(false), lineNo(0) { getline(); }
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

enum ParagraphType {
    PLAIN_TEXT,
    ENUMERATED,
    ITEM,
};

class Ostring {
    string        content;
    size_t        lineNo; // where string came from
    ParagraphType type;
    unsigned      number; // only valid if type==ENUMERATED

public:

    Ostring(const string& s, size_t line_no, ParagraphType type_)
        : content(s),
          lineNo(line_no),
          type(type_)
    {
        h2x_assert(type != ENUMERATED);
    }
    Ostring(const string& s, size_t line_no, ParagraphType type_, unsigned num)
        : content(s),
          lineNo(line_no),
          type(type_),
          number(num)
    {
        h2x_assert(type == ENUMERATED);
        h2x_assert(num>0);
    }


    operator const string&() const { return content; }
    operator string&() { return content; }

    const string& as_string() const { return content; }
    string& as_string() { return content; }

    size_t get_lineno() const { return lineNo; }

    const ParagraphType& get_type() const { return type; }
    unsigned get_number() const {
        h2x_assert(type == ENUMERATED);
        return number;
    }

    // some wrapper to make Ostring act like string
    const char *c_str() const { return content.c_str(); }
};

typedef list<Ostring> Ostrings;

class Section {
    SectionType type;
    Ostrings    content;

public:
    Section(SectionType type_) : type(type_) {}

    const Ostrings& Content() const { return content; }
    Ostrings& Content() { return content; }

    SectionType get_type() const { return type; }

    size_t StartLineno() const { return content.front().get_lineno(); }
};


#if defined(WARN_MISSING_HELP)
static void check_TODO(const char *line, const Reader& reader) {
    if (strstr(line, "@@@") != NULL || strstr(line, "TODO") != NULL) {
        reader.attach_warning(strf("TODO: %s", line));
    }
}
#else
inline void check_TODO(const char *, const Reader&) { }
#endif // WARN_MISSING_HELP

// ----------------------------
//      class NamedSection

class NamedSection : public MessageAttachable {
private:
    string  name;
    Section section;

    string location_description() const OVERRIDE { return string("in SECTION '")+name+"'"; }
    size_t line_number() const OVERRIDE { return section.StartLineno(); }

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

class Link {
    string target;
    size_t source_lineno;

public:
    Link(const string& target_, size_t source_lineno_)
        : target(target_)
        , source_lineno(source_lineno_)
    {}

    const string& Target() const { return target; }
    size_t SourceLineno() const { return source_lineno; }
};

typedef list<Link> Links;

// ------------------------
//      class Helpfile

class Helpfile {
private:
    Links         uplinks;
    Links         references;
    Links         auto_references;
    Section       title;
    NamedSections sections;
    string        inputfile;

public:
    Helpfile() : title(SEC_FAKE) {}
    virtual ~Helpfile() {}

    void readHelp(istream& in, const string& filename);
    void writeXML(FILE *out, const string& page_name);
    void extractInternalLinks();

    const Section& get_title() const { return title; }
};

inline bool isWhite(char c) { return c == ' '; }

inline bool isEmptyOrComment(const char *s) {
    if (s[0] == '#') return true;
    for (int off = 0; ; ++off) {
        if (s[off] == 0) return true;
        if (!isWhite(s[off])) break;
    }

    return false;
}

inline const char *extractKeyword(const char *line, string& keyword) {
    // returns NULL if no keyword was found
    // otherwise returns position behind keyword and sets value of 'keyword'

    const char *space = strchr(line, ' ');
    if (space && space>line) {
        keyword = string(line, 0, space-line);
        return space;
    }
    else if (!space) { // test for keyword w/o content behind
        if (line[0]) { // not empty
            keyword = line;
            return strchr(line, 0);
        }
    }
    return 0;
}

inline const char *eatWhite(const char *line) {
    // skips whitespace
    while (isWhite(*line)) ++line;
    return line;
}

inline void pushParagraph(Section& sec, string& paragraph, size_t lineNo, ParagraphType& type, unsigned num) {
    if (paragraph.length()) {
        if (type == ENUMERATED) {
            sec.Content().push_back(Ostring(paragraph, lineNo, type, num));
        }
        else {
            sec.Content().push_back(Ostring(paragraph, lineNo, type));
        }
        type      = PLAIN_TEXT;
        paragraph = "";
    }
}

inline const char *firstChar(const char *s) {
    while (isWhite(s[0])) ++s;
    return s;
}

inline bool is_startof_itemlist_element(const char *contentStart) {
    return
        (contentStart[0] == '-' ||
         contentStart[0] == '*')
        &&
        isspace(contentStart[1])
        &&
        !(isspace(contentStart[2]) ||
          contentStart[2] == '-');
}

#define MAX_ALLOWED_ENUM 99 // otherwise it start interpreting years as enums 

static bool startsWithNumber(string& s, unsigned& number, bool do_erase) {
    // tests if first line starts with 'number.'
    // if true then the number is removed from the string

    size_t off = s.find_first_not_of(" \n");
    if (off == string::npos) return false;
    if (!isdigit(s[off])) return false;

    size_t num_start = off;
    number           = 0;

    for (; isdigit(s[off]); ++off) {
        number = number*10 + (s[off]-'0');
    }
    if (number>MAX_ALLOWED_ENUM) return false;

    if (s[off] != '.' && s[off] != ')') return false;
    if (s[off+1] != ' ') return false;

    if (do_erase) {
        // remove 'number.' from string :
        ++off;
        while (s[off+1] == ' ') ++off;
        s.erase(num_start, off-num_start+1);
    }

    return true;
}

static void parseSection(Section& sec, const char *line, int indentation, Reader& reader) {
    string        paragraph         = line;
    size_t        para_start_lineno = reader.getLineNo();
    ParagraphType type              = PLAIN_TEXT;
    unsigned      num               = 0;

    h2x_assert(sec.Content().empty());

    while (1) {
        line = reader.getNext();
        if (!line) break;

        if (isEmptyOrComment(line)) {
            pushParagraph(sec, paragraph, para_start_lineno, type, num);
            check_TODO(line, reader);
        }
        else {
            string      keyword;
            const char *rest = extractKeyword(line, keyword);

            if (rest) { // a new keyword
                reader.back();
                break;
            }

            check_TODO(line, reader);

            string Line = line;

            if (sec.get_type() == SEC_OCCURRENCE) {
                pushParagraph(sec, paragraph, para_start_lineno, type, num);
            }
            else {
                const char *firstNonWhite = firstChar(line);
                if (is_startof_itemlist_element(firstNonWhite)) {
                    h2x_assert(firstNonWhite != line);

                    pushParagraph(sec, paragraph, para_start_lineno, type, num);

                    Line[firstNonWhite-line] = ' ';
                    type = ITEM; // is reset in call to pushParagraph
                }
                else {
                    unsigned foundNum;
                    if (startsWithNumber(Line, foundNum, true)) {
                        pushParagraph(sec, paragraph, para_start_lineno, type, num);

                        type = ENUMERATED;
                        num  = foundNum;

                        if (!num) {
                            throw "Enumerations starting with zero are not supported";
                        }
                    }
                }
            }

            if (paragraph.length()) {
                paragraph = paragraph+"\n"+Line;
            }
            else {
                paragraph         = string("\n")+Line;
                para_start_lineno = reader.getLineNo();
            }
        }
    }

    pushParagraph(sec, paragraph, para_start_lineno, type, num);

    if (sec.Content().size()>0 && indentation>0) {
        string spaces;
        spaces.reserve(indentation);
        spaces.append(indentation, ' ');

        string& ostr = sec.Content().front();
        ostr         = string("\n") + spaces + ostr;
    }
}
inline void check_specific_duplicates(const string& link, const Links& existing, bool add_warnings) {
    for (Links::const_iterator ex = existing.begin(); ex != existing.end(); ++ex) {
        if (ex->Target() == link) {
            if (add_warnings) add_warning(strf("First Link to '%s' was found here.", ex->Target().c_str()), ex->SourceLineno());
            throw strf("Link to '%s' duplicated here.", link.c_str());
        }
    }
}
inline void check_duplicates(const string& link, const Links& uplinks, const Links& references, bool add_warnings) {
    check_specific_duplicates(link, uplinks, add_warnings);
    check_specific_duplicates(link, references, add_warnings);
}

void Helpfile::readHelp(istream& in, const string& filename) {
    if (!in.good()) throw unattached_message(strf("Can't read from '%s'", filename.c_str()));

    Reader read(in);

    inputfile = filename; // remember file read (for comment)

    const char *line;
    const char *name_only = strrchr(filename.c_str(), '/');

    h2x_assert(name_only);
    ++name_only;

    try {
        while (1) {
            line = read.getNext();
            if (!line) break;

            if (isEmptyOrComment(line)) {
                check_TODO(line, read);
                continue;
            }

            check_TODO(line, read);

            string      keyword;
            const char *rest = extractKeyword(line, keyword);

            if (rest) {         // found a keyword
                if (keyword == "UP") {
                    rest = eatWhite(rest);
                    if (strlen(rest)) {
                        check_duplicates(rest, uplinks, references, true);
                        if (strcmp(name_only, rest) == 0) throw "UP link to self";

                        uplinks.push_back(Link(rest, read.getLineNo()));
                    }
                }
                else if (keyword == "SUB") {
                    rest = eatWhite(rest);
                    if (strlen(rest)) {
                        check_duplicates(rest, uplinks, references, true);
                        if (strcmp(name_only, rest) == 0) throw "SUB link to self";

                        references.push_back(Link(rest, read.getLineNo()));
                    }
                }
                else if (keyword == "TITLE") {
                    rest = eatWhite(rest);
                    // parseSection(title, rest, rest-line, read);
                    parseSection(title, rest, 0, read);

                    if (title.Content().empty()) throw "empty TITLE not allowed";

                    const char *t = title.Content().front().c_str();

                    if (strstr(t, "Standard help file form") != 0) {
                        throw strf("Illegal title for help file: '%s'", t);
                    }
                }
                else {
                    if (keyword == "NOTE")    keyword = "NOTES";
                    if (keyword == "EXAMPLE") keyword = "EXAMPLES";
                    if (keyword == "WARNING") keyword = "WARNINGS";

                    SectionType stype = SEC_NONE;
                    int         idx;
                    for (idx = 0; knownSections[idx]; ++idx) {
                        if (knownSections[idx] == keyword) {
                            stype = SectionType(idx);
                            break;
                        }
                    }

                    if (knownSections[idx]) {
                        if (stype == SEC_SECTION) {
                            string  section_name = eatWhite(rest);
                            Section sec(stype);

                            parseSection(sec, "", 0, read);
                            sections.push_back(NamedSection(section_name, sec));
                        }
                        else {
                            Section sec(stype);

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
    catch (string& err)     { throw read.attached_message(err); }
    catch (const char *err) { throw read.attached_message(err); }
}

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
            state = (*c == '.' || *c == ',') ? DOT : CHAR;
        }
    }

    if (lastIndent<0) {
        equal_indent = false;
    }

    if (equal_indent) {
        foundIndentation = lastIndent-1;
        h2x_assert(foundIndentation >= 0);
    }
    return equal_indent;
}

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

static string correctIndentation(const string& text, int change) {
    // removes 'remove' spaces from every line

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

inline size_t countSpaces(const string& text) {
    size_t first = text.find_first_not_of(' ');
    if (first == string::npos) return INT_MAX; // empty line
    return first;
}

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

// -----------------------------
//      class ParagraphTree

class ParagraphTree : public MessageAttachable, virtual Noncopyable {
    ParagraphTree *brother;     // has same indentation as this
    ParagraphTree *son;         // indentation + 1

    Ostring otext;              // text of the Section (containing linefeeds)

    bool reflow;                // should the paragraph be reflown ? (true if indentation is equal for all lines of text)
    int  indentation;           // the real indentation of the blank (behind removed enumeration)


    string location_description() const OVERRIDE { return "in paragraph starting here"; }
    size_t line_number() const OVERRIDE { return otext.get_lineno(); }

    ParagraphTree(Ostrings::const_iterator begin, const Ostrings::const_iterator end)
        : son(NULL),
          otext(*begin),
          indentation(0)
    {
        h2x_assert(begin != end);

        string& text = otext;

        reflow = shouldReflow(text, indentation);
        if (!reflow) {
            size_t reststart = text.find('\n', 1);

            if (reststart == 0) {
                attach_warning("[internal] Paragraph starts with LF -> reflow calculation will probably fail");
            }

            if (reststart != string::npos) {
                int    rest_indent = -1;
                string rest        = text.substr(reststart);
                bool   rest_reflow = shouldReflow(rest, rest_indent);

                if (rest_reflow) {
                    int first_indent = countSpaces(text.substr(1));
                    if (get_type() == PLAIN_TEXT) {
                        size_t last         = text.find_last_not_of(' ', reststart-1);
                        bool   is_header    = last != string::npos && text[last] == ':';

                        if (!is_header && rest_indent == (first_indent+8)) {
#if defined(DEBUG)
                            size_t textstart = text.find_first_not_of(" \n");
                            h2x_assert(textstart != string::npos);
#endif // DEBUG

                            text   = text.substr(0, reststart)+correctIndentation(rest, -8);
                            reflow = shouldReflow(text, indentation);
                        }
                    }
                    else {
                        int diff = rest_indent-first_indent;
                        if (diff>0) {
                            text   = text.substr(0, reststart)+correctIndentation(rest, -diff);
                            reflow = shouldReflow(text, indentation);
                        }
                        else if (diff<0) {
                            // paragraph with more indent on first line (occurs?)
                            attach_warning(strf("[internal] unhandled: more indentation on the 1st line (diff=%i)", diff));
                        }
                    }
                }
            }
        }

        if (!reflow) {
            indentation = scanMinIndentation(text);
        }
        text = correctIndentation(text, -indentation);
        if (get_type() == ITEM) {
            h2x_assert(indentation >= 2);
            indentation -= 2;
        }

        brother = buildParagraphTree(++begin, end);
    }

    void brothers_to_sons(ParagraphTree *new_brother);

public:
    virtual ~ParagraphTree() {
        delete brother;
        delete son;
    }

    ParagraphType get_type() const { return otext.get_type(); }

    bool is_itemlist_member() const { return get_type() == ITEM; }
    unsigned get_enumeration() const { return get_type() == ENUMERATED ? otext.get_number() : 0; }

    const char *readable_type() const {
        const char *res = NULL;
        switch (get_type()) {
            case PLAIN_TEXT: res = "PLAIN_TEXT"; break;
            case ITEM:       res = "ITEM";       break;
            case ENUMERATED: res = "ENUMERATED"; break;
        }
        return res;
    }

    size_t countTextNodes() {
        size_t nodes        = 1; // this
        if (son) nodes     += son->countTextNodes();
        if (brother) nodes += brother->countTextNodes();
        return nodes;
    }

#if defined(DUMP_PARAGRAPHS)
    void print_indent(ostream& out, int indent) { while (indent-->0) out << ' '; }
    char *masknl(const char *text) {
        char *result = strdup(text);
        for (int i = 0; result[i]; ++i) {
            if (result[i] == '\n') result[i] = '|';
        }
        return result;
    }
    void dump(ostream& out, int indent = 0) {
        print_indent(out, indent+1);
        {
            char *mtext = masknl(otext.as_string().c_str());
            out << "text='" << mtext << "'\n";
            free(mtext);
        }

        print_indent(out, indent+1);
        out << "type='" << readable_type() << "' ";
        if (get_type() == ENUMERATED) {
            out << "enumeration='" << otext.get_number() << "' ";
        }
        out << "reflow='" << reflow << "' ";
        out << "indentation='" << indentation << "'\n";

        if (son) {
            print_indent(out, indent+2); cout << "son:\n";
            son->dump(out, indent+2);
            cout << "\n";
        }
        if (brother) {
            print_indent(out, indent); cout << "brother:\n";
            brother->dump(out, indent);
        }
    }
#endif // DUMP_PARAGRAPHS

    static ParagraphTree* buildParagraphTree(Ostrings::const_iterator begin, const Ostrings::const_iterator end) {
        if (begin == end) return 0;
        return new ParagraphTree(begin, end);
    }
    static ParagraphTree* buildParagraphTree(const NamedSection& N) {
        ParagraphTree *ptree = 0;
        const Ostrings& S = N.getSection().Content();
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

    ParagraphTree *predecessor(ParagraphTree *before_this) {
        if (brother == before_this) return this;
        if (!brother) return 0;
        return brother->predecessor(before_this);
    }

    void append(ParagraphTree *new_brother) {
        if (!brother) brother = new_brother;
        else brother->append(new_brother);
    }

    bool is_some_brother(const ParagraphTree *other) const {
        return (other == brother) || (brother && brother->is_some_brother(other));
    }

    ParagraphTree* takeAllInFrontOf(ParagraphTree *after) {
        ParagraphTree *removed    = this;
        ParagraphTree *after_pred = this;

        h2x_assert(is_some_brother(after));

        while (1) {
            h2x_assert(after_pred);
            h2x_assert(after_pred->brother); // takeAllInFrontOf called with non-existing 'after'

            if (after_pred->brother == after) { // found after
                after_pred->brother = 0; // unlink
                break;
            }
            after_pred = after_pred->brother;
        }

        return removed;
    }

    ParagraphTree *firstListMember() {
        switch (get_type()) {
            case PLAIN_TEXT: break;
            case ITEM: return this;
            case ENUMERATED: {
                if (get_enumeration() == 1) return this;
                break;
            }
        }
        if (brother) return brother->firstListMember();
        return NULL;
    }

    ParagraphTree *nextListMemberAfter(const ParagraphTree& previous) {
        if (indentation<previous.indentation) return NULL;
        if (indentation == previous.indentation && get_type() == previous.get_type()) {
            if (get_type() != ENUMERATED) return this;
            if (get_enumeration() > previous.get_enumeration()) return this;
            return NULL;
        }
        if (!brother) return NULL;
        return brother->nextListMemberAfter(previous);
    }
    ParagraphTree *nextListMember() const {
        return brother ? brother->nextListMemberAfter(*this) : NULL;
    }

    ParagraphTree* firstWithLessIndentThan(int wanted_indentation) {
        if (indentation < wanted_indentation) return this;
        if (!brother) return 0;
        return brother->firstWithLessIndentThan(wanted_indentation);
    }

    void format_indentations();
    void format_lists();

private:
    static ParagraphTree* buildNewParagraph(const string& Text, size_t beginLineNo, ParagraphType type) {
        Ostrings S;
        S.push_back(Ostring(Text, beginLineNo, type));
        return new ParagraphTree(S.begin(), S.end());
    }
    ParagraphTree *xml_write_list_contents();
    ParagraphTree *xml_write_enum_contents();
    void xml_write_textblock();

public:
    void xml_write();
};

#if defined(DUMP_PARAGRAPHS)
static void dump_paragraph(ParagraphTree *para) {
    // helper function for use in gdb
    para->dump(cout, 0);
}
#endif

void ParagraphTree::brothers_to_sons(ParagraphTree *new_brother) {
    /*! folds down brothers to sons
     * @param new_brother brother of 'this->brother', will become new brother.
     * If new_brother == NULL -> make all brothers sons.
     */

    if (new_brother) {
        h2x_assert(is_some_brother(new_brother));

        if (brother != new_brother) {
#if defined(DEBUG)
            if (son) {
                son->attach_warning("Found unexpected son (in brothers_to_sons)");
                brother->attach_warning("while trying to transform paragraphs from here ..");
                new_brother->attach_warning(".. to here ..");
                attach_warning(".. into sons of this paragraph.");
                return;
            }
#endif

            h2x_assert(!son);
            h2x_assert(brother);

            if (new_brother == NULL) { // all brothers -> sons
                son     = brother;
                brother = NULL;
            }
            else {
                son     = brother->takeAllInFrontOf(new_brother);
                brother = new_brother;
            }
        }
    }
    else {
        h2x_assert(!son);
        son     = brother;
        brother = NULL;
    }
}
void ParagraphTree::format_lists() {
    // reformats tree such that all items/enumerations are brothers
    ParagraphTree *member = firstListMember();
    if (member) {
        for (ParagraphTree *curr = this; curr != member; curr = curr->brother) {
            h2x_assert(curr);
            if (curr->son) curr->son->format_lists();
        }

        for (ParagraphTree *next = member->nextListMember();
             next;
             member = next, next = member->nextListMember())
        {
            member->brothers_to_sons(next);
            h2x_assert(member->brother == next);

            if (member->son) member->son->format_lists();
        }

        h2x_assert(!member->son); // member is the last item

        if (member->brother) {
            ParagraphTree *non_member = member->brother->firstWithLessIndentThan(member->indentation+1);
            member->brothers_to_sons(non_member);
        }

        if (member->son)     member->son->format_lists();
        if (member->brother) member->brother->format_lists();
    }
    else {
        for (ParagraphTree *curr = this; curr; curr = curr->brother) {
            h2x_assert(curr);
            if (curr->son) curr->son->format_lists();
        }
    }
}

void ParagraphTree::format_indentations() {
    if (brother) {
        ParagraphTree *same_indent = brother->firstWithLessIndentThan(indentation+1);
#if defined(WARN_POSSIBLY_WRONG_INDENTATION_CORRECTION)
        if (same_indent && indentation != same_indent->indentation) {
            same_indent->attach_warning("indentation is assumed to be same as ..")
            attach_warning(".. here");
        }
#endif
        brothers_to_sons(same_indent); // if same_indent==NULL -> make all brothers childs
        if (brother) brother->format_indentations();
    }

    if (son) son->format_indentations();
}

// -----------------
//      LinkType

enum LinkType {
    LT_UNKNOWN = 0,
    LT_HTTP    = 1,
    LT_FTP     = 2,
    LT_FILE    = 4,
    LT_EMAIL   = 8,
    LT_HLP     = 16,
    LT_PS      = 32,
    LT_PDF     = 64
};

static const char *link_id[] = {
    "unknown",
    "www",
    "www",
    "www",
    "email",
    "hlp",
    "ps",
    "pdf",
};

static string LinkType2id(LinkType type) {
    int idx = 0;
    while (type >= 1) {
        idx++;
        type = LinkType(type>>1);
    }
    return link_id[idx];
}

inline const char *getExtension(const string& name) {
    size_t last_dot = name.find_last_of('.');
    if (last_dot == string::npos) {
        return NULL;
    }
    return name.c_str()+last_dot+1;
}

static LinkType detectLinkType(const string& link_target) {
    LinkType    type = LT_UNKNOWN;
    const char *ext  = getExtension(link_target);

    if      (ext && strcasecmp(ext, "hlp") == 0)            type = LT_HLP;
    else if (link_target.find("http://")   == 0)            type = LT_HTTP;
    else if (link_target.find("ftp://")    == 0)            type = LT_FTP;
    else if (link_target.find("file://")   == 0)            type = LT_FILE;
    else if (link_target.find('@')         != string::npos) type = LT_EMAIL;
    else if (ext && strcasecmp(ext, "ps")  == 0)            type = LT_PS;
    else if (ext && strcasecmp(ext, "pdf") == 0)            type = LT_PDF;

    return type;
}

// --------------------------------------------------------------------------------



static string locate_helpfile(const string& helpname) {
    // search for 'helpname' in various helpfile locations

#define PATHS 2
    static string path[PATHS] = { "oldhelp/", "genhelp/" };
    struct stat st;

    for (size_t p = 0; p<PATHS; p++) {
        string fullname = path[p]+helpname;
        if (stat(fullname.c_str(), &st) == 0) {
            return fullname;
        }
    }
    return "";
#undef PATHS
}

static string locate_document(const string& docname) {
    // search for 'docname' or 'docname.gz' in various helpfile locations

    string located = locate_helpfile(docname);
    if (located.empty()) {
        located = locate_helpfile(docname+".gz");
    }
    return located;
}

static void add_link_attributes(XML_Tag& link, LinkType type, const string& dest, size_t source_line) {
    if (type == LT_UNKNOWN) {
        string msg = string("Invalid link (dest='")+dest+"')";
        throw LineAttachedMessage(msg, source_line);
    }

    link.add_attribute("dest", dest);
    link.add_attribute("type", LinkType2id(type));
    link.add_attribute("source_line", source_line);

    if (type&(LT_HLP|LT_PDF|LT_PS)) {               // other links (www, email) cannot be checked for existence here
        string fullhelp = ((type&LT_HLP) ? locate_helpfile : locate_document)(dest);
        if (fullhelp.empty()) {
            link.add_attribute("missing", "1");
            string warning = strf("Dead link to '%s'", dest.c_str());
            h2x_assert(source_line<1000); // illegal line number ?
            add_warning(warning, source_line);
        }
    }
}

static void print_XML_Text_expanding_links(const string& text, size_t lineNo) {
    size_t found = text.find("LINK{", 0);
    if (found != string::npos) {
        size_t inside_link = found+5;
        size_t close = text.find('}', inside_link);

        if (close == string::npos) throw "unclosed 'LINK{}'";

        string   link_target = text.substr(inside_link, close-inside_link);
        LinkType type        = detectLinkType(link_target);
        string   dest        = link_target;

        // cppcheck-suppress unusedScopedObject
        XML_Text(text.substr(0, found));

        {
            XML_Tag link("LINK");
            link.set_on_extra_line(false);
            add_link_attributes(link, type, dest, lineNo);
        }

        print_XML_Text_expanding_links(text.substr(close+1), lineNo);
    }
    else {
        XML_Text t(text);
    }
}

void ParagraphTree::xml_write_textblock() {
    XML_Tag textblock("T");
    textblock.add_attribute("reflow", reflow ? "1" : "0");

    {
        string        usedText;
        const string& text = otext;
        if (reflow) {
            usedText = correctIndentation(text, (textblock.Indent()+1) * the_XML_Document->indentation_per_level);
        }
        else {
            usedText = text;
        }
        print_XML_Text_expanding_links(usedText, otext.get_lineno());
    }
}

ParagraphTree *ParagraphTree::xml_write_list_contents() {
    h2x_assert(is_itemlist_member());
#if defined(WARN_FIXED_LAYOUT_LIST_ELEMENTS)
    if (!reflow) attach_warning("ITEM not reflown (check output)");
#endif
    {
        XML_Tag entry("ENTRY");
        entry.add_attribute("item", "1");
        xml_write_textblock();
        if (son) son->xml_write();
    }
    if (brother && brother->is_itemlist_member()) {
        return brother->xml_write_list_contents();
    }
    return brother;
}
ParagraphTree *ParagraphTree::xml_write_enum_contents() {
    h2x_assert(get_enumeration());
#if defined(WARN_FIXED_LAYOUT_LIST_ELEMENTS)
    if (!reflow) attach_warning("ENUMERATED not reflown (check output)");
#endif
    {
        XML_Tag entry("ENTRY");
        entry.add_attribute("enumerated", strf("%i", otext.get_number()));
        xml_write_textblock();
        if (son) son->xml_write();
    }
    if (brother && brother->get_enumeration()) {
        int diff = brother->get_enumeration()-get_enumeration();
        if (diff != 1) {
            attach_warning("Non-consecutive enumeration detected between here..");
            brother->attach_warning(".. and here");
        }
        return brother->xml_write_enum_contents();
    }
    return brother;
}

void ParagraphTree::xml_write() {
    try {
        ParagraphTree *next = NULL;
        if (get_enumeration()) {
            XML_Tag enu("ENUM");
            if (get_enumeration() != 1) {
                attach_warning(strf("First enum starts with '%u.' (maybe previous enum was not detected)", get_enumeration()));
            }
            next = xml_write_enum_contents();
#if defined(WARN_LONESOME_LIST_ELEMENTS)
            if (next == brother) attach_warning("Suspicious single-element-ENUM");
#endif
        }
        else if (is_itemlist_member()) {
            XML_Tag list("LIST");
            next = xml_write_list_contents();
#if defined(WARN_LONESOME_LIST_ELEMENTS)
            if (next == brother) attach_warning("Suspicious single-element-LIST");
#endif
        }
        else {
            {
                XML_Tag para("P");
                xml_write_textblock();
                if (son) son->xml_write();
            }
            next = brother;
        }
        if (next) next->xml_write();
    }
    catch (string& err) { throw attached_message(err); }
    catch (const char *err) { throw attached_message(err); }
}

static void create_top_links(const Links& links, const char *tag) {
    for (Links::const_iterator s = links.begin(); s != links.end(); ++s) {
        XML_Tag link(tag);
        add_link_attributes(link, detectLinkType(s->Target()), s->Target(), s->SourceLineno());
    }
}

void Helpfile::writeXML(FILE *out, const string& page_name) {
    XML_Document xml("PAGE", "arb_help.dtd", out);

    xml.skip_empty_tags       = true;
    xml.indentation_per_level = 2;

    xml.getRoot().add_attribute("name", page_name);
#if defined(DEBUG)
    xml.getRoot().add_attribute("edit_warning", "devel"); // inserts a edit warning into development version
#else
    xml.getRoot().add_attribute("edit_warning", "release"); // inserts a different edit warning into release version
#endif // DEBUG

    {
        // cppcheck-suppress unusedScopedObject
        XML_Comment(string("automatically generated from ../")+inputfile+' ');
    }

    create_top_links(uplinks, "UP");
    create_top_links(references, "SUB");
    create_top_links(auto_references, "SUB");

    {
        XML_Tag title_tag("TITLE");
        const Ostrings& T = title.Content();
        for (Ostrings::const_iterator s = T.begin(); s != T.end(); ++s) {
            if (s != T.begin()) { XML_Text text("\n"); }
            XML_Text text(*s);
        }
    }

    for (NamedSections::const_iterator named_sec = sections.begin(); named_sec != sections.end(); ++named_sec) {
        XML_Tag section_tag("SECTION");
        section_tag.add_attribute("name", named_sec->getName());

        ParagraphTree *ptree = ParagraphTree::buildParagraphTree(*named_sec);

#if defined(DEBUG)
        size_t textnodes = ptree->countTextNodes();
#endif
#if defined(DUMP_PARAGRAPHS)
        cout << "Dump of section '" << named_sec->getName() << "' (before format_lists):\n";
        ptree->dump(cout);
        cout << "----------------------------------------\n";
#endif

        ptree->format_lists();

#if defined(DUMP_PARAGRAPHS)
        cout << "Dump of section '" << named_sec->getName() << "' (after format_lists):\n";
        ptree->dump(cout);
        cout << "----------------------------------------\n";
#endif
#if defined(DEBUG)
        size_t textnodes2 = ptree->countTextNodes();
        h2x_assert(textnodes2 == textnodes); // if this occurs format_lists has an error
#endif

        ptree->format_indentations();

#if defined(DUMP_PARAGRAPHS)
        cout << "Dump of section '" << named_sec->getName() << "' (after format_indentations):\n";
        ptree->dump(cout);
        cout << "----------------------------------------\n";
#endif
#if defined(DEBUG)
        size_t textnodes3 = ptree->countTextNodes();
        h2x_assert(textnodes3 == textnodes2); // if this occurs format_indentations has an error
#endif

        ptree->xml_write();

        delete ptree;
    }
}

void Helpfile::extractInternalLinks() {
    for (NamedSections::const_iterator named_sec = sections.begin(); named_sec != sections.end(); ++named_sec) {
        const Section& sec = named_sec->getSection();
        try {
            const Ostrings& s = sec.Content();

            for (Ostrings::const_iterator li = s.begin(); li != s.end(); ++li) {
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
                        try {
                            check_specific_duplicates(link_target, references,      false); // check only sublinks here
                            check_specific_duplicates(link_target, uplinks,         false); // check only uplinks here
                            check_specific_duplicates(link_target, auto_references, false); // check only sublinks here

                            // only auto-add inline reference if none of the above checks has thrown
                            auto_references.push_back(Link(link_target, sec.StartLineno()));
                        }
                        catch (string& err) {
                            ; // silently ignore inlined
                        }
                    }
                    start = close+1;
                }
            }
        }
        catch (string& err) {
            throw named_sec->attached_message("'"+err+"' while scanning LINK{}");
        }
    }
}

static void show_err(const string& err, size_t lineno, const string& helpfile) {
    if (err.find(helpfile+':') != string::npos) {
        cerr << err;
    }
    else if (lineno == NO_LINENUMBER_INFO) {
        cerr << helpfile << ":1: [in unknown line] " << err;
    }
    else {
        cerr << helpfile << ":" << lineno << ": " << err;
    }
    cerr << '\n';
}
inline void show_err(const LineAttachedMessage& line_err, const string& helpfile) {
    show_err(line_err.Message(), line_err.Lineno(), helpfile);
}
inline void show_warning(const LineAttachedMessage& line_err, const string& helpfile) {
    show_err(string("Warning: ")+line_err.Message(), line_err.Lineno(), helpfile);
}
inline void show_warnings(const string& helpfile) {
    for (list<LineAttachedMessage>::const_iterator wi = warnings.begin(); wi != warnings.end(); ++wi) {
        show_warning(*wi, helpfile);
    }
}
static void show_warnings_and_error(const LineAttachedMessage& error, const string& helpfile) {
    show_warnings(helpfile);
    show_err(error, helpfile);
}

int ARB_main(int argc, char *argv[]) {
    if (argc != 3) {
        cerr << "Usage: arb_help2xml <ARB helpfile> <XML output>\n";
        return EXIT_FAILURE;
    }

    Helpfile help;
    string   arb_help;

    try {
        try {
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
                    // arb_help contains 'oldhelp/name.hlp'
                    size_t slash = arb_help.find('/');
                    size_t dot   = arb_help.find_last_of('.');

                    if (slash == string::npos || dot == string::npos) {
                        throw string("parameter <ARB helpfile> has to be in format 'oldhelp/name.hlp' (not '"+arb_help+"')");
                    }

                    string page_name(arb_help, slash+1, dot-slash-1);
                    help.writeXML(out, page_name);
                    fclose(out);
                }
                catch (...) {
                    fclose(out);
                    remove(xml_output.c_str());
                    throw;
                }
            }

            show_warnings(arb_help);

            return EXIT_SUCCESS;
        }
        catch (string& err)              { throw unattached_message(err); }
        catch (const char * err)         { throw unattached_message(err); }
        catch (LineAttachedMessage& err) { throw; }
        catch (...)                      { throw unattached_message("unknown exception in arb_help2xml"); }
    }
    catch (LineAttachedMessage& err) { show_warnings_and_error(err, arb_help); }
    catch (...) { h2x_assert(0); }

    return EXIT_FAILURE;
}

// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

static arb_test::match_expectation help_file_compiles(const char *helpfile, const char *expected_title, const char *expected_error_part) {
    using namespace   arb_test;
    expectation_group expected;

    ifstream in(helpfile);

    LineAttachedMessage *error = NULL;

    Helpfile help;
    try {
        help.readHelp(in, helpfile);
        help.extractInternalLinks();

        FILE *devnul = fopen("/dev/null", "wt");
        if (!devnul) throw unattached_message("can't write to null device");
        help.writeXML(devnul, "dummy");
        fclose(devnul);
    }
    catch (LineAttachedMessage& err) { error = new LineAttachedMessage(err); }
    catch (...)                      { error = new LineAttachedMessage(unattached_message("unknown exception")); }

    if (expected_error_part) {
        expected.add(that(error).does_differ_from_NULL());
        if (error) expected.add(that(error->Message()).does_contain(expected_error_part));
    }
    else {
        expected.add(that(error).is_equal_to_NULL());
        if (!error) {
            Section title = help.get_title();
            const Ostrings& title_strings = title.Content();

            expected.add(that(title_strings.front().as_string()).is_equal_to(expected_title));
            expected.add(that(title_strings.size()).is_equal_to(1));
        }
        else {
            show_warnings_and_error(*error, helpfile);
        }
    }

    delete error;

    return all().ofgroup(expected);
}

#define HELP_FILE_COMPILES(name,expTitle)      TEST_EXPECTATION(help_file_compiles(name,expTitle,NULL))
#define HELP_FILE_COMPILE_ERROR(name,expError) TEST_EXPECTATION(help_file_compiles(name,NULL,expError))

void TEST_hlp2xml_conversion() {
    TEST_EXPECT_ZERO(chdir("../../HELP_SOURCE"));

    HELP_FILE_COMPILES("genhelp/agde_treepuzzle.hlp", "treepuzzle");        // genhelp/agde_treepuzzle.hlp

    HELP_FILE_COMPILES("oldhelp/markbyref.hlp", "Mark by reference");        // oldhelp/markbyref.hlp
    HELP_FILE_COMPILES("oldhelp/ad_align.hlp",  "Alignment Administration"); // oldhelp/ad_align.hlp
    HELP_FILE_COMPILES("genhelp/copyright.hlp", "Copyrights");               // genhelp/copyright.hlp

    HELP_FILE_COMPILE_ERROR("akjsdlkad.hlp", "Can't read from"); // no such file
}


// #define TEST_AUTO_UPDATE // uncomment to update expected xml // @@@ comment-out!

void TEST_hlp2xml_output() {
    string tested_helpfile[] = {
        "unittest"
    };

    string HELP_SOURCE = "../../HELP_SOURCE/";
    string LIB         = "../../lib/";
    string EXPECTED    = "help/";

    for (size_t i = 0; i<ARRAY_ELEMS(tested_helpfile); ++i) {
        string xml  = HELP_SOURCE + "Xml/" + tested_helpfile[i] + ".xml";
        string html = LIB + "help_html/" + tested_helpfile[i] + ".html";
        string hlp  = LIB + "help/" + tested_helpfile[i] + ".hlp";

        string xml_expected  = EXPECTED + tested_helpfile[i] + ".xml";
        string html_expected = EXPECTED + tested_helpfile[i] + ".html";
        string hlp_expected  = EXPECTED + tested_helpfile[i] + ".hlp";

        string cmd;

#if defined(TEST_AUTO_UPDATE)
# if defined(NDEBUG)
#  error please use auto-update only in DEBUG mode
# endif
        cmd = string("cp ") + xml  + ' ' + xml_expected;  TEST_EXPECT_NO_ERROR(GBK_system(cmd.c_str()));
        cmd = string("cp ") + html + ' ' + html_expected; TEST_EXPECT_NO_ERROR(GBK_system(cmd.c_str()));
        cmd = string("cp ") + hlp  + ' ' + hlp_expected;  TEST_EXPECT_NO_ERROR(GBK_system(cmd.c_str()));
#else // !defined(TEST_AUTO_UPDATE)

# if defined(DEBUG)
        int expected_xml_difflines = 0;
        int expected_hlp_difflines = 0;
# else // !defined(DEBUG)
        int expected_xml_difflines = 1; // value of "edit_warning" differs - see .@edit_warning
        int expected_hlp_difflines = 1; // resulting warning in helpfile
# endif
        TEST_EXPECT_TEXTFILE_DIFFLINES(xml_expected.c_str(),  xml.c_str(),  expected_xml_difflines);
        TEST_EXPECT_TEXTFILE_DIFFLINES_IGNORE_DATES(html_expected.c_str(), html.c_str(), 0); // html contains the update-date
        TEST_EXPECT_TEXTFILE_DIFFLINES(hlp_expected.c_str(),  hlp.c_str(),  expected_hlp_difflines);
#endif
    }
}

#endif // UNIT_TESTS

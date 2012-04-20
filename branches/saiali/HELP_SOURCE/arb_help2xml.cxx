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

#include <list>
#include <set>
#include <iostream>
#include <fstream>

#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <climits>

#include <sys/stat.h>

using namespace std;

#define h2x_assert(bed) arb_assert(bed)

#if defined(DEBUG)
#define WARN_MISSING_HELP
#endif // DEBUG


// #define DUMP_DATA // use this to see internal data (class Helpfile)
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


STATIC_ATTRIBUTED(__ATTR__VFORMAT(1), string vstrf(const char *format, va_list argPtr)) {
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

STATIC_ATTRIBUTED(__ATTR__FORMAT(1), string strf(const char *format, ...)) {
    va_list argPtr;
    va_start(argPtr, format);
    string result = vstrf(format, argPtr);
    va_end(argPtr);

    return result;
}

// --------------------------------------
//      warnings
// --------------------------------------

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


static list<LineAttachedMessage> warnings;

inline LineAttachedMessage make_warning(const string& warning, size_t lineno) {
    return LineAttachedMessage(string("Warning: ")+warning, lineno);
}
inline void add_warning(const string& warning, size_t lineno) {
    warnings.push_back(make_warning(warning, lineno));
}
inline void preadd_warning(const string& warning, size_t lineno) {
    LineAttachedMessage line_message = make_warning(warning, lineno);
    if (warnings.size() < 2) {
        warnings.push_front(line_message);
    }
    else {
        LineAttachedMessage prev_message = warnings.back();
        warnings.pop_back();
        warnings.push_back(line_message);
        warnings.push_back(prev_message);
    }
}

// ----------------------
//      class Reader
// ----------------------

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
                while (eol >= lineBuffer && eol[0] == ' ') eol--;
                if (eol > lineBuffer) {
                    // now eol points to last character
                    if (eol[0] == '-' && isalnum(eol[-1])) {
                        add_warning("manual hyphenation detected", lineNo);
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


typedef list<string> Strings;

class Section {
    SectionType type;
    Strings     content;
    size_t      start_lineno;

public:
    Section(SectionType type_, size_t start_lineno_) : type(type_), start_lineno(start_lineno_) {}

    const Strings& Content() const { return content; }
    Strings& Content() { return content; }

    SectionType get_type() const { return type; }

    size_t StartLineno() const { return start_lineno; }
    void set_StartLineno(size_t start_lineno_) { start_lineno = start_lineno_; }
};


#if defined(WARN_MISSING_HELP)
void check_TODO(const char *line, const Reader& reader) {
    if (strstr(line, "@@@") != NULL || strstr(line, "TODO") != NULL) {
        string warn = strf("TODO: %s", line);
        add_warning(warn.c_str(), reader.getLineNo());
    }
}
#endif // WARN_MISSING_HELP

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

//  -----------------------
//      class Helpfile
//  -----------------------
class Helpfile {
private:
    Links         uplinks;
    Links         references;
    Links         auto_references;
    Section       title;
    NamedSections sections;
    string        inputfile;

public:
    Helpfile() : title(SEC_FAKE, -1U) {}
    virtual ~Helpfile() {}

    void readHelp(istream& in, const string& filename);
    void writeXML(FILE *out, const string& page_name);
    void extractInternalLinks();

    const Section& get_title() const { return title; }
};

inline bool isWhite(char c) { return c == ' '; }

#if defined(DUMP_DATA)
static void display(const Strings& strings, const string& title, FILE *out) {
    fprintf(out, "  %s:\n", title.c_str());
    for (Strings::const_iterator s = strings.begin(); s != strings.end(); ++s) {
        fprintf(out, "    '%s'\n", s->c_str());
    }
}
static void display(const Sections& sections, const string& title, FILE *out) {
    fprintf(out, "%s:\n", title.c_str());
    for (Sections::const_iterator s = sections.begin(); s != sections.end(); ++s) {
        display(s->second, s->first, out);
    }
}
#endif // DUMP_DATA

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

inline void pushParagraph(Section& sec, string& paragraph) {
    if (paragraph.length()) {
        sec.Content().push_back(paragraph);
        paragraph = "";
    }
}

inline const char *firstChar(const char *s) {
    while (isWhite(s[0])) ++s;
    return s;
}

static void parseSection(Section& sec, const char *line, int indentation, Reader& reader) {
    string paragraph          = line;
    int    lines_in_paragraph = 1;

    if (sec.StartLineno() == -1U) {
        sec.set_StartLineno(reader.getLineNo());
    }

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

#if defined(WARN_MISSING_HELP)
            check_TODO(line, reader);
#endif // WARN_MISSING_HELP

            string Line = line;

            if (sec.get_type() == SEC_OCCURRENCE) {
                pushParagraph(sec, paragraph); lines_in_paragraph = 0;
            }
            else {
                const char *first = firstChar(line);
                if (first[0] == '-') {
                    pushParagraph(sec, paragraph); lines_in_paragraph = 0;
                    Line[first-line] = ' ';
                }
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

    if (sec.Content().size()>0 && indentation>0) {
        string spaces;
        spaces.reserve(indentation);
        spaces.append(indentation, ' ');

        Strings::iterator p = sec.Content().begin();

        *p = string("\n")+spaces+*p;
    }
}
inline void check_duplicates(const string& link, const char * /* where */, const Links& existing, bool add_warnings) {
    for (Links::const_iterator ex = existing.begin(); ex != existing.end(); ++ex) {
        if (ex->Target() == link) {
            if (add_warnings) add_warning(strf("First Link to '%s' was found here.", ex->Target().c_str()), ex->SourceLineno());
            throw strf("Link to '%s' duplicated here.", link.c_str());
        }
    }
}
inline void check_duplicates(const string& link, const Links& uplinks, const Links& references, bool add_warnings) {
    check_duplicates(link, "UP", uplinks, add_warnings);
    check_duplicates(link, "SUB", references, add_warnings);
}

void Helpfile::readHelp(istream& in, const string& filename) {
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
                continue;
            }

#if defined(WARN_MISSING_HELP)
            check_TODO(line, read);
#endif // WARN_MISSING_HELP

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
                            Section sec(stype, read.getLineNo());

                            parseSection(sec, "", 0, read);
                            sections.push_back(NamedSection(section_name, sec));
                        }
                        else {
                            Section sec(stype, read.getLineNo());

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

    catch (string& err) { throw LineAttachedMessage(err, read.getLineNo()); }
    catch (const char *err) { throw LineAttachedMessage(err, read.getLineNo()); }
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

static bool startsWithNumber(string& s, long long &number, bool do_erase = true) {
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

static int get_first_indentation(const string& s) {
    // returns the indentation of the first line containing other than spaces
    size_t text_start   = s.find_first_not_of(" \n");
    size_t prev_lineend = s.find_last_of('\n', text_start);

    if (prev_lineend == string::npos) return text_start;
    return text_start-prev_lineend-1;
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

//  ----------------------------
//      class ParagraphTree
//  ----------------------------
class ParagraphTree : virtual Noncopyable {
private:
    ParagraphTree *brother;     // has same indentation as this
    ParagraphTree *son;         // indentation + 1

    bool is_enumerated;         // 1., 2.,  usw.
    long long enumeration;           // the value of the enumeration (undefined if !is_enumerated)

    bool reflow;                // should the paragraph be reflown ? (true if indentation is equal for all lines of text)
    int  indentation;           // the real indentation of the black (after enumeration was removed)

    string text;                // text of the Section (containing linefeeds)
    size_t lineNo;              // line number where Paragraph starts

    ParagraphTree(Strings::const_iterator begin, const Strings::const_iterator end, size_t beginLineNo) {
        h2x_assert(begin != end);

        text = *begin;
        son  = 0;

        enumeration   = 0;
        is_enumerated = startsWithNumber(text, enumeration);

        lineNo = beginLineNo;

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
                int    rest_indent = -1;
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

                        text   = text.substr(0, reststart)+correctIndentation(rest, -8);
                        reflow = shouldReflow(text, indentation);
                    }
                }
            }
        }

        if (!reflow) {
            indentation = scanMinIndentation(text);
        }

        text    = correctIndentation(text, -indentation);
        brother = buildParagraphTree(++begin, end, beginLineNo);
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

    static ParagraphTree* buildParagraphTree(Strings::const_iterator begin, const Strings::const_iterator end, size_t beginLineNo) {
        if (begin == end) return 0;
        return new ParagraphTree(begin, end, beginLineNo);
    }
    static ParagraphTree* buildParagraphTree(const NamedSection& N) {
        ParagraphTree  *ptree = 0;
        const Strings&  S     = N.getSection().Content();
        if (S.empty()) throw string("Tried to build an empty ParagraphTree (Section=")+N.getName()+")";
        else ptree = buildParagraphTree(S.begin(), S.end(), N.getSection().StartLineno());
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
    static ParagraphTree* buildNewParagraph(const string& Text, size_t beginLineNo) {
        Strings S;
        S.push_back(Text);
        return new ParagraphTree(S.begin(), S.end(), beginLineNo);
    }
    ParagraphTree *extractEmbeddedEnum(int lookfor) {
        size_t this_lineend = text.find('\n');

        while (this_lineend != string::npos) {
            long long number;
            string embedded = text.substr(this_lineend);
            if (startsWithNumber(embedded, number, false)) {
                if (number == lookfor) {
                    text.erase(this_lineend);
                    return buildNewParagraph(embedded, lineNo);
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

            for (lookfor = 2; ; ++lookfor) {
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

ParagraphTree* ParagraphTree::format_indentations() {
    if (brother) {
        if (is_enumerated) {
            if (brother) brother = brother->format_indentations();
            if (son) son = son->format_indentations();
        }
        else {
            ParagraphTree *same_indent = brother->firstWithSameOrLowerIndent(indentation);
            if (same_indent) {
                if (same_indent == brother) {
                    brother     = brother->format_indentations();
                    if (son) son = son->format_indentations();
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

        if (close != string::npos) {
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

            return print_XML_Text_expanding_links(text.substr(close+1), lineNo);
        }
    }

    XML_Text t(text);
}

void ParagraphTree::xml_write(bool ignore_enumerated, bool write_as_entry) {
    if (is_enumerated && !ignore_enumerated) {
        XML_Tag e("ENUM");
        xml_write(true);
    }
    else {
        {
            XML_Tag para(is_enumerated || write_as_entry ? "ENTRY" : "P");
            {
                XML_Tag textblock("T");
                textblock.add_attribute("reflow", reflow ? "1" : "0");
                {
                    string usedText;
                    if (reflow) {
                        usedText = correctIndentation(text, (textblock.Indent()+1) * the_XML_Document->indentation_per_level);
                    }
                    else {
                        usedText = text;
                    }
                    print_XML_Text_expanding_links(usedText, lineNo);
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
        }
        if (brother) brother->xml_write(ignore_enumerated, write_as_entry);
    }
}

static void create_top_links(const Links& links, const char *tag) {
    for (Links::const_iterator s = links.begin(); s != links.end(); ++s) {
        XML_Tag link(tag);
        add_link_attributes(link, detectLinkType(s->Target()), s->Target(), s->SourceLineno());
    }
}

void Helpfile::writeXML(FILE *out, const string& page_name) {
#if defined(DUMP_DATA)
    display(uplinks,         "Uplinks",         stdout);
    display(references,      "References",      stdout);
    display(auto_references, "Auto-References", stdout);
    display(title,           "Title",           stdout);
    display(sections,        "Sections",        stdout);
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

    {
        // cppcheck-suppress unusedScopedObject
        XML_Comment(string("automatically generated from ../")+inputfile+' ');
    }

    create_top_links(uplinks, "UP");
    create_top_links(references, "SUB");
    create_top_links(auto_references, "SUB");
    
    {
        XML_Tag title_tag("TITLE");
        Strings& T = title.Content();
        for (Strings::const_iterator s = T.begin(); s != T.end(); ++s) {
            if (s != T.begin()) { XML_Text text("\n"); }
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

void Helpfile::extractInternalLinks() {
    for (NamedSections::const_iterator named_sec = sections.begin(); named_sec != sections.end(); ++named_sec) {
        const Section& sec = named_sec->getSection();
        try {
            const Strings& s   = sec.Content();

            for (Strings::const_iterator li = s.begin(); li != s.end(); ++li) {
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
                            check_duplicates(link_target, "SUB", references, true); // check only sublinks here
                            check_duplicates(link_target, "UP", uplinks, false); // check only sublinks here
                            check_duplicates(link_target, "AUTO-SUB", auto_references, false); // check only sublinks here
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
            throw LineAttachedMessage(string("'"+err+"' while scanning LINK{} in SECTION '"+named_sec->getName()+'\''),
                                      sec.StartLineno());
        }
    }
}

static void show_err(const string& err, size_t lineno, const string& helpfile) {
    if (err.find(helpfile+':') != string::npos) {
        cerr << err;
    }
    else if (lineno == -1U) {
        cerr << helpfile << ":1: [in unknown line] " << err;
    }
    else {
        cerr << helpfile << ":" << lineno << ": " << err;
    }
    cerr << '\n';
}
static void show_err(const LineAttachedMessage& line_err, const string& helpfile) {
    show_err(line_err.Message(), line_err.Lineno(), helpfile);
}

static void show_warnings(const string& helpfile) {
    for (list<LineAttachedMessage>::const_iterator wi = warnings.begin(); wi != warnings.end(); ++wi) {
        show_err(*wi, helpfile);
    }
}

static void show_warnings_and_error(const LineAttachedMessage& error, const string& helpfile) {
    show_warnings(helpfile);
    show_err(error, helpfile);
}

static void show_warnings_and_error(const string& error, const string& helpfile) {
    show_warnings_and_error(LineAttachedMessage(error, -1U), helpfile);
}

int ARB_main(int argc, const char *argv[]) {
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
    catch (LineAttachedMessage& err) { show_warnings_and_error(err, arb_help); }
    catch (string& err)              { show_warnings_and_error(err, arb_help); }
    catch (const char * err)         { show_warnings_and_error(err, arb_help); }
    catch (...)                      { show_warnings_and_error("unknown exception in arb_help2xml", arb_help); }

    return EXIT_FAILURE;
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <test_unit.h>

void TEST_hlp2xml_conversion() {
    // oldhelp/ad_align.hlp
    string   arb_help = "../../HELP_SOURCE/oldhelp/ad_align.hlp";
    ifstream in(arb_help.c_str());

    Helpfile help;
    help.readHelp(in, arb_help);

    Section        title   = help.get_title();
    const Strings& strings = title.Content();

    TEST_ASSERT_EQUAL(strings.front().c_str(), "Alignment Administration");
    TEST_ASSERT(strings.size() == 1);
}

#endif // UNIT_TESTS

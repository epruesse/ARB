// ================================================================ //
//                                                                  //
//   File      : arb_proto_2_xsub.cxx                               //
//   Purpose   : generate ARB.xs for perl interface                 //
//                                                                  //
//   ReCoded by Ralf Westram (coder@reallysoft.de) in December 2009 //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include <arbdb.h>
#include <arb_strbuf.h>
#include <BufferedFileReader.h>

#include <string>
#include <vector>
#include <set>

#include <cctype>
#include <unistd.h>
#include <arb_str.h>
#include <arb_diff.h>

#if defined(DEBUG)
#if defined(DEVEL_RALF)
// #define TRACE
#endif // DEVEL_RALF
#endif // DEBUG

using namespace std;

// --------------------------------------------------------------------------------

#define CHAR_PTR       "char *"
#define CONST_CHAR_PTR "const char *"

// --------------------------------------------------------------------------------

struct Error {
    virtual ~Error() {}
    virtual void print() const = 0;
};

class ProgramError : public Error {
    string error;
public:
    ProgramError(string message) : error(message) {}
    ProgramError(const char *message) : error(message) {}
    ~ProgramError() OVERRIDE {}

    void print() const OVERRIDE {
        fprintf(stderr, "arb_proto_2_xsub: Error: %s\n", error.c_str());
    }
};

class InputFileError : public Error {
    string located_error;
public:
    InputFileError(LineReader& fileBuffer, string message)      : located_error(fileBuffer.lineError(message)) {}
    InputFileError(LineReader& fileBuffer, const char *message) : located_error(fileBuffer.lineError(message)) {}
    ~InputFileError() OVERRIDE {}

    void print() const OVERRIDE {
        fputs(located_error.c_str(), stderr);
        fputc('\n', stderr);
    }
};

// --------------------------------------------------------------------------------

class CommentSkippingFileBuffer : public BufferedFileReader {
    string open_comment;
    string close_comment;
    string eol_comment;

    void throw_error(const char *message) __ATTR__NORETURN { throw InputFileError(*this, message); }

    string read_till_close_comment(string curr_line, size_t comment_startLineNumber) {
        bool seen_end = false;
        while (!seen_end) {
            size_t close = curr_line.find(close_comment);
            if (close != string::npos) {
                curr_line = curr_line.substr(close+close_comment.length());
                seen_end  = true;
            }
            else {
                if (!BufferedFileReader::getLine(curr_line)) {
                    setLineNumber(comment_startLineNumber);
                    throw_error("end of file reached while skipping comment");
                }
            }
        }
        return curr_line;
    }

public:
    CommentSkippingFileBuffer(const string&  filename_,
                              FILE          *in,
                              const char    *openComment,
                              const char    *closeComment,
                              const char    *eolComment)
        : BufferedFileReader(filename_, in)
        , open_comment(openComment)
        , close_comment(closeComment)
        , eol_comment(eolComment)
    {}

    bool getLine(string& line) OVERRIDE {
        if (BufferedFileReader::getLine(line)) {
            size_t open = line.find(open_comment);
            size_t eol  = line.find(eol_comment);

            if (open != eol) {                      // comment found
                if (open<eol) {
                    if (eol != string::npos) {
                        throw_error(GBS_global_string("'%s' inside '%s %s'", eol_comment.c_str(), open_comment.c_str(), close_comment.c_str()));
                    }
                    line = line.substr(0, open) + read_till_close_comment(line.substr(open+2), getLineNumber());
                }
                else {
                    arb_assert(eol<open);
                    if (open != string::npos) {
                        throw_error(GBS_global_string("'%s' behind '%s'", open_comment.c_str(), eol_comment.c_str()));
                    }
                    line = line.substr(0, eol);
                }
            }
            return true;
        }
        return false;
    }
};

// --------------------------------------------------------------------------------

inline bool is_empty_code(const char *code) { return !code[0]; }
inline bool contains_preprozessorCode(const char *code) { return strchr(code, '#') != NULL; }
inline bool contains_braces(const char *code) { return strpbrk(code, "{}") != NULL; }
inline bool is_typedef(const char *code) { return ARB_strBeginsWith(code, "typedef"); }
inline bool is_forward_decl(const char *code) { return ARB_strBeginsWith(code, "class") || ARB_strBeginsWith(code, "struct"); }

inline bool is_prototype(const char *code) {
    return
        !is_empty_code(code)             &&
        !contains_preprozessorCode(code) &&
        !contains_braces(code)           &&
        !is_typedef(code)                &&
        !is_forward_decl(code);
}

inline void trace_over_braces(const char *code, int& brace_counter) {
    while (code) {
        const char *brace = strpbrk(code, "{}");
        if (!brace) break;

        if (*brace == '{') {
            ++brace_counter;
        }
        else {
            arb_assert(*brace == '}');
            --brace_counter;
        }
        code = brace+1;
    }
}

// --------------------------------------------------------------------------------

inline char *get_token_and_incr_lineno(const char*& code, const char *separator, size_t *lineno) {
    char *token = NULL;
    if (code) {
        const char *sep_pos = strpbrk(code, separator);

        if (!sep_pos) {
            if (!code[0]) {                         // end of code
                token = NULL;
                code  = NULL;
            }
            else {
                token = ARB_strdup(code);
                code  = NULL;
            }
        }
        else {
            token = ARB_strpartdup(code, sep_pos-1);

            const char *behind_sep = sep_pos + strspn(sep_pos, separator); // next non 'separator' char
            if (lineno) {
                int no_of_linefeeds = 0;
                while (code<behind_sep) if (*++code == '\n') ++no_of_linefeeds;

                *lineno += no_of_linefeeds;
            }
            else {
                code = behind_sep;
            }
        }
    }
    return token;
}

inline char *get_token(const char*& code, const char *separator) {
    return get_token_and_incr_lineno(code, separator, NULL);
}

inline bool is_ID_char(char c)  { return isalnum(c) || c == '_'; }

inline const char *next_closing_paren(const char *code) {
    const char *open_paren  = strchr(code, '(');
    const char *close_paren = strchr(code, ')');

    if (!open_paren || (close_paren && close_paren<open_paren)) return close_paren;

    close_paren = next_closing_paren(open_paren+1);
    return next_closing_paren(close_paren+1);
}

inline const char *next_comma_outside_parens(const char *code) {
    const char *comma = strchr(code, ',');
    if (comma) {
        const char *open_paren = strchr(code, '(');
        if (open_paren && open_paren<comma) {
            const char *close_paren = next_closing_paren(open_paren+1);
            if (!close_paren) throw "Unbalanced parenthesis";
            comma = next_comma_outside_parens(close_paren+1);
        }
    }
    return comma;
}

inline bool find_open_close_paren(const char *code, size_t& opening_paren_pos, size_t& closing_paren_pos) {
    const char *open_paren = strchr(code, '(');
    if (open_paren) {
        const char *close_paren = next_closing_paren(open_paren+1);
        if (close_paren) {
            opening_paren_pos = open_paren-code;
            closing_paren_pos = close_paren-code;
            return true;
        }
    }
    return false;
}

inline string concat_type_and_name(const string& type, const string& name) {
    if (type.at(type.length()-1) == '*') return type+name;
    return type+' '+name;
}

// ----------------
//      TypeMap

class TypeMap {
    // representation of types mapped in 'typemap' file
    set<string> defined_types;

public:
    TypeMap() {}

    void load(LineReader& typemap);
    bool has_definition_for(const string& type_decl) const {
        return defined_types.find(type_decl) != defined_types.end();
    }
};

// -------------
//      Type

enum TypeClass {
    INVALID_TYPE,                                   // invalid

    VOID,                                           // no parameter
    SIMPLE_TYPE,                                    // simple types like int, float, double, ...
    CONST_CHAR,                                     // 'const char *'
    HEAP_COPY,                                      // type is 'char*' and interpreted as heap-copy
    CONVERSION_FUNCTION,                            // convert type using GBP_-conversion functions
    TYPEMAPPED,                                     // type is defined in file 'typemap'

    CANT_HANDLE,                                    // type cannot be used in perl interface
    FORBIDDEN,                                      // usage forbidden via 'NOT4PERL'
};

#if defined(TRACE)
#define TypeClass2CSTR(type) case type: return #type
const char *get_TypeClass_name(TypeClass type_class) {
    switch (type_class) {
        TypeClass2CSTR(INVALID_TYPE);
        TypeClass2CSTR(VOID);
        TypeClass2CSTR(SIMPLE_TYPE);
        TypeClass2CSTR(CONST_CHAR);
        TypeClass2CSTR(HEAP_COPY);
        TypeClass2CSTR(CONVERSION_FUNCTION);
        TypeClass2CSTR(TYPEMAPPED);
        TypeClass2CSTR(CANT_HANDLE);
        TypeClass2CSTR(FORBIDDEN);
    }
    return NULL;
}
#undef TypeClass2CSTR
#endif // TRACE

inline string type2id(const string& type) {
    char *s = GBS_string_eval(type.c_str(),
                              "const =:"           // remove const (for less ugly names)
                              " =:"                // remove spaces
                              "\\*=Ptr"            // rename '*'
                              , NULL);

    string id(s);
    free(s);
    return id;
}
inline string conversion_function_name(const string& fromType, const string& toType) {
    string from = type2id(fromType);
    string to   = type2id(toType);
    return string("GBP_")+from+"_2_"+to;
}
inline string constCastTo(const string& expr, const string& targetType) {
    return string("const_cast<")+targetType+">("+expr+")";
}

class Type { // return- or parameter-type
    string    c_type;
    string    perl_type;
    TypeClass type_class;

    string unify_type_decl(const char *code) {
        string type_decl;
        enum { SPACE, STAR, ID, UNKNOWN } last = SPACE, curr;
        for (int i = 0; code[i]; ++i) {
            char c = code[i];

            switch (c) {
                case ' ': curr = SPACE; break;
                case '*': curr = STAR; break;
                default: curr = is_ID_char(c) ? ID : UNKNOWN; break;
            }

            if (last != SPACE && curr != last) type_decl += ' ';
            if (curr != SPACE)                 type_decl += c;

            last = curr;
        }

        return last == SPACE
            ? type_decl.substr(0, type_decl.length()-1)
            : type_decl;
    }

    void throw_if_enum() const {
        size_t enum_pos = c_type.find("enum ");
        if (enum_pos != string::npos) {
            const char *enum_type = c_type.c_str()+enum_pos;
            const char *enum_name = enum_type+5;
            throw GBS_global_string("do not use '%s', simply use '%s'", enum_type, enum_name);
        }
    }

    string convertExpression(const string& expr, const string& fromType, const string& toType) const {
        arb_assert(possible_in_xsub());
        throw_if_enum();
        if (get_TypeClass() == CONVERSION_FUNCTION) {
            string conversion_function = conversion_function_name(fromType, toType);
            return conversion_function+"("+expr+")"; // result is toType
        }
        return expr;
    }

    bool cant_handle(const string& type_decl) {
        return
            strpbrk(type_decl.c_str(), "().*") != NULL         || // function-parameters, pointer-types not handled in ctor
            type_decl.find("GB_CB")            != string::npos || // some typedef'ed function-parameters
            type_decl.find("CharPtrArray")     != string::npos ||
            type_decl.find("StrArray")         != string::npos ||
            type_decl.find("GB_Link_Follower") != string::npos;
    }

    bool is_forbidden(const string& type_decl) {
        return
            type_decl.find("NOT4PERL")            != string::npos || // 'NOT4PERL' declares prototype as "FORBIDDEN"
            type_decl.find("GBQUARK")             != string::npos || // internal information, hide from perl
            type_decl.find("GB_COMPRESSION_MASK") != string::npos || // internal information, hide from perl
            type_decl.find("GB_CBUFFER")          != string::npos || // internal ARBDB buffers
            type_decl.find("GB_BUFFER")           != string::npos; // memory managed by ARBDB
    }


public:
    static TypeMap globalTypemap;

    Type() : type_class(INVALID_TYPE) {}
    Type(const char *code) {
        c_type = unify_type_decl(code);

        if (c_type == "void") { type_class = VOID; }
        else if (c_type == CONST_CHAR_PTR ||
                 c_type == "GB_ERROR"     ||
                 c_type == "GB_CSTR")
        {
            type_class = CONST_CHAR;
            perl_type  = CHAR_PTR;
        }
        else if (c_type == CHAR_PTR) {
            type_class = HEAP_COPY;
            perl_type  = c_type;
        }
        // [Code-TAG: enum_type_replacement]
        // for each enum type converted here, you need to support a
        // conversion function in ../ARBDB/adperl.cxx@enum_conversion_functions
        else if (c_type == "GB_CASE"        ||
                 c_type == "GB_CB_TYPE"     ||
                 c_type == "GB_TYPES"       ||
                 c_type == "GB_UNDO_TYPE"   ||
                 c_type == "GB_SEARCH_TYPE" ||
                 c_type == "GB_alignment_type")
        {
            type_class = CONVERSION_FUNCTION;
            perl_type  = CHAR_PTR;
        }
        else if (globalTypemap.has_definition_for(c_type)) {
            type_class = TYPEMAPPED;
            perl_type  = c_type;
        }
        else if (cant_handle(c_type)) { type_class = CANT_HANDLE; }
        else if (is_forbidden(c_type)) { type_class = FORBIDDEN; } // Caution: this line catches all '*' types not handled above
        else {
            type_class = SIMPLE_TYPE;
            perl_type  = c_type;
        }
    }

    const string& c_decl() const { return c_type; }
    const string& perl_decl() const { return perl_type; }

    TypeClass get_TypeClass() const { return type_class; }
    bool isVoid() const { return get_TypeClass() == VOID; }

    bool possible_in_xsub() const { return type_class != CANT_HANDLE && type_class != FORBIDDEN; }

    string convert_argument_for_C(const string& perl_arg) const {
        if (perl_decl() == CHAR_PTR) {
            if (c_decl() == CHAR_PTR) throw "argument of type 'char*' is forbidden";
            string const_perl_arg = constCastTo(perl_arg, CONST_CHAR_PTR); // ensure C uses 'const char *'
            return convertExpression(const_perl_arg, CONST_CHAR_PTR, c_decl());
        }
        return convertExpression(perl_arg, perl_decl(), c_decl());
    }
    string convert_result_for_PERL(const string& c_expr) const {
        arb_assert(type_class != HEAP_COPY);
        if (perl_decl() == CHAR_PTR) {
            string const_c_expr = convertExpression(c_expr, c_decl(), CONST_CHAR_PTR);
            return constCastTo(const_c_expr, CHAR_PTR);
        }
        return convertExpression(c_expr, c_decl(), perl_decl());
    }

#if defined(TRACE)
    void dump_if_impossible_in_xsub(FILE *out) const {
        if (!possible_in_xsub()) {
            fprintf(out, "TRACE: - impossible type '%s' (TypeClass='%s')\n",
                    c_type.c_str(), get_TypeClass_name(type_class));
        }
    }
#endif // TRACE

};

TypeMap Type::globalTypemap;

// ------------------
//      Parameter

class Parameter {
    Type   type;
    string name;

    static long nonameCount;

public:
    Parameter() {}
    Parameter(const char *code) {
        const char *last = strchr(code, 0)-1;
        while (last[0] == ' ') --last;

        const char *name_start = last;
        while (name_start >= code && is_ID_char(name_start[0])) --name_start;
        name_start++;

        if (name_start>code) {
            string type_def(code, name_start-code);
            name = string(name_start, last-name_start+1);
            type = Type(type_def.c_str());

            if (type.possible_in_xsub() && !type.isVoid() && name.empty()) {
                name = GBS_global_string("noName%li", ++nonameCount);
            }
        }
        else if (strcmp(name_start, "void") == 0) {
            string no_type(name_start, last-name_start+1);
            name = "";
            type = Type(no_type.c_str());
        }
        else {
            throw string("can't parse '")+code+"' (expected 'type name')";
        }
    }

    const string& get_name() const { return name; }
    const Type& get_type() const { return type; }

    TypeClass get_TypeClass() const { return get_type().get_TypeClass(); }
    bool isVoid() const { return get_TypeClass() == VOID; }

    string perl_typed_param() const { return concat_type_and_name(type.perl_decl(), name); }
    string c_typed_param   () const { return concat_type_and_name(type.c_decl   (), name); }
};

long Parameter::nonameCount = 0;

// ------------------
//      Prototype

typedef vector<Parameter>         Arguments;
typedef Arguments::const_iterator ArgumentIter;

class Prototype {
    Parameter function;                             // return-type + function_name
    Arguments arguments;

    void parse_arguments(const char *arg_list) {
        const char *comma = next_comma_outside_parens(arg_list);
        if (comma) {
            {
                char *first_param = ARB_strpartdup(arg_list, comma-1);
                arguments.push_back(Parameter(first_param));
                free(first_param);
            }
            parse_arguments(comma+1);
        }
        else { // only one parameter
            arguments.push_back(Parameter(arg_list));
        }
    }

public:
    Prototype(const char *code) {
        size_t open_paren, close_paren;
        if (!find_open_close_paren(code, open_paren, close_paren)) {
            throw "expected parenthesis";
        }

        string return_type_and_name(code, open_paren);
        function = Parameter(return_type_and_name.c_str());

        string arg_list(code+open_paren+1, close_paren-open_paren-1);
        parse_arguments(arg_list.c_str());
    }

    const Type& get_return_type() const { return function.get_type(); }
    const string& get_function_name() const { return function.get_name(); }

    ArgumentIter args_begin() const { return arguments.begin(); }
    ArgumentIter args_end() const { return arguments.end(); }

    string argument_names_list() const {
        string       argument_list;
        bool         first   = true;
        ArgumentIter arg_end = arguments.end();

        for (ArgumentIter param = arguments.begin(); param != arg_end; ++param) {
            if (!param->isVoid()) {
                if (first) first    = false;
                else argument_list += ", ";

                argument_list += param->get_name();
            }
        }
        return argument_list;
    }

    string call_arguments() const {
        string       argument_list;
        bool         first   = true;
        ArgumentIter arg_end = arguments.end();

        for (ArgumentIter arg = arguments.begin(); arg != arg_end; ++arg) {
            if (!arg->isVoid()) {
                if (first) first    = false;
                else argument_list += ", ";

                argument_list += arg->get_type().convert_argument_for_C(arg->get_name());
            }
        }
        return argument_list;
    }

    bool possible_as_xsub() const {
        if (get_return_type().possible_in_xsub()) {
            ArgumentIter arg_end = arguments.end();
            for (ArgumentIter arg = arguments.begin(); arg != arg_end; ++arg) {
                if (!arg->get_type().possible_in_xsub()) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

#if defined(TRACE)
    void dump_types_impossible_in_xsub(FILE *out) const {
        get_return_type().dump_if_impossible_in_xsub(out);
        ArgumentIter arg_end = arguments.end();
        for (ArgumentIter arg = arguments.begin(); arg != arg_end; ++arg) {
            arg->get_type().dump_if_impossible_in_xsub(out);
        }
    }
#endif // TRACE
};

inline void trim(string& text) {
    const char *whiteSpace = " \t";
    size_t      leading    = text.find_first_not_of(whiteSpace);
    size_t      trailing   = text.find_last_not_of(whiteSpace, leading);

    if (trailing != string::npos) {
        text = text.substr(leading, text.length()-leading-trailing);
    }
}

void TypeMap::load(LineReader& typemap_reader) {
    string line;
    while (typemap_reader.getLine(line)) {
        if (line == "TYPEMAP") {
            while (typemap_reader.getLine(line)) {
                trim(line);
                if (!line.empty()) {
                    Parameter     typemapping(line.c_str());
                    const string& c_type = typemapping.get_type().c_decl();
                    defined_types.insert(c_type);
                }
            }
            return;
        }
    }

    throw InputFileError(typemap_reader, "Expected to see 'TYPEMAP'");
}

// ----------------
//      Package

class Package : virtual Noncopyable {
    string         prefix;                          // e.g. 'P2A' or 'P2AT'
    string         name;                            // e.g. 'ARB' or 'BIO'
    GBS_strstruct *generated_code;
    GB_HASH       *functions_to_skip;

public:
    Package(const char *name_, const char *prefix_)
        : prefix(prefix_)
        , name(name_)
    {
        generated_code    = GBS_stropen(100000);
        functions_to_skip = GBS_create_hash(1000, GB_MIND_CASE);
    }
    ~Package() {
        GBS_free_hash(functions_to_skip);
        GBS_strforget(generated_code);
    }

    bool matches_package_prefix(const string& text) const { return text.find(prefix) == 0 && text.at(prefix.length()) == '_'; }

    void mark_function_defined(const string& function) { GBS_write_hash(functions_to_skip, function.c_str(), 1); }
    bool not_defined(const string& function) const { return GBS_read_hash(functions_to_skip, function.c_str()) == 0; }

    const string& get_prefix() const { return prefix; }

    void append_code(const string& code) { GBS_strncat(generated_code, code.c_str(), code.length()); }
    void append_code(const char *code) { append_code(string(code)); }
    void append_code(char code) { GBS_chrcat(generated_code, code); }

    void append_linefeed(size_t count = 1) { while (count--) append_code("\n"); }

    void print_xsubs(FILE *file) {
        fputs("# --------------------------------------------------------------------------------\n", file);
        fprintf(file, "MODULE = ARB  PACKAGE = %s  PREFIX = %s_\n\n", name.c_str(), prefix.c_str());
        fputs(GBS_mempntr(generated_code), file);
    }
};

// ----------------------
//      xsubGenerator

class xsubGenerator {
    Package arb;
    Package bio;

    void generate_xsub(const Prototype& prototype);
public:
    xsubGenerator()
        : arb("ARB", "P2A")
        , bio("BIO", "P2AT")
    {}

    void mark_handcoded_functions(BufferedFileReader& handcoded) {
        string line;
        while (handcoded.getLine(line)) {
            Package *package = NULL;

            if      (arb.matches_package_prefix(line)) package = &arb;
            else if (bio.matches_package_prefix(line)) package = &bio;

            if (package) {
                size_t open_paren = line.find('(');
                if (open_paren != string::npos) {
                    package->mark_function_defined(line.substr(0, open_paren));
                }
            }

        }
        handcoded.rewind();
    }

    void generate_all_xsubs(LineReader& prototype_reader);

    void print_xsubs(FILE *out) {
        arb.print_xsubs(out);
        bio.print_xsubs(out);
    }
};

inline string prefix_before(const string& name, char separator) {
    size_t sep_offset = name.find_first_of(separator);
    if (sep_offset != string::npos) {
        return name.substr(0, sep_offset);
    }
    return "";
}

inline void GBS_spaces(GBS_strstruct *out, int space_count) {
    const char *spaces = "          ";
    arb_assert(space_count <= 10);
    GBS_strncat(out, spaces+(10-space_count), space_count);
}

void xsubGenerator::generate_xsub(const Prototype& prototype) {
    const string&  c_function_name = prototype.get_function_name();
    string         function_prefix = prefix_before(c_function_name, '_');
    Package       *package         = NULL;

    if (function_prefix == "GB" || function_prefix == "GBC") {
        package = &arb;
    }
    else if (function_prefix == "GBT" || function_prefix == "GEN") {
        package = &bio;
    }

    if (package) {
        string perl_function_name = package->get_prefix() + c_function_name.substr(function_prefix.length());

        if (package->not_defined(perl_function_name)) {
            package->mark_function_defined(perl_function_name); // do NOT xsub functions twice

            // generate xsub function header
            const Type& return_type = prototype.get_return_type();
            {
                string argument_names_list = prototype.argument_names_list();
                string function_header     = return_type.isVoid() ? "void" : return_type.perl_decl();

                function_header += '\n';
                function_header += perl_function_name+'('+argument_names_list+")\n";

                ArgumentIter arg_end = prototype.args_end();
                for (ArgumentIter arg = prototype.args_begin(); arg != arg_end; ++arg) {
                    if (!arg->isVoid()) {
                        string type_decl = string("    ") + arg->perl_typed_param() + '\n';
                        function_header += type_decl;
                    }
                }

                package->append_code(function_header);
                package->append_linefeed();
            }

            // generate xsub function body
            string call_c_function = c_function_name+'('+prototype.call_arguments()+")";
            if (return_type.isVoid()) {
                package->append_code("  PPCODE:\n    ");
                package->append_code(call_c_function);
                package->append_code(';');
            }
            else {
                string assign_RETVAL = "    ";

                switch (return_type.get_TypeClass()) {
                    case CONST_CHAR:
                    case CONVERSION_FUNCTION:
                    case SIMPLE_TYPE:
                    case TYPEMAPPED:
                        assign_RETVAL = string("    RETVAL = ") + return_type.convert_result_for_PERL(call_c_function);
                        break;

                    case HEAP_COPY:
                        // temporarily store heapcopy in static pointer
                        // defined at ../PERL2ARB/ARB_ext.c@static_pntr
                        assign_RETVAL =
                            string("    freeset(static_pntr, ") + call_c_function+");\n"+
                            "    RETVAL = static_pntr";
                        break;

                    case VOID:
                    case INVALID_TYPE:
                    case CANT_HANDLE:
                    case FORBIDDEN:
                        arb_assert(0);
                }

                string body =
                    string("  CODE:\n") +
                    assign_RETVAL + ";\n" +
                    "\n" +
                    "  OUTPUT:\n" +
                    "    RETVAL";

                package->append_code(body);
            }
            package->append_linefeed(3);
        }
#if defined(TRACE)
        else {
            fprintf(stderr, "TRACE: '%s' skipped\n", c_function_name.c_str());
        }
#endif // TRACE
    }
#if defined(TRACE)
    else {
        fprintf(stderr, "TRACE: Skipped function: '%s' (prefix='%s')\n", c_function_name.c_str(), function_prefix.c_str());
    }
#endif // TRACE
}

static void print_prototype_parse_error(LineReader& prototype_reader, const char *err, const char *prototype) {
    InputFileError(prototype_reader, GBS_global_string("%s (can't xsub '%s')", err, prototype)).print();
}

void xsubGenerator::generate_all_xsubs(LineReader& prototype_reader) {
    bool   error_occurred     = false;
    string line;
    int    open_brace_counter = 0;

    while (prototype_reader.getLine(line)) {
        const char *lineStart          = line.c_str();
        size_t      leading_whitespace = strspn(lineStart, " \t");
        const char *prototype          = lineStart+leading_whitespace;

        if (!open_brace_counter && is_prototype(prototype)) {
            try {
                Prototype proto(prototype);
                if (proto.possible_as_xsub()) {
                    generate_xsub(proto);
                }
#if defined(TRACE)
                else {
                    fprintf(stderr, "TRACE: prototype '%s' not possible as xsub\n", prototype);
                    proto.dump_types_impossible_in_xsub(stderr);
                }
#endif // TRACE
            }
            catch(string& err) {
                print_prototype_parse_error(prototype_reader, err.c_str(), prototype);
                error_occurred = true;
            }
            catch(const char *err) {
                print_prototype_parse_error(prototype_reader, err, prototype);
                error_occurred = true;
            }
            catch(...) { arb_assert(0); }
        }
        else {
#if defined(TRACE)
            fprintf(stderr, "TRACE: not a prototype: '%s'\n", prototype);
#endif // TRACE
            trace_over_braces(prototype, open_brace_counter);
        }
    }

    if (error_occurred) throw ProgramError("could not generate xsubs for all prototypes");
}

static void print_xs_default(BufferedFileReader& xs_default, const char *proto_filename, FILE *out) {
    fprintf(out,
            "/* This file has been generated from\n"
            " *    %s and\n"
            " *    %s\n */\n"
            "\n",
           xs_default.getFilename().c_str(),
           proto_filename);

    xs_default.copyTo(out);
    xs_default.rewind();
}

static BufferedFileReader *createFileBuffer(const char *filename) {
    FILE *in = fopen(filename, "rt");
    if (!in) {
        GB_export_IO_error("reading", filename);
        throw ProgramError(GB_await_error());
    }
    return new BufferedFileReader(filename, in);
}
static BufferedFileReader *createCommentSkippingFileBuffer(const char *filename) {
    FILE *in = fopen(filename, "rt");
    if (!in) {
        GB_export_IO_error("reading", filename);
        throw ProgramError(GB_await_error());
    }
    return new CommentSkippingFileBuffer(filename, in, "/*", "*/", "//");
}



static void loadTypemap(const char *typemap_filename) {
    SmartPtr<BufferedFileReader> typemap = createFileBuffer(typemap_filename);
    Type::globalTypemap.load(*typemap);
}

int ARB_main(int argc, char **argv)
{
    bool error_occurred = false;
    try {
        if (argc != 4) {
            fputs("arb_proto_2_xsub converts GB_prototypes for the ARB perl interface\n"
                  "Usage: arb_proto_2_xsub <prototypes.h> <xs-header> <typemap>\n"
                  "       <prototypes.h> contains prototypes of ARBDB library\n"
                  "       <xs-header>    may contain prototypes, which will not be\n"
                  "                      overwritten by generated default prototypes\n"
                  "       <typemap>      contains type-conversion-definitions, which can\n"
                  "                      be handled by xsubpp\n"
                  , stderr);

            throw ProgramError("Wrong number of command line arguments");
        }
        else {
            const char *proto_filename   = argv[1];
            const char *xs_default_name  = argv[2];
            const char *typemap_filename = argv[3];

            loadTypemap(typemap_filename);

            // generate xsubs
            SmartPtr<BufferedFileReader> xs_default = createFileBuffer(xs_default_name);

            xsubGenerator generator;
            generator.mark_handcoded_functions(*xs_default);
            {
                SmartPtr<BufferedFileReader> prototypes = createCommentSkippingFileBuffer(proto_filename);
                generator.generate_all_xsubs(*prototypes);
            }

            // write xsubs
            FILE *out = stdout;
            print_xs_default(*xs_default, proto_filename, out);
            generator.print_xsubs(out);
        }
    }
    catch (Error& err) {
        err.print();
        error_occurred = true;
    }
    catch (...) {
        ProgramError("Unexpected exception").print();
        error_occurred = true;
    }

    if (error_occurred) {
        ProgramError("failed").print();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#ifndef TEST_UNIT_H
#include <test_unit.h>
#endif
#include <ut_valgrinded.h>

// #define TEST_AUTO_UPDATE // uncomment this to update expected results

inline GB_ERROR valgrinded_system(const char *cmdline) {
    char *cmddup = ARB_strdup(cmdline);
    make_valgrinded_call(cmddup);

    GB_ERROR error = GBK_system(cmddup);
    free(cmddup);
    return error;
}

void TEST_arb_proto_2_xsub() {
    TEST_EXPECT_ZERO(chdir("xsub"));

    const char *outname  = "ap2x.out";
    const char *expected = "ap2x.out.expected";

    char *cmd = GBS_global_string_copy("arb_proto_2_xsub ptype.header default.xs typemap > %s", outname);
    TEST_EXPECT_NO_ERROR(valgrinded_system(cmd));

#if defined(TEST_AUTO_UPDATE)
    TEST_COPY_FILE(outname, expected);
#else
    TEST_EXPECT_TEXTFILE_DIFFLINES(expected, outname, 0);
#endif
    TEST_EXPECT_ZERO_OR_SHOW_ERRNO(unlink(outname));

    free(cmd);
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------

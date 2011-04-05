// ================================================================ //
//                                                                  //
//   File      : aisc.h                                             //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#ifndef AISC_H
#define AISC_H

#ifndef AISC_PROTO_H
#include "aisc_proto.h"
#endif
#ifndef AISC_PARSER_H
#include "aisc_parser.h"
#endif
#ifndef _UNISTD_H
#include <unistd.h>
#endif

// ------------------------------------------------------------

struct hash_entry {
    char       *key;
    char       *val;
    hash_entry *next;
};

class var_ref {
    hash_entry *e;
public:
    var_ref() : e(NULL) {}
    var_ref(hash_entry *e_) : e(e_) {}

    operator bool() const { return e; } // refer to existing variable ? 

    const char *read() const { return e ? e->val : NULL; }
    void write(const char *val) {
        aisc_assert(e);
        freeset(e->val, nulldup(val));
    }
};

class hash {
    int          size;
    hash_entry **entries;

    int index(const char *key) const;

    const hash_entry *find_entry(const char *key, int idx) const;
    hash_entry *find_entry(const char *key, int idx) {
        return const_cast<hash_entry*>(const_cast<const hash*>(this)->find_entry(key, idx));
    }

public:
    hash(int size_);
    ~hash();

    var_ref ref(const char *key)  { return var_ref(find_entry(key, index(key))); }
    const var_ref ref(const char *key) const { return const_cast<hash*>(this)->ref(key); }

    const char *read(const char *key) const { return ref(key).read(); }
    void write(const char *key, const char *val);
};

// ------------------------------------------------------------

class HeaderList;


class Output {
    FILE *fp;
    char *id;   // internal name used in AISC
    char *name; // file-system name

    bool     have_open_loc; // opened from user code ?
    Location open_loc;
    
    bool terminating;

    bool wasOpened() const { return fp && name; }

    void close_file() {
        if (wasOpened()) {
            if (have_open_loc && terminating) {
                print_error(&open_loc, "file opened here");
                print_error(&Location::guess_pc(), "is still open on exit");
            }
            fclose(fp);
        }
        fp = NULL;
    }
    
    void setup() { fp = NULL; id = NULL; name = NULL; }

    void cleanup() { close_file(); free(id); free(name); }
    void reuse() { cleanup(); setup(); }

public:

    Output() { terminating = false; setup(); }
    ~Output() { terminating = true; cleanup(); }

    bool inUse() const { return fp; }

    void assign(FILE *fp_, const char *id_, const char *name_, const Location *openedAt) {
        aisc_assert(!inUse());
        aisc_assert(fp_);
        aisc_assert(id_);
        fp   = fp_;
        id   = strdup(id_);
        name = strdup(name_);

        have_open_loc = openedAt;
        if (openedAt) open_loc = *openedAt;
    }
    void assign(FILE *fp_, const char *id_, const char *name_) {
        assign(fp_, id_, name_, (const Location*)NULL);
    }
    void assign(FILE *fp_, const char *id_, const char *name_, const Code *openedAt_) {
        assign(fp_, id_, name_, &openedAt_->source);
    }
    
    void assign_stdout(const char *id_) {
        aisc_assert(!inUse());
        fp   = stdout;
        id   = strdup(id_);
        name = NULL;
    }

    void close_and_unlink() {
        close_file();
        if (name) {
            fprintf(stderr, "Unlinking %s\n", name);
            unlink(name);
        }
        reuse();
    }
    void close() { reuse(); }

    bool hasID(const char *Name) const { return id && strcmp(id, Name) == 0; }

    void putchar(char c) { aisc_assert(fp); putc(c, fp); }
    void putstring(const char *s) { for (char c = *s; *s; ++s) { putchar(c); } }
};

class Formatter {
    char outtab[256];
    int  tabstop;
    int  tabpos;
    int  tabs[10];
public: 

    Formatter();

    void set_tabstop(int ts) {
        tabstop = ts;
        for (int i = 0; i < 9; i++) {
            tabs[i] = i * ts;
        }
    }
    void set_tab(int idx, int indent) {
        aisc_assert(idx >= 0 && idx<10);
        tabs[idx] = indent;
    }

    int write(Output& out, const char *str);
};

struct Stack {
    const Token *cursor;
    
    const Code  *pc;
    hash  *hs;
    Stack *next;
};

class Data {
    TokenListBlock *rootBlock;
    const Token    *cursor;
public:

    Data() {
        cursor    = NULL;
        rootBlock = NULL;
    }
    ~Data() { delete rootBlock; }

    void set_tokens(TokenListBlock *newRoot) {
        delete rootBlock;
        rootBlock = newRoot;
    }
    const TokenListBlock *get_tokens() const { return rootBlock; }

    const Token *get_cursor() const { return cursor; }
    void set_cursor(const Token *newCursor) { cursor = newCursor; }

    const Token *find_token(const Token *curs, const char *str, LookupScope scope) const;
    const Token *find_qualified_token(const char *str, LookupScope scope) const;
};

class Interpreter {
    Parser parser;

    Data data;             // currently loaded data

    Code       *prg;       // the complete program
    const Code *pc;        // current program counter
    const Code *nextpc;

    int    stack_size;
    Stack *stack;
    hash  *functions;      // and labels

    Formatter  formatter;
    Output     output[OPENFILES]; // open files
    Output    *current_output;    // pointer to one element of 'output'

    static const int MAX_COMMANDS = 30;
    class Command **command_table;
    void command_table_setup(bool setup);

    void pop();

    Output *find_output_for_ID(const char *fileID) {
        if (fileID) {
            for (int i = 0; i < OPENFILES; i++) {
                if (output[i].hasID(fileID)) return &output[i];
            }
        }
        return NULL;
    }

    void write_var(const char *name, const char *val) { stack->hs->write(name, val); }
    const char *read_var(const char *name) { return stack->hs->read(name); }

    void define_fun(const char *name, const Code *co);
    const Code *find_fun(const char *name);

    int run_program();

    void jump(const Code *to) { nextpc = to; }

    int do_close(const char *str);
    int do_create(const char *str);
    int do_data(const char *str);
    int do_dumpdata(const char *filename);
    int do_error(const char *str) { print_error(at(), str); return 1; }
    int do_exit() { nextpc = 0; return 0; }
    int do_for(const char *str);
    int do_gosub(const char *str);
    int do_goto(const char *str);
    int do_if(char *str);
    int do_moveto(const char *str);
    int do_next();
    int do_open(const char *str);
    int do_out(const char *str);
    int do_pop();
    int do_push();
    int do_return();
    int do_set(const char *str);
    int do_tab(const char *str);
    int do_tabstop(const char *str);
    int do_warning(const char *str) { print_warning(at(), str); return 0; }
    int do_write_current(const char *str) { return formatter.write(*current_output, str); }
    int do_write_stdout(const char *str) { return formatter.write(output[0], str); }

    int            compile_program();
    const Command *find_command(const Code *co);

    bool set_data(const char *filename, int offset_in_line);

public:

    static const Interpreter *instance;

    Interpreter() {
        pc     = NULL;
        nextpc = NULL;

        command_table_setup(true);

        stack_size = 0;
        functions = new hash(HASHSIZE);

        prg   = NULL;
        stack = NULL;

        output[0].assign_stdout("stdout");
        output[1].assign_stdout("*");
        current_output = &output[0];

        ASSERT_RESULT(int, 0, do_push());
        
        aisc_assert(!instance); // singleton!
        instance = this;
    }
    ~Interpreter() {
        aisc_assert(instance == this);
        instance = NULL;

        delete prg;
        delete functions;
        while (stack) pop();
        command_table_setup(false);

    }

    int launch(int argc, char ** argv);


    const Code *at() const { return pc; }
    const Data& get_data() const { return data; }

    const char *read_local(const char *key) const;
    var_ref get_local(const char *key);
};

#else
#error aisc.h included twice
#endif // AISC_H


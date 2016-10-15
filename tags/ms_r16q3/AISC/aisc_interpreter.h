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

    void const_violation();

public:
    var_ref() : e(NULL) {}
    var_ref(hash_entry *e_) : e(e_) {}

    operator bool() const { return e; } // refer to existing variable ?

    bool write_protected() const {
        hash_entry *prot = e->next;
        return prot && strcmp(prot->key, e->key) == 0 && prot->val == NULL;
    }
    void write_protect() {
        if (!write_protected()) {
            hash_entry *prot = (hash_entry *)calloc(sizeof(hash_entry), 1);

            prot->key  = strdup(e->key);
            prot->val  = NULL;
            prot->next = e->next;

            e->next = prot;
        }
    }

    const char *read() const { return e ? e->val : NULL; }
    int write(const char *val) __ATTR__USERESULT {
        aisc_assert(e);

        if (write_protected()) {
            const_violation();
            return -1;
        }
        freeset(e->val, nulldup(val));
        return 0;
    }

};

class hash : virtual Noncopyable {
    int          size;
    hash_entry **entries;

    int Index(const char *key) const;

    const hash_entry *find_entry(const char *key, int idx) const;
    hash_entry *find_entry(const char *key, int idx) {
        return const_cast<hash_entry*>(const_cast<const hash*>(this)->find_entry(key, idx));
    }

public:
    hash(int size_);
    ~hash();

    var_ref ref(const char *key)  { return var_ref(find_entry(key, Index(key))); }
    const var_ref ref(const char *key) const { return const_cast<hash*>(this)->ref(key); }

    const char *read(const char *key) const { return ref(key).read(); }
    void write(const char *key, const char *val);
};

// ------------------------------------------------------------

template<typename T>
inline void realloc_unleaked(T*& ptr, size_t new_size) {
    T *new_ptr = (T*)realloc(ptr, new_size);
    if (!new_ptr) {
        free(ptr);
        throw "out of memory";
    }
    ptr = new_ptr;
}

// ------------------------------------------------------------

static const int  LINEBUFSIZE  = 250;
static const char ALIGN_MARKER = '\1';

class LineBuf : virtual Noncopyable {
    int   size;
    int   used;
    char *buf;
    int   markers;

    void clear() {
        size    = 0;
        used    = 0;
        buf     = NULL;
        markers = 0;
    }

public:
    LineBuf() { clear(); }
    ~LineBuf() { free(buf); }

    void put(char c) {
        if (used >= size) {
            size = size*3/2+1;
            if (size<LINEBUFSIZE) { size = LINEBUFSIZE; }
            realloc_unleaked(buf, size);
        }
        buf[used++] = c;
        if (c == ALIGN_MARKER) ++markers;
    }

    int length() const { return used; }
    char *take() {
        put(0);
        char *b = buf;
        clear();
        return b;
    }

    bool needsAlignment() const { return markers; }
};

class LineQueue : virtual Noncopyable {
    int    count;
    int    size;
    char **queue;

    void clear() {
        for (int i = 0; i<count; ++i) freenull(queue[i]);
        count = 0;
    }
public:
    LineQueue()
        : count(0),
          size(10),
          queue((char**)malloc(size*sizeof(*queue)))
    {}
    ~LineQueue() {
        clear();
        free(queue);
    }

    bool empty() const { return count == 0; }

    void add(char *line) {
        if (count >= size) {
            size  = size*3/2+1;
            realloc_unleaked(queue, size*sizeof(*queue));
        }
        aisc_assert(line[strlen(line)-1] == '\n');
        
        queue[count++] = line;
    }

    void alignInto(LineQueue& dest);

    void flush(FILE *out) {
        for (int i = 0; i<count; ++i) {
            fputs(queue[i], out);
        }
        clear();
    }
};

class Formatter {
    char outtab[256]; // decode table for $x (0 = handle special, character to print otherwise)
    int  tabstop;     // normal tabstop (for $t)
    int  tabs[10];    // predefined tabs ($0..$9) - default to multiples of 'tabstop' if not overridden
    int  column;      // position in line during printing
    int  indent;      // extra indentation
    bool printed_sth;

    LineBuf currentLine;
    LineQueue toAlign; 
    LineQueue spool; 

    void outputchar(char c) {
        currentLine.put(c);
    }

    void print_char(char c) {
        if (!printed_sth && (indent || column)) {
            int ipos = indent*tabstop + column;
            for (int i = 0; i<ipos; ++i) outputchar(' ');
        }
        outputchar(c);
        column++;
        printed_sth = true;
    }

    void tab_to_pos(int pos) {
        if (pos>column) {
            if (printed_sth) {
                while (column < pos) {
                    outputchar(' ');
                    column++;
                }
            }
            else {
                column = pos;
            }
        }
    }

    void align() { if (!toAlign.empty()) toAlign.alignInto(spool); }
    
    void finish_line() {
        outputchar('\n');
        column      = 0;
        printed_sth = false;

        if (currentLine.needsAlignment()) {
            toAlign.add(currentLine.take());
        }
        else {
            align();
            spool.add(currentLine.take());
        }
    }

public:

    Formatter();

    void set_tabstop(int ts) {
        tabstop = ts;
        for (int i = 0; i <= 9; i++) {
            tabs[i] = i * ts;
        }
    }
    void set_tab(int idx, int pos) {
        aisc_assert(idx >= 0 && idx<10);
        tabs[idx] = pos;
    }

    int get_indent() const { return indent; }
    void set_indent(int indent_) { indent = indent_; }

    int write(const char *str);
    void flush(FILE *out) { spool.flush(out); }
    void final_flush(FILE *out) { align(); flush(out); }
};

class Output : virtual Noncopyable {
    FILE *fp;
    char *id;   // internal name used in AISC
    char *name; // file-system name

    bool     have_open_loc; // opened from user code ?
    Location open_loc;

    bool terminating;

    Formatter formatter;

    class PrintMaybe *maybe;

    bool wasOpened() const { return fp && name; }

    void close_file() {
        if (wasOpened()) {
            formatter.final_flush(fp);
            if (have_open_loc && terminating) {
                print_error(&open_loc, "file opened here");
                print_error(&Location::guess_pc(), "is still open on exit");
            }
            fclose(fp);
        }
        fp = NULL;
    }

    void setup();
    void cleanup();
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

    int write(const char *line);
    Formatter& get_formatter() { return formatter; }

    void maybe_start();
    int maybe_write(const char *line);
    int maybe_end();
};

struct Stack {
    const Token *cursor;
    
    const Code  *pc;
    hash  *hs;
    Stack *next;
};

class Data : virtual Noncopyable {
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

    void dump_cursor_pos(FILE *out) const;
};

class Interpreter : virtual Noncopyable {
    Parser parser;

    Data data;             // currently loaded data

    Code       *prg;       // the complete program
    const Code *pc;        // current program counter
    const Code *nextpc;

    int    stack_size;
    Stack *stack;
    hash  *functions;      // and labels

    Output  output[OPENFILES];    // open files
    Output *current_output;       // pointer to one element of 'output'

    static const int MAX_COMMANDS = 32;
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
    Formatter& current_formatter() { return current_output->get_formatter(); }

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
    int do_if(const char *str);
    int do_moveto(const char *str);
    int do_next();
    int do_open(const char *str);
    int do_out(const char *str);
    int do_pop();
    int do_push();
    int do_return();
    int do_set(const char *str);
    int do_makeconst(const char *str);
    int do_tab(const char *str);
    int do_tabstop(const char *str);
    int do_indent(const char *str);
    int do_warning(const char *str) { print_warning(at(), str); return 0; }

    int do_write_current(const char *str) { return current_output->write(str); }
    int do_newline() { return current_output->write(""); }
    int do_write_stdout(const char *str) { return output[0].write(str); }

    int do_write_maybe_start() { current_output->maybe_start(); return 0; }
    int do_write_maybe(const char *str) { return current_output->maybe_write(str); }
    int do_write_maybe_end() { return current_output->maybe_end(); }
    
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




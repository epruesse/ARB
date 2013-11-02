// ================================================================
/*                                                                  */
//   File      : aisc_commands.c
//   Purpose   :
/*                                                                  */
//   Institute of Microbiology (Technical University Munich)
//   http://www.arb-home.de/
/*                                                                  */
// ================================================================

#include "aisc_interpreter.h"
#include "aisc_eval.h"

Location Location::exit_pc;
int      Location::global_error_count = 0;

const Location& Location::guess_pc() {
    if (exit_pc.valid()) return exit_pc;

    if (Interpreter::instance) {
        const Code *pc = Interpreter::instance->at();
        if (pc) return pc->source;
    }

    static Location dummy(0, "no_idea_where");
    return dummy;
}

void Location::print_internal(const char *msg, const char *msg_type, const char *launcher_file, int launcher_line) const {
#if defined(MASK_ERRORS)
    fputs("(disabled) ", stderr);
#endif
    fputs("./", stderr);
    fprint_location(stderr);

    if (msg_type) {
        fputs(msg_type, stderr);
        fputs(": ", stderr);
    }

    if (msg) {
        fputs(msg, stderr);
        if (Interpreter::instance) {
            const Data& data = Interpreter::instance->get_data();
            if (data.get_cursor()) {
                fputs(" (cursor ", stderr);
                data.dump_cursor_pos(stderr);
                fputs(")", stderr);
            }
        }
        fputc('\n', stderr);
    }

    if (launcher_file) {
#if defined(MASK_ERRORS)
        fputs("(disabled) ", stderr);
#endif
        fputs("../AISC/", stderr);
        fprint_location(launcher_file, launcher_line, stderr);
        fprintf(stderr, "%s was launched from here\n", msg_type);
    }
}

#define ERRBUFSIZE 500
const char *formatted(const char *format, ...) {
    // goes to header:  __ATTR__FORMAT(1)

    static char *errbuf = 0;
    if (!errbuf) { errbuf = (char*)malloc(ERRBUFSIZE+1); }

    va_list argPtr;
    va_start(argPtr, format);
    int     chars = vsprintf(errbuf, format, argPtr);
    if (chars>ERRBUFSIZE) {
        fprintf(stderr, "%s:%i: Error: Buffer overflow!\n", __FILE__, __LINE__);
        vfprintf(stderr, format, argPtr);
        fputc('\n', stderr);

        va_end(argPtr);
        exit(EXIT_FAILURE);
    }
    va_end(argPtr);

    return errbuf;
}
#undef ERRBUFSIZE

inline void print_tabs(int count, FILE *out) {
    while (count--) fputc('\t', out);
}

static void write_aisc(const TokenListBlock *block, FILE *out, int indentation) {
    for (const TokenList *list = block->first_list(); list; list = list->next_list()) {
        print_tabs(indentation, out);

        const Token *first = list->first_token();
        for (const Token *tok = first; tok; tok = tok->next_token()) {
            if (tok != first) fputc(',', out);

            if (tok->is_block()) {
                fprintf(out, "'%s'={\n", tok->get_key());
                const TokenListBlock *content = tok->get_content();
                if (content) write_aisc(content, out, indentation + 1);
                print_tabs(indentation+1, out);
                fputc('}', out);
            }
            else {
                fprintf(out, "\t'%s'=(~%s~)",
                        tok->get_key(),
                        tok->has_value() ? tok->get_value() : "");
            }
        }
        fprintf(out, ";\n");
    }
}

// --------------------
//      Interpreter

const Interpreter *Interpreter::instance = NULL;

void Interpreter::define_fun(const char *name, const Code *co) {
    const Code *exists = find_fun(name);
    if (exists) {
        printf_error(co, "Attempt to redefine '%s'", name);
        print_error(exists, "first definition was here");
    }
    else {
        char buffer[100];
        sprintf(buffer, "%li", (long)co);
        functions->write(name, buffer);
    }
}

const Code *Interpreter::find_fun(const char *name) {
    const char *fn = functions->read(name);
    return fn ? (Code*)atol(fn) : NULL;
}


int Interpreter::do_dumpdata(const char *filename) {
    if (!data.get_tokens()) {
        print_error(at(), "DUMPDATA can only be used after DATA");
        return 1;
    }
    FILE *out = stdout;
    if (filename[0]) {
        printf("Dumping data to '%s'\n", filename);
        out = fopen(filename, "wt");
        if (!out) {
            printf_error(at(), "cant write to file '%s' (DUMPDATA)", filename);
            return 1;
        }
    }
    else {
        puts("DUMPDATA:");
    }
    write_aisc(data.get_tokens(), out, 0);
    if (filename[0]) fclose(out);
    return 0;
}

int Interpreter::do_data(const char *str) {
    if (strlen(str) < 2) {
        printf_error(at(), "no parameter '%s'", at()->str);
        return 1;
    }

    if (!set_data(str, 4)) { // hack to correct column reported on errors 
        printf_error(at(), "occurred in following script-part: '%s'", at()->str);
        return 1;
    }
    return 0;
}

int Interpreter::do_indent(const char *str) {
    int diff       = atoi(str);
    int indent     = current_formatter().get_indent();
    int new_indent = indent+diff;

    if (new_indent<0) {
        printf_error(at(), "invalid resulting indentation %i", new_indent);
        return 1;
    }

    current_formatter().set_indent(new_indent);
    return 0;
}

int Interpreter::do_tabstop(const char *str) {
    int ts = atoi(str);
    if ((ts < 1) || (ts > 1024)) {
        printf_error(at(), "illegal TABSTOP %i (legal: [1..1024])", ts);
        return 1;
    }

    current_formatter().set_tabstop(ts);
    return 0;
}

class ArgParser : virtual Noncopyable {
    char            *args;
    char            *strtok_arg;
    const Location&  loc;

    void init(const char *a) { strtok_arg = args = strdup(a); }
public:
    ArgParser(const char *arguments, const Location& loc_) : loc(loc_) { init(arguments); }
    ArgParser(const char *arguments, const Code *co) : loc(co->source) { init(arguments); }
    ~ArgParser() { free(args); }

    const char *get(const char *paramDescription) {
        const char *res = strtok(strtok_arg, " \t,;\n");
        strtok_arg      = NULL;
        if (!res) printf_error(&loc, "Missing argument '%s'", paramDescription);
        return res;
    }
};

int Interpreter::do_tab(const char *str) {
    ArgParser args(str, at());

    const char *s1 = args.get("TABCOUNT");
    if (s1) {
        const char *s2 = args.get("TABVALUE");
        if (s2) {
            int ts = atoi(s1);
            if ((ts < 0) || (ts > 9)) print_error(at(), "wrong TABCOUNT");
            else {
                int val = atoi(s2);
                if ((val < 0) || (val > 1000)) print_error(at(), "wrong TABVALUE");
                else {
                    current_formatter().set_tab(ts, val);
                    return 0;
                }
            }
        }
    }
    return 1;
}

int Interpreter::do_open(const char *str) {
    ArgParser args(str, at());

    const char *fileID = args.get("fileID");
    if (fileID) {
        const char *fn = args.get("filename");
        if (strlen(fn) < 3) printf_error(at(), "Filename '%s' too short (<3)", fn);
        else {
            Output *alreadyOpen = find_output_for_ID(fileID);
            if (alreadyOpen) printf_error(at(), "File '%s' already opened", fileID);
            else {
                int i;
                for (i = 0; i < OPENFILES; i++) {
                    if (!output[i].inUse()) break;
                }
                if (i == OPENFILES) print_error(at(), "Too many open files");
                else {
                    FILE *file = fopen(fn, "wt");
                    if (file) {
                        output[i].assign(file, fileID, fn, at());
                        return 0;
                    }
                    printf_error(at(), "Cannot write to file '%s' (OPEN)", fn);
                }
            }
        }
    }
    return 1;
}

int Interpreter::do_close(const char *str) {
    ArgParser   args(str, at());
    const char *fileID = args.get("fileID");

    if (fileID) {
        Output *known = find_output_for_ID(fileID);
        if (known) {
            known->close();
            return 0;
        }
        printf_error(at(), "File '%s' not open or already closed", str);
    }
    return 1;
}

int Interpreter::do_out(const char *str) {
    ArgParser   args(str, at());
    const char *fileID = args.get("fileID");

    if (fileID) {
        Output *known = find_output_for_ID(fileID);
        if (known) {
            current_output = known;
            return 0;
        }
        printf_error(at(), "File '%s' not open or already closed", str);
    }
    return 1;
}

inline char *get_ident(const char*& code, const Code *at) {
    const char *start  = strstr(code, "$(");
    if (!start) {
        print_error(at, "expected to see '$(' while parsing for ident/selector");
        return NULL;
    }
    const char *end = strchr(start+2, ')');
    if (!end) {
        print_error(at, "missing closing ')' after ident/selector");
        return NULL;
    }

    code = end+1;

    char *ident = copy_string_part(start+2, end-1);
    if (!ident[0]) {
        print_error(at, "Empty ident/selector");
        freenull(ident);
    }
    return ident;
}

int Interpreter::do_moveto(const char *str) {
    int   err      = 1;
    char *selector = get_ident(str, at());
    if (selector) {
        LookupScope  scope = strrchr(selector, '/') ? LOOKUP_LIST : LOOKUP_BLOCK;
        const Token *fo    = data.find_qualified_token(selector, scope);
        if (!fo) {
            printf_error(at(), "Could not find data '%s'", selector);
        }
        else {
            data.set_cursor(fo);
            err = 0;
        }
        free(selector);
    }
    return err;
}

void var_ref::const_violation() {
    printf_error(Interpreter::instance->at(),
                 "Attempt to modify write-protected variable '%s'", e->key);
}

int Interpreter::do_makeconst(const char *str) {
    int   err   = 1;
    char *ident = get_ident(str, at());

    if (ident) {
        var_ref var = get_local(ident);
        if (!var) {
            printf_error(at(), "undefined Ident '%s' in CONST (use CREATE first)", ident);
        }
        else {
            var.write_protect();
        }
        free(ident);
    }

    return err;
}

int Interpreter::do_set(const char *str) {
    int   err   = 1;
    char *ident = get_ident(str, at());
    if (ident) {
        var_ref var = get_local(ident);
        if (!var) {
            printf_error(at(), "undefined Ident '%s' in SET (use CREATE first)", ident);
        }
        else {
            SKIP_SPACE_LF(str);
            if (str[0] == '=') {
                ++str;
                SKIP_SPACE_LF(str);
            }
            err = var.write(str);
        }
        free(ident);
    }
    return err;
}

int Interpreter::do_create(const char *str) {
    char *var = get_ident(str, at());
    const char *def = read_var(var);

    int result = 1;
    if (def) printf_error(at(), "Ident '%s' in CREATE already defined", var);
    else {
        SKIP_SPACE_LF(str);
        if (*str == '=') {
            str++;
            SKIP_SPACE_LF(str);
        }
        write_var(var, str);
        result = 0;
    }
    free(var);
    return result;
}

int Interpreter::do_if(const char *str) {
    int  err       = 0;
    bool condition = false;

    if (str) { // expression is not undefined
        const char *cursor     = strpbrk(str, "=~<");
        if (!cursor) condition = true; // if no operator found -> assume condition true (even if empty)
        else {
            char op = *cursor;
            aisc_assert(cursor>str);
            bool negate = cursor[-1] == '!';

            const char *la = cursor - negate;
            --la;
            SKIP_SPACE_LF_BACKWARD(la, str);

            char *left = copy_string_part(str, la);
            cursor++;

            char        right_buffer[strlen(cursor)+1];
            const char *right = right_buffer;

            while (cursor && !condition) {
                SKIP_SPACE_LF(cursor);
                const char *kom  = strchr(cursor, ',');
                const char *next = kom;

                if (kom) {
                    next++;
                    --kom;
                    SKIP_SPACE_LF_BACKWARD(kom, str);

                    int len    = kom-cursor+1;
                    memcpy(right_buffer, cursor, len);
                    right_buffer[len] = 0;
                }
                else {
                    right = cursor;
                }

                switch (op) {
                    case '=': condition = strcmp(left, right) == 0; break;
                    case '~': condition = strstr(left, right) != 0; break;
                    case '<': condition = atoi(left) < atoi(right); break;
                    default:
                        print_error(at(), formatted("Unhandled operator (op='%c') applied to '%s'", op, str));
                        err = 1;
                        break;
                }

                if (err) break;
                if (negate) condition = !condition;
                cursor = next;
            }

            free(left);
        }
    }
    
    if (!err && !condition) jump(at()->ELSE->next);
    return err;
}

struct for_data {
    char        *forstr;
    const Token *forcursor;
    long         forval;
    long         forend;
    for_data    *next;
};

inline for_data& add_for_data_to(const Code *co) {
    for_data *fd = (for_data *)calloc(sizeof(for_data), 1);

    fd->next = co->fd;
    co->fd   = fd;

    return *fd;
}

inline void remove_for_data_from(const Code *co) {
    for_data *fd = co->fd;
    free(fd->forstr);
    co->fd = fd->next;
    free(fd);
}

int Interpreter::do_push() {
    if (stack_size++ >= STACKSIZE) {
        print_error(at(), "Stack size exceeded");
        return 1;
    }

    Stack *st = (Stack *)calloc(sizeof(Stack), 1);

    st->cursor = data.get_cursor();
    st->pc     = at();
    st->hs     = new hash(HASHSIZE);
    st->next   = stack;

    stack = st;
    
    return 0;
}

void Interpreter::pop() {
    aisc_assert(stack_size>0);

    Stack *st = stack;
    if (st) {
        delete st->hs;
        data.set_cursor(st->cursor);
        stack = st->next;
        free(st);
        stack_size--;
    }
}

int Interpreter::do_pop() {
    if (stack_size<2) {
        print_error(at(), "Nothing to Pop");
        return 1;
    }
    pop();
    return 0;
}

int Interpreter::do_gosub(const char *str) {
    int result = 1;
    if (do_push() == 0) {
        char *params;
        char *fun_name;

        {
            const char *s;
            for (s = str; !is_SPACE_SEP_LF_EOS(*s); s++) ;

            fun_name = copy_string_part(str, s-1);

            if (*s) {
                s++;
                SKIP_SPACE_LF(s);
                params = strdup(s);
            }
            else {
                params = NULL;
            }
        }

        const Code *fun = find_fun(fun_name);
        if (!fun) {
            printf_error(at(), "Function '%s' not found", fun_name);
        }
        else {
            bool        eval_failed;
            char       *fpara_eval = Expression(data, fun->source, fun->str, false).evaluate(eval_failed);

            const char *fpara  = fpara_eval;
            SKIP_SPACE_LF(fpara);
            if (!*fpara) fpara = 0;

            char       *npara  = 0;
            const char *nfpara = 0;

            int err = eval_failed;
            for (char *para = params; !err && para; para=npara, fpara=nfpara) {
                if (!fpara) {
                    printf_error(at(), "Too many Parameters '%s'", para);
                    printf_error(fun, "in call to '%s'", fun_name);

                    err = -1;
                }
                else {
                    {
                        char *s;
                        for (s = para; !is_SEP_LF_EOS(*s); s++) ;
                        if (*s) {
                            *s = 0;
                            npara = s+1;
                            SKIP_SPACE_LF(npara);
                        }
                        else npara = NULL;
                    }

                    char *fpara_name;
                    {
                        const char *s;
                        for (s = fpara; !is_SEP_LF_EOS(*s); s++) ;
                        fpara_name = copy_string_part(fpara, s-1);
                        if (*s) {
                            nfpara = s+1;
                            SKIP_SPACE_LF(nfpara);
                        }
                        else nfpara = NULL;
                    }

                    char *para_eval = Expression(data, at()->source, para, false).evaluate(eval_failed);

                    if (eval_failed) {
                        print_error(at(), formatted("Could not evaluate expression '%s' for parameter '%s'", para, fpara_name));
                    }
                    else {
                        const char *s = read_var(fpara_name);
                        if (s) {
                            print_error(fun, formatted("duplicated formal parameter '%s' in definition of function '%s'", fpara_name, fun_name));
                            err = -1;
                        }
                        else {
                            write_var(fpara_name, para_eval);
                        }
                    }
                    free(para_eval);
                    free(fpara_name);
                }
            }

            if (!err && fpara) {
                printf_error(at(), "Missing parameter '%s'", fpara);
                printf_error(fun, "in call to '%s'", fun_name);
                err = -1;
            }

            if (!err) jump(fun->next);
            result = err;
            free(fpara_eval);
        }
        free(fun_name);
        free(params);
    }
    return result;
}

int Interpreter::do_goto(const char *str) {
    const Code *func = find_fun(str);
    if (!func) {
        printf_error(at(), "Function '%s' not found", str);
        return 1;
    }
    jump(func->next);
    return 0;
}

int Interpreter::do_return() {
    jump(stack->pc->next);
    return do_pop();
}

int Interpreter::do_for(const char *str) {
    int   err   = 1;
    char *ident = get_ident(str, at());
    if (ident) {
        const char *eq = strchr(str, '=');
        if (eq) {
            ++eq;
            SKIP_SPACE_LF(eq);

            const char *to = strstr(eq, "TO");
            if (!to) print_error(at(), "TO not found in FOR - expecting e.g. 'FOR $(i) = a TO b'");
            else {
                to  += 2;
                SKIP_SPACE_LF(to);

                for_data& fd = add_for_data_to(pc);
                fd.forval    = atol(eq);
                fd.forend    = atol(to);

                if (fd.forval > fd.forend) {
                    nextpc = pc->NEXT->next;
                    remove_for_data_from(pc);
                    err = 0;
                }
                else {
                    var_ref var = get_local(ident);
                    if (!var) {
                        printf_error(at(), "Undefined Ident '%s' in FOR (use CREATE first)", ident);
                    }
                    else {
                        err       = var.write(formatted("%li", fd.forval));
                        fd.forstr = strdup(ident);
                    }
                }
                // cppcheck-suppress memleak (fd.forstr is released from Interpreter::do_next)
            }
        }
        else {
            const char  *p  = strrchr(ident, '/');
            const Token *fo = data.find_qualified_token(ident, p ? LOOKUP_LIST : LOOKUP_BLOCK);
            if (!fo) {
                nextpc = pc->NEXT->next;
            }
            else {
                for_data& fd = add_for_data_to(pc);
                fd.forstr    = strdup(p ? p+1 : ident);
                fd.forcursor = data.get_cursor();
                data.set_cursor(fo);
                // cppcheck-suppress memleak (fd.forstr is released from Interpreter::do_next)
            }
            err = 0;
        }
        
        free(ident);
    }
    return err;
}

int Interpreter::do_next() {
    // handles NEXT (end of for-loop) and CONTINUE (loop from inside)

    for_data& fd = *pc->FOR->fd;
    if (fd.forcursor) {
        const Token *fo = data.find_token(data.get_cursor(), fd.forstr, LOOKUP_BLOCK_REST);
        if (fo) {
            nextpc = pc->FOR->next;
            data.set_cursor(fo);
            return 0;
        }
        data.set_cursor(fd.forcursor);
    }
    else {
        if (fd.forval < fd.forend) {
            fd.forval++;
            nextpc = pc->FOR->next;
            return get_local(fd.forstr).write(formatted("%li", fd.forval));
        }

    }
    nextpc = pc->FOR->ENDFOR->next;
    remove_for_data_from(pc->FOR);
    return 0;
}

// -------------------
//      Dispatcher

typedef int (Interpreter::*NoArgFun)();
typedef int (Interpreter::*ArgFun)(const char *);

enum CallMode {
    STANDARD_CALL = 0,

    // bit values:
    DONT_EVAL           = 1,
    EVAL_VAR_DECL       = 2,
    EVAL_PLAIN          = 4,
    TERMINATED_ON_ERROR = 8,
};

const int DISPATCH_LIMIT = 5000000; // abort after 5mio dispatches to one command-type (DEBUG only)

struct Command {
    virtual ~Command() {}
    virtual bool matches(const char *code) const   = 0;
    virtual int call(Interpreter& interpret) const = 0;

    const Location& pc() const {
        return Interpreter::instance->at()->source;
    }
};
struct NoSuchCommand : public Command {
    virtual bool matches(const char *) const OVERRIDE { return true; } // catch all
    virtual int call(Interpreter& interpret) const OVERRIDE {
        printf_error(interpret.at(), "Unknown command '%s'", interpret.at()->str);
        return -1; // bail out
    }
};
class NamedCommand : public Command, virtual Noncopyable {
    const char *name;
    int         len;
    mutable int dispatched;
protected:
    virtual int check_result(int res, char *evaluated_args) const {
        dispatched++;
        if (dispatched>DISPATCH_LIMIT) {
            print_error(&pc(), "possible deadlock detected");
            res = -1;
        }
#if defined(TRACE)
        pc().start_message("TRACE");
        if (res) fprintf(stderr, "[result=%i]", res);
        fputs(name, stderr);
        if (evaluated_args) {
            fputc(' ', stderr);
            fputs(evaluated_args, stderr);
        }
        fputc('\n', stderr);
        
#endif
        free(evaluated_args);
        return res;
    }
public:
    NamedCommand(const char *cmd) : name(cmd), len(strlen(name)), dispatched(0) {}
    bool matches(const char *code) const OVERRIDE { return strncmp(code, name, len) == 0; }
    int length() const { return len; }
};

class SimpleCmmd : public NamedCommand {
    NoArgFun fun;
public:
    SimpleCmmd(const char *cmd, NoArgFun fun_) : NamedCommand(cmd), fun(fun_) {}
    virtual int call(Interpreter& interpret) const OVERRIDE {
        return check_result((interpret.*fun)(), NULL);
    }
};

class ArgCommand : public NamedCommand {
    ArgFun   fun;
    CallMode emode;

    char *eval(Interpreter& interpret, bool& failed) const {
        int offset = length();
        if (!(emode&EVAL_PLAIN)) offset++;

        if (emode & DONT_EVAL) {
            failed = false;
            return strdup(interpret.at()->str+offset);
        }

        Expression expr(interpret.get_data(), interpret.at()->source, interpret.at()->str+offset, false);
        return (emode&EVAL_VAR_DECL)
            ? expr.evalVarDecl(failed)
            : expr.evaluate(failed);
    }
    virtual int check_result(int res, char *evaluated_args) const OVERRIDE {
        return NamedCommand::check_result(res && (emode&TERMINATED_ON_ERROR) ? -1 : res, evaluated_args);
    }
    
public:
    ArgCommand(const char *cmd, ArgFun fun_, CallMode emode_ = STANDARD_CALL)
        : NamedCommand(cmd), fun(fun_), emode(emode_) {}
    virtual int call(Interpreter& interpret) const OVERRIDE {
        int   res  = 1;
        bool eval_failed;
        char *args = eval(interpret, eval_failed);
        if (args && !eval_failed)  {
            char *trimmed = args;
            if (!(emode&EVAL_PLAIN)) SKIP_SPACE_LF(trimmed);
            
            res = (interpret.*fun)(trimmed);
        }
        else {
            print_error(interpret.at(), "Failed to evaluate arguments");
            res = -1;
        }
        return check_result(res, args);
    }
};


void Interpreter::command_table_setup(bool setup) {
    if (setup) {
        command_table = new Command*[MAX_COMMANDS+1];

        int i = 0;
        
        command_table[i++] = new ArgCommand("MOVETO",   &Interpreter::do_moveto,        EVAL_VAR_DECL);
        command_table[i++] = new ArgCommand("SET",      &Interpreter::do_set,           EVAL_VAR_DECL);
        command_table[i++] = new ArgCommand("CONST",    &Interpreter::do_makeconst,     EVAL_VAR_DECL);
        command_table[i++] = new ArgCommand("CREATE",   &Interpreter::do_create,        EVAL_VAR_DECL);

        command_table[i++] = new ArgCommand("PRINT",    &Interpreter::do_write_current, EVAL_PLAIN);
        command_table[i++] = new ArgCommand("P ",       &Interpreter::do_write_current, EVAL_PLAIN);
        command_table[i++] = new ArgCommand("PP",       &Interpreter::do_write_stdout);
        command_table[i++] = new SimpleCmmd("--",       &Interpreter::do_newline);
        command_table[i++] = new SimpleCmmd("PMSTART",  &Interpreter::do_write_maybe_start);
        command_table[i++] = new SimpleCmmd("PMEND",    &Interpreter::do_write_maybe_end);
        command_table[i++] = new ArgCommand("PM",       &Interpreter::do_write_maybe);

        command_table[i++] = new ArgCommand("GOSUB",    &Interpreter::do_gosub,         DONT_EVAL);
        command_table[i++] = new ArgCommand("CALL",     &Interpreter::do_gosub,         DONT_EVAL);
        command_table[i++] = new ArgCommand("GOTO",     &Interpreter::do_goto);
        command_table[i++] = new SimpleCmmd("RETURN",   &Interpreter::do_return);
        command_table[i++] = new SimpleCmmd("PUSH",     &Interpreter::do_push);
        command_table[i++] = new SimpleCmmd("POP",      &Interpreter::do_pop);
        command_table[i++] = new SimpleCmmd("CONTINUE", &Interpreter::do_next);

        command_table[i++] = new ArgCommand("OPEN",     &Interpreter::do_open,         TERMINATED_ON_ERROR);
        command_table[i++] = new ArgCommand("CLOSE",    &Interpreter::do_close);
        command_table[i++] = new ArgCommand("OUT",      &Interpreter::do_out,          TERMINATED_ON_ERROR);

        command_table[i++] = new ArgCommand("INDENT",   &Interpreter::do_indent);
        command_table[i++] = new ArgCommand("TABSTOP",  &Interpreter::do_tabstop);
        command_table[i++] = new ArgCommand("TAB",      &Interpreter::do_tab);
        command_table[i++] = new SimpleCmmd("EXIT",     &Interpreter::do_exit);

        command_table[i++] = new ArgCommand("DATA",     &Interpreter::do_data,         TERMINATED_ON_ERROR);
        command_table[i++] = new ArgCommand("DUMPDATA", &Interpreter::do_dumpdata,     TERMINATED_ON_ERROR);
        command_table[i++] = new ArgCommand("ERROR",    &Interpreter::do_error,        TERMINATED_ON_ERROR);
        command_table[i++] = new ArgCommand("WARNING",  &Interpreter::do_warning);

        command_table[i++] = new NoSuchCommand; // should be last!
        command_table[i++] = NULL;

        aisc_assert(i<MAX_COMMANDS);
    }
    else { // cleanup
        for (int i = 0; command_table[i]; ++i) delete command_table[i];
        delete [] command_table;
    }
}

const Command *Interpreter::find_command(const Code *co) {
    for (int c = 0; command_table[c]; ++c) {
        const Command& cmd = *command_table[c];
        if (cmd.matches(co->str)) {
            return &cmd;
        }
    }
    aisc_assert(0); // NoSuchCommand missing ? 
    return NULL;
}

int Interpreter::run_program() {
    parser.set_source(prg->source.get_path(), 1);

    for (pc = prg; pc; pc = nextpc) {
        nextpc = pc->next;

        switch (pc->command) {
            case CT_IF: {
                Expression  expr(data, pc->source, pc->str, true);
                bool        eval_failed;
                char       *val = expr.evaluate(eval_failed);
                int         err = eval_failed;
                if (!err) err = do_if(val);   // execute even with val == NULL!
                free(val);
                if  (err) return err;
                break;
            }

            case CT_ELSE:
                nextpc = pc->IF->ENDIF->next;
            case CT_ENDIF:
                break;

            case CT_FOR: {
                Expression  expr(data, pc->source, pc->str, false);
                bool        eval_failed;
                char       *val   = expr.evalVarDecl(eval_failed);
                bool        abort = !val || eval_failed || do_for(val);
                free(val);
                if (abort) return 1;
                break;
            }

            case CT_NEXT:
                if (do_next()) return 1;
            case CT_ENDFOR:
                break;

            case CT_FUNCTION:
                print_error(at(), "fatal: ran into FUNCTION (missing EXIT?)");
                break;

                break;

            case CT_OTHER_CMD: {
                int res = pc->cmd->call(*this);
                if (res == -1) return 1;
                break;
            }

            case CT_LABEL:
            case CT_ELSEIF:
            case NO_COMMAND:
                printf_error(at(), "internal error: Expected not to reach command type=%i", pc->command);
                return 1;
        }

        if (!nextpc) { // end of execution ?
            Location::announce_exit_pc(pc->source);
        }
    }
    return Location::get_error_count();
}


// ================================================================
/*                                                                  */
//   File      : aisc.c
//   Purpose   : ARB integrated source compiler
/*                                                                  */
//   Institute of Microbiology (Technical University Munich)
//   http://www.arb-home.de
/*                                                                  */
// ================================================================

#include "aisc_interpreter.h"
#include <cctype>
#include <list>
#include <string>

using namespace std;

// AISC_MKPT_PROMOTE:#ifndef AISC_DEF_H
// AISC_MKPT_PROMOTE:#include "aisc_def.h"
// AISC_MKPT_PROMOTE:#endif

char *read_aisc_file(const char *path, const Location *loc) {
    char *buffer = 0;
    FILE *input  = fopen(path, "rt");

    if (!input) {
        printf_error(loc, "file '%s' not found", path);
    }
    else {
        if (fseek(input, 0, 2)==-1) {
            printf_error(loc, "file '%s' not seekable", path);
        }
        else {
            int data_size = (int)ftell(input);

            if (data_size == 0) {
                Location fileLoc(0, path);
                print_error(&fileLoc, "file is empty");
            }
            else {
                data_size++;
                rewind(input);
                buffer =  (char *)malloc(data_size+1);
                data_size = fread(buffer, 1, data_size, input);
                buffer[data_size] = 0;
            }
        }
        fclose(input);
    }
    return buffer;
}

inline int max(int i, int j) { return i<j ? j : i; }

void LineQueue::alignInto(LineQueue& dest) {
    int offset = 0;
    int len[count];

    for (int i = 0; i<count; ++i) len[i] = strlen(queue[i]);

    while (1) {
        int max_mark_pos = -1;
        int mark_pos[count];
        for (int i = 0; i<count; ++i) {
            char *line   = queue[i];
            char *mark   = len[i]>offset ? strchr(line+offset, ALIGN_MARKER) : NULL;
            mark_pos[i]  = mark ? mark-line : -1;
            max_mark_pos = max(max_mark_pos, mark_pos[i]);
        }
        if (max_mark_pos == -1) break;

        for (int i = 0; i<count; ++i) {
            if (mark_pos[i] >= 0) {
                int insert = max_mark_pos-mark_pos[i];
                aisc_assert(insert >= 0);
                if (insert >= 0) {
                    int   new_len  = len[i]+insert;
                    char *new_line = (char*)malloc(new_len+1);

                    memcpy(new_line, queue[i], mark_pos[i]);
                    memset(new_line+mark_pos[i], ' ', insert);
                    strcpy(new_line+max_mark_pos, queue[i]+mark_pos[i]+1);

                    len[i] = new_len;
                    freeset(queue[i], new_line);
                }
            }
        }

        offset = max_mark_pos;
    }

    for (int i = 0; i<count; ++i) {
        dest.add(queue[i]);
        queue[i] = NULL;
    }
    count = 0;
}

struct queued_line {
    string line;
    int    indentation;
    queued_line(const char *line_, int indent) : line(line_), indentation(indent) { }
};

typedef list<queued_line> PrintQueue;

class PrintMaybe : virtual Noncopyable {
    Output&          out;
    const Location&  started_at;
    bool             printed_sth;
    PrintMaybe      *next;
    PrintQueue       queue;

public:
    PrintMaybe(PrintMaybe *head, Output& out_, const Location& loc)
        : out(out_),
          started_at(loc), 
          printed_sth(false),
          next(head)
    {
    }
    ~PrintMaybe() {
    }

    void add(const char *line) {
        if (printed_sth) {
            out.write(line);
        }
        else {
            queue.push_back(queued_line(line, out.get_formatter().get_indent()));
        }
    }
    void spool() {
        aisc_assert(!printed_sth);
        printed_sth = true;
        if (!queue.empty()) {
            Formatter& formatter  = out.get_formatter();
            int        old_indent = formatter.get_indent();
            for (PrintQueue::iterator i = queue.begin(); i != queue.end(); ++i) {
                queued_line& ql = *i;
                formatter.set_indent(ql.indentation);
                out.write(ql.line.c_str());
            }
            formatter.set_indent(old_indent);
            queue.clear();
        }
    }

    void will_print() {
        if (next) next->will_print();
        if (!printed_sth) {
            spool();
        }
    }

    void not_destroyed_error() {
        print_error(&started_at, "PMSTART without matching PMEND");
        if (next) next->not_destroyed_error();
    }

    static void pop(PrintMaybe*& head) {
        PrintMaybe *next = head->next;
        head->next       = NULL;
        delete head;
        head             = next;
    }
};

void Output::setup() {
    fp    = NULL;
    id    = NULL;
    name  = NULL;
    maybe = NULL;

    have_open_loc = false;
}
void Output::cleanup() {
    close_file();
    free(id);
    free(name);
    if (maybe) {
        maybe->not_destroyed_error();
        delete maybe;
    }
}

void Output::maybe_start() {
    maybe = new PrintMaybe(maybe, *this, Interpreter::instance->at()->source);
}
int Output::maybe_write(const char *line) {
    if (!maybe) {
        print_error(Interpreter::instance->at(), "no PMSTART before PM");
        return -1;
    }
    maybe->add(line);
    return 0;
}
int Output::maybe_end() {
    if (!maybe) {
        print_error(Interpreter::instance->at(), "no PMSTART before PMEND");
        return -1;
    }
    PrintMaybe::pop(maybe);
    return 0;
}

int Output::write(const char *line) {
    if (!inUse()) {
        print_error(Interpreter::instance->at(), "Fatal: attempt to write to unused file");
        return -1;
    }

    if (maybe) maybe->will_print();

    int res = formatter.write(line);
    formatter.flush(fp);
    return res;
}

Formatter::Formatter()
    : tabstop(4),
      column(0),
      indent(0),
      printed_sth(false)
{
    set_tabstop(4);

    for (int i = 0; i < 256; i++) {
        outtab[i] = i;
    }

    outtab[(unsigned char)'n']  = '\n';
    outtab[(unsigned char)'t']  = '\t';
    outtab[(unsigned char)'|']  = ALIGN_MARKER;
    outtab[(unsigned char)'0']  = 0;
    outtab[(unsigned char)'1']  = 0;
    outtab[(unsigned char)'2']  = 0;
    outtab[(unsigned char)'3']  = 0;
    outtab[(unsigned char)'4']  = 0;
    outtab[(unsigned char)'5']  = 0;
    outtab[(unsigned char)'6']  = 0;
    outtab[(unsigned char)'7']  = 0;
    outtab[(unsigned char)'8']  = 0;
    outtab[(unsigned char)'9']  = 0;
    outtab[(unsigned char)'\\'] = 0;
}

int Formatter::write(const char *str) {
    int         no_nl = 0;
    const char *p     = str;
    char        c;

    while ((c = *(p++))) {
        if (c == '$') {
            c = *(p++);
            if (!c)
                break;

            if (!outtab[(unsigned)(c)]) {
                if (c == '\\') {
                    no_nl = 1;
                }
                else if (isdigit(c)) {
                    int pos = tabs[c - '0'];
                    tab_to_pos(pos);
                }
                continue;
            }
            else {
                c = outtab[(unsigned)(c)];
            }
        }
        if (c == '\t') {
            int pos = ((column/tabstop)+1)*tabstop;
            tab_to_pos(pos);
        }
        else if (c == '\n') {
            if (no_nl) {
                no_nl = 0;
            }
            else {
                finish_line();
            }
        }
        else if (c == '@') {
            if (strncmp(p, "SETSOURCE", 9) == 0) { // skip '@SETSOURCE file, line@'
                p = strchr(p, '@');
                if (!p) {
                    print_error(Interpreter::instance->at(), "expected '@' after '@SETSOURCE' (injected code)");
                    return 1;
                }
                p++;
            }
            else {
                print_char(c);
            }
        }
        else {
            print_char(c);
        }
    }
    if (!no_nl) {
        finish_line();
    }
    return 0;
}

bool Interpreter::set_data(const char *dataBlock, int offset_in_line) {
    parser.set_line_start(dataBlock, offset_in_line);
    parser.set_source(at()->source); // remove ? 
    data.set_tokens(parser.parseTokenListBlock(dataBlock));
    return data.get_tokens();
}


int Interpreter::launch(int argc, char ** argv) {
    int exitcode = EXIT_FAILURE;

    // save CL-arguments as variables (with names like 'argv[0]' ..) 
    {
        for (int i=0; i<argc; i++) {
            write_var(formatted("argv[%i]", i), argv[i]);
        }
        write_var("argc", formatted("%i", argc));
    }

    {
        char *buf = read_aisc_file(argv[1], NULL);
        if (buf) {
            prg = parser.parse_program(buf, argv[1]);
            free(buf);
        }
    }
    
    if (!prg) {
        fputs("Nothing to execute\n", stderr);
    }
    else {
        if (compile_program()) {
            fprintf(stderr, "Compilation of '%s' failed\n", argv[1]);
        }
        else {
            if (run_program()) {
                if (!Location::get_error_count()) {
                    print_error(at(), "AISC compiler bailed out w/o error");
                }
                fputs("AISC reports errors\n", stderr);
                for (int i = 0; i < OPENFILES; i++) output[i].close_and_unlink();
                fflush(stdout);
            }
            else {
                aisc_assert(!Location::get_error_count());
                exitcode = EXIT_SUCCESS;
            }
        }
    }

    return exitcode;
}

int main(int argc, char ** argv) {
    int exitcode = EXIT_FAILURE;

    if (argc < 2) {
        fprintf(stderr, "AISC - ARB integrated source compiler\n");
        fprintf(stderr, "Usage: aisc [fileToCompile]+\n");
        fprintf(stderr, "Error: missing file name\n");
    }
    else {
        try {
            exitcode = Interpreter().launch(argc, argv);
        }
        catch (const char *err) {
            fprintf(stderr, "\nAISC: exception: %s [terminating]\n", err);
            exitcode = EXIT_FAILURE;
        }
        catch (...) {
            fprintf(stderr, "\nAISC: unknown exception [terminating]\n");
            exitcode = EXIT_FAILURE;
        }
    }
    return exitcode;
}



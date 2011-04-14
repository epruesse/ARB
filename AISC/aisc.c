/* ================================================================ */
/*                                                                  */
/*   File      : aisc.c                                             */
/*   Purpose   : ARB integrated source compiler                     */
/*                                                                  */
/*   Institute of Microbiology (Technical University Munich)        */
/*   http://www.arb-home.de/                                        */
/*                                                                  */
/* ================================================================ */

#include "aisc_interpreter.h"

// AISC_MKPT_PROMOTE:#ifndef AISC_DEF_H
// AISC_MKPT_PROMOTE:#include "aisc_def.h"
// AISC_MKPT_PROMOTE:#endif

char string_buf[256];

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

Formatter::Formatter() {
    tabstop = 8;
    tabpos  = 0;
    set_tabstop(4);

    for (int i = 0; i < 256; i++) {
        outtab[i] = i;
    }

    outtab[(unsigned char)'n']  = '\n';
    outtab[(unsigned char)'t']  = '\t';
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

int Formatter::write(Output& out, const char *str) {
    if (!out.inUse()) {
        print_error(Interpreter::instance->at(), "Fatal: attempt to write to unused file");
        return -1;
    }

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
                    continue;
                }
                if ((c >= '0') && (c <= '9')) {
                    int pos = tabs[c - '0'];
                    if (pos < tabpos)
                        continue;

                    int lt = tabpos / tabstop;
                    int rt = pos / tabstop;

                    while (lt < rt) {
                        out.putchar('\t');
                        lt++;
                        tabpos /= tabstop;
                        tabpos++;
                        tabpos *= tabstop;
                    }
                    while (tabpos < pos) {
                        out.putchar(' ');
                        tabpos++;
                    }

                    continue;
                }
                continue;
            }
            else {
                c = outtab[(unsigned)(c)];
            }
        }
        if (c == '\t') {
            out.putchar(c);

            tabpos /= tabstop;
            tabpos++;
            tabpos *= tabstop;
        }
        else if (c == '\n') {
            if (no_nl) {
                no_nl = 0;
            }
            else {
                out.putchar(c);
                tabpos = 0;
            }
        }
        else if (c == '@') {
            if (strncmp(p, "SETSOURCE", 9) == 0) { /* skip '@SETSOURCE file, line@' */
                p = strchr(p, '@');
                if (!p) {
                    print_error(Interpreter::instance->at(), "expected '@' after '@SETSOURCE' (injected code)");
                    return 1;
                }
                p++;
            }
            else {
                out.putchar(c);
                tabpos++;
            }
        }
        else {
            out.putchar(c);
            tabpos++;
        }
    }
    if (!no_nl) {
        out.putchar('\n');
        tabpos = 0;
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
        exitcode = Interpreter().launch(argc, argv);
    }
    return exitcode;
}



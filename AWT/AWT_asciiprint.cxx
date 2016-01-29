// ================================================================ //
//                                                                  //
//   File      : AWT_asciiprint.cxx                                 //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "awt_asciiprint.hxx"
#include "awt.hxx"

#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_question.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arbdbt.h>

static double awt_aps_get_xy_ratio(AW_root *awr) {
    // AWT_asciiprint_paper_size psize = (AWT_asciiprint_paper_size)awr->awar(AWAR_APRINT_PAPER_SIZE)->read_int();
    AWT_asciiprint_orientation ori = AWT_asciiprint_orientation(awr->awar(AWAR_APRINT_ORIENTATION)->read_int());
    double res = 1.0;
    switch (ori) {
        case AWT_APRINT_ORIENTATION_PORTRAIT:
            res = 112.0/90.0;
            break;
        case AWT_APRINT_ORIENTATION_LANDSCAPE:
            res = 112.0/50.0;
            break;
        case AWT_APRINT_ORIENTATION_DOUBLE_PORTRAIT:
            res = 103.0/90.0;
            break;
    }
    return res;
}

static int awt_aps_get_default_lines_per_page(AW_root *awr) {
    AWT_asciiprint_orientation ori = AWT_asciiprint_orientation(awr->awar(AWAR_APRINT_ORIENTATION)->read_int());
    switch (ori) {
        case AWT_APRINT_ORIENTATION_PORTRAIT:
            return 80;
        case AWT_APRINT_ORIENTATION_LANDSCAPE:
            return 60;
        case AWT_APRINT_ORIENTATION_DOUBLE_PORTRAIT:
            return 80;
    }
    return -1;
}


static void awt_aps_calc_pages_needed(AW_root *awr) {
    int mag = awr->awar(AWAR_APRINT_MAGNIFICATION)->read_int();
    if (mag < 25) {
        awr->awar(AWAR_APRINT_MAGNIFICATION)->write_int(mag*2);
        return;
    }
    if (mag > 250) {
        awr->awar(AWAR_APRINT_MAGNIFICATION)->write_int(250);
        return;
    }

    int x = awr->awar(AWAR_APRINT_SX)->read_int() * mag / 100;
    int y = awr->awar(AWAR_APRINT_SY)->read_int() * mag / 100;
    int default_lpp = awt_aps_get_default_lines_per_page(awr);
    double xy_ratio = awt_aps_get_xy_ratio(awr);
    int default_cpp = int(default_lpp * xy_ratio);

    awr->awar(AWAR_APRINT_DX)->write_float(float(x)/default_cpp);
    awr->awar(AWAR_APRINT_DY)->write_float(float(y)/default_lpp);
    x += default_cpp-1;
    y += default_lpp-1;
    x /= default_cpp;
    y /= default_lpp;

    awr->awar(AWAR_APRINT_PAGES)->write_int(x* y);
}


static void awt_aps_set_magnification_to_fit_xpage(AW_root *awr) {
    int x = awr->awar(AWAR_APRINT_SX)->read_int();

    int dx = int(awr->awar(AWAR_APRINT_DX)->read_float()+.5);
    if (dx < 1) dx = 1;
    if (dx > 99) dx = 99;

    int default_lpp = awt_aps_get_default_lines_per_page(awr);
    double xy_ratio = awt_aps_get_xy_ratio(awr);
    int default_cpp = int(default_lpp * xy_ratio);
    int mag = 100 * default_cpp * dx / x;
    awr->awar(AWAR_APRINT_MAGNIFICATION)->write_int(mag);
    awt_aps_calc_pages_needed(awr);
}

static void awt_aps_set_magnification_to_fit_ypage(AW_root *awr) {
    int y = awr->awar(AWAR_APRINT_SY)->read_int();

    int dy = int(awr->awar(AWAR_APRINT_DY)->read_float()+.5);
    if (dy < 1) dy = 1;
    if (dy > 99) dy = 99;

    int default_lpp = awt_aps_get_default_lines_per_page(awr);
    int mag = 100 * default_lpp * dy / y;
    awr->awar(AWAR_APRINT_MAGNIFICATION)->write_int(mag);
    awt_aps_calc_pages_needed(awr);
}

static void awt_aps_set_magnification_to_fit_xpage(AW_window *aww) {
    awt_aps_set_magnification_to_fit_xpage(aww->get_root());
}
static void awt_aps_set_magnification_to_fit_ypage(AW_window *aww) {
    awt_aps_set_magnification_to_fit_ypage(aww->get_root());
}
static void awt_aps_text_changed(AW_root *awr) {
    char *text = awr->awar(AWAR_APRINT_TEXT)->read_string();
    {
        char *rtext = GBS_replace_tabs_by_spaces(text);
        delete text;
        text = rtext;
    }    int maxx, y;
    maxx = 1; y = 0;
    char *s;
    char *ns;
    for (s = text; s; s=ns) {
        ns = strchr(s, '\n');
        if (ns) {
            ns[0] = 0;
            ns++;
        }
        int slen = strlen(s);
        if (slen > maxx) {
            maxx = slen;
        }
        y++;
    }
    if (!y) y++;
    awr->awar(AWAR_APRINT_SX)->write_int(maxx);
    awr->awar(AWAR_APRINT_SY)->write_int(y);
    delete text;
    awt_aps_set_magnification_to_fit_xpage(awr);
}

static void write_file(const char *filename, const char *file) {
    fprintf(stderr, "Printing to ASCII file '%s'\n", filename);
    FILE *f = fopen(filename, "r");
    if (f) {
        fclose(f);
        if (aw_question("overwrite_file", GBS_global_string("File '%s' already exist", filename), "Overwrite,Cancel")) {
            return;
        }
    }
    f = fopen(filename, "w");
    if (!f) {
        aw_message(GBS_global_string("Cannot write to '%s'", filename));
        return;
    }
    fprintf(f, "%s", file);
    fclose(f);
}

static char *printFile(AW_root *awr) {
    const char *home = GB_getenv("HOME");
    const char *name = awr->awar(AWAR_APRINT_FILE)->read_char_pntr();
    return GBS_global_string_copy("%s/%s", home, name);
}

static void awt_aps_go(AW_window *aww) {
    AW_root *awr  = aww->get_root();
    char    *text = awr->awar(AWAR_APRINT_TEXT)->read_string();

    freeset(text, GBS_replace_tabs_by_spaces(text));

    AWT_asciiprint_destination dest = (AWT_asciiprint_destination)awr->awar(AWAR_APRINT_PRINTTO)->read_int();
    if (dest == AWT_APRINT_DEST_FILE_ASCII) {
        char *file = printFile(awr);
        write_file(file, text);
        free(file);
    }
    else {
        char *tmp_file;
        FILE *tmpf;
        {
            char *name = GB_unique_filename("arb_aprint", "txt");
            tmpf = GB_fopen_tempfile(name, "wt", &tmp_file);
            free(name);
        }

        GB_ERROR error = NULL;
        if (!tmpf) {
            error = GBS_global_string("awt_aps_go: %s", GB_await_error());
        }
        else {
            char *y_begin = text;
            int last_y = 0;

            double xy_ratio = awt_aps_get_xy_ratio(awr);
            int    mag      = awr->awar(AWAR_APRINT_MAGNIFICATION)->read_int();

            int default_lpp = awt_aps_get_default_lines_per_page(awr);
            int default_cpp = int(default_lpp * xy_ratio);
            default_cpp     = default_cpp * 100 / mag;
            default_lpp     = default_lpp * 100 / mag;

            int text_width  = awr->awar(AWAR_APRINT_SX)->read_int();
            int text_height = awr->awar(AWAR_APRINT_SY)->read_int();

            int x;
            int y;

            for (y = 0; y < text_height; y += default_lpp) {
                while (last_y < y) {
                    last_y ++;
                    y_begin = strchr(y_begin, '\n');
                    if (!y_begin) break;
                    y_begin++;
                }
                if (!y_begin) break;

                for (x = 0; x < text_width; x += default_cpp) {
                    char *line = y_begin;
                    int i;
                    for (i=0; i<default_lpp; i++) {
                        if (line) {
                            char *next_line = strchr(line, '\n');
                            int line_length;
                            if (next_line) {
                                line_length = next_line - line; // exclusive '\n'
                                next_line ++;
                            }
                            else {
                                line_length = strlen(line);
                            }
                            if (line_length > x + default_cpp) {
                                line_length = x + default_cpp;
                            }
                            if (line_length > x) {
                                fwrite(line + x, sizeof(char), line_length - x, tmpf);
                            }
                            line = next_line;
                        }
                        fprintf(tmpf, "\n");
                    }
                }
            }
            fclose(tmpf);

            char *a2ps_call = 0;
            {
                AWT_asciiprint_orientation ori = AWT_asciiprint_orientation(awr->awar(AWAR_APRINT_ORIENTATION)->read_int());
                const char *oristring = "";
                switch (ori) {
                    case AWT_APRINT_ORIENTATION_PORTRAIT:
                        oristring = "-p -1 ";
                        break;
                    case AWT_APRINT_ORIENTATION_LANDSCAPE:
                        oristring = "-l -1 ";
                        break;
                    case AWT_APRINT_ORIENTATION_DOUBLE_PORTRAIT:
                        oristring = "-p -2 ";
                        break;
                }
                const char *header          = awr->awar(AWAR_APRINT_TITLE)->read_char_pntr();
                char       *quotedHeaderArg = GBK_singlequote(GBS_global_string("-H%s", header));
                a2ps_call                   = GBS_global_string_copy("arb_a2ps -ns -nP %s %s -l%i %s",
                                                                     quotedHeaderArg, oristring, default_lpp, tmp_file);
                free(quotedHeaderArg);
            }

            const char *scall = 0;
            switch (dest) {
                case AWT_APRINT_DEST_PRINTER: {
                    char *printer = awr->awar(AWAR_APRINT_PRINTER)->read_string();
                    scall = GBS_global_string("%s |%s; rm -f %s", a2ps_call, printer, tmp_file);
                    free(printer);
                    break;
                }
                case AWT_APRINT_DEST_FILE_PS: {
                    char *file       = printFile(awr);
                    char *quotedFile = GBK_singlequote(file);
                    fprintf(stderr, "Printing to PS file %s\n", quotedFile);
                    scall            = GBS_global_string("%s >%s;rm -f %s", a2ps_call, quotedFile, tmp_file);
                    free(quotedFile);
                    free(file);
                    break;
                }
                case AWT_APRINT_DEST_PREVIEW: {
                    char *tmp_file2;
                    {
                        char *name_only;
                        GB_split_full_path(tmp_file, NULL, NULL, &name_only, NULL);
                        tmp_file2 = GB_create_tempfile(GBS_global_string("%s.ps", name_only));
                        free(name_only);
                    }

                    if (!tmp_file2) error = GB_await_error();
                    else {
                        scall = GBS_global_string("%s >%s;(%s %s;rm -f %s %s)&",
                                                  a2ps_call, tmp_file2,
                                                  GB_getenvARB_GS(), tmp_file2,
                                                  tmp_file, tmp_file2);
                        free(tmp_file2);
                    }
                    break;
                }
                default:
                    awt_assert(0);
                    break;
            }

            if (scall) error = GBK_system(scall);
            free(a2ps_call);
        }
        if (error) aw_message(error);
        free(tmp_file);
    }
    free(text);
}

static void cutExt(char *name, const char *removeExt) {
    char *found = strstr(name, removeExt);
    if (found && strcmp(found, removeExt) == 0) {
        found[0] = 0;
    }
}
static char *correct_extension(const char *name, const char *newExt) {
    char *noExt = strdup(name);
    cutExt(noExt, ".ps");
    cutExt(noExt, ".txt");

    char *result = (char*)malloc(strlen(noExt)+strlen(newExt)+1);
    strcpy(result, noExt);
    strcat(result, newExt);
    if (strcmp(name, result) == 0) freenull(result);

    free(noExt);
    return result;
}

static void aps_correct_filename(AW_root *aw_root) {
    int type = aw_root->awar(AWAR_APRINT_PRINTTO)->read_int();
    if (type == AWT_APRINT_DEST_FILE_PS || type == AWT_APRINT_DEST_FILE_ASCII) {
        AW_awar    *awar_name = aw_root->awar(AWAR_APRINT_FILE);
        const char *name      = awar_name->read_char_pntr();
        char       *new_name  = NULL;

        if (type == AWT_APRINT_DEST_FILE_PS) {
            new_name = correct_extension(name, ".ps");
        }
        else {
            awt_assert((type == AWT_APRINT_DEST_FILE_ASCII));
            new_name = correct_extension(name, ".txt");
        }
        if (new_name) {
            awar_name->write_string(new_name);
            free(new_name);
        }
    }
}

void AWT_create_ascii_print_window(AW_root *awr, const char *text_to_print, const char *title) {
    static AW_window_simple *aws = 0;

    awr->awar_string(AWAR_APRINT_TEXT)->write_string(text_to_print);
    if (title) {
        awr->awar_string(AWAR_APRINT_TITLE)->write_string(title);
    }
    if (aws) {
        awr->awar_float(AWAR_APRINT_DX)->write_float(1.0);
    }
    else {
        aws = new AW_window_simple;
        aws->init(awr, "PRINT", "PRINT");
        aws->load_xfig("awt/ascii_print.fig");
        awr->awar_string(AWAR_APRINT_TITLE);
        awr->awar_string(AWAR_APRINT_TEXT)                                  ->add_callback(awt_aps_text_changed);

        awr->awar_int(AWAR_APRINT_PAPER_SIZE, (int)AWT_APRINT_PAPERSIZE_A4) ->add_callback(awt_aps_set_magnification_to_fit_xpage);
        awr->awar_int(AWAR_APRINT_MAGNIFICATION, 100)                       ->add_callback(awt_aps_calc_pages_needed);
        awr->awar_int(AWAR_APRINT_PAGES, 1);
        awr->awar_int(AWAR_APRINT_SX, 1);
        awr->awar_int(AWAR_APRINT_SY, 1);

        awr->awar_float(AWAR_APRINT_DX, 1.0);
        awr->awar_float(AWAR_APRINT_DY, 1.0);

        awr->awar_string(AWAR_APRINT_FILE, "print.ps")->add_callback(aps_correct_filename);

        awr->awar_int(AWAR_APRINT_ORIENTATION, (int)AWT_APRINT_ORIENTATION_PORTRAIT)->add_callback(awt_aps_set_magnification_to_fit_xpage);
        awr->awar_int(AWAR_APRINT_PRINTTO, int(AWT_APRINT_DEST_PRINTER))->add_callback(aps_correct_filename);
        aps_correct_filename(awr);

        {
            char *print_command;
            if (getenv("PRINTER")) {
                print_command = GBS_eval_env("lpr -h -P$(PRINTER)");
            }
            else {
                print_command = strdup("lpr -h");
            }

            awr->awar_string(AWAR_APRINT_PRINTER, print_command);
            free(print_command);
        }

        awt_aps_text_changed(awr);

        aws->at("close");
        aws->callback(AW_POPDOWN);
        aws->create_button("CLOSE", "CLOSE");


        aws->at("help");
        aws->callback(makeHelpCallback("asciiprint.hlp"));
        aws->create_button("HELP", "HELP");

        aws->at("go");
        aws->callback(awt_aps_go);
        aws->create_button("PRINT", "PRINT");

        aws->at("title");
        aws->create_input_field(AWAR_APRINT_TITLE);

        aws->at("text");
        aws->create_text_field(AWAR_APRINT_TEXT);

        aws->button_length(5);
        aws->at("rows");
        aws->create_button(0, AWAR_APRINT_SY);

        aws->at("columns");
        aws->create_button(0, AWAR_APRINT_SX);

        aws->at("magnification");
        aws->create_input_field(AWAR_APRINT_MAGNIFICATION, 4);

        aws->at("paper_size");
        {
            aws->create_toggle_field(AWAR_APRINT_PAPER_SIZE, 1);
            aws->insert_toggle("A4", "A", int(AWT_APRINT_PAPERSIZE_A4));
            aws->insert_toggle("US", "U", int(AWT_APRINT_PAPERSIZE_US));
            aws->update_toggle_field();
        }

        aws->at("orientation");
        {
            aws->create_toggle_field(AWAR_APRINT_ORIENTATION, 1);
            aws->insert_toggle("#print/portrait.xpm", "P", int(AWT_APRINT_ORIENTATION_PORTRAIT));
            aws->insert_toggle("#print/landscape.xpm", "P", int(AWT_APRINT_ORIENTATION_LANDSCAPE));
            aws->update_toggle_field();
        }


        aws->at("pages");
        aws->create_button(0, AWAR_APRINT_PAGES);

        aws->at("dcol");
        aws->callback(awt_aps_set_magnification_to_fit_xpage);
        aws->create_input_field(AWAR_APRINT_DX, 4);

        aws->at("drows");
        aws->callback(awt_aps_set_magnification_to_fit_ypage);
        aws->create_input_field(AWAR_APRINT_DY, 4);


        aws->at("printto");
        aws->create_toggle_field(AWAR_APRINT_PRINTTO);
        aws->insert_toggle("Printer", "P", int(AWT_APRINT_DEST_PRINTER));
        aws->insert_toggle("File (Postscript)", "F", int(AWT_APRINT_DEST_FILE_PS));
        aws->insert_toggle("File (ASCII)", "A", int(AWT_APRINT_DEST_FILE_ASCII));
        aws->insert_toggle("Preview", "V", int(AWT_APRINT_DEST_PREVIEW));
        aws->update_toggle_field();

        aws->at("printer");
        aws->create_input_field(AWAR_APRINT_PRINTER, 16);

        aws->at("filename");
        aws->create_input_field(AWAR_APRINT_FILE, 16);
    }
    aws->activate();
}


void AWT_show_file(AW_root *awr, const char *filename) {
    char *text = GB_read_file(filename);
    if (!text) {
        aw_message(GB_await_error());
    }
    else {
        AWT_create_ascii_print_window(awr, text, filename);
        free(text);
    }
}

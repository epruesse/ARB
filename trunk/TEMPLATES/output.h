//  ==================================================================== //
//                                                                       //
//    File      : output.h                                               //
//    Purpose   : class for indented output to FILE                      //
//    Time-stamp: <Fri Feb/13/2004 17:26 MET Coder@ReallySoft.de>        //
//                                                                       //
//                                                                       //
//  Coded by Ralf Westram (coder@reallysoft.de) in September 2003        //
//  Copyright Department of Microbiology (Technical University Munich)   //
//                                                                       //
//  Visit our web site at: http://www.arb-home.de/                       //
//                                                                       //
//                                                                       //
//  ==================================================================== //

#ifndef OUTPUT_H
#define OUTPUT_H

#ifndef _STDARG_H
#include <stdarg.h>
#endif

//  ---------------------
//      class output
//  ---------------------

class output {
private:
    int   indentation;
    bool  printing_points;
    int   max_points;
    int   points_printed;
    int   points_per_line;
    FILE *out;

    inline void cr() const { fputc('\n', stdout); }

    inline void  print_indent() {
        for (int i = 0; i<indentation; ++i) fputc(' ', stdout);
    }

    void goto_indentation() {
        if (printing_points) {
            new_point_line();
            // here we stop printing points
            points_printed  = 0;
            max_points      = 0;
            printing_points = false;
        }
        else print_indent();
    }

    void new_point_line() {
        if (max_points) {   // do we know max_points ?
            fprintf(stdout, " %3i%%", int(double(points_printed)/max_points*100+.5));
        }
        cr();
        print_indent();
    }

public:
    output(FILE *out_ = stdout, int breakPointsAt = 60)
        : indentation(0)
        , printing_points(false)
        , max_points(0)
        , points_printed(0)
        , points_per_line(breakPointsAt)
        , out(out_)
    {}

    void indent(int howMuch) { indentation   += howMuch; }
    void unindent(int howMuch) { indentation -= howMuch; }

    void vput(const char *s, va_list argPtr) {
        goto_indentation();
        vfprintf(stdout, s, argPtr);
        cr();
    }

    void put(const char *s, ...) __attribute__((format(printf, 2, 3)));

    void put()  {
        goto_indentation();
        cr();
    }

    void setMaxPoints(int maxP) {
        max_points = maxP;
    }

    void point() {
        if (!printing_points) {
            goto_indentation();
            printing_points = true;
        }
        else if ((points_printed%points_per_line) == 0 && points_printed) {
            new_point_line();
        }

        fputc('.', stdout);
        ++points_printed;
        fflush(stdout);
    }
};

inline void output::put(const char *s, ...) {
    va_list parg;
    va_start(parg, s);
    vput(s, parg);
}



//  ---------------------
//      class indent
//  ---------------------
// create an instance of indent to increase indentation
// indentation automatically resets when that instance leaves the scope

class indent {
private:
    int     indentation;
    output& out;

public:
    indent(output& out_, int ind = 2) : indentation(ind), out(out_) {
        out.indent(indentation);
    }
    ~indent() {
        out.unindent(indentation);
    }
};



#else
#error output.h included twice
#endif // OUTPUT_H


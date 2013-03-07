// ============================================================= //
//                                                               //
//   File      : aw_xargs.hxx                                    //
//   Purpose   :                                                 //
//                                                               //
//   Coded by Ralf Westram (coder@reallysoft.de) in March 2012   //
//   Institute of Microbiology (Technical University Munich)     //
//   http://www.arb-home.de/                                     //
//                                                               //
// ============================================================= //

#ifndef AW_XARGS_HXX
#define AW_XARGS_HXX

#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif
#ifndef _XtIntrinsic_h
#include <X11/Intrinsic.h>
#endif

class aw_xargs : virtual Noncopyable {
    Arg    *arg;
    size_t  max;
    size_t  count;
public:
    aw_xargs(size_t maxcount)
        : arg(new Arg[maxcount]),
          max(maxcount),
          count(0)
    {}
    ~aw_xargs() {
        delete [] arg;
    }

    Arg *list() { return arg; }
    int size() { return count; }

    void add(String name, XtArgVal value) {
        aw_assert(count<max);
        XtSetArg(arg[count], name, value);
        count++;
    }

    void assign_to_widget(Widget w) {
        XtSetValues(w, list(), size());
    }
};


#else
#error aw_xargs.hxx included twice
#endif // AW_XARGS_HXX

#ifndef _AP_ERROR_INC
#define _AP_ERROR_INC

#ifndef ATTRIBUTES_H
#include <attributes.h>
#endif
#ifndef ARBTOOLS_H
#include <arbtools.h>
#endif


class AP_ERR : virtual Noncopyable {
    static int  mode; // output mode 0 = no warnings, 1 = warnings
    const char *text; // pointer to errortext

public:
    AP_ERR(const char *errorstring);                // sets error
    AP_ERR(const char *, const int core) __ATTR__NORETURN;
    AP_ERR(const char *, const char *, const int core) __ATTR__NORETURN;
    AP_ERR(const char *, const char *);
    ~AP_ERR();

    const char *show();                             // shows error messages
    void        set_mode(int i);                    // set error mode
};

#endif

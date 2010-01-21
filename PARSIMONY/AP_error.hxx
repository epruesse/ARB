#ifndef _AP_ERROR_INC
#define _AP_ERROR_INC

class AP_ERR
{
    static int  mode;                               // output mode 0 = no warnings, 1 = warnings
    int         anzahl;                             // errortext count
    const char *text;                               // pointer to errortext

public:
    AP_ERR(const char *errorstring);                // sets error
    AP_ERR(const char *, const int core);
    AP_ERR(const char *, const char *, const int core);
    AP_ERR(const char *, const char *);
    ~AP_ERR();
    
    const char *show();                             // shows error messages
    void        set_mode(int i);                    // set error mode
};

#endif

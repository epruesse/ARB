#ifndef _AP_ERROR_INC
#define _AP_ERROR_INC

class AP_ERR
    {
    static int modus;   // Ausgabemodus 0 = keine Warnings, 1 = Warnings
    int  anzahl;    // Anzahl der fehlertexte
    const char *text;   // Zeiger auf fehlertext

    public:
    AP_ERR(const char *errorstring); // setzt den Fehler
    AP_ERR(const char *,const int core);
    AP_ERR(const char *,const char *,const int core);
    AP_ERR(const char *,const char *);
    ~AP_ERR();
    const char *show(); // shows error messages
    void set_mode(int i); // set error mode
    };

#endif

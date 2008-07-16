#ifndef awt_automata_hxx_included
#define awt_automata_hxx_included


typedef long    AW_CL;          // generic client data type (void *)

class AWT_auto_states {
public:
    AWT_auto_states *children;
    int              value_is_malloced;
    AW_CL            value;
    
    AWT_auto_states();
    ~AWT_auto_states();
};

class AWT_automata: protected  AWT_auto_states {
    char gaps[256];
    int  value_is_malloced;

public:
    AWT_automata(int free_value = 0);
    ~AWT_automata();
    
    char  *insert(char *str, AW_CL value);
    void   set_gaps(char *gaps);
    // returns error msg
    AW_CL  get_fwd(char *str, int pos);      // returns val forward
    AW_CL  get_bwd(char *str, int pos);      // returns val backward
}


#endif

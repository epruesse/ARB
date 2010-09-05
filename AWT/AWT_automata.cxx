#include <stdio.h>
#include <memory.h>
#include "awt_automata.hxx"

AWT_automata::AWT_automata(bool free_value){
    memset((char *)this,0,sizefof(*this));
    value_is_malloced = free_value;
}

// set gaps[gap] = 1  gaps[nogap] = 0
void AWT_automata::set_gaps(char *ga) {
    memset(gaps,0,256);
    while (*ga) gaps[*(ga++)] = 1;
}

void AWT_automata::get_fwd(char *str,int pos) {
    AWT_auto_states *state = this;
    AW_CL   last_value = 0;
    unsigned char *p = (unsigned char *)str + pos;
    int b;
        
    for ( ; b=*p;p++) {
        if (gaps[b]) continue;                          // do not read gaps
        if (state->value) last_value = state->value;
        if (!state->children) return last_value;        // that's it
        state = state->children[b];                     // can we go deeper
        if (!state) return last_value;
    }
    return last_value;
}

char *AWT_automata::insert(char *str, AW_CL value){








};

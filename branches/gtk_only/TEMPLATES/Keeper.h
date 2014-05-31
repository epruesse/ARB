// =========================================================== //
//                                                             //
//   File      : Keeper.h                                      //
//   Purpose   : Keep allocated data until termination         //
//                                                             //
//   Coded by Ralf Westram (coder@reallysoft.de) in May 2014   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef KEEPER_H
#define KEEPER_H

#ifndef _GLIBCXX_LIST
#include <list>
#endif

template <typename T>
class Keeper {
    typedef std::list<T> kept;
    kept                 elems;

    void destroy(T elem); // needs to be declared for custom T

public:
    Keeper() {}
    ~Keeper() {
        for (typename kept::iterator i = elems.begin(); i != elems.end(); ++i) {
            destroy(*i);
        }
    }
    void keep(T elem) { elems.push_back(elem); }
};

// predefined specializations:
template<> inline void Keeper<char*>::destroy(char *s) { free(s); }


#else
#error Keeper.h included twice
#endif // KEEPER_H

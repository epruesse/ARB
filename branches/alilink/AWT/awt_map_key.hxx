// =========================================================== //
//                                                             //
//   File      : awt_map_key.hxx                               //
//   Purpose   :                                               //
//                                                             //
//   Institute of Microbiology (Technical University Munich)   //
//   http://www.arb-home.de/                                   //
//                                                             //
// =========================================================== //

#ifndef AWT_MAP_KEY_HXX
#define AWT_MAP_KEY_HXX

#define MAX_MAPPED_KEYS 20

class AW_root;
class AW_window;

class ed_key {
    char mapping[256];

public:
    ed_key();

    char map_key(char) const;
    void create_awars(AW_root *root);

    void rehash_mapping(AW_root *awr);
};

AW_window *create_key_map_window(AW_root *root);



#else
#error awt_map_key.hxx included twice
#endif // AWT_MAP_KEY_HXX

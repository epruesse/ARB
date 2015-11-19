// ================================================================ //
//                                                                  //
//   File      : AWT_map_key.cxx                                    //
//   Purpose   :                                                    //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "awt_map_key.hxx"
#include <arb_msg.h>
#include <aw_window.hxx>
#include <aw_root.hxx>
#include <aw_awar.hxx>

ed_key::ed_key() {
    int i;
    for (i=0; i<256; i++) mapping[i] = i;
}

char ed_key::map_key(char k) const {
    int i = k & 0xff;
    return mapping[i];
}

inline char read_mapping_awar(AW_root *awr, int idx, const char *subkey) {
    const char *awar_name    = GBS_global_string("key_mapping/key_%i/%s", idx, subkey);
    const char *awar_content = awr->awar(awar_name)->read_char_pntr();
    return awar_content[0];
}

void ed_key::rehash_mapping(AW_root *awr) {
    for (int i=0; i<256; i++) mapping[i] = i;

    long enable = awr->awar("key_mapping/enable")->read_int();
    if (enable) {
        for (int i=0; i<MAX_MAPPED_KEYS; i++) {
            char source = read_mapping_awar(awr, i, "source");
            char dest   = read_mapping_awar(awr, i, "dest");

            if (source && dest) mapping[safeCharIndex(source)] = dest;
        }
    }
}

static void ed_rehash_mapping(AW_root *awr, ed_key *ek) {
    ek->rehash_mapping(awr);
}

void ed_key::create_awars(AW_root *awr) {
    RootCallback rehash_mapping_cb = makeRootCallback(ed_rehash_mapping, this);

    for (int i=0; i<MAX_MAPPED_KEYS; i++) {
        char source[256]; sprintf(source, "key_mapping/key_%i/source", i);
        char dest[256];   sprintf(dest,   "key_mapping/key_%i/dest",   i);
        awr->awar_string(source, "", AW_ROOT_DEFAULT); awr->awar(source)->add_callback(rehash_mapping_cb);
        awr->awar_string(dest,   "", AW_ROOT_DEFAULT); awr->awar(dest)  ->add_callback(rehash_mapping_cb);
    }
    awr->awar_int("key_mapping/enable", 1, AW_ROOT_DEFAULT);
    awr->awar("key_mapping/enable")->add_callback(rehash_mapping_cb);
    rehash_mapping(awr);
}

AW_window *create_key_map_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "KEY_MAPPING_PROPS", "KEY MAPPINGS");
    aws->load_xfig("ed_key.fig");

    aws->callback((AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("nekey_map.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("data");
    aws->auto_space(10, 0);

    char source[256];
    char dest[256];
    int i;
    for (i=0; i<MAX_MAPPED_KEYS; i++) {
        sprintf(source, "key_mapping/key_%i/source", i);
        sprintf(dest, "key_mapping/key_%i/dest", i);
        aws->create_input_field(source, 3);
        aws->create_input_field(dest, 3);
        aws->at_newline();
    }
    aws->at("enable");
    aws->create_toggle("key_mapping/enable");
    return (AW_window *)aws;
}


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
#include "awt_config_manager.hxx"

#define KEYS_PER_COLUMN 10
#define MAPPED_KEYS     (2*KEYS_PER_COLUMN)

#define AWAR_KEYMAPPING_ENABLE "key_mapping/enable"

ed_key::ed_key() {
    int i;
    for (i=0; i<256; i++) mapping[i] = i;
}

char ed_key::map_key(char k) const {
    int i = k & 0xff;
    return mapping[i];
}

inline const char *mapping_awar_name(int idx, const char *subkey) {
    return GBS_global_string("key_mapping/key_%i/%s", idx, subkey);
}
inline char read_mapping_awar(AW_root *awr, int idx, const char *subkey) {
    const char *awar_content = awr->awar(mapping_awar_name(idx, subkey))->read_char_pntr();
    return awar_content[0];
}

void ed_key::rehash_mapping(AW_root *awr) {
    for (int i=0; i<256; i++) mapping[i] = i;

    long enable = awr->awar(AWAR_KEYMAPPING_ENABLE)->read_int();
    if (enable) {
        for (int i=0; i<MAPPED_KEYS; i++) {
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

    for (int i=0; i<MAPPED_KEYS; i++) {
        awr->awar_string(mapping_awar_name(i, "source"))->add_callback(rehash_mapping_cb);
        awr->awar_string(mapping_awar_name(i, "dest"))->add_callback(rehash_mapping_cb);
    }
    awr->awar_int(AWAR_KEYMAPPING_ENABLE, 1)->add_callback(rehash_mapping_cb);
    rehash_mapping(awr);
}

static void setup_keymap_config(AWT_config_definition& cdef) {
    for (int i=0; i<MAPPED_KEYS; i++) {
        cdef.add(mapping_awar_name(i, "source"), GBS_global_string("src%02i", i));
        cdef.add(mapping_awar_name(i, "dest"),   GBS_global_string("dst%02i", i));
    }
}

AW_window *create_key_map_window(AW_root *root) {
    AW_window_simple *aws = new AW_window_simple;
    aws->init(root, "KEY_MAPPINGS", "KEY MAPPINGS");
    aws->load_xfig("ed_key.fig");

    aws->callback(AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->callback(makeHelpCallback("nekey_map.hlp"));
    aws->at("help");
    aws->create_button("HELP", "HELP", "H");

    aws->at("enable");
    aws->create_toggle(AWAR_KEYMAPPING_ENABLE);

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "keymap", makeConfigSetupCallback(setup_keymap_config));

    aws->auto_space(10, 10);

    aws->at("ascii2"); int ax2 = aws->get_at_xposition();
    aws->at("ascii1"); int ax1 = aws->get_at_xposition();
    aws->at("key2");   int kx2 = aws->get_at_xposition();
    aws->at("key1");   int kx1 = aws->get_at_xposition();

    int y = aws->get_at_yposition();

    for (int i=0; i<KEYS_PER_COLUMN; ++i) {
        aws->at(kx1, y); aws->create_input_field(mapping_awar_name(i,                 "source"), 2);
        aws->at(ax1, y); aws->create_input_field(mapping_awar_name(i,                 "dest"),   2);
        aws->at(kx2, y); aws->create_input_field(mapping_awar_name(i+KEYS_PER_COLUMN, "source"), 2);
        aws->at(ax2, y); aws->create_input_field(mapping_awar_name(i+KEYS_PER_COLUMN, "dest"),   2);

        aws->at_newline();
        y = aws->get_at_yposition();
    }

    return aws;
}


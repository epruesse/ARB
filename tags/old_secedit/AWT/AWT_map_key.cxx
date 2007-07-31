#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include "awt_map_key.hxx"

ed_key::ed_key(void)
{
    int i;
    for (i=0;i<256;i++) map[i] = i;
}

char ed_key::map_key(char k){
    int i = k & 0xff;
    return map[i];
}

void ed_rehash_mapping(AW_root *awr, ed_key *ek)
{
    int i;
    for (i=0;i<256;i++) ek->map[i] = i;
    char source[256];
    char dest[256];
    char *ps,*pd;
    long enable = awr->awar("key_mapping/enable")->read_int();
    if (enable){
        for (i=0;i<MAX_MAPPED_KEYS;i++) {
            sprintf(source,"key_mapping/key_%i/source",i);
            sprintf(dest,"key_mapping/key_%i/dest",i);
            ps = awr->awar(source)->read_string();
            pd = awr->awar(dest)->read_string();
            if (strlen(ps) && strlen(pd) ) {
                ek->map[(unsigned char)ps[0]] = pd[0];
            }
            free(ps);
            free(pd);
        }
    }
}

void ed_key::create_awars(AW_root *awr)
{
    char source[256];
    char dest[256];
    int i;
    for (i=0;i<MAX_MAPPED_KEYS;i++) {
        sprintf(source,"key_mapping/key_%i/source",i);
        sprintf(dest,"key_mapping/key_%i/dest",i);
        awr->awar_string(source,"",AW_ROOT_DEFAULT);
        awr->awar(source)->add_callback((AW_RCB1)ed_rehash_mapping,(AW_CL)this);
        awr->awar_string(dest,"",AW_ROOT_DEFAULT);
        awr->awar(dest)->add_callback((AW_RCB1)ed_rehash_mapping,(AW_CL)this);
    }
    awr->awar_int("key_mapping/enable",1,AW_ROOT_DEFAULT);
    awr->awar("key_mapping/enable")->add_callback((AW_RCB1)ed_rehash_mapping,(AW_CL)this);
    ed_rehash_mapping(awr,this);
}

AW_window *create_key_map_window(AW_root *root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init( root, "KEY_MAPPING_PROPS", "KEY MAPPINGS");
    aws->load_xfig("ed_key.fig");

    aws->callback( (AW_CB0)AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE", "CLOSE","C");

    aws->callback( AW_POPUP_HELP,(AW_CL)"nekey_map.hlp");
    aws->at("help");
    aws->create_button("HELP", "HELP","H");

    aws->at("data");
    aws->auto_space(10,0);

    char source[256];
    char dest[256];
    int i;
    for (i=0;i<MAX_MAPPED_KEYS;i++) {
        sprintf(source,"key_mapping/key_%i/source",i);
        sprintf(dest,"key_mapping/key_%i/dest",i);
        aws->create_input_field(source,3);
        aws->create_input_field(dest,3);
        aws->at_newline();
    }
    aws->at("enable");
    aws->create_toggle("key_mapping/enable");
    return (AW_window *)aws;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <malloc.h> */

#include <sys/types.h>
#include <netinet/in.h>		/* hton ntoh */

#include "adlocal.h"
/*#include "arbdb.h"*/
#include "arbdbt.h"

const char *gb_read_dict_data(GBDATA *gb_dict, long *size) {
    char *data = 0;
    if (gb_dict->flags.compressed_data) {
        GB_internal_error("Dictionary is compressed");
        data = GB_read_bytes(gb_dict);
    }
    else {
        data = GB_read_bytes_pntr(gb_dict);
    }
    *size = GB_read_bytes_count(gb_dict);
    return data;
}

GB_ERROR gb_load_dictionary_data(GBDATA *gb_main,const char *key, char **dict_data, long *size) {
    /* returns dictionary data (like saved in DB)
     * in a block allocated by gbm_get_mem(.., GBM_DICT_INDEX)
     */
    GB_MAIN_TYPE *Main  = GB_MAIN(gb_main);
    GB_ERROR      error = 0;

    *dict_data = 0;
    *size      = -1;
    gb_main    = (GBDATA *)Main->data;

    if (key[0] == '@') {
        error = GB_export_error("No dictionaries for system fields");
    }
    else {
        GBDATA *gb_key_data = Main->gb_key_data;
        GBDATA *gb_name;

        GB_push_my_security(gb_main);
        gb_name = GB_find(gb_key_data,"@name",key,down_2_level);

        if (gb_name){
            GBDATA *gb_key  = GB_get_father(gb_name);
            GBDATA *gb_dict = GB_find(gb_key,"@dictionary",0,down_level);
            if (gb_dict) {
                const char *data = gb_read_dict_data(gb_dict, size);
                char       *copy = gbm_get_mem(*size, GBM_DICT_INDEX);
                memcpy(copy, data, *size);
                *dict_data       = copy;
            }
        }
        GB_pop_my_security(gb_main);
    }
    return error;
}

GB_DICTIONARY *gb_create_dict(GBDATA *gb_dict){
    GB_DICTIONARY *dict = (GB_DICTIONARY *)GB_calloc(sizeof(GB_DICTIONARY),1);
    const char    *data;
    GB_NINT       *idata;
    long           size;

    data = gb_read_dict_data(gb_dict, &size);
    GB_write_security_write(gb_dict,7);

    idata = (GB_NINT *)data;
    dict->words = ntohl(*idata++);
    dict->textlen = (int)(size - sizeof(GB_NINT)*(1+dict->words*2));

    dict->offsets = idata;
    dict->resort =  idata+dict->words;
    dict->text = (unsigned char*)(idata+2*dict->words);

    return dict;
}

void delete_gb_dictionary(GB_DICTIONARY *dict){
    GB_FREE(dict);
}

void gb_system_key_changed_cb(GBDATA *gbd,int *cl, GB_CB_TYPE type){
    GB_MAIN_TYPE *Main = gb_get_main_during_cb();
    GBQUARK q = (GBQUARK)(long) cl;

    if (type == GB_CB_DELETE){
        delete_gb_dictionary(Main->keys[q].dictionary);
        Main->keys[q].dictionary = 0;
        Main->keys[q].gb_key = 0;
    }else{
        gb_load_single_key_data(gbd,q);
    }
}

void gb_system_master_changed_cb(GBDATA *gbd,int *cl, GB_CB_TYPE type){
    GB_MAIN_TYPE *Main = gb_get_main_during_cb();
    GBQUARK q = (GBQUARK)(long) cl;
    if (type == GB_CB_DELETE){
        Main->keys[q].gb_master_ali = 0;
    }else{
        gb_load_single_key_data(gbd,q);
    }
}

void gb_load_single_key_data(GBDATA *gb_main, GBQUARK q) {
    GB_MAIN_TYPE         *Main = GB_MAIN(gb_main);
    struct gb_key_struct *ks   = &Main->keys[q];
    const char           *key  = ks->key;

    if (!Main->gb_key_data){
        ks->compression_mask = -1;
        return;
    }

    gb_main = (GBDATA *)Main->data;
    if (key[0] == '@'){
        ks->compression_mask = 0;
        ks->dictionary       = 0;
        ks->gb_key_disabled  = 1;
        ks->gb_master_ali    = 0;
    }
    else {
        GBDATA *gb_key_data = Main->gb_key_data;
        GBDATA *gb_key,*gb_name,*gb_dict;
        GB_push_my_security(gb_main);
        gb_name = GB_find(gb_key_data,"@name",key,down_2_level);
        if (gb_name){
            gb_key= GB_get_father(gb_name);
        }else{
            gb_key = gb_create_container(gb_key_data,"@key");
            gb_name = gb_create(gb_key,"@name",GB_STRING);
            GB_write_string(gb_name,key);
        }

        GB_ensure_callback(gb_key,(GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE),gb_system_key_changed_cb,(int *)q);

        ks->compression_mask = (int)GBT_read_int2(gb_key,"compression_mask",-1);
        gb_dict = GB_find(gb_key,"@dictionary",0,down_level);
        if (gb_dict){
            if (ks->dictionary) delete_gb_dictionary(ks->dictionary);
            ks->dictionary = gb_create_dict(gb_dict);
        }else{
            if (ks->dictionary) GB_internal_error("Uups Dictionary deleted");
            ks->dictionary = 0;
        }
        ks->gb_key = gb_key;
        {
            char buffer[256];
            memset(buffer,0,256);
            sprintf(buffer,"%s/@master_data/@%s",GB_SYSTEM_FOLDER,key);
            ks->gb_master_ali = GB_search(gb_main,buffer,GB_FIND);
            if (ks->gb_master_ali){
                GB_remove_callback(ks->gb_master_ali,(GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE),gb_system_master_changed_cb,(int *)q);
                GB_add_callback   (ks->gb_master_ali,(GB_CB_TYPE)(GB_CB_CHANGED|GB_CB_DELETE),gb_system_master_changed_cb,(int *)q);
            }
        }
        GB_pop_my_security(gb_main);
    }
}

GB_ERROR gb_save_dictionary_data(GBDATA *gb_main,const char *key,const char *dict, int size) {
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    GB_ERROR error = 0;
    gb_main = (GBDATA *)Main->data;
    if (key[0] == '@') {
        error = GB_export_error("No dictionaries for system fields");
    }
    else {
        GBDATA *gb_key_data = Main->gb_key_data;
        GBDATA *gb_key,*gb_name,*gb_dict;
        GB_push_my_security(gb_main);
        gb_name = GB_find(gb_key_data,"@name",key,down_2_level);
        if (gb_name){
            gb_key= GB_get_father(gb_name);
        }else{
            gb_key = gb_create_container(gb_key_data,"@key");
            gb_name = gb_create(gb_key,"@name",GB_STRING);
            GB_write_string(gb_name,key);
        }
        if (dict) {
            gb_dict = gb_search(gb_key,"@dictionary",GB_BYTES,1);
            error   = GB_write_bytes(gb_dict,dict,size);
        }
        else {
            gb_dict = GB_find(gb_key, "@dictionary", 0, down_level);
            if (gb_dict) {
                GB_delete(gb_dict); /* delete existing dictionary */
            }
        }
        GB_pop_my_security(gb_main);
    }
    if (!error){
        GBQUARK q = gb_key_2_quark(Main,key);
        gb_load_single_key_data(gb_main,q);
    }
    return error;
}

GB_ERROR gb_load_key_data_and_dictionaries(GBDATA *gb_main){
    GB_MAIN_TYPE *Main = GB_MAIN(gb_main);
    GBDATA *gb_key_data = gb_search(gb_main,GB_SYSTEM_FOLDER "/" GB_SYSTEM_KEY_DATA, GB_CREATE_CONTAINER,1);
    GBDATA *gb_key,*gb_next_key=0;
    int	key;

    Main->gb_key_data = gb_key_data;
    if (!Main->local_mode) return 0;	/* do not create anything at the client side */

    GB_push_my_security(gb_main);
    /* First step: delete unused key  */
    for (gb_key = GB_find(gb_key_data,"@key",0,down_level);
         gb_key;
         gb_key = gb_next_key)
    {
        GBDATA *gb_name = GB_find(gb_key,"@name",0,down_level);
        char *name = GB_read_string(gb_name);
        GBQUARK quark = gb_key_2_quark(Main,name);
        free(name);
        gb_next_key = GB_find(gb_key,"@key",0,search_next | this_level);

        if (quark<=0 || quark >= Main->sizeofkeys || !Main->keys[quark].key){
            GB_delete(gb_key);	/* unused key */
            continue;
        }
    }
    GB_create_index(gb_key_data,"@name",Main->sizeofkeys *2);

    gb_key_2_quark(Main,"@name");
    gb_key_2_quark(Main,"@key");
    gb_key_2_quark(Main,"@dictionary");
    gb_key_2_quark(Main,"compression_mask");

    for (key=1;key<Main->sizeofkeys;key++){
        char *k = Main->keys[key].key;
        if (!k) continue;
        gb_load_single_key_data(gb_main,key);
    }


    GB_pop_my_security(gb_main);
    return 0;
}

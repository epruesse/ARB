#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <malloc.h> */
#include <memory.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>

#include "adlocal.h"
#include "arbdbt.h"
#include "adGene.h"
#include "aw_awars.hxx"

#define GBT_GET_SIZE 0
#define GBT_PUT_DATA 1

#define DEFAULT_LENGTH        0.1 /* default length of tree-edge w/o given length */
#define DEFAULT_LENGTH_MARKER -1.0 /* tree-edges w/o length are marked with this value during read and corrected in GBT_scale_tree */

/********************************************************************************************
                    some alignment header functions
********************************************************************************************/

char **GBT_get_alignment_names(GBDATA *gbd)
{
    /* get all alignment names out of a database
       (array of strings, the last stringptr is zero)
    */

    GBDATA *presets;
    GBDATA *ali;
    GBDATA *name;
    long size;

    char **erg;
    presets = GB_search(gbd,"presets",GB_CREATE_CONTAINER);
    size = 0;
    for (   ali = GB_find(presets,"alignment",0,down_level);
            ali;
            ali = GB_find(ali,"alignment",0,this_level|search_next) ) {
        size ++;
    }
    erg = (char **)GB_calloc(sizeof(char *),(size_t)size+1);
    size = 0;
    for (   ali = GB_find(presets,"alignment",0,down_level);
            ali;
            ali = GB_find(ali,"alignment",0,this_level|search_next) ) {
        name = GB_find(ali,"alignment_name",0,down_level);
        if (!name) {
            erg[size] = (char *)GB_STRDUP("alignment_name ???");
        }else{
            erg[size] = GB_read_string(name);
        }
        size ++;
    }
    return erg;
}

GB_ERROR GBT_check_alignment_name(const char *alignment_name)
{
    GB_ERROR error;
    if ( (error = GB_check_key(alignment_name)) ) return error;
    if (strncmp(alignment_name,"ali_",4)){
        return GB_export_error("your alignment_name '%s' must start with 'ali_'",
                               alignment_name);
    }
    return 0;
}

GBDATA *GBT_create_alignment(   GBDATA *gbd,
                                const char *name, long len, long aligned,
                                long security, const char *type )
{
    GBDATA *gb_presets;
    GBDATA *gbn;
    char c[2];
    GB_ERROR error;
    c[1] = 0;
    gb_presets = GB_search(gbd,"presets",GB_CREATE_CONTAINER);
    if (!gb_presets) return 0;

    error = GBT_check_alignment_name(name);
    if (error) {
        GB_export_error(error);
        return 0;
    }

    if ( aligned<0 ) aligned = 0;
    if ( aligned>1 ) aligned = 1;
    if ( security<0) {
        error = GB_export_error("Negative securities not allowed");
        return 0;
    }
    if ( security>6) {
        error = GB_export_error("Securities greater 6 are not allowed");
        return 0;
    }
    if (!strstr("dna:rna:ami:usr",type)) {
        error = GB_export_error("Unknown alignment type '%s'",type);
        return 0;
    }
    gbn = GB_find(gb_presets,"alignment_name",name,down_2_level);
    if (gbn) {
        error = GB_export_error("Alignment '%s' already exits",name);
        return 0;
    }

    gbd = GB_create_container(gb_presets,"alignment");
    GB_write_security_delete(gbd,6);
    gbn =   GB_create(gbd,"alignment_name",GB_STRING);
    GB_write_string(gbn,name);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,6);

    gbn =   GB_create(gbd,"alignment_len",GB_INT);
    GB_write_int(gbn,len);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,0);

    gbn =   GB_create(gbd,"aligned",GB_INT);
    GB_write_int(gbn,aligned);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,0);
    gbn =   GB_create(gbd,"alignment_write_security",GB_INT);
    GB_write_int(gbn,security);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,6);

    gbn =   GB_create(gbd,"alignment_type",GB_STRING);
    GB_write_string(gbn,type);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,0);
    return gbd;
}

NOT4PERL GB_ERROR GBT_check_alignment(GBDATA *gb_main, GBDATA *preset_alignment, GB_HASH *species_name_hash)
/* check if alignment is of the correct size
   and whether all data is present.
   Sets the security deletes and writes.

   If 'species_name_hash' is not NULL,
   it initially has to contain value == 1 for each existing species.
   Afterwards it contains value == 2 for each species where an alignment has been found.
*/
{
    GBDATA   *gb_species_data  = GBT_find_or_create(gb_main,"species_data",7);
    GBDATA   *gb_extended_data = GBT_find_or_create(gb_main,"extended_data",7);
    GB_ERROR  error         = 0;
    char     *ali_name      = 0;
    {
        GBDATA *gb_ali_name = GB_find(preset_alignment,"alignment_name",0,down_level);
        if (!gb_ali_name) {
            error = "alignment without 'alignment_name' -- database corrupted";
        }
        else {
            ali_name = GB_read_string(gb_ali_name);
        }
    }

    if (!error) {
        long    security_write = -1;
        long    stored_ali_len = -1;
        long    found_ali_len  = -1;
        long    aligned        = 1;
        GBDATA *gb_ali_len     = 0;

        {
            GBDATA *gb_ali_wsec = GB_find(preset_alignment,"alignment_write_security",0,down_level);
            if (!gb_ali_wsec) {
                error = "has no 'alignment_write_security' entry";
            }
            else {
                security_write = GB_read_int(gb_ali_wsec);
            }
        }


        if (!error) {
            gb_ali_len = GB_find(preset_alignment,"alignment_len",0,down_level);
            if (!gb_ali_len) {
                error = "has no 'alignment_len' entry";
            }
            else {
                stored_ali_len = GB_read_int(gb_ali_len);
            }
        }

        if (!error) {
            GBDATA *gb_species;
            for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
                 gb_species && !error;
                 gb_species = GBT_next_species(gb_species))
            {
                GBDATA     *gb_name        = GB_find(gb_species,"name",0,down_level);
                const char *name           = 0;
                int         alignment_seen = 0;
                GBDATA     *gb_ali         = 0;

                if (gb_name) {
                    name = GB_read_char_pntr(gb_name);
                    if (species_name_hash) {
                        int seen = GBS_read_hash(species_name_hash, name);

                        gb_assert(seen != 0); // species_name_hash not initialized correctly
                        if (seen == 2) alignment_seen = 1; // already seen an alignment
                    }
                }
                else {
                    gb_name = GB_create(gb_species,"name",GB_STRING);
                    GB_write_string(gb_name,"unknown"); // @@@ create a unique name here
                }

                GB_write_security_delete(gb_name,7);
                GB_write_security_write(gb_name,6);

                gb_ali = GB_find(gb_species, ali_name, 0, down_level);
                if (gb_ali) {
                    /* GB_write_security_delete(ali,security_write); */
                    GBDATA *gb_data = GB_find(gb_ali, "data", 0, down_level);
                    if (!gb_data) {
                        gb_data = GB_create(gb_ali,"data",GB_STRING);
                        GB_write_string(gb_data, "Error: entry 'data' was missing and therefore was filled with this text.");

                        GB_warning("No '%s/data' entry for species '%s' (has been filled with dummy data)", ali_name, name);
                    }
                    else {
                        if (GB_read_type(gb_data) != GB_STRING){
                            GB_delete(gb_data);
                            error = GBS_global_string("'%s/data' of species '%s' had wrong DB-type (%s) and has been deleted!",
                                                      ali_name, name, GB_read_key_pntr(gb_data));
                        }
                        else {
                            long data_len = GB_read_string_count(gb_data);
                            if (found_ali_len != data_len) {
                                if (found_ali_len>0)        aligned       = 0;
                                if (found_ali_len<data_len) found_ali_len = data_len;
                            }
                            /* error = GB_write_security_write(data,security_write);
                               if (error) return error;*/
                            GB_write_security_delete(gb_data,7);

                            if (!alignment_seen && species_name_hash) { // mark as seen
                                GBS_write_hash(species_name_hash, name, 2); // 2 means "species has data in at least 1 alignment"
                            }
                        }
                    }
                }

                GB_write_security_delete(gb_species,security_write);
            }
        }

        if (!error) {
            GBDATA *gb_sai;
            for (gb_sai = GBT_first_SAI_rel_exdata(gb_extended_data);
                 gb_sai && !error;
                 gb_sai = GBT_next_SAI(gb_sai) )
            {
                GBDATA *gb_sai_name = GB_find(gb_sai, "name", 0, down_level);
                GBDATA *gb_ali;

                if (!gb_sai_name) continue;

                GB_write_security_delete(gb_sai_name,7);

                gb_ali = GB_find(gb_sai, ali_name, 0, down_level);
                if (gb_ali) {
                    GBDATA *gb_sai_data;
                    for (gb_sai_data = GB_find(gb_ali, 0, 0, down_level) ;
                         gb_sai_data;
                         gb_sai_data = GB_find(gb_sai_data, 0, 0, this_level|search_next))
                    {
                        long type = GB_read_type(gb_sai_data);
                        long data_len;

                        if (type == GB_DB || type < GB_BITS) continue;
                        if (GB_read_key_pntr(gb_sai_data)[0] == '_') continue; // e.g. _STRUCT (of secondary structure)

                        data_len = GB_read_count(gb_sai_data);

                        if (found_ali_len != data_len) {
                            if (found_ali_len>0)        aligned       = 0;
                            if (found_ali_len<data_len) found_ali_len = data_len;
                        }
                    }
                }
            }
        }

        if (stored_ali_len != found_ali_len) {
            error = GB_write_int(gb_ali_len, found_ali_len);
            if (error) return error;
        }
        {
            GBDATA *gb_aligned = GB_search(preset_alignment, "aligned", GB_INT);
            if (GB_read_int(gb_aligned) != aligned) {
                error = GB_write_int(gb_aligned, aligned);
                if (error) return error;
            }
        }

        if (error) {
            error = GBS_global_string("alignment '%s' %s\nDatabase corrupted - try to fix if possible, save with different name and restart application.",
                                      ali_name, error);
        }

        free(ali_name);
    }
    return error;
}

GB_ERROR GBT_rename_alignment(GBDATA *gbd,const char *source,const char *dest, int copy, int dele)
{
    /*  if copy == 1 then create a copy
        if dele == 1 then delete old */
    GBDATA *gb_presets;
    GBDATA *gb_species_data;
    GBDATA *gb_extended_data;
    GBDATA *gb_species;
    GBDATA *gb_extended;
    GBDATA *gb_ali;
    GBDATA *gb_new;
    GBDATA *gb_using;
    GB_ERROR error;
    GBDATA *gb_old_alignment;
    gb_presets = GB_find(gbd,"presets",0,down_level);
    if (!gb_presets) return "presets not found";
    gb_species_data = GB_find(gbd,"species_data",0,down_level);
    if (!gb_species_data) return "species_data not found";
    gb_extended_data = GB_find(gbd,"extended_data",0,down_level);
    if (!gb_extended_data) return "extended_data not found";

    gb_old_alignment = GBT_get_alignment(gbd,source);
    if (!gb_old_alignment) return "source not found";
    if (copy) {
        GBDATA *gbh = GBT_get_alignment(gbd,dest);
        GBDATA *gb_new_alignment;
        GBDATA *gb_alignment_name;
        if (gbh) return "destination already exists";
        if ( (error = GBT_check_alignment_name(dest)) ) return error;
        gb_new_alignment =
            GB_create_container(gb_presets,"alignment");
        error = GB_copy(gb_new_alignment, gb_old_alignment);
        if (error) return error;
        gb_alignment_name =
            GB_search(gb_new_alignment,"alignment_name",GB_FIND);
        error = GB_write_string(gb_alignment_name,dest);
        if (error) return error;
    }
    if (dele) {
        error = GB_delete(gb_old_alignment);
        if (error) return error;
    }

    gb_using = GB_search(gb_presets,"use",GB_STRING);
    if (dele && copy){
        error = GB_write_string(gb_using,dest);
        if (error) return error;
    }
    for (   gb_species = GB_find(gb_species_data,"species",0,down_level);
            gb_species;
            gb_species = GB_find(gb_species,"species",0,this_level|search_next) ){
        gb_ali = GB_find(gb_species,source,0,down_level);
        if (!gb_ali) continue;
        if (copy){
            gb_new = GB_find(gb_species,dest,0,down_level);
            if (gb_new) return "Destination name exist in a species";
            gb_new = GB_create_container(gb_species,dest);
            if (!gb_new) return GB_get_error();
            error = GB_copy(gb_new,gb_ali);
            if (error) return error;
        }
        if (dele) {
            error = GB_delete(gb_ali);
            if (error) return error;
        }
    }

    for (   gb_extended = GB_find(gb_extended_data,"extended",0,down_level);
            gb_extended;
            gb_extended = GB_find(gb_extended,"extended",0,this_level|search_next) ){
        gb_ali = GB_find(gb_extended,source,0,down_level);
        if (!gb_ali) continue;
        if (copy){
            gb_new = GB_find(gb_extended,dest,0,down_level);
            if (gb_new) return "Destination name exist in a SAI";
            gb_new = GB_create_container(gb_extended,dest);
            if (!gb_new) return GB_get_error();
            error = GB_copy(gb_new,gb_ali);
            if (error) return error;
        }
        if (dele) {
            error = GB_delete(gb_ali);
            if (error) return error;
        }
    }
    return 0;
}

GBDATA *GBT_find_or_create(GBDATA *Main,const char *key,long delete_level)
{
    GBDATA *gbd;
    gbd = GB_find(Main,key,0,down_level);
    if (gbd) return gbd;
    gbd = GB_create_container(Main,key);
    GB_write_security_delete(gbd,delete_level);
    return gbd;
}

/********************************************************************************************
                    check the database !!!
********************************************************************************************/

static long check_for_species_without_data(const char *species_name, long value, void *counterPtr) {
    if (value == 1) {
        long cnt = *((long*)counterPtr);
        if (cnt<40) {
            GB_warning("Species '%s' has no data in any alignment", species_name);
        }
        *((long*)counterPtr) = cnt+1;
    }
    return value; // new hash value
}

GB_ERROR GBT_check_data(GBDATA *Main, const char *alignment_name)
/* if alignment_name == 0 -> check all existing alignments
 * otherwise check only one alignment
 */
{
    GB_ERROR  error             = 0;
    GBDATA   *gb_sd             = GBT_find_or_create(Main,"species_data",7);
    GBDATA   *gb_presets        = GBT_find_or_create(Main,"presets",7);
    GB_HASH  *species_name_hash = 0;

    GBT_find_or_create(Main,"extended_data",7);
    GBT_find_or_create(Main,"tree_data",7);

    if (alignment_name) {
        GBDATA *gb_ali_name = GB_find(gb_presets, "alignment_name", alignment_name, down_2_level);
        if (!gb_ali_name) {
            error = GBS_global_string("Alignment '%s' does not exist - it can't be checked.", alignment_name);
        }
    }

    if (!error) {
        // check whether we have an default alignment
        GBDATA *gb_use = GB_find(gb_presets, "use",0,down_level);
        if (!gb_use) {
            // if we have no default alignment -> look for any alignment
            GBDATA *gb_ali_name = GB_find(gb_presets,"alignment_name",alignment_name,down_2_level);

            if (gb_ali_name) {
                // use first alignment found
                GBDATA *gb_ali = GB_get_father(gb_ali_name);
                gb_ali_name    = GB_find(gb_ali,"alignment_name",0,down_level);

                gb_use = GB_create(gb_presets,"use",GB_STRING);
                GB_write_string(gb_use, GB_read_char_pntr(gb_ali_name));
            }
        }
    }

    if (!alignment_name && !error) {
        // if all alignments are checked -> use species_name_hash to detect duplicated species and species w/o data
        GBDATA *gb_species;
        species_name_hash = GBS_create_hash(GBS_SPECIES_HASH_SIZE, 1);

        for (gb_species = GBT_first_species_rel_species_data(gb_sd);
             gb_species;
             gb_species = GBT_next_species(gb_species))
        {
            GBDATA *gb_name = GB_find(gb_species, "name", 0, down_level);
            if (gb_name) {
                const char *name = GB_read_char_pntr(gb_name);
                if (GBS_read_hash(species_name_hash, name) != 0) {
                    error = GBS_global_string("Species name '%s' used twice -- database corrupt", name);
                }
                else {
                    GBS_write_hash(species_name_hash, name, 1);
                }
            }
        }
    }

    if (!error) {
        GBDATA *gb_ali;

        for (gb_ali = GB_find(gb_presets,"alignment",0,down_level);
             gb_ali && !error;
             gb_ali = GB_find(gb_ali,"alignment",0,this_level|search_next))
        {
            error = GBT_check_alignment(Main, gb_ali, species_name_hash);
        }
    }

    if (species_name_hash) {
        if (!error) {
            long counter = 0;
            GBS_hash_do_loop2(species_name_hash, check_for_species_without_data, &counter);
            if (counter>0) {
                GB_warning("Found %li species without alignment data (only some were listed)", counter);
            }
        }

        GBS_free_hash(species_name_hash);
    }

    return error;
}

char *gbt_insert_delete(const char *source, long len, long destlen, long *newsize, long pos, long nchar, long mod, char insert_what, char insert_tail)
{
    /* removes or inserts bytes in a string
       len ==   len of source
       destlen == if != 0 than cut or append characters to get this len
       newsize      the result
       pos      where to insert/delete
       nchar        and how many items
       mod      size of an item
       insert_what  insert this character
    */

    char *newval;

    pos *=mod;
    nchar *= mod;
    len *= mod;
    destlen *= mod;
    if (!destlen) destlen = len;        /* if no destlen is set than keep len */

    if ( (nchar <0) && (pos-nchar>destlen)) nchar = pos-destlen;

    if (len > destlen) {
        len = destlen;          /* cut tail */
        newval = (char *)GB_calloc(sizeof(char),(size_t)(destlen+nchar+1));
    }else if (len < destlen) {              /* append tail */
        newval = (char *)malloc((size_t)(destlen+nchar+1));
        memset(newval,insert_tail,(int)(destlen+nchar));
        newval[destlen+nchar] = 0;
    }else{
        newval = (char *)GB_calloc(sizeof(char),(size_t)(len+nchar+1));
    }
    *newsize = (destlen+nchar)/mod;
    newval[*newsize] = 0;                   /* only for strings */

    if (pos>len){       /* no place to insert / delete */
        GB_MEMCPY(newval,source,(size_t)len);
        return 0;
    }

    if (nchar < 0) {
        if (pos-nchar>len) nchar = -(len-pos);      /* clip maximum characters to delete */
    }

    if (nchar > 0)  {                   /* insert */
        GB_MEMCPY(newval,source,(size_t)pos);
        memset(newval+pos,insert_what,(size_t)nchar);
        GB_MEMCPY(newval+pos+nchar,source+pos,(size_t)(len-pos));
    }else{
        GB_MEMCPY(newval,source,(size_t)pos);
        GB_MEMCPY(newval+pos,source+pos-nchar, (size_t)(len - pos + nchar));
    }
    return newval;
}

GB_ERROR gbt_insert_character_gbd(GBDATA *gb_data, long len, long pos, long nchar, const char *delete_chars, const char *species_name){
    GB_TYPES type;
    long    size =0,l;
    register long i;
    long    dlen;
    const char  *cchars;
    char    *chars;
    GB_ERROR error = 0;
    GBDATA *gb;

    ad_assert(pos>=0);
    type = GB_read_type(gb_data);
    if (type >= GB_BITS && type != GB_DB) {
        size = GB_read_count(gb_data);
        if ((len == size) && (!nchar)) return 0;
    }

    if (GB_read_key_pntr(gb_data)[0] == '_') return 0;

    switch(type) {
        case GB_DB:
            for (   gb = GB_find(gb_data,0,0,down_level);
                    gb;
                    gb = GB_find(gb,0,0,search_next | this_level)){
                error = gbt_insert_character_gbd(gb,len,pos,nchar,delete_chars, species_name);
                if (error) return error;
            }
            break;

        case GB_STRING:
            chars = GB_read_char_pntr(gb_data);
            if (!chars) return GB_get_error();

            if (pos>len) break;

            if (nchar > 0) {            /* insert character */
                if (        (pos >0 && chars[pos-1] == '.')     /* @@@@ */
                            ||  chars[pos]  == '.') {
                    chars = gbt_insert_delete( chars,size,len, &dlen, pos, nchar, sizeof(char), '.' ,'.' );
                }else{
                    chars = gbt_insert_delete( chars,size,len, &dlen, pos, nchar, sizeof(char), '-', '.' );
                }

            }else{
                l = pos+(-nchar);
                if (l>size) l = size;
                for (i = pos; i<l; i++){
                    if (delete_chars[((unsigned char *)chars)[i]]) {
                        return GB_export_error(
                                               "You tried to delete '%c' in species %s position %li  -> Operation aborted",
                                               chars[i],species_name,i);
                    }
                }
                chars = gbt_insert_delete( (char *)chars,size,len, &dlen, pos, nchar, sizeof(char), '-', '.' );
            }
            if (chars) {
                error = GB_write_string(gb_data,chars);
                free(chars);
            }
            break;
        case GB_BITS:
            cchars = GB_read_bits_pntr(gb_data,'-','+');
            if (!cchars) return GB_get_error();
            chars = gbt_insert_delete( cchars,size,len, &dlen, pos, nchar, sizeof(char), '-','-' );
            if (chars) { error = GB_write_bits(gb_data,chars,dlen,"-");
            free(chars);
            }
            break;
        case GB_BYTES:
            cchars = GB_read_bytes_pntr(gb_data);
            if (!cchars) return GB_get_error();
            chars = gbt_insert_delete( cchars,size,len, &dlen, pos, nchar, sizeof(char), 0,0 );
            if (chars) {
                error = GB_write_bytes(gb_data,chars,dlen);
                free(chars);
            }
            break;
        case GB_INTS:{
            GB_UINT4 *longs;
            GB_CUINT4 *clongs;
            clongs = GB_read_ints_pntr(gb_data);
            if (!clongs) return GB_get_error();
            longs = (GB_UINT4 *)gbt_insert_delete( (char *)clongs,size,len, &dlen, pos, nchar, sizeof(GB_UINT4), 0 ,0);
            if (longs) {
                error = GB_write_ints(gb_data,longs,dlen);
                free((char *)longs);
            }
            break;
        }
        case GB_FLOATS:{
            float   *floats;
            GB_CFLOAT   *cfloats;
            cfloats = GB_read_floats_pntr(gb_data);
            if (!cfloats) return GB_get_error();
            floats = (float *)gbt_insert_delete( (char *)cfloats,size,len, &dlen, pos, nchar, sizeof(float), 0 , 0);
            if (floats) {
                error = GB_write_floats(gb_data,floats,dlen);
                free((char *)floats);
            }
            break;
        }
        default:
            break;
    }
    return error;
}


GB_ERROR gbt_insert_character_species(GBDATA *gb_species,const char *ali_name, long len, long pos, long nchar, const char   *delete_chars)
{
    GBDATA *gb_name;
    GBDATA *gb_ali;
    char    *species_name = 0;
    GB_ERROR error = 0;

    gb_ali = GB_find(gb_species,ali_name,0,down_level);
    if (!gb_ali) return 0;
    gb_name = GB_find(gb_species,"name",0,down_level);
    if (gb_name) species_name = GB_read_string(gb_name);
    error = gbt_insert_character_gbd(gb_ali,len,pos,nchar,delete_chars, species_name);
    if (error) error = GB_export_error("Species/SAI '%s': %s",species_name,error);
    free(species_name);
    return error;
}

GB_ERROR gbt_insert_character(GBDATA *gb_species_data, const char *species,const char *name, long len, long pos, long nchar, const char *delete_chars)
{
    GBDATA *gb_species;
    GB_ERROR error;

    long species_count = GBT_count_species(GB_get_root(gb_species_data));
    long count         = 0;

    for (gb_species = GB_find(gb_species_data,species,0,down_level);
         gb_species;
         gb_species = GB_find(gb_species,species,0,this_level|search_next))
    {
        count++;
        GB_status((double)count/species_count);
        error = gbt_insert_character_species(gb_species,name,len,pos,nchar,delete_chars);
        if (error) return error;
    }
    return 0;
}

GB_ERROR GBT_check_lengths(GBDATA *Main,const char *alignment_name)
{
    GBDATA *gb_ali;
    GBDATA *gb_presets;
    GBDATA *gb_species_data;
    GBDATA *gb_extended_data;
    GBDATA *gbd;
    GBDATA *gb_len;
    GB_ERROR error;

    gb_presets       = GBT_find_or_create(Main,"presets",7);
    gb_species_data  = GBT_find_or_create(Main,"species_data",7);
    gb_extended_data = GBT_find_or_create(Main,"extended_data",7);

    for (   gb_ali = GB_find(gb_presets,"alignment",0,down_level);
            gb_ali;
            gb_ali = GB_find(gb_ali,"alignment",0,this_level|search_next) )
    {
        gbd    = GB_find(gb_ali,"alignment_name",alignment_name,down_level);
        gb_len = GB_find(gb_ali,"alignment_len",0,down_level);
        if (gbd) {
            error = gbt_insert_character(gb_extended_data,"extended",
                                         GB_read_string(gbd),
                                         GB_read_int(gb_len),0,0,0);
            if (error) return error;

            error = gbt_insert_character(gb_species_data,"species",
                                         GB_read_string(gbd),
                                         GB_read_int(gb_len),0,0,0);
            if (error) return error;
        }
    }
    return 0;
}

GB_ERROR GBT_format_alignment(GBDATA *Main, const char *alignment_name) {
    GB_ERROR err = 0;

    if (strcmp(alignment_name, GENOM_ALIGNMENT) != 0) { // NEVER EVER format 'ali_genom'
        err           = GBT_check_data(Main, alignment_name); // detect max. length
        if (!err) err = GBT_check_lengths(Main, alignment_name); // format sequences in alignment
        if (!err) err = GBT_check_data(Main, alignment_name); // sets state to "formatted"
    }
    else {
        err = "It's forbidden to format '" GENOM_ALIGNMENT "'!";
    }
    return err;
}


GB_ERROR GBT_insert_character(GBDATA *Main,char *alignment_name, long pos, long count, char *char_delete)
{
    /*  insert 'count' characters at position pos
        if count < 0    then delete position to position+|count| */
    GBDATA *gb_ali;
    GBDATA *gb_presets;
    GBDATA *gb_species_data;
    GBDATA *gb_extended_data;
    GBDATA *gbd;
    GBDATA *gb_len;
    long len;
    int ch;
    GB_ERROR error;
    char char_delete_list[256];

    if (pos<0) {
        error = GB_export_error("Illegal sequence position");
        return error;
    }

    gb_presets = GBT_find_or_create(Main,"presets",7);
    gb_species_data = GBT_find_or_create(Main,"species_data",7);
    gb_extended_data = GBT_find_or_create(Main,"extended_data",7);

    if (strchr(char_delete,'%') ) {
        memset(char_delete_list,0,256);
    }else{
        for (ch = 0;ch<256; ch++) {
            if (char_delete) {
                if (strchr(char_delete,ch)) char_delete_list[ch] = 0;
                else            char_delete_list[ch] = 1;
            }else{
                char_delete_list[ch] = 0;
            }
        }
    }

    for (gb_ali = GB_find(gb_presets, "alignment", 0, down_level);
         gb_ali;
         gb_ali = GB_find(gb_ali, "alignment", 0, this_level | search_next))
    {
        char *use;
        gbd = GB_find(gb_ali, "alignment_name", alignment_name, down_level);
        if (gbd) {
            gb_len = GB_find(gb_ali, "alignment_len", 0, down_level);
            len = GB_read_int(gb_len);
            if (pos > len)
                return
                    GB_export_error("GBT_insert_character: insert position %li exceeds length %li", pos, len);
            if (count < 0) {
                if (pos - count > len)
                    count = pos - len;
            }
            error = GB_write_int(gb_len, len + count);

            if (error) return error;
            use = GB_read_string(gbd);
            error = gbt_insert_character(gb_species_data, "species", use, len, pos, count, char_delete_list);
            if (error) {
                free(use);
                return error;
            }
            error = gbt_insert_character(gb_extended_data, "extended", use, len, pos, count, char_delete_list);
            free(use);
            if (error) return error;
        }
    }

    GB_disable_quicksave(Main,"a lot of sequences changed");
    return 0;
}


/********************************************************************************************
                    some tree write functions
********************************************************************************************/


GB_ERROR GBT_delete_tree(GBT_TREE *tree)
     /* frees a tree only in memory (not in the database)
        to delete the tree in Database
        just call GB_delete((GBDATA *)gb_tree);
    */
{
    GB_ERROR error;
    if (tree->name)  free(tree->name);
    if (tree->remark_branch) free(tree->remark_branch);
    if (tree->is_leaf == 0) {
        if ( (error=GBT_delete_tree( tree->leftson) ) ) return error;
        if ( (error=GBT_delete_tree( tree->rightson)) ) return error;
    }
    if (!tree->father || !tree->tree_is_one_piece_of_memory){
        free((char *)tree);
    }
    return 0;
}

GB_ERROR GBT_write_group_name(GBDATA *gb_group_name, const char *new_group_name) {
    GB_ERROR error = 0;
    size_t   len   = strlen(new_group_name);

    if (len >= GB_GROUP_NAME_MAX) {
        char *shortened = (char*)GB_calloc(10+len+1, 1);
        strcpy(shortened, "Too long: ");
        strcpy(shortened+10, new_group_name);
        strcpy(shortened+GB_GROUP_NAME_MAX-4, "..."); // cut end
        error = GB_write_string(gb_group_name, shortened);
    }
    else {
        error = GB_write_string(gb_group_name, new_group_name);
    }
    return error;
}

long gbt_write_tree_nodes(GBDATA *gb_tree,GBT_TREE *node,long startid)
{
    long me;
    GB_ERROR error;
    const char *key;
    GBDATA *gb_id,*gb_name,*gb_any;
    if (node->is_leaf) return 0;
    me = startid;
    if (node->name && (strlen(node->name)) ) {
        if (!node->gb_node) {
            node->gb_node = GB_create_container(gb_tree,"node");
        }
        gb_name = GB_search(node->gb_node,"group_name",GB_STRING);
        error = GBT_write_group_name(gb_name, node->name);
/*         error = GB_write_string(gb_name,node->name); */
        if (error) return -1;
    }
    if (node->gb_node){         /* delete not used nodes else write id */
        gb_any = GB_find(node->gb_node,0,0,down_level);
        if (gb_any) {
            key = GB_read_key_pntr(gb_any);
            if (!strcmp(key,"id")){
                gb_any = GB_find(gb_any,0,0,this_level|search_next);
            }
        }

        if (gb_any){
            gb_id = GB_search(node->gb_node,"id",GB_INT);
#if defined(DEBUG) && defined(DEVEL_RALF)
            {
                int old = GB_read_int(gb_id);
                if (old != me) {
                    printf("id changed in gbt_write_tree_nodes(): old=%i new=%li (tree-node=%p; gb_node=%p)\n",
                           old, me, node, node->gb_node);
                }
            }
#endif /* DEBUG */
            error = GB_write_int(gb_id,me);
            GB_write_usr_private(node->gb_node,0);
            if (error) return -1;
        }else{
#if defined(DEBUG) && defined(DEVEL_RALF)
            {
                GBDATA *gb_id2 = GB_find(node->gb_node, "id", 0, down_level);
                int     id     = 0;
                if (gb_id2) id = GB_read_int(gb_id2);

                printf("deleting node w/o info: tree-node=%p; gb_node=%p prev.id=%i\n", node, node->gb_node, id);
            }
#endif /* DEBUG */
            GB_delete(node->gb_node);
            node->gb_node = 0;
        }
    }
    startid++;
    if (!node->leftson->is_leaf) {
        startid = gbt_write_tree_nodes(gb_tree,node->leftson,startid);
        if (startid<0) return startid;
    }

    if (!node->rightson->is_leaf) {
        startid = gbt_write_tree_nodes(gb_tree,node->rightson,startid);
        if (startid<0) return startid;
    }
    return startid;
}

GB_CPNTR gbt_write_tree_rek_new(GBDATA *gb_tree, GBT_TREE *node, char *dest, long mode)
{
    char buffer[40];        /* just real numbers */
    char    *c1;

    if ( (c1 = node->remark_branch) ) {
        int c;
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'R';
            while ( (c= *(c1++))  ) {
                if (c == 1) continue;
                *(dest++) = c;
            }
            *(dest++) = 1;
        }else{
            dest += strlen(c1) + 2;
        }
    }

    if (node->is_leaf){
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'L';
            if (node->name) strcpy(dest,node->name);
            while ( (c1= (char *)strchr(dest,1)) ) *c1 = 2;
            dest += strlen(dest);
            *(dest++) = 1;
            return dest;
        }else{
            if (node->name) return dest+1+strlen(node->name)+1; /* N name term */
            return dest+1+1;
        }
    }else{
        sprintf(buffer,"%g,%g;",node->leftlen,node->rightlen);
        if (mode == GBT_PUT_DATA) {
            *(dest++) = 'N';
            strcpy(dest,buffer);
            dest += strlen(buffer);
        }else{
            dest += strlen(buffer)+1;
        }
        dest = gbt_write_tree_rek_new(gb_tree,node->leftson,dest,mode);
        dest = gbt_write_tree_rek_new(gb_tree,node->rightson,dest,mode);
        return dest;
    }
}

GB_ERROR gbt_write_tree(GBDATA *gb_main, GBDATA *gb_tree, const char *tree_name, GBT_TREE *tree, int plain_only)
{
    /* writes a tree to the database.

    If tree is loaded by function GBT_read_tree(..) then 'tree_name' should be zero !!!!!!
    else 'gb_tree' should be set to zero.

    to copy a tree call GB_copy((GBDATA *)dest,(GBDATA *)source);
    or set recursively all tree->gb_node variables to zero (that unlinks the tree),

    if 'plain_only' == 1 only the plain tree string is written

    */
    GBDATA   *gb_node,    *gb_tree_data;
    GBDATA   *gb_node_next;
    GBDATA   *gb_nnodes, *gbd;
    long      size;
    GB_ERROR  error;
    char     *ctree,*t_size;
    GBDATA   *gb_ctree;

    gb_assert(!plain_only || (tree_name == 0)); // if plain_only == 1 -> set tree_name to 0

    if (!tree) return 0;
    if (gb_tree && tree_name) return GB_export_error("you cannot change tree name to %s",tree_name);
    if ((!gb_tree) && (!tree_name)) return GB_export_error("please specify a tree name");

    if (tree_name) {
        error = GBT_check_tree_name(tree_name);
        if (error) return error;
        gb_tree_data = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
        gb_tree = GB_search(gb_tree_data,tree_name,GB_CREATE_CONTAINER);
    }

    if (!plain_only) {
        /* now delete all old style tree data */
        for (   gb_node = GB_find(gb_tree,"node",0,down_level);
                gb_node;
                gb_node = GB_find(gb_node,"node",0,this_level|search_next))
        {
            GB_write_usr_private(gb_node,1);
        }
    }

    gb_ctree  = GB_search(gb_tree,"tree",GB_STRING);
    t_size    = gbt_write_tree_rek_new(gb_tree, tree, 0, GBT_GET_SIZE);
    ctree     = (char *)GB_calloc(sizeof(char),(size_t)(t_size+1));
    t_size    = gbt_write_tree_rek_new(gb_tree, tree, ctree, GBT_PUT_DATA);
    *(t_size) = 0;
    error     = GB_set_compression(gb_main,0); /* no more compressions */
    error     = GB_write_string(gb_ctree,ctree);
    error     = GB_set_compression(gb_main,-1); /* allow all types of compression */

    free(ctree);
    if (!plain_only) {
        if (!error) size = gbt_write_tree_nodes(gb_tree,tree,0);
        if (error || (size<0)) {
            GB_print_error();
            return GB_get_error();
        }

        for (   gb_node = GB_find(gb_tree,"node",0,down_level); /* delete all ghost nodes */
                gb_node;
                gb_node = gb_node_next)
        {
            gb_node_next = GB_find(gb_node,"node",0,this_level|search_next);
            gbd = GB_find(gb_node,"id",0,down_level);
            if (!gbd || GB_read_usr_private(gb_node)) {
                error = GB_delete(gb_node);
                if (error) GB_print_error();
            }
        }
        gb_nnodes = GB_search(gb_tree,"nnodes",GB_INT);
        error = GB_write_int(gb_nnodes,size);
    }

    if (error) return error;
    return 0;
}

GB_ERROR GBT_write_tree(GBDATA *gb_main, GBDATA *gb_tree, const char *tree_name, GBT_TREE *tree) {
    return gbt_write_tree(gb_main, gb_tree, tree_name, tree, 0);
}
GB_ERROR GBT_write_plain_tree(GBDATA *gb_main, GBDATA *gb_tree, char *tree_name, GBT_TREE *tree) {
    return gbt_write_tree(gb_main, gb_tree, tree_name, tree, 1);
}


GB_ERROR GBT_write_tree_rem(GBDATA *gb_main,const char *tree_name, const char *remark) {
    GBDATA *ali_cont = GBT_get_tree(gb_main,tree_name);
    GBDATA *tree_rem =  GB_search(ali_cont,"remark",    GB_STRING);
    return GB_write_string(tree_rem,remark);
}
/********************************************************************************************
                    some tree read functions
********************************************************************************************/

GBT_TREE *gbt_read_tree_rek(char **data, long *startid, GBDATA **gb_tree_nodes, long structure_size, int size_of_tree)
{
    GBT_TREE *node;
    GBDATA *gb_group_name;
    char    c;
    char    *p1;

    static char *membase;
    if (structure_size>0){
        node = (GBT_TREE *)GB_calloc(1,(size_t)structure_size);
    }else{
        if (!startid[0]){
            membase =(char *)GB_calloc(size_of_tree+1,(size_t)(-2*structure_size)); /* because of inner nodes */
        }
        node = (GBT_TREE *)membase;
        node->tree_is_one_piece_of_memory = 1;
        membase -= structure_size;
    }

    c = *((*data)++);

    if (c=='R') {
        p1 = strchr(*data,1);
        *(p1++) = 0;
        node->remark_branch = GB_STRDUP(*data);
        c = *(p1++);
        *data = p1;
    }


    if (c=='N') {
        p1 = (char *)strchr(*data,',');
        *(p1++) = 0;
        node->leftlen = GB_atof(*data);
        *data = p1;
        p1 = (char *)strchr(*data,';');
        *(p1++) = 0;
        node->rightlen = GB_atof(*data);
        *data = p1;
        if ((*startid < size_of_tree) && (node->gb_node = gb_tree_nodes[*startid])){
            gb_group_name = GB_find(node->gb_node,"group_name",0,down_level);
            if (gb_group_name) {
                node->name = GB_read_string(gb_group_name);
            }
        }
        (*startid)++;
        node->leftson = gbt_read_tree_rek(data,startid,gb_tree_nodes,structure_size,size_of_tree);
        if (!node->leftson) {
            free((char *)node);
            return 0;
        }
        node->rightson = gbt_read_tree_rek(data,startid,gb_tree_nodes,structure_size,size_of_tree);
        if (!node->rightson) {
            free((char *)node);
            return 0;
        }
        node->leftson->father = node;
        node->rightson->father = node;
    }else if (c=='L') {
        node->is_leaf = GB_TRUE;
        p1 = (char *)strchr(*data,1);
        *p1 = 0;
        node->name = (char *)GB_STRDUP(*data);
        *data = p1+1;
    }else{
        GB_internal_error("Error reading tree 362436");
        return 0;
    }
    return node;
}


/** Loads a tree from the database into any user defined structure.
    make sure that the first eight members members of your
    structure looks exectly like GBT_TREE, You should send the size
    of your structure ( minimum sizeof GBT_TREE) to this
    function. If size < 0 then the tree is allocated as just one
    big piece of memery, which can be freed by free((char
    *)root_of_tree) + deleting names or GBT_delete_tree. tree_name
    is the name of the tree in the db return NULL if any error
    occur */

static GBT_TREE *read_tree_and_size_internal(GBDATA *gb_tree, GBDATA *gb_ctree, int structure_size, int size) {
    GBDATA    *gb_node;
    GBDATA   **gb_tree_nodes;
    long       startid[1];
    char      *fbuf;
    char      *cptr[1];
    GBT_TREE  *node;

    gb_tree_nodes = (GBDATA **)GB_calloc(sizeof(GBDATA *),(size_t)size);
    if (gb_tree) {
        for (   gb_node = GB_find(gb_tree,"node",0,down_level);
                gb_node;
                gb_node = GB_find(gb_node,"node",0,this_level|search_next))
        {
            long    i;
            GBDATA *gbd = GB_find(gb_node,"id",0,down_level);
            if (!gbd) continue;

            /*{ GB_export_error("ERROR while reading tree '%s' 4634",tree_name);return 0;}*/
            i = GB_read_int(gbd);
            if ( i<0 || i>= size ){
                GB_internal_error("An inner node of the tree is corrupt");
            }else{
                gb_tree_nodes[i] = gb_node;
            }
        }
    }
    startid[0] = 0;
    fbuf = cptr[0] = GB_read_string(gb_ctree);
    node = gbt_read_tree_rek(cptr, startid, gb_tree_nodes, structure_size,(int)size);
    free((char *)gb_tree_nodes);
    free (fbuf);
    return node;
}

GBT_TREE *GBT_read_tree_and_size(GBDATA *gb_main,const char *tree_name, long structure_size, int *tree_size) /* read a tree */ {
    GBDATA *gb_tree;
    GBDATA *gb_nnodes;
    GBDATA *gb_tree_data;
    GB_ERROR error;
    long size;
    GBDATA *gb_ctree;
    if (!tree_name) {
        GB_export_error("no treename given"); return 0;
    }
    error =GBT_check_tree_name(tree_name); if (error) return 0;

    gb_tree_data = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    gb_tree = GB_find(gb_tree_data, tree_name, 0, down_level) ;
    if (!gb_tree) {
        GB_export_error("tree '%s' not found",tree_name);
        return 0;
    }
    gb_nnodes = GB_find(gb_tree,"nnodes",0,down_level);
    if (!gb_nnodes) {
        GB_export_error("Empty tree '%s'",tree_name);
        return 0;
    }
    size = GB_read_int(gb_nnodes);
    if (!size) {
        GB_export_error("zero sized tree '%s'",tree_name);
        return 0;
    }
    gb_ctree = GB_search(gb_tree,"tree",GB_FIND);
    if (gb_ctree) {             /* new style tree */
        GBT_TREE *t               = read_tree_and_size_internal(gb_tree, gb_ctree, structure_size, size);
        if (tree_size) *tree_size = size; /* return size of tree */
        return t;
    }
    GB_export_error("Sorry old tree format not supported any more");
    return 0;
}

GBT_TREE *GBT_read_tree(GBDATA *gb_main,const char *tree_name, long structure_size) {
    return GBT_read_tree_and_size(gb_main, tree_name, structure_size, 0);
}

GBT_TREE *GBT_read_plain_tree(GBDATA *gb_main, GBDATA *gb_ctree, long structure_size) {
    GBT_TREE *t;

    GB_push_transaction(gb_main);
    t = read_tree_and_size_internal(0, gb_ctree, structure_size, 0);
    GB_pop_transaction(gb_main);

    return t;
}

/********************************************************************************************
                    link the tree tips to the database
********************************************************************************************/
long GBT_count_nodes(GBT_TREE *tree){
    if ( tree->is_leaf ) {
        return 1;
    }
    return GBT_count_nodes(tree->leftson) + GBT_count_nodes(tree->rightson);
}

GB_ERROR gbt_link_tree_to_hash_rek(GBT_TREE *tree, GBDATA *gb_species_data, long nodes, long *counter)
{
    GB_ERROR error = 0;
    GBDATA *gbd;
    if ( !tree->is_leaf ) {
        error = gbt_link_tree_to_hash_rek(tree->leftson,gb_species_data,nodes,counter);
        if (!error) error = gbt_link_tree_to_hash_rek(tree->rightson,gb_species_data,nodes,counter);
        return error;
    }
    if (nodes){
        GB_status(*counter/(double)nodes);
        (*counter) ++;
    }

    tree->gb_node = 0;
    if (!tree->name) return 0;
    gbd = GB_find(gb_species_data,"name",tree->name, down_2_level);
    if (!gbd){
        return 0;
    }
    tree->gb_node = GB_get_father(gbd);
    return error;
}


/** Link a given tree to the database. That means that for all tips the member
    gb_node is set to the database container holding the species data.
*/
GB_ERROR GBT_link_tree(GBT_TREE *tree,GBDATA *gb_main,GB_BOOL show_status)
{
    GBDATA *gb_species_data;
    GB_ERROR error = 0;
    long nodes =  0;
    long counter = 0;
    if (show_status) nodes = GBT_count_nodes(tree) + 1;
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    error = gbt_link_tree_to_hash_rek(tree,gb_species_data,nodes,&counter);
    return error;
}

/** Unlink a given tree from the database.
*/
void GBT_unlink_tree(GBT_TREE *tree)
{
    tree->gb_node = 0;
    if (!tree->is_leaf) {
        GBT_unlink_tree(tree->leftson);
        GBT_unlink_tree(tree->rightson);
    }
}



/********************************************************************************************
                    load a tree from file system
********************************************************************************************/

#define MAX_COMMENT_SIZE 1024

#define GBT_READ_CHAR(io,c)                                                     \
for (c=' '; (c==' ') || (c=='\n') || (c=='\t')|| (c=='[') ;){                   \
    c=getc(io);                                                                 \
    if (c == '\n' ) gbt_line_cnt++;                                             \
    if (c == '[') {                                                             \
        if (gbt_tree_comment_size && gbt_tree_comment_size<MAX_COMMENT_SIZE){   \
            gbt_tree_comment[gbt_tree_comment_size++] = '\n';                   \
        }                                                                       \
        c = getc(io);                                                           \
        for (; (c!=']') && (c!=EOF); c = getc(io)) {                            \
            if (gbt_tree_comment_size<MAX_COMMENT_SIZE) {                       \
                gbt_tree_comment[gbt_tree_comment_size++] = c;                  \
            }                                                                   \
        }                                                                       \
        c                                     = ' ';                            \
    }                                                                           \
}                                                                               \
gbt_last_character = c;

#define GBT_GET_CHAR(io,c)                      \
c = getc(io);                                   \
if (c == '\n' ) gbt_line_cnt++;                 \
gbt_last_character = c;

static int  gbt_last_character    = 0;
static int  gbt_line_cnt          = 0;
static char gbt_tree_comment[MAX_COMMENT_SIZE+1]; // all comments from a tree file are collected here
static int  gbt_tree_comment_size = 0; // all comments from a tree file are collected here

/* ----------------------------------------------- */
/*      static void clear_tree_comment(void)       */
/* ----------------------------------------------- */
static void clear_tree_comment(void) {
    gbt_tree_comment_size = 0;
}

/* ---------------------------------------------- */
/*      double gbt_read_number(FILE * input)      */
/* ---------------------------------------------- */
double gbt_read_number(FILE * input)
{
    char            strng[256];
    char           *s;
    int             c;
    double          fl;
    s = &(strng[0]);
    c = gbt_last_character;
    while (((c <= '9') && (c >= '0')) || (c == '.') || (c == '-') || (c=='e') || (c=='E') ) {
        *(s++) = c;
        c = getc(input);
    }
    while ((c == ' ') || (c == '\n') || (c == '\t')){
        if (c == '\n' ) gbt_line_cnt++;
        c = getc(input);
    }
    gbt_last_character = c;
    *s = 0;
    fl = GB_atof(strng);
    return fl;
}

/** Read in a quoted or unquoted string. in quoted strings double quotes ('') are replaced by (') */
char *gbt_read_quoted_string(FILE *input){
    char buffer[1024];
    int c;
    char *s;
    s = buffer;
    c = gbt_last_character;
    if ( c == '\'' ) {
        GBT_GET_CHAR(input,c);
        while ( (c!= EOF) && (c!='\'') ) {
        gbt_lt_double_quot:
            *(s++) = c;
            if ((s-buffer) > 1000) {
                *s = 0;
                GB_export_error("Error while reading tree: Name '%s' longer than 1000 bytes",buffer);
                return 0;
            }
            GBT_GET_CHAR(input,c);
        }
        if (c == '\'') {
            GBT_READ_CHAR(input,c);
            if (c == '\'') goto gbt_lt_double_quot;
        }
    }else{
        while ( c== '_') GBT_READ_CHAR(input,c);
        while ( c== ' ') GBT_READ_CHAR(input,c);
        while ( (c != ':') && (c!= EOF) && (c!=',') &&
                (c!=';') && (c!= ')') )
        {
            *(s++) = c;
            if ((s-buffer) > 1000) {
                *s = 0;
                GB_export_error("Error while reading tree: Name '%s' longer than 1000 bytes",buffer);
                return 0;
            }
            GBT_READ_CHAR(input,c);
        }
    }
    *s = 0;
    return GB_STRDUP(buffer);
}

/* ---------------------------------------------------------------- */
/*      static void setBranchName(GBT_TREE *node, char *name)       */
/* ---------------------------------------------------------------- */
/* detect bootstrap values */
/* name has to be stored in node or must be free'ed */

static double max_found_bootstrap = -1;
static double max_found_branchlen = -1;

static void setBranchName(GBT_TREE *node, char *name) {
    char   *end       = 0;
    double  bootstrap = strtod(name, &end);

    if (end == name) {          // no digits -> no bootstrap
        node->name = name;
    }
    else {
        bootstrap = bootstrap*100.0 + 0.5; // needed if bootstrap values are between 0.0 and 1.0 */
        // downscaling in done later!

        if (bootstrap>max_found_bootstrap) {
            max_found_bootstrap = bootstrap;
        }

        assert(node->remark_branch == 0);
        node->remark_branch  = GB_strdup(GBS_global_string("%i%%", (int)bootstrap));

        if (end[0] != 0) {      // sth behind bootstrap value
            if (end[0] == ':') ++end; // ARB format for nodes with bootstraps AND node name is 'bootstrap:nodename'
            node->name = GB_strdup(end);
        }
        free(name);
    }
}

static GBT_TREE *gbt_load_tree_rek(FILE *input,int structuresize, const char *tree_file_name)
{
    int             c;
    GB_BOOL     loop_flag;
    GBT_TREE *nod,*left,*right;

    if ( gbt_last_character == '(' ) {  /* make node */

        nod = (GBT_TREE *)GB_calloc(structuresize,1);
        GBT_READ_CHAR(input,c);
        left = gbt_load_tree_rek(input,structuresize, tree_file_name);
        if (!left ) return 0;
        nod->leftson = left;
        left->father = nod;
        if (    gbt_last_character != ':' &&
                gbt_last_character != ',' &&
                gbt_last_character != ';' &&
                gbt_last_character != ')'
                ) {
            char *str = gbt_read_quoted_string(input);
            if (!str) return 0;

            setBranchName(left, str);
            /* left->name = str; */
        }
        if (gbt_last_character !=  ':') {
            nod->leftlen = DEFAULT_LENGTH_MARKER;
        }else{
            GBT_READ_CHAR(input,c);
            nod->leftlen = gbt_read_number(input);
            if (nod->leftlen>max_found_branchlen) {
                max_found_branchlen = nod->leftlen;
            }
        }
        if ( gbt_last_character == ')' ) {  /* only a single node !!!!, skip this node */
            GB_FREE(nod);           /* delete superflous father node */
            left->father = NULL;
            return left;
        }
        if ( gbt_last_character != ',' ) {
            GB_export_error ( "error in '%s':  ',' expected '%c' found; line %i",
                              tree_file_name,
                              gbt_last_character,
                              gbt_line_cnt);
            return NULL;
        }
        loop_flag = GB_FALSE;
        while (gbt_last_character == ',') {
            if (loop_flag==GB_TRUE) {       /* multi branch tree */
                left = nod;
                nod = (GBT_TREE *)GB_calloc(structuresize,1);
                nod->leftson = left;
                nod->leftson->father = nod;
            }else{
                loop_flag = GB_TRUE;
            }
            GBT_READ_CHAR(input, c);
            right = gbt_load_tree_rek(input,structuresize, tree_file_name);
            if (right == NULL) return NULL;
            nod->rightson = right;
            right->father = nod;
            if (    gbt_last_character != ':' &&
                    gbt_last_character != ',' &&
                    gbt_last_character != ';' &&
                    gbt_last_character != ')'
                    ) {
                char *str = gbt_read_quoted_string(input);
                if (!str) return 0;
                setBranchName(right, str);
                /* right->name = str; */
            }
            if (gbt_last_character != ':') {
                nod->rightlen = DEFAULT_LENGTH_MARKER;
            }else{
                GBT_READ_CHAR(input, c);
                nod->rightlen = gbt_read_number(input);
                if (nod->rightlen>max_found_branchlen) {
                    max_found_branchlen = nod->rightlen;
                }
            }
        }
        if ( gbt_last_character != ')' ) {
            GB_export_error ( "error in '%s':  ')' expected '%c' found: line %i ",
                              tree_file_name,
                              gbt_last_character,gbt_line_cnt);
            return NULL;
        }
        GBT_READ_CHAR(input,c);     /* remove the ')' */

    }else{
        char *str = gbt_read_quoted_string(input);
        if (!str) return 0;
        nod = (GBT_TREE *)GB_calloc(structuresize,1);
        nod->is_leaf = GB_TRUE;
        nod->name = str;
        return nod;
    }
    return nod;
}
/* ------------------------------------------------------------------ */
/*      void GBT_scale_bootstraps(GBT_TREE *tree, double scale)       */
/* ------------------------------------------------------------------ */
/* void GBT_scale_bootstraps(GBT_TREE *tree, double scale) { */
/*     if (tree->leftson) GBT_scale_bootstraps(tree->leftson, scale); */
/*     if (tree->rightson) GBT_scale_bootstraps(tree->rightson, scale); */
/*     if (tree->remark_branch) { */
/*         const char *end          = 0; */
/*         double      bootstrap    = strtod(tree->remark_branch, (char**)&end); */
/*         GB_BOOL     is_bootstrap = end[0] == '%' && end[1] == 0; */

/*         free(tree->remark_branch); */
/*         tree->remark_branch = 0; */

/*         if (is_bootstrap) { */
/*             bootstrap = bootstrap*scale+0.5; */
/*             tree->remark_branch  = GB_strdup(GBS_global_string("%i%%", (int)bootstrap)); */
/*         } */
/*     } */
/* } */
void GBT_scale_tree(GBT_TREE *tree, double length_scale, double bootstrap_scale) {
    if (tree->leftson) {
        if (tree->leftlen<-0.01) tree->leftlen  = DEFAULT_LENGTH;
        else                     tree->leftlen *= length_scale;
        GBT_scale_tree(tree->leftson, length_scale, bootstrap_scale);
    }
    if (tree->rightson) {
        if (tree->rightlen<-0.01) tree->rightlen  = DEFAULT_LENGTH;
        else                      tree->rightlen *= length_scale;
        GBT_scale_tree(tree->rightson, length_scale, bootstrap_scale);
    }

    if (tree->remark_branch) {
        const char *end          = 0;
        double      bootstrap    = strtod(tree->remark_branch, (char**)&end);
        GB_BOOL     is_bootstrap = end[0] == '%' && end[1] == 0;

        free(tree->remark_branch);
        tree->remark_branch = 0;

        if (is_bootstrap) {
            bootstrap = bootstrap*bootstrap_scale+0.5;
            tree->remark_branch  = GB_strdup(GBS_global_string("%i%%", (int)bootstrap));
        }
    }
}

/* Load a newick compatible tree from file 'path',
   structure size should be >0, see GBT_read_tree for more information
   if commentPtr != NULL -> set it to a malloc copy of all concatenated comments found in tree file
*/
GBT_TREE *GBT_load_tree(const char *path, int structuresize, char **commentPtr, int allow_length_scaling)
{
    FILE        *input;
    GBT_TREE    *tree;
    int     c;
    if ((input = fopen(path, "r")) == NULL) {
        GB_export_error("Import tree: file '%s' not found", path);
        return 0;
    }

    clear_tree_comment();

    GBT_READ_CHAR(input,c);
    gbt_line_cnt = 1;
    {
        const char *name_only = strrchr(path, '/');
        if (name_only) ++name_only;
        else name_only = path;

        max_found_bootstrap = -1;
        max_found_branchlen = -1;
        tree                = gbt_load_tree_rek(input,structuresize, name_only);

        {
            double bootstrap_scale = 1.0;
            double branchlen_scale = 1.0;

            if (max_found_bootstrap >= 101.0) { // bootstrap values were given in percent
                bootstrap_scale = 0.01;
            }
            if (max_found_branchlen >= 1.01) { // branchlengths had range [0;100]
                if (allow_length_scaling) {
                    branchlen_scale = 0.01;
                }
            }

            GBT_scale_tree(tree, branchlen_scale, bootstrap_scale); // scale bootstraps and branchlengths
        }
    }
    fclose(input);

    if (commentPtr) {
        assert(*commentPtr == 0);
        if (gbt_tree_comment_size) *commentPtr = GB_STRDUP(gbt_tree_comment);
    }

    return tree;
}

GBDATA *GBT_get_tree(GBDATA *gb_main, const char *tree_name) {
    /* returns the datapntr to the database structure, which is the container for the tree */
    GBDATA *gb_treedata;
    GBDATA *gb_tree;
    gb_treedata = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    gb_tree = GB_find(gb_treedata, tree_name, 0, down_level) ;
    return gb_tree;
}

long GBT_size_of_tree(GBDATA *gb_main, const char *tree_name) {
    GBDATA *gb_tree = GBT_get_tree(gb_main,tree_name);
    GBDATA *gb_nnodes;
    if (!gb_tree) return -1;
    gb_nnodes = GB_find(gb_tree,"nnodes",0,down_level);
    if (!gb_nnodes) return -1;
    return GB_read_int(gb_nnodes);
}

char *GBT_find_largest_tree(GBDATA *gb_main){
    GBDATA *gb_treedata;
    GBDATA *gb_tree;
    GBDATA *gb_nnodes;
    long nnodes;
    char *largest = 0;
    long maxnodes = 0;
    gb_treedata = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    for (   gb_tree = GB_find(gb_treedata,0,0,down_level);
            gb_tree;
            gb_tree = GB_find(gb_tree, 0,0,this_level|search_next)){
        gb_nnodes = GB_find(gb_tree,"nnodes",0,down_level);
        if (!gb_nnodes) continue;
        nnodes = GB_read_int(gb_nnodes);
        if (nnodes> maxnodes) {
            if (largest) free(largest);
            largest = GB_read_key(gb_tree);
            maxnodes = nnodes;
        }
    }
    return largest;
}

char *GBT_find_latest_tree(GBDATA *gb_main){
    char **names = GBT_get_tree_names(gb_main);
    char *name = 0;
    char **pname;
    if (!names) return 0;
    for (pname = names;*pname;pname++) name = *pname;
    if (name) name = strdup(name);
    GBT_free_names(names);
    return name;
}

char *GBT_tree_info_string(GBDATA *gb_main,const  char *tree_name)
{
    char buffer[1024];
    GBDATA *gb_tree;
    GBDATA *gb_rem;
    GBDATA *gb_nnodes;
    long    size;
    memset(buffer,0,1024);

    gb_tree = GBT_get_tree(gb_main,tree_name);
    if (!gb_tree) {
        GB_export_error("tree '%s' not found",tree_name);
        return 0;
    }

    gb_nnodes = GB_find(gb_tree,"nnodes",0,down_level);
    if (!gb_nnodes) {
        GB_export_error("nnodes not found in tree '%s'",tree_name);
        return 0;
    }

    size = GB_read_int(gb_nnodes);

    sprintf(buffer,"%-15s (%4i:%i)",tree_name,(int)size+1,GB_read_security_write(gb_tree));
    gb_rem = GB_find(gb_tree,"remark",0,down_level);
    if (gb_rem) {
        strcat(buffer,"    ");
        strncat(buffer,GB_read_char_pntr(gb_rem), 500-strlen(buffer));
    }
    return GB_STRDUP(buffer);
}

GB_ERROR GBT_check_tree_name(const char *tree_name)
{
    GB_ERROR error;
    if ( (error = GB_check_key(tree_name)) ) return error;
    if (strncmp(tree_name,"tree_",5)){
        return GB_export_error("your treename '%s' does not begin with 'tree_'",tree_name);
    }
    return 0;
}

char **GBT_get_tree_names(GBDATA *Main){
    /* returns an null terminated array of string pointers */

    long count;
    GBDATA *gb_treedata;
    GBDATA *gb_tree;
    char **erg;
    gb_treedata = GB_find(Main,"tree_data",0,down_level);
    if (!gb_treedata) return 0;
    count = 0;
    for (   gb_tree = GB_find(gb_treedata,0,0,down_level);
            gb_tree;
            gb_tree = GB_find(gb_tree,0,0,this_level|search_next) ){
        count ++;
    }
    erg = (char **)GB_calloc(sizeof(char *),(size_t)count+1);
    count = 0;
    for (   gb_tree = GB_find(gb_treedata,0,0,down_level);
            gb_tree;
            gb_tree = GB_find(gb_tree,0,0,this_level|search_next) ){
        erg[count] = GB_read_key(gb_tree);
        count ++;
    }
    return erg;
}

char *GBT_get_next_tree_name(GBDATA *gb_main, const char *tree){
    GBDATA *gb_treedata;
    GBDATA *gb_tree = 0;
    gb_treedata = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
    if (tree){
        gb_tree = GB_find(gb_treedata,tree,0,down_level);
    }
    if (gb_tree){
        gb_tree = GB_find(gb_tree,0,0,this_level|search_next);
    }else{
        gb_tree = GB_find(gb_treedata,0,0,down_level);
    }
    if (gb_tree) return GB_read_key(gb_tree);
    return 0;
}

GB_ERROR GBT_free_names(char **names)
{
    char **pn;
    for (pn = names; *pn;pn++) free(*pn);
    free((char *)names);
    return 0;
}

int gbt_sum_leafs(GBT_TREE *tree){
    if (tree->is_leaf){
        return 1;
    }
    return gbt_sum_leafs(tree->leftson) + gbt_sum_leafs(tree->rightson);
}

GB_CSTR *gbt_fill_species_names(GB_CSTR *des,GBT_TREE *tree) {
    if (tree->is_leaf){
        des[0] = tree->name;
        return des+1;
    }
    des = gbt_fill_species_names(des,tree->leftson);
    des = gbt_fill_species_names(des,tree->rightson);
    return des;
}

/* creates an array of all species names in a tree,
   the names is not strdupped !!! */

GB_CSTR *GBT_get_species_names_of_tree(GBT_TREE *tree){
    int size = gbt_sum_leafs(tree);
    GB_CSTR *result = (GB_CSTR *)GB_calloc(sizeof(char *),size +1);
#if defined(DEBUG)
    GB_CSTR *check =
#endif // DEBUG
        gbt_fill_species_names(result,tree);
    assert(check - size == result);
    return result;
}

/* search for an existing or an alternate tree */
char *GBT_existing_tree(GBDATA *Main, const char *tree) {
    GBDATA *gb_treedata;
    GBDATA *gb_tree;
    gb_treedata = GB_find(Main,"tree_data",0,down_level);
    if (!gb_treedata) return 0;
    gb_tree = GB_find(gb_treedata,tree,0,down_level);
    if (gb_tree) return GB_STRDUP(tree);
    gb_tree = GB_find(gb_treedata,0,0,down_level);
    if (!gb_tree) return 0;
    return GB_read_key(gb_tree);
}

void gbt_export_tree_node_print_remove(char *str){
    int i,len;
    len = strlen (str);
    for(i=0;i<len;i++) {
        if (str[i] =='\'') str[i] ='.';
        if (str[i] =='\"') str[i] ='.';
    }
}

void gbt_export_tree_rek(GBT_TREE *tree,FILE *out){
    if (tree->is_leaf) {
        gbt_export_tree_node_print_remove(tree->name);
        fprintf(out," '%s' ",tree->name);
    }else{
        fprintf(out, "(");
        gbt_export_tree_rek(tree->leftson,out);
        fprintf(out, ":%.5f,", tree->leftlen);
        gbt_export_tree_rek(tree->rightson,out);
        fprintf(out, ":%.5f", tree->rightlen);
        fprintf(out, ")");
        if (tree->name){
            fprintf(out, "'%s'",tree->name);
        }
    }
}


GB_ERROR GBT_export_tree(GBDATA *gb_main,FILE *out,GBT_TREE *tree, GB_BOOL triple_root){
    GBUSE(gb_main);
    if(triple_root){
        GBT_TREE *one,*two,*three;
        if (tree->is_leaf){
            return GB_export_error("Tree is two small, minimum 3 nodes");
        }
        if (tree->leftson->is_leaf && tree->rightson->is_leaf){
            return GB_export_error("Tree is two small, minimum 3 nodes");
        }
        if (tree->leftson->is_leaf){
            one = tree->leftson;
            two = tree->rightson->leftson;
            three = tree->rightson->rightson;
        }else{
            one = tree->leftson->leftson;
            two = tree->leftson->rightson;
            three = tree->rightson;
        }
        fprintf(out, "(");
        gbt_export_tree_rek(one,out);
        fprintf(out, ":%.5f,", 1.0);
        gbt_export_tree_rek(two,out);
        fprintf(out, ":%.5f,", 1.0);
        gbt_export_tree_rek(three,out);
        fprintf(out, ":%.5f)", 1.0);
    }else{
        gbt_export_tree_rek(tree,out);
    }
    return 0;
}

/********************************************************************************************
                    species functions
********************************************************************************************/


GBDATA *GBT_create_species(GBDATA *gb_main,const char *name)
{
    /* Search for a species, when species do not exist create it */
    GBDATA *species;
    GBDATA *gb_name;
    GBDATA *gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
    species = GB_find(gb_species_data,"name",name,down_2_level);
    if (species) return GB_get_father(species);
    if ((int)strlen(name) <2) {
        GB_export_error("create species failed: too short name '%s'",name);
        return 0;
    }
    species = GB_create_container(gb_species_data,"species");
    gb_name = GB_create(species,"name",GB_STRING);
    GB_write_string(gb_name,name);
    GB_write_flag(species,1);
    return species;
}
GBDATA *GBT_create_species_rel_species_data(GBDATA *gb_species_data,const char *name)
{
    /* Search for a species, when species do not exist create it */
    GBDATA *species;
    GBDATA *gb_name;
    species = GB_find(gb_species_data,"name",name,down_2_level);
    if (species) return GB_get_father(species);
    if ((int)strlen(name) <2) {
        GB_export_error("create species failed: too short name '%s'",name);
        return 0;
    }
    species = GB_create_container(gb_species_data,"species");
    gb_name = GB_create(species,"name",GB_STRING);
    GB_write_string(gb_name,name);
    GB_write_flag(species,1);
    return species;
}

GBDATA *GBT_create_SAI(GBDATA *gb_main,const char *name)
{
    /* Search for an extended, when extended do not exist create it */
    GBDATA *extended;
    GBDATA *gb_name;
    GBDATA *gb_extended_data = GB_search(gb_main, "extended_data", GB_CREATE_CONTAINER);
    extended = GB_find(gb_extended_data,"name",name,down_2_level);
    if (extended) return GB_get_father(extended);
    if ((int)strlen(name) <2) {
        GB_export_error("create SAI failed: too short name '%s'",name);
        return 0;
    }
    extended = GB_create_container(gb_extended_data,"extended");
    gb_name = GB_create(extended,"name",GB_STRING);
    GB_write_string(gb_name,name);
    GB_write_flag(extended,1);
    return extended;
}

GBDATA *GBT_add_data(GBDATA *species,const char *ali_name, const char *key, GB_TYPES type)
{
    /* the same as GB_search(species, 'ali_name/key', GB_CREATE) */
    GBDATA *gb_gb;
    GBDATA *gb_data;
    if (GB_check_key(ali_name)) {
        return 0;
    }
    if (GB_check_hkey(key)) {
        return 0;
    }
    gb_gb             = GB_find(species,ali_name,0,down_level);
    if (!gb_gb) gb_gb = GB_create_container(species,ali_name);

    if (type == GB_STRING) {
        gb_data = GB_search(gb_gb, key, GB_FIND);
        if (!gb_data){
            gb_data = GB_search(gb_gb, key, GB_STRING);
            GB_write_string(gb_data,"...");
        }
    }else{
        gb_data = GB_search(gb_gb, key, type);
    }
    return gb_data;
}


GB_ERROR GBT_write_sequence(GBDATA *gb_data, const char *ali_name, long ali_len, const char *sequence) {
    /* writes a sequence which is generated by GBT_add_data,
     * cuts sequence after alignment len only if bases e ".-nN" */
    int slen = strlen(sequence);
    int old_char = 0;
    GB_ERROR error = 0;
    if (slen > ali_len) {
        int i;
        for (i= slen -1; i>=ali_len; i--) {
            if (!strchr("-.nN",sequence[i])) break;     /* real base after end of alignment */
        }
        i++;                            /* points to first 0 after alignment */
        if (i > ali_len){
            GBDATA *gb_main = GB_get_root(gb_data);
            ali_len = GBT_get_alignment_len(gb_main,ali_name);
            if (slen > ali_len){                /* check for modified alignment len */
                GBT_set_alignment_len(gb_main,ali_name,i);
                ali_len = i;
            }
        }
        if (slen > ali_len){
            old_char = sequence[ali_len];
            ((char*)sequence)[ali_len] = 0;
        }
    }
    error = GB_write_string(gb_data,sequence);
    if (slen> ali_len) ((char*)sequence)[ali_len] = old_char;
    return error;
}


GBDATA *GBT_gen_accession_number(GBDATA *gb_species,const char *ali_name){
    GBDATA *gb_acc;
    GBDATA *gb_data;
    char *sequence;
    char buf[100];
    long id;

    gb_acc = GB_find(gb_species,"acc",0,down_level);
    if (gb_acc) return gb_acc;
    /* Search a valid alignment */
    gb_data = GBT_read_sequence(gb_species,ali_name);
    if (!gb_data) return 0;
    sequence = GB_read_char_pntr(gb_data);
    id = GBS_checksum(sequence,1,".-");
    sprintf(buf,"ARB_%lX",id);
    gb_acc = GB_search(gb_species,"acc",GB_STRING);
    GB_write_string(gb_acc,buf);
    return gb_acc;
}


int GBT_is_partial(GBDATA *gb_species, int default_value, int define_if_undef) {
    // checks whether a species has a partial or full sequence
    //
    // Note: partial sequences should not be used for tree calculations
    //
    // returns: 0 if sequence is full
    //          1 if sequence is partial
    //          -1 in case of error
    //
    // if the sequence has no 'ARB_partial' entry it returns 'default_value'
    // if 'define_if_undef' is true then create an 'ARB_partial'-entry with the default value

    int       result     = -1;
    GB_ERROR  error      = 0;
    GBDATA   *gb_partial = GB_find(gb_species, "ARB_partial", 0, down_level);

    if (gb_partial) {
        result = GB_read_int(gb_partial);
        if (result != 0 && result != 1) {
            error = "Illegal value for 'ARB_partial' (only 1 or 0 allowed)";
        }
    }
    else {
        if (define_if_undef) {
            gb_partial = GB_create(gb_species, "ARB_partial", GB_INT);
            if (gb_partial) error = GB_write_int(gb_partial, default_value);
            else error = GB_get_error();
        }

        result = default_value;
    }

    if (error) {
        GB_export_error(error);
        return -1;
    }
    return result;
}


/********************************************************************************************
                    some simple find procedures
********************************************************************************************/
GBDATA *GBT_get_species_data(GBDATA *gb_main) {
    return GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
}

GBDATA *GBT_first_marked_species_rel_species_data(GBDATA *gb_species_data)
{
    return GB_first_marked(gb_species_data,"species");
}

GBDATA *GBT_first_marked_species(GBDATA *gb_main)
{
    return GB_first_marked(GBT_get_species_data(gb_main), "species");
}
GBDATA *GBT_next_marked_species(GBDATA *gb_species)
{
    return GB_next_marked(gb_species,"species");
}

GBDATA *GBT_first_species_rel_species_data(GBDATA *gb_species_data)
{
    GBDATA *gb_species;
    gb_species = GB_find(gb_species_data,"species",0,down_level);
    return gb_species;
}
GBDATA *GBT_first_species(GBDATA *gb_main)
{
    return GB_find(GBT_get_species_data(gb_main),"species",0,down_level);;
}

GBDATA *GBT_next_species(GBDATA *gb_species)
{
    gb_species = GB_find(gb_species,"species",0,this_level|search_next);
    return gb_species;
}

GBDATA *GBT_find_species_rel_species_data(GBDATA *gb_species_data,const char *name)
{

    GBDATA *gb_species_name;
    gb_species_name = GB_find(gb_species_data,"name",name,down_2_level);
    if (!gb_species_name) return 0;
    return GB_get_father(gb_species_name);
}

GBDATA *GBT_find_species(GBDATA *gb_main,const char *name)
{
    GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    GBDATA *gb_species_name;
    gb_species_name = GB_find(gb_species_data,"name",name,down_2_level);
    if (!gb_species_name) return 0;
    return GB_get_father(gb_species_name);
}


GBDATA *GBT_first_marked_extended(GBDATA *gb_extended_data)
{
    GBDATA *gb_extended;
    for (   gb_extended = GB_find(gb_extended_data,"extended",0,down_level);
            gb_extended;
            gb_extended = GB_find(gb_extended,"extended",0,this_level|search_next)){
        if (GB_read_flag(gb_extended)) return gb_extended;
    }
    return 0;
}

GBDATA *GBT_next_marked_extended(GBDATA *gb_extended)
{
    while ( (gb_extended = GB_find(gb_extended,"extended",0,this_level|search_next))  ){
        if (GB_read_flag(gb_extended)) return gb_extended;
    }
    return 0;
}
/* Search SAIs */
GBDATA *GBT_first_SAI(GBDATA *gb_main)
{
    GBDATA *gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
    GBDATA *gb_extended;
    gb_extended = GB_find(gb_extended_data,"extended",0,down_level);
    return gb_extended;
}

GBDATA *GBT_next_SAI(GBDATA *gb_extended)
{
    gb_extended = GB_find(gb_extended,"extended",0,this_level|search_next);
    return gb_extended;
}

GBDATA *GBT_find_SAI(GBDATA *gb_main,const char *name)
{
    GBDATA *gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
    return GBT_find_species_rel_species_data(gb_extended_data,name);
}
/* Search SAIs rel extended_data */

GBDATA *GBT_first_SAI_rel_exdata(GBDATA *gb_extended_data)
{
    GBDATA *gb_extended;
    gb_extended = GB_find(gb_extended_data,"extended",0,down_level);
    return gb_extended;
}

GBDATA *GBT_find_SAI_rel_exdata(GBDATA *gb_extended_data,const char *name)
{
    return GBT_find_species_rel_species_data(gb_extended_data,name);
}

char *GBT_create_unique_species_name(GBDATA *gb_main,const char *default_name){
    GBDATA *gb_species;
    int c = 1;
    char *buffer;
    gb_species = GBT_find_species(gb_main,default_name);
    if (!gb_species) return strdup(default_name);

    buffer = (char *)GB_calloc(sizeof(char),strlen(default_name)+10);
    while (gb_species){
        sprintf(buffer,"%s_%i",default_name,c++);
        gb_species = GBT_find_species(gb_main,buffer);
    }
    return buffer;
}

/********************************************************************************************
                    editor configurations
********************************************************************************************/

GBDATA *GBT_find_configuration(GBDATA *gb_main,const char *name){
    GBDATA *gb_configuration_data = GB_search(gb_main,AWAR_CONFIG_DATA,GB_DB);
    GBDATA *gb_configuration_name;
    gb_configuration_name = GB_find(gb_configuration_data,"name",name,down_2_level);
    if (!gb_configuration_name) return 0;
    return GB_get_father(gb_configuration_name);
}

GBDATA *GBT_create_configuration(GBDATA *gb_main, const char *name){
    GBDATA *gb_configuration_data;
    GBDATA *gb_configuration;
    GBDATA *gb_name;
    gb_configuration = GBT_find_configuration(gb_main,name);
    if (gb_configuration) return gb_configuration;
    gb_configuration_data = GB_search(gb_main,AWAR_CONFIG_DATA,GB_DB);
    gb_configuration = GB_create_container(gb_configuration_data,AWAR_CONFIG);
    gb_name = GBT_search_string(gb_configuration,"name",name);
    return gb_configuration;
}
/********************************************************************************************
                    mark and unmark species
********************************************************************************************/
void GBT_mark_all(GBDATA *gb_main, int flag)
{
    GBDATA *gb_species;
    GB_push_transaction(gb_main);

    if (flag == 2) {
        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species) )
        {
            GB_write_flag(gb_species,!GB_read_flag(gb_species));
        }
    }
    else {
        gb_assert(flag == 0 || flag == 1);

        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species) )
        {
            GB_write_flag(gb_species,flag);
        }
    }
    GB_pop_transaction(gb_main);
}
void GBT_mark_all_that(GBDATA *gb_main, int flag, int (*condition)(GBDATA*, void*), void *cd)
{
    GBDATA *gb_species;
    GB_push_transaction(gb_main);

    if (flag == 2) {
        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species) )
        {
            if (condition(gb_species, cd)) {
                GB_write_flag(gb_species,!GB_read_flag(gb_species));
            }
        }
    }
    else {
        gb_assert(flag == 0 || flag == 1);

        for (gb_species = GBT_first_species(gb_main);
             gb_species;
             gb_species = GBT_next_species(gb_species) )
        {
            int curr_flag = GB_read_flag(gb_species);
            if (curr_flag != flag && condition(gb_species, cd)) {
                GB_write_flag(gb_species,flag);
            }
        }
    }
    GB_pop_transaction(gb_main);
}

long GBT_count_marked_species(GBDATA *gb_main)
{
    long cnt = 0;
    GBDATA *gb_species_data;
    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    cnt = GB_number_of_marked_subentries(gb_species_data);
    GB_pop_transaction(gb_main);
    return cnt;
}

long GBT_count_species(GBDATA *gb_main) /* does not work in clients like ARB_EDIT4/ARB_DIST... (use GBT_recount_species)*/
{
    long cnt = 0;
    GBDATA *gb_species_data;
    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    cnt = GB_number_of_subentries(gb_species_data);
    GB_pop_transaction(gb_main);
    return cnt;
}

long GBT_recount_species(GBDATA *gb_main) /* workaround for GBT_count_species (use this in clients) */
{
    long    cnt     = 0;
    GBDATA *gb_species_data;
    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    cnt             = GB_rescan_number_of_subentries(gb_species_data);
    GB_pop_transaction(gb_main);
    return cnt;
}

char *GBT_store_marked_species(GBDATA *gb_main, int unmark_all)
{
    /* stores the currently marked species in a string
       if (unmark_all != 0) then unmark them too
    */

    void   *out   = GBS_stropen(10000);
    GBDATA *gb_species;
    int     first = 1;

    for (gb_species = GBT_first_marked_species(gb_main);
         gb_species;
         gb_species = GBT_next_marked_species(gb_species))
    {
        GBDATA  *gb_name = GB_find(gb_species, "name", 0, down_level);
        GB_CSTR  name    = GB_read_char_pntr(gb_name);

        if (first) {
            first = 0;
        }
        else {
            GBS_chrcat(out, ';');
        }
        GBS_strcat(out, name);
        if (unmark_all) GB_write_flag(gb_species, 0);
    }

    return GBS_strclose(out);
}

NOT4PERL GB_ERROR GBT_with_stored_species(GBDATA *gb_main, const char *stored, species_callback doit, int *clientdata) {
    /* call function 'doit' with all species stored in 'stored' */

#define MAX_NAME_LEN 20
    char     name[MAX_NAME_LEN+1];
    GB_ERROR error = 0;

    while (!error) {
        char   *p   = strchr(stored, ';');
        int     len = p ? (p-stored) : (int)strlen(stored);
        GBDATA *gb_species;

        gb_assert(len <= MAX_NAME_LEN);
        memcpy(name, stored, len);
        name[len] = 0;

        gb_species = GBT_find_species(gb_main, name);
        if (gb_species) {
            error = doit(gb_species, clientdata);
        }
        else {
            error = "Some stored species where not found.";
        }

        if (!p) break;
        stored = p+1;
    }
#undef MAX_NAME_LEN
    return error;
}

static GB_ERROR restore_mark(GBDATA *gb_species, int *clientdata) {
    GBUSE(clientdata);
    GB_write_flag(gb_species, 1);
    return 0;
}

GB_ERROR GBT_restore_marked_species(GBDATA *gb_main, const char *stored_marked) {
    /* restores the species-marks to a state currently saved
       into 'stored_marked' by GBT_store_marked_species
    */

    GBT_mark_all(gb_main, 0);   /* unmark all species */
    return GBT_with_stored_species(gb_main, stored_marked, restore_mark, 0);
}

/********************************************************************************************
                    read species information
********************************************************************************************/

GBDATA *GBT_read_sequence(GBDATA *gb_species,const char *use)
{
    GBDATA *gb_ali = GB_find(gb_species,use,0,down_level);
    if (!gb_ali) return 0;
    return GB_find(gb_ali,"data",0,down_level);
}

char *GBT_read_name(GBDATA *gb_species)
{
    GBDATA *gb_name = GB_find(gb_species,"name",0,down_level);
    if (!gb_name) return 0;
    return GB_read_string(gb_name);
}

/********************************************************************************************
                    alignment procedures
********************************************************************************************/

char *GBT_get_default_alignment(GBDATA *gb_main)
{
    GBDATA *gb_use;
    gb_use = GB_search(gb_main,"presets/use",GB_FIND);
    if (!gb_use) return 0;
    return GB_read_string(gb_use);
}

GB_ERROR GBT_set_default_alignment(GBDATA *gb_main,const char *alignment_name)
{
    GBDATA *gb_use;
    gb_use = GB_search(gb_main,"presets/use",GB_STRING);
    if (!gb_use) return 0;
    return GB_write_string(gb_use,alignment_name);
}

char *GBT_get_default_helix(GBDATA *gb_main)
{
    gb_main = gb_main;
    return GB_STRDUP("HELIX");
}

char *GBT_get_default_helix_nr(GBDATA *gb_main)
{
    gb_main = gb_main;
    return GB_STRDUP("HELIX_NR");
}

char *GBT_get_default_ref(GBDATA *gb_main)
{
    gb_main = gb_main;
    return GB_STRDUP("ECOLI");
}


GBDATA *GBT_get_alignment(GBDATA *gb_main,const char *use)
{
    GBDATA *gb_alignment_name;
    GBDATA *gb_presets;
    gb_presets = GB_search(gb_main,"presets",GB_CREATE_CONTAINER);
    gb_alignment_name = GB_find(gb_presets,"alignment_name",use,down_2_level);
    if (!gb_alignment_name) {
        GB_export_error("alignment '%s' not found",use);
        return 0;
    }
    return GB_get_father(gb_alignment_name);
}

long GBT_get_alignment_len(GBDATA *gb_main,const char *use)
{
    GBDATA *gb_alignment;
    GBDATA *gb_alignment_len;
    gb_alignment = GBT_get_alignment(gb_main,use);
    if (!gb_alignment) return -1;
    gb_alignment_len = GB_search(gb_alignment,"alignment_len",GB_FIND);
    return GB_read_int(gb_alignment_len);
}

GB_ERROR GBT_set_alignment_len(GBDATA *gb_main, const char *use,long new_len)
{
    GBDATA *gb_alignment;
    GBDATA *gb_alignment_len;
    GBDATA *gb_alignment_aligned;
    GB_ERROR error = 0;
    gb_alignment = GBT_get_alignment(gb_main,use);
    if (!gb_alignment) return GB_export_error("Alignment '%s' not found",use);
    gb_alignment_len = GB_search(gb_alignment,"alignment_len",GB_FIND);
    gb_alignment_aligned = GB_search(gb_alignment,"aligned",GB_FIND);
    GB_push_my_security(gb_main);
    error = GB_write_int(gb_alignment_len,new_len);             /* write new len */
    if (!error) error = GB_write_int(gb_alignment_aligned,0);       /* sequences will be unaligned */
    GB_pop_my_security(gb_main);
    return error;
}

int GBT_get_alignment_aligned(GBDATA *gb_main,const char *use)
{
    GBDATA *gb_alignment;
    GBDATA *gb_alignment_aligned;
    gb_alignment = GBT_get_alignment(gb_main,use);
    if (!gb_alignment) return -1;
    gb_alignment_aligned = GB_search(gb_alignment,"aligned",GB_FIND);
    return GB_read_int(gb_alignment_aligned);
}

char *GBT_get_alignment_type_string(GBDATA *gb_main,const char *use)
{
    GBDATA *gb_alignment;
    GBDATA *gb_alignment_type;

    gb_alignment = GBT_get_alignment(gb_main,use);
    if (!gb_alignment) {
        return 0;
    }
    gb_alignment_type = GB_search(gb_alignment,"alignment_type",GB_FIND);
    return GB_read_string(gb_alignment_type);
}

GB_alignment_type GBT_get_alignment_type(GBDATA *gb_main, const char *use)
{
    char *ali_type;
    GB_alignment_type at;

    ali_type = GBT_get_alignment_type_string(gb_main, use);
    at = GB_AT_UNKNOWN;

    if (ali_type) {
        switch(ali_type[0])
        {
            case 'r': if (strcmp(ali_type, "rna")==0) at = GB_AT_RNA; break;
            case 'd': if (strcmp(ali_type, "dna")==0) at = GB_AT_DNA; break;
            case 'a': if (strcmp(ali_type, "ami")==0) at = GB_AT_AA; break;
            case 'p': if (strcmp(ali_type, "pro")==0) at = GB_AT_AA; break;
            default: ad_assert(0); break;
        }
        free(ali_type);
    }
    return at;
}

GB_BOOL GBT_is_alignment_protein(GBDATA *gb_main,const char *alignment_name) {
    return GBT_get_alignment_type(gb_main,alignment_name) == GB_AT_AA;
}

/********************************************************************************************
                    check routines
********************************************************************************************/
GB_ERROR GBT_check_arb_file(const char *name)
     /* Checks whether the name of a file seemed to be an arb file */
     /* if == 0 it was an arb file */
{
    FILE *in;
    long i;
    char buffer[100];
    if (strchr(name,':')) return 0;
    in = fopen(name,"r");
    if (!in) return GB_export_error("Cannot find file '%s'",name);
    i = gb_read_in_long(in, 0);
    if ( (i== 0x56430176) || (i == GBTUM_MAGIC_NUMBER) || (i == GBTUM_MAGIC_REVERSED)) {
        fclose(in);
        return 0;
    }
    rewind(in);
    fgets(buffer,50,in);
    fclose(in);
    if (!strncmp(buffer,"/*ARBDB AS",10)) {
        return 0;
    };


    return GB_export_error("'%s' is not an arb file",name);
}

/********************************************************************************************
                    analyse the database
********************************************************************************************/
#define GBT_SUM_LEN 4096
/* maximum length of path */

struct {
    GB_HASH *hash_table;
    int count;
    char **result;
    GB_TYPES type;
    char *buffer;
} gbs_scan_db_data;

void gbt_scan_db_rek(GBDATA *gbd,char *prefix, int deep)
{
    GB_TYPES type = GB_read_type(gbd);
    GBDATA *gb2;
    const char *key;
    int len_of_prefix;
    if (type == GB_DB) {
        len_of_prefix = strlen(prefix);
        for (   gb2 = GB_find(gbd,0,0,down_level);  /* find everything */
                gb2;
                gb2 = GB_find(gb2,0,0,this_level|search_next))
        {
            if (deep){
                key = GB_read_key_pntr(gb2);
                sprintf(&prefix[len_of_prefix],"/%s",key);
                gbt_scan_db_rek(gb2,prefix,1);
            }
            else {
                prefix[len_of_prefix] = 0;
                gbt_scan_db_rek(gb2,prefix,1);
            }
        }
        prefix[len_of_prefix] = 0;
    }
    else {
        if (GB_check_hkey(prefix+1)) {
            prefix = prefix;        /* for debugging purpose */
        }
        else {
            prefix[0] = (char)type;
            GBS_incr_hash( gbs_scan_db_data.hash_table, prefix);
        }
    }
}

long gbs_scan_db_count(const char *key,long val)
{
    gbs_scan_db_data.count++;
    key = key;
    return val;
}

long gbs_scan_db_insert(const char *key,long val, void *v_datapath)
{
    if (!v_datapath) {
        gbs_scan_db_data.result[gbs_scan_db_data.count++]  = GB_STRDUP(key);
    }
    else {
        char *datapath = (char*)v_datapath;
        if (GBS_strscmp(datapath, key+1) == 0) { // datapath matches
            char *subkey = GB_STRDUP(key+strlen(datapath)); // cut off prefix
            subkey[0]    = key[0]; // copy type

            gbs_scan_db_data.result[gbs_scan_db_data.count++] = subkey;
        }
    }
    return val;
}

long gbs_scan_db_compare(const char *left,const char *right){
    return strcmp(&left[1],&right[1]);
}


char **GBT_scan_db(GBDATA *gbd, const char *datapath) {
    /* returns a NULL terminated array of 'strings'
       each string is the path to a node beyond gbd;
       every string exists only once
       the first character of a string is the type of the entry
       the strings are sorted alphabetically !!!

       if datapath              != 0, only keys with prefix datapath are scanned and
       the prefix is removed from the resulting key_names
    */
    gbs_scan_db_data.hash_table  = GBS_create_hash(1024,0);
    gbs_scan_db_data.buffer      = (char *)malloc(GBT_SUM_LEN);
    strcpy(gbs_scan_db_data.buffer,"");
    gbt_scan_db_rek(gbd, gbs_scan_db_data.buffer,0);

    gbs_scan_db_data.count = 0;
    GBS_hash_do_loop(gbs_scan_db_data.hash_table,gbs_scan_db_count);

    gbs_scan_db_data.result = (char **)GB_calloc(sizeof(char *),gbs_scan_db_data.count+1);
    /* null terminated result */

    gbs_scan_db_data.count = 0;
    GBS_hash_do_loop2(gbs_scan_db_data.hash_table,gbs_scan_db_insert, (void*)datapath);

    GBS_free_hash(gbs_scan_db_data.hash_table);

    GB_mergesort((void **)gbs_scan_db_data.result,0,gbs_scan_db_data.count, (gb_compare_two_items_type)gbs_scan_db_compare,0);

    free(gbs_scan_db_data.buffer);
    return gbs_scan_db_data.result;
}

/********************************************************************************************
                send a message to the db server to AWAR_ERROR_MESSAGES
********************************************************************************************/

static void new_gbt_message_created_cb(GBDATA *gb_pending_messages, int *cd, GB_CB_TYPE cbt) {
    static int avoid_deadlock = 0;

    GBUSE(cd);
    GBUSE(cbt);

    if (!avoid_deadlock) {
        GBDATA *gb_msg;

        avoid_deadlock++;
        GB_push_transaction(gb_pending_messages);

        for (gb_msg = GB_find(gb_pending_messages, "msg", 0, down_level); gb_msg;) {
            {
                const char *msg = GB_read_char_pntr(gb_msg);
                GB_warning("%s", msg);
            }
            {
                GBDATA *gb_next_msg = GB_find(gb_msg, "msg", 0, this_level|search_next);
                GB_delete(gb_msg);
                gb_msg              = gb_next_msg;
            }
        }

        GB_pop_transaction(gb_pending_messages);
        avoid_deadlock--;
    }
}

void GBT_install_message_handler(GBDATA *gb_main) {
    GBDATA *gb_pending_messages;

    GB_push_transaction(gb_main);
    gb_pending_messages = GB_search(gb_main, AWAR_ERROR_CONTAINER, GB_CREATE_CONTAINER);
    GB_add_callback(gb_pending_messages, GB_CB_SON_CREATED, new_gbt_message_created_cb, 0);
    GB_pop_transaction(gb_main);

#if defined(DEBUG) && 0
    GBT_message(GB_get_root(gb_pending_messages), GBS_global_string("GBT_install_message_handler installed for gb_main=%p", gb_main));
#endif /* DEBUG */
}


void GBT_message(GBDATA *gb_main, const char *msg) {
    // when called in client(or server) this always causes the DB server to show the message (as GB_warning)
    GBDATA *gb_pending_messages;
    GBDATA *gb_msg;

    gb_assert(msg);

    GB_push_transaction(gb_main);

    gb_pending_messages = GB_search(gb_main, AWAR_ERROR_CONTAINER, GB_CREATE_CONTAINER);
    gb_msg              = GB_create(gb_pending_messages, "msg", GB_STRING);

    GB_write_string(gb_msg, msg);

    GB_pop_transaction(gb_main);
}



GB_HASH *GBT_generate_species_hash(GBDATA *gb_main,int ncase)
{
    GB_HASH *hash = GBS_create_hash(GBS_SPECIES_HASH_SIZE,ncase);
    GBDATA *gb_species;
    GBDATA *gb_name;
    for (   gb_species = GBT_first_species(gb_main);
            gb_species;
            gb_species = GBT_next_species(gb_species)){
        gb_name = GB_find(gb_species,"name",0,down_level);
        if (!gb_name) continue;
        GBS_write_hash(hash,GB_read_char_pntr(gb_name),(long)gb_species);
    }
    return hash;
}

GB_HASH *GBT_generate_marked_species_hash(GBDATA *gb_main)
{
    GB_HASH *hash = GBS_create_hash(GBS_SPECIES_HASH_SIZE,0);
    GBDATA *gb_species;
    GBDATA *gb_name;
    for (   gb_species = GBT_first_marked_species(gb_main);
            gb_species;
            gb_species = GBT_next_marked_species(gb_species)){
        gb_name = GB_find(gb_species,"name",0,down_level);
        if (!gb_name) continue;
        GBS_write_hash(hash,GB_read_char_pntr(gb_name),(long)gb_species);
    }
    return hash;
}

GB_HASH *GBT_generate_SAI_hash(GBDATA *gb_main)
{
    GB_HASH *hash = GBS_create_hash(1000, 0);
    GBDATA  *gb_ext;

    for (gb_ext = GBT_first_SAI(gb_main);
         gb_ext;
         gb_ext = GBT_next_SAI(gb_ext))
    {
        GBDATA *gb_name = GB_find(gb_ext, "name", 0, down_level);
        if (gb_name) {
            GBS_write_hash(hash, GB_read_char_pntr(gb_name), (long)gb_ext);
        }
    }
    return hash;
}

/********************************************************************************************
                Rename one or many species (only one session at a time/ uses
                commit abort transaction)
********************************************************************************************/
struct gbt_renamed_struct {
    int     used_by;
    char    data[1];

};

struct gbt_rename_struct {
    GB_HASH *renamed_hash;
    GB_HASH *old_species_hash;
    GB_HASH *new_species_hash;
    GBDATA *gb_main;
    GBDATA *gb_species_data;
    int all_flag;
} gbtrst;

/* starts the rename session, if allflag = 1 then the whole database is to be
        renamed !!!!!!! */
GB_ERROR GBT_begin_rename_session(GBDATA *gb_main, int all_flag)
{
    GB_ERROR error = GB_push_transaction(gb_main);
    long nspecies;

    if (error) return error;

    gbtrst.gb_main = gb_main;
    gbtrst.gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    nspecies = GB_number_of_subentries(gbtrst.gb_species_data);
    if (!all_flag) {
        gbtrst.renamed_hash = GBS_create_hash(256,0);
        gbtrst.old_species_hash = 0;
        gbtrst.new_species_hash = 0;
    }else{
        gbtrst.renamed_hash = GBS_create_hash(nspecies,0);
        gbtrst.old_species_hash = GBT_generate_species_hash(gb_main,0);
        gbtrst.new_species_hash = GBS_create_hash(nspecies,1);
    }
    gbtrst.all_flag = all_flag;
    return error;
}
/* returns errors */
GB_ERROR GBT_rename_species(const char *oldname,const  char *newname)
{
    GBDATA *gb_species;
    GBDATA *gb_name;
    GB_ERROR error;

#if defined(DEBUG) && 1
    if (isdigit(oldname[0])) {
        printf("oldname='%s' newname='%s'\n", oldname, newname);
    }
#endif

    if (!gbtrst.all_flag) {
        if (gbtrst.new_species_hash){
            gb_species = (GBDATA *)GBS_read_hash(gbtrst.new_species_hash,newname);
        }else{
            gb_species = GBT_find_species_rel_species_data(gbtrst.gb_species_data,newname);
        }
        if (gb_species) {
            return GB_export_error("Species name '%s' already exists ",newname);
        }
    }

    if (gbtrst.old_species_hash){
        gb_species = (GBDATA *)GBS_read_hash(gbtrst.old_species_hash,oldname);
    }
    else {
        gb_species = GBT_find_species_rel_species_data(gbtrst.gb_species_data,oldname);
    }

    if (!gb_species) {
        return GB_export_error("Species name '%s' does not exists: ",oldname);
    }
    gb_name = GB_find(gb_species,"name",0,down_level);
    GB_push_my_security(gbtrst.gb_main);
    error = GB_write_string(gb_name,newname);
    GB_pop_my_security(gbtrst.gb_main);
    if (!error){
        struct gbt_renamed_struct *rns;
        if (gbtrst.old_species_hash){
            GBS_write_hash(gbtrst.old_species_hash,oldname,0);
            GBS_write_hash(gbtrst.new_species_hash,newname,(long)gb_species);
        }
        rns = (struct gbt_renamed_struct *)GB_calloc(strlen(newname) + sizeof (struct gbt_renamed_struct),sizeof(char));
        strcpy(&rns->data[0],newname);
        GBS_write_hash(gbtrst.renamed_hash,oldname,(long)rns);
    }
    return error;
}


GB_ERROR GBT_abort_rename_session(void)
{
    GB_ERROR error;
    if (gbtrst.renamed_hash) GBS_free_hash_free_pointer(gbtrst.renamed_hash);
    gbtrst.renamed_hash = 0;
    if (gbtrst.old_species_hash) GBS_free_hash(gbtrst.old_species_hash);
    if (gbtrst.new_species_hash) GBS_free_hash(gbtrst.new_species_hash);
    gbtrst.old_species_hash = 0;
    gbtrst.new_species_hash = 0;
    error = GB_abort_transaction(gbtrst.gb_main);
    return error;
}

GB_ERROR gbt_rename_tree_rek(GBT_TREE *tree,int tree_index){
    char buffer[256];
    static int counter = 0;
    if (!tree) return 0;
    if (tree->is_leaf){
        if (tree->name){
            struct gbt_renamed_struct *rns = (struct gbt_renamed_struct *)GBS_read_hash(gbtrst.renamed_hash,tree->name);
            if (rns){
                char *newname;
                if (rns->used_by == tree_index){ /* species more than once in the tree */
                    sprintf(buffer,"%s_%i",rns->data,counter++);
                    GB_warning("Species '%s' more than once in a tree, creating zombie '%s'",tree->name,buffer);
                    newname = buffer;
                }else{
                    newname = &rns->data[0];
                }
                free(tree->name);
                tree->name = GB_STRDUP(newname);
                rns->used_by = tree_index;
            }
        }
    }else{
        gbt_rename_tree_rek(tree->leftson,tree_index);
        gbt_rename_tree_rek(tree->rightson,tree_index);
    }
    return 0;
}

#ifdef __cplusplus
extern "C"
#endif
GB_ERROR GBT_commit_rename_session(int (*show_status)(double gauge)){
    GB_ERROR error;
    char **tree_names;
    char **tree_name;
    GBT_TREE *tree;
    int tree_index = 1;
    int tree_count = 0;
    int count = 0;

    tree_names = GBT_get_tree_names(gbtrst.gb_main);
    for (tree_name = tree_names; tree_name[0]; tree_name++) {
        tree_count++;
    }

    for (tree_name = tree_names; tree_name[0]; tree_name++) {
        count++;
        if (show_status) show_status((double)count/tree_count);
        tree = GBT_read_tree(gbtrst.gb_main,*tree_name,-sizeof(GBT_TREE));
        if (tree){
            gbt_rename_tree_rek(tree,tree_index++);
            GBT_write_tree(gbtrst.gb_main,0,*tree_name,tree);
            GBT_delete_tree(tree);
        }
    }
    if (gbtrst.renamed_hash) GBS_free_hash_free_pointer(gbtrst.renamed_hash);
    gbtrst.renamed_hash = 0;
    if (gbtrst.old_species_hash) GBS_free_hash(gbtrst.old_species_hash);
    if (gbtrst.new_species_hash) GBS_free_hash(gbtrst.new_species_hash);
    gbtrst.old_species_hash = 0;
    gbtrst.new_species_hash = 0;
    if (tree_names) GBS_free_names(tree_names);
    error = GB_pop_transaction(gbtrst.gb_main);
    return error;
}

GBDATA **GBT_gen_species_array(GBDATA *gb_main, long *pspeccnt)
{
    GBDATA *gb_species;
    GBDATA *gb_species_data = GBT_find_or_create(gb_main,"species_data",7);
    GBDATA **result;
    *pspeccnt = 0;
    for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
         gb_species;
         gb_species = GBT_next_species(gb_species)){
        (*pspeccnt) ++;
    }
    result = (GBDATA **)malloc((size_t)(sizeof(GBDATA *)* (*pspeccnt)));
    *pspeccnt = 0;
    for (gb_species = GBT_first_species_rel_species_data(gb_species_data);
         gb_species;
         gb_species = GBT_next_species(gb_species)){
        result[(*pspeccnt)++]=gb_species;
    }
    return result;
}


char *GBT_read_string(GBDATA *gb_main, const char *awar_name){
    /* Search and read an database field */
    GBDATA *gbd;
    char *result;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        fprintf(stderr,"GBT_read_string error: Cannot find %s\n",awar_name);
        GB_pop_transaction(gb_main);
        return GB_STRDUP("");
    }
    result = GB_read_string(gbd);
    GB_pop_transaction(gb_main);
    return result;
}

long GBT_read_int(GBDATA *gb_main, const char *awar_name)
{
    GBDATA *gbd;
    long result;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        fprintf(stderr,"GBT_read_int error: Cannot find %s\n",awar_name);
        GB_pop_transaction(gb_main);
        return 0;
    }
    result = GB_read_int(gbd);
    GB_pop_transaction(gb_main);
    return result;
}

double GBT_read_float(GBDATA *gb_main, const char *awar_name)
{
    GBDATA *gbd;
    float result;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        fprintf(stderr,"GBT_read_float error: Cannot find %s\n",awar_name);
        GB_pop_transaction(gb_main);
        return 0.0;
    }
    result = GB_read_float(gbd);
    GB_pop_transaction(gb_main);
    return result;
}

GBDATA *GBT_search_string(GBDATA *gb_main, const char *awar_name, const char *def){
    GBDATA *gbd;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        gbd = GB_search(gb_main,awar_name,GB_STRING);
        GB_write_string(gbd,def);
    }
    GB_pop_transaction(gb_main);
    return gbd;
}

GBDATA *GBT_search_int(GBDATA *gb_main, const char *awar_name, int def){
    GBDATA *gbd;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        gbd = GB_search(gb_main,awar_name,GB_INT);
        GB_write_int(gbd,def);
    }
    GB_pop_transaction(gb_main);
    return gbd;
}

GBDATA *GBT_search_float(GBDATA *gb_main, const char *awar_name, double def){
    GBDATA *gbd;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        gbd = GB_search(gb_main,awar_name,GB_FLOAT);
        GB_write_float(gbd,def);
    }
    GB_pop_transaction(gb_main);
    return gbd;
}

char *GBT_read_string2(GBDATA *gb_main, const char *awar_name, const char *def)
{
    GBDATA *gbd;
    char *result;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        gbd = GB_search(gb_main,awar_name,GB_STRING);
        GB_write_string(gbd,def);
    }
    result = GB_read_string(gbd);
    GB_pop_transaction(gb_main);
    return result;
}

long GBT_read_int2(GBDATA *gb_main, const char *awar_name, long def)
{
    GBDATA *gbd;
    long result;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        gbd = GB_search(gb_main,awar_name,GB_INT);
        GB_write_int(gbd,def);
    }
    result = GB_read_int(gbd);
    GB_pop_transaction(gb_main);
    return result;
}

double GBT_read_float2(GBDATA *gb_main, const char *awar_name, double def)
{
    GBDATA *gbd;
    double result;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FIND);
    if (!gbd) {
        gbd = GB_search(gb_main,awar_name,GB_FLOAT);
        GB_write_float(gbd,def);
    }
    result = GB_read_float(gbd);
    GB_pop_transaction(gb_main);
    return result;
}

GB_ERROR GBT_write_string(GBDATA *gb_main, const char *awar_name, const char *def)
{
    GBDATA *gbd;
    GB_ERROR error;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_STRING);
    error = GB_write_string(gbd,def);
    GB_pop_transaction(gb_main);
    return error;
}

GB_ERROR GBT_write_int(GBDATA *gb_main, const char *awar_name, long def)
{
    GBDATA *gbd;
    GB_ERROR error;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_INT);
    error = GB_write_int(gbd,def);
    GB_pop_transaction(gb_main);
    return error;
}

GB_ERROR GBT_write_float(GBDATA *gb_main, const char *awar_name, double def)
{
    GBDATA *gbd;
    GB_ERROR error;
    GB_push_transaction(gb_main);
    gbd = GB_search(gb_main,awar_name,GB_FLOAT);
    error = GB_write_float(gbd,def);
    GB_pop_transaction(gb_main);
    return error;
}

GBDATA *GB_test_link_follower(GBDATA *gb_main,GBDATA *gb_link,const char *link){
    GBDATA *linktarget = GB_search(gb_main,"tmp/link/string",GB_STRING);
    GBUSE(gb_link);
    GB_write_string(linktarget,GBS_global_string("Link is '%s'",link));
    return GB_get_father(linktarget);
}

/********************************************************************************************
                    SAVE & LOAD
********************************************************************************************/

/** Open a database, create an index for species and extended names,
    disable saving the database in the PT_SERVER directory */

GBDATA *GBT_open(const char *path,const char *opent,const char *disabled_path)
{
    GBDATA *gbd = GB_open(path,opent);
    GBDATA *species_data;
    GBDATA *extended_data;
    GBDATA *gb_tmp;
    long    hash_size;

    if (!gbd) return gbd;
    if (!disabled_path) disabled_path = "$(ARBHOME)/lib/pts/*";
    GB_disable_path(gbd,disabled_path);
    GB_begin_transaction(gbd);

    if (!strchr(path,':')){
        species_data = GB_search(gbd, "species_data", GB_FIND);
        if (species_data){
            hash_size = GB_number_of_subentries(species_data);
            if (hash_size < GBT_SPECIES_INDEX_SIZE) hash_size = GBT_SPECIES_INDEX_SIZE;
            GB_create_index(species_data,"name",hash_size);

            extended_data = GB_search(gbd, "extended_data", GB_CREATE_CONTAINER);
            hash_size = GB_number_of_subentries(extended_data);
            if (hash_size < GBT_SAI_INDEX_SIZE) hash_size = GBT_SAI_INDEX_SIZE;
            GB_create_index(extended_data,"name",hash_size);
        }
    }
    gb_tmp = GB_search(gbd,"tmp",GB_CREATE_CONTAINER);
    GB_set_temporary(gb_tmp);
    {               /* install link followers */
        GB_MAIN_TYPE *Main = GB_MAIN(gbd);
        Main->table_hash = GBS_create_hash(256,0);
        GB_install_link_follower(gbd,"REF",GB_test_link_follower);
    }
    GBT_install_table_link_follower(gbd);
    GB_commit_transaction(gbd);
    return gbd;
}

/********************************************************************************************
                    REMOTE COMMANDS
********************************************************************************************/

#define AWAR_REMOTE_BASE_TPL            "tmp/remote/%s/"
#define MAX_REMOTE_APPLICATION_NAME_LEN 30
#define MAX_REMOTE_AWAR_STRING_LEN      (11+MAX_REMOTE_APPLICATION_NAME_LEN+1+6+1)

struct gbt_remote_awars {
    char awar_action[MAX_REMOTE_AWAR_STRING_LEN];
    char awar_result[MAX_REMOTE_AWAR_STRING_LEN];
    char awar_awar[MAX_REMOTE_AWAR_STRING_LEN];
    char awar_value[MAX_REMOTE_AWAR_STRING_LEN];
};

static void gbt_build_remote_awars(struct gbt_remote_awars *awars, const char *application) {
    int length;

    gb_assert(strlen(application) <= MAX_REMOTE_APPLICATION_NAME_LEN);

    length = sprintf(awars->awar_action, AWAR_REMOTE_BASE_TPL, application);
    gb_assert(length < (MAX_REMOTE_AWAR_STRING_LEN-6)); // Note :  6 is length of longest name appended below !

    strcpy(awars->awar_result, awars->awar_action);
    strcpy(awars->awar_awar, awars->awar_action);
    strcpy(awars->awar_value, awars->awar_action);

    strcpy(awars->awar_action+length, "action");
    strcpy(awars->awar_result+length, "result");
    strcpy(awars->awar_awar+length,   "awar");
    strcpy(awars->awar_value+length,  "value");
}

static GBDATA *gbt_remote_search_awar(GBDATA *gb_main, const char *awar_name) {
    GBDATA *gb_action;
    while (1) {
        GB_begin_transaction(gb_main);
        gb_action = GB_search(gb_main, awar_name, GB_FIND);
        GB_commit_transaction(gb_main);
        if (gb_action) break;
        GB_usleep(2000);
    }
    return gb_action;
}

static GB_ERROR gbt_wait_for_remote_action(GBDATA *gb_main, GBDATA *gb_action, const char *awar_read) {
    GB_ERROR error = 0;

    /* wait to end of action */
    while(1) {
        char *ac;
        GB_usleep(2000);
        GB_begin_transaction(gb_main);
        ac = GB_read_string(gb_action);
        if (ac[0] == 0) { // action has been cleared from remote side
            GBDATA *gb_result = GB_search(gb_main, awar_read, GB_STRING);
            error             = GB_read_char_pntr(gb_result); // check for errors
            free(ac);
            GB_commit_transaction(gb_main);
            break;
        }
        free(ac);
        GB_commit_transaction(gb_main);
    }

    return error; // may be error or result
}

GB_ERROR GBT_remote_action(GBDATA *gb_main, const char *application, const char *action_name){
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_action;

    gbt_build_remote_awars(&awars, application);
    gb_action = gbt_remote_search_awar(gb_main, awars.awar_action);

    GB_begin_transaction(gb_main);
    GB_write_string(gb_action, action_name); /* write command */
    GB_commit_transaction(gb_main);

    return gbt_wait_for_remote_action(gb_main, gb_action, awars.awar_result);
}

GB_ERROR GBT_remote_awar(GBDATA *gb_main, const char *application, const char *awar_name, const char *value) {
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_awar;

    gbt_build_remote_awars(&awars, application);
    gb_awar = gbt_remote_search_awar(gb_main, awars.awar_awar);

    {
        GBDATA *gb_value;

        GB_begin_transaction(gb_main);
        gb_value = GB_search(gb_main, awars.awar_value, GB_STRING);
        GB_write_string(gb_awar, awar_name);
        GB_write_string(gb_value, value);
        GB_commit_transaction(gb_main);
    }

    return gbt_wait_for_remote_action(gb_main, gb_awar, awars.awar_result);
}

const char *GBT_remote_read_awar(GBDATA *gb_main, const char *application, const char *awar_name) {
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_awar;
    const char              *result = 0;

    gbt_build_remote_awars(&awars, application);
    gb_awar = gbt_remote_search_awar(gb_main, awars.awar_awar);

    {
        GBDATA *gb_action;

        GB_begin_transaction(gb_main);
        gb_action = GB_search(gb_main, awars.awar_action, GB_STRING);
        GB_write_string(gb_awar, awar_name);
        GB_write_string(gb_action, "AWAR_REMOTE_READ");
        GB_commit_transaction(gb_main);
    }

    result = gbt_wait_for_remote_action(gb_main, gb_awar, awars.awar_value);
    return result;
}

const char *GBT_remote_touch_awar(GBDATA *gb_main, const char *application, const char *awar_name) {
    struct gbt_remote_awars  awars;
    GBDATA                  *gb_awar;

    gbt_build_remote_awars(&awars, application);
    gb_awar = gbt_remote_search_awar(gb_main, awars.awar_awar);

    {
        GBDATA *gb_action;

        GB_begin_transaction(gb_main);
        gb_action = GB_search(gb_main, awars.awar_action, GB_STRING);
        GB_write_string(gb_awar, awar_name);
        GB_write_string(gb_action, "AWAR_REMOTE_TOUCH");
        GB_commit_transaction(gb_main);
    }

    return gbt_wait_for_remote_action(gb_main, gb_awar, awars.awar_result);
}

/*  ---------------------------------------------------------------------------------  */
/*      char *GBT_read_gene_sequence(GBDATA *gb_gene, GB_BOOL use_revComplement)       */
/*  ---------------------------------------------------------------------------------  */
/* GBT_read_gene_sequence is intentionally located here (otherwise we get serious linkage problems) */

char *GBT_read_gene_sequence(GBDATA *gb_gene, GB_BOOL use_revComplement) {
    /* read the sequence for the specified gene */

    GB_ERROR  error  = 0;
    char     *result = 0;

    GBDATA *gb_pos1       = GB_find(gb_gene, "pos_begin", 0, down_level);
    GBDATA *gb_pos2       = GB_find(gb_gene, "pos_end", 0, down_level);
    GBDATA *gb_complement = GB_find(gb_gene, "complement", 0, down_level);
    GBDATA *gb_species    = GB_get_father(GB_get_father(gb_gene));

    if (!gb_pos1) {
        if (!gb_pos2) {
            error = "no pos_begin/pos_end entries found";
        }
        else {
            error = "no pos_begin entry found";
        }
    }
    else if (!gb_pos2) {
        error = "no pos_end entry found";
    }
    else {
        long pos1       = gb_pos1 ? GB_read_int(gb_pos1) : -1;
        long pos2       = gb_pos2 ? GB_read_int(gb_pos2) : -1;
        int  complement = gb_complement ? GB_read_byte(gb_complement)!=0 : 0;

        if (pos1<1 || pos2<1 || pos2<pos1) {
            error = "Illegal gene positions";
        }
        else {
            GBDATA *gb_seq    = GBT_read_sequence(gb_species, "ali_genom");
            GBDATA *gb_joined = GB_find(gb_gene, "pos_joined", 0, down_level);
            int     parts     = gb_joined ? GB_read_int(gb_joined) : 1;

            if (parts>1) {
                error = "Using sequence of joined genes not supported yet!"; /* @@@ */
            }

            if (!error) {
                long seq_length = GB_read_count(gb_seq);
                if (pos2>seq_length) { // positions are [1..n]
                    error = GBS_global_string("Illegal gene position(s): endpos = %li, seq.length=%li", pos2, seq_length);
                }
            }

            if (!error) {
                const char *seq_data = GB_read_char_pntr(gb_seq);
                long        length   = pos2-pos1+1;

                result = (char*)malloc(length+1);
                memcpy(result, seq_data+pos1-1, length);
                result[length] = 0;

                if (complement && use_revComplement) {
                    char T_or_U;
                    error = GBT_determine_T_or_U(GB_AT_DNA, &T_or_U, "reverse-complement");
                    if (!error) GBT_reverseComplementNucSequence(result, length, T_or_U);
                }
                /* @@@ FIXME: sequence is wrong with reverse complement */

                if (error)  {
                    free(result);
                    result = 0;
                }
            }
        }
    }

    if (error) {
        GBDATA *gb_name   = GB_find(gb_gene, "name", 0, down_level);
        char   *gene_name = GBS_strdup(gb_name ? GB_read_char_pntr(gb_name) : "<unnamed gene>");
        char   *species_name;

        gb_name      = GB_find(gb_species, "name", 0, down_level);
        species_name = GBS_strdup(gb_name ? GB_read_char_pntr(gb_name) : "<unnamed species>");

        error = GB_export_error("%s (in %s/%s)", error, species_name, gene_name);
    }

    return result;
}


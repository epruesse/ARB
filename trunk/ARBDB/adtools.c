#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>

#include "adlocal.h"
#include "arbdb.h"
#include "arbdbt.h"

#define GBT_GET_SIZE 0
#define GBT_PUT_DATA 1

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
    for (	ali = GB_find(presets,"alignment",0,down_level);
            ali;
            ali = GB_find(ali,"alignment",0,this_level|search_next) ) {
        size ++;
    }
    erg = (char **)GB_calloc(sizeof(char *),(size_t)size+1);
    size = 0;
    for (	ali = GB_find(presets,"alignment",0,down_level);
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


GBDATA *GBT_create_alignment(	GBDATA *gbd,
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

    if (GB_check_key(name)) {
        return 0;
    }
    if (strncmp("ali_",name,4) ){
        error = GB_export_error("your alignment_name '%s' must start with 'ali_'",name);
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
    gbn =	GB_create(gbd,"alignment_name",GB_STRING);
    GB_write_string(gbn,name);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,6);

    gbn =	GB_create(gbd,"alignment_len",GB_INT);
    GB_write_int(gbn,len);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,0);

    gbn =	GB_create(gbd,"aligned",GB_INT);
    GB_write_int(gbn,aligned);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,0);
    gbn =	GB_create(gbd,"alignment_write_security",GB_INT);
    GB_write_int(gbn,security);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,6);

    gbn =	GB_create(gbd,"alignment_type",GB_STRING);
    GB_write_string(gbn,type);
    GB_write_security_delete(gbn,7);
    GB_write_security_write(gbn,0);
    return gbd;
}

GB_ERROR GBT_check_alignment(GBDATA *Main, GBDATA *preset_alignment)
     /* check if alignment is of the correct size,
		if all data is present
		sets the security deletes and writes
	*/
{
    GBDATA *species;
    GBDATA *ali;
    GBDATA *data;
    GBDATA *gbd;
    GBDATA *gb_len;
    GBDATA *gb_name;
    GBDATA *species_data;
    GBDATA *extended_data;

    long	ali_len;
    long	aligned;
    long	security_write;
    long	len;
    char 	*ali_name;
    GB_ERROR error;
    species_data = GBT_find_or_create(Main,"species_data",7);
    extended_data = GBT_find_or_create(Main,"extended_data",7);
    aligned = 1;
    gbd = GB_find(preset_alignment,"alignment_name",0,down_level);
    if (!gbd) return GB_export_error("alignment_name not found");
    ali_name=GB_read_string(gbd);

    gbd = GB_find(preset_alignment,"alignment_write_security",0,down_level);
    if (!gbd) return GB_export_error("alignment_write_security not found");
    security_write=GB_read_int(gbd);

    gb_len = GB_find(preset_alignment,"alignment_len",0,down_level);
    if (!gb_len) return GB_export_error("alignment_len not found");
    ali_len=-1;

    for (	species = GBT_first_species_rel_species_data(species_data);
            species;
            species = GBT_next_species(species) ){
        gb_name = GB_find(species,"name",0,down_level);
        if (!gb_name) {
            gb_name = GB_create(species,"name",GB_STRING);
            GB_write_string(gb_name,"unknown");
        }
        GB_write_security_delete(gb_name,7);
        GB_write_security_write(gb_name,6);


        ali = GB_find(species,ali_name,0,down_level);
        if (ali){
            /* GB_write_security_delete(ali,security_write); */
            data = GB_find(ali,"data",0,down_level);
            if (data) {
                if (GB_read_type(data) != GB_STRING){
                    GB_delete(data);
                    GB_internal_error("CHECK ERROR unknown type\n");
                    return GB_export_error("CHECK ERROR unknown data type",GB_read_key_pntr(data));
                }
            }
            if (!data) {
                data = GB_create(ali,"data",GB_STRING);
                GB_write_string(data,"sorry, key data not found");
                printf("CHECK ERROR key data not found\n");
                return GB_export_error("CHECK ERROR unknown data");
            }
            len = GB_read_string_count(data);
            if (ali_len<0) ali_len = len;
            if (ali_len != len) {
                aligned = 0;
                if (len>ali_len) ali_len = len;
            }
            /* error = GB_write_security_write(data,security_write);
               if (error) return error;*/
            GB_write_security_delete(data,7);
        }
        GB_write_security_delete(species,security_write);
    }
    for (	species = GBT_first_SAI_rel_exdata(extended_data);
            species;
            species = GBT_next_SAI(species) ){
        gb_name = GB_find(species,"name",0,down_level);

        if (!gb_name) continue;
        GB_write_security_delete(gb_name,7);

        ali = GB_find(species,ali_name,0,down_level);
        if (ali){
            for (	data = GB_find(ali,0,0,down_level) ;
                    data;
                    data = GB_find(data,0,0,this_level|search_next)){
                long type = GB_read_type(data);
                if (type == GB_DB || type < GB_BITS) continue;
                len = GB_read_count(data);
                if (ali_len<0) ali_len = len;
                if (len>ali_len) ali_len = len;
            }
        }
    }


    if (GB_read_int(gb_len) != ali_len) {
        error = GB_write_int(gb_len,ali_len);
        if (error) return error;
    }
    gbd = GB_search(preset_alignment,"aligned",GB_INT);
    if (GB_read_int(gbd)!= aligned) {
        error = GB_write_int(gbd,aligned);
        if (error) return error;
    }
    free(ali_name);
    return 0;
}

GB_ERROR GBT_rename_alignment(GBDATA *gbd,const char *source,const char *dest, int copy, int dele)
{
    /* 	if copy == 1 then create a copy
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
    for (	gb_species = GB_find(gb_species_data,"species",0,down_level);
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

    for (	gb_extended = GB_find(gb_extended_data,"extended",0,down_level);
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

GB_ERROR GBT_check_data(GBDATA *Main,char *alignment_name)
{
    GBDATA *gb_presets;
    GBDATA *gb_ali;
    GBDATA *gb_sd;
    GBDATA *gb_ed;
    GBDATA *gb_td;
    GBDATA *gb_use;
    GBDATA *gbd;
    GB_ERROR error;
    char buffer[256];

    gb_sd = GBT_find_or_create(Main,"species_data",7);
    gb_ed = GBT_find_or_create(Main,"extended_data",7);
    gb_td = GBT_find_or_create(Main,"tree_data",7);
    gb_presets = GBT_find_or_create(Main,"presets",7);

    if (alignment_name) {
        gbd = GB_find(	gb_presets,"alignment_name",alignment_name,down_2_level);
        if (!gbd) {
            return GB_export_error("Alignment '%s' does not exist",alignment_name);
        }
    }

    gb_use = GB_find(gb_presets, "use",0,down_level);
    if (!gb_use) {
        gbd = GB_find(	gb_presets,"alignment_name",alignment_name,down_2_level);
        if (gbd) {
            gb_use = GB_create(gb_presets,"use",GB_STRING);
            gbd = GB_get_father(gbd);
            gbd = GB_find(gbd,"alignment_name",0,down_level);
            sprintf(buffer,"%s",GB_read_char_pntr(gbd));
            GB_write_string(gb_use,buffer);
        }
    }
    for (	gb_ali = GB_find(gb_presets,"alignment",0,down_level);
            gb_ali;
            gb_ali = GB_find(gb_ali,"alignment",0,this_level|search_next) ){
        error = GBT_check_alignment(Main,gb_ali);
        if (error) return error;
    }
    return 0;
}

char *gbt_insert_delete(const char *source, long len, long destlen, long *newsize, long pos, long nchar, long mod, char insert_what, char insert_tail)
{
    /* removes or inserts bytes in a string
       len ==	len of source
       destlen == if != 0 than cut or append characters to get this len
       newsize		the result
       pos		where to insert/delete
       nchar		and how many items
       mod		size of an item
       insert_what	insert this character
    */

    char *newval;

    pos *=mod;
    nchar *= mod;
    len *= mod;
    destlen *= mod;
    if (!destlen) destlen = len;		/* if no destlen is set than keep len */

    if ( (nchar <0) && (pos-nchar>destlen)) nchar = pos-destlen;

    if (len > destlen) {
        len = destlen;			/* cut tail */
        newval = (char *)GB_calloc(sizeof(char),(size_t)(destlen+nchar+1));
    }else if (len < destlen) {				/* append tail */
        newval = (char *)malloc((size_t)(destlen+nchar+1));
        memset(newval,insert_tail,(int)(destlen+nchar));
        newval[destlen+nchar] = 0;
    }else{
        newval = (char *)GB_calloc(sizeof(char),(size_t)(len+nchar+1));
    }
    *newsize = (destlen+nchar)/mod;
    newval[*newsize] = 0;					/* only for strings */

    if (pos>len){		/* no place to insert / delete */
        GB_MEMCPY(newval,source,(size_t)len);
        return 0;
    }

    if (nchar < 0) {
        if (pos-nchar>len) nchar = -(len-pos);		/* clip maximum characters to delete */
    }

    if (nchar > 0)	{					/* insert */
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
    long	size =0,l;
    register long i;
    long	dlen;
    const char	*cchars;
    char	*chars;
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
            for (	gb = GB_find(gb_data,0,0,down_level);
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

            if (nchar > 0) {			/* insert character */
                if (		(pos >0 && chars[pos-1]	== '.')		/* @@@@ */
                            ||	chars[pos]	== '.') {
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
                                               "You tried to delete '%c' in species %s position %i  -> Operation aborted",
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
            if (chars) { error = GB_write_bits(gb_data,chars,dlen,'-');
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
            float	*floats;
            GB_CFLOAT	*cfloats;
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


GB_ERROR gbt_insert_character_species(GBDATA *gb_species,const char *ali_name, long len, long pos, long nchar, const char	*delete_chars)
{
    GBDATA *gb_name;
    GBDATA *gb_ali;
    char	*species_name = 0;
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

GB_ERROR gbt_insert_character(GBDATA *gb_species_data, const char *species,const char *name, long len, long pos, long nchar, const char	*delete_chars)
{
    GBDATA *gb_species;
    GB_ERROR error;

    for (	gb_species = GB_find(gb_species_data,species,0,down_level);
            gb_species;
            gb_species = GB_find(gb_species,species,0,this_level|search_next)){
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

    gb_presets		= GBT_find_or_create(Main,"presets",7);
    gb_species_data		= GBT_find_or_create(Main,"species_data",7);
    gb_extended_data	= GBT_find_or_create(Main,"extended_data",7);

    for (	gb_ali = GB_find(gb_presets,"alignment",0,down_level);
            gb_ali;
            gb_ali = GB_find(gb_ali,"alignment",0,this_level|search_next) ){
        gbd = GB_find(gb_ali,"alignment_name",alignment_name,down_level);
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


GB_ERROR GBT_insert_character(GBDATA *Main,char *alignment_name, long pos, long count, char *char_delete)
{
    /*	insert 'count' characters at position pos
        if count < 0	then delete position to position+|count| */
    GBDATA *gb_ali;
    GBDATA *gb_presets;
    GBDATA *gb_species_data;
    GBDATA *gb_extended_data;
    GBDATA *gbd;
    GBDATA *gb_len;
    long len;
    int	ch;
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
                else			char_delete_list[ch] = 1;
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
                    GB_export_error("GBT_insert_character: insert position %i exceeds length %i", pos, len);
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
     /*	frees a tree only in memory (not in the database)
		to delete the tree in Database
		just call GB_delete((GBDATA *)gb_tree);
	*/
{
    GB_ERROR error;
    if (tree->name) free(tree->name);
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
        error = GB_write_string(gb_name,node->name);
        if (error) return -1;
    }
    if (node->gb_node){			/* delete not used nodes else write id */
        gb_any = GB_find(node->gb_node,0,0,down_level);
        if (gb_any) {
            key = GB_read_key_pntr(gb_any);
            if (!strcmp(key,"id")){
                gb_any = GB_find(gb_any,0,0,this_level|search_next);
            }
        }

        if (gb_any){
            gb_id = GB_search(node->gb_node,"id",GB_INT);
            error = GB_write_int(gb_id,me);
            GB_write_usr_private(node->gb_node,0);
            if (error) return -1;
        }else{
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
    char buffer[40];		/* just real numbers */
    char	*c1;

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
            if (node->name) return dest+1+strlen(node->name)+1;	/* N name term */
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

GB_ERROR GBT_write_tree(GBDATA *gb_main, GBDATA *gb_tree, char *tree_name, GBT_TREE *tree)
{
    /* writes a tree to the database.
       If tree is loaded by function GBT_read_tree(..) than tree_name should be zero !!!!!!
       else gb_tree should be set to zero.


       to copy a tree call GB_copy((GBDATA *)dest,(GBDATA *)source);
       or set set recursively all tree->gb_node variables to zero (that unlinks the tree),

	*/
    GBDATA *gb_node,	*gb_tree_data;
    GBDATA *gb_node_next;
    GBDATA *gb_nnodes, *gbd;
    long	size;
    GB_ERROR error;
    char	*ctree,*t_size;
    GBDATA	*gb_ctree;

    if (!tree) return 0;
    if (gb_tree && tree_name) return GB_export_error("you cannot change tree name to %s",tree_name);
    if ((!gb_tree) && (!tree_name)) return GB_export_error("please specify a tree name");

    if (tree_name) {
        error = GBT_check_tree_name(tree_name);
        if (error) return error;
        gb_tree_data = GB_search(gb_main,"tree_data",GB_CREATE_CONTAINER);
        gb_tree = GB_search(gb_tree_data,tree_name,GB_CREATE_CONTAINER);
    }
    /* now delete all old style tree data */
    for (	gb_node = GB_find(gb_tree,"node",0,down_level);
            gb_node;
            gb_node = GB_find(gb_node,"node",0,this_level|search_next)){
        GB_write_usr_private(gb_node,1);
    }
    gb_ctree = GB_search(gb_tree,"tree",GB_STRING);
    t_size = gbt_write_tree_rek_new(gb_tree, tree, 0, GBT_GET_SIZE);
    ctree= (char *)GB_calloc(sizeof(char),(size_t)(t_size+1));
    t_size = gbt_write_tree_rek_new(gb_tree, tree, ctree, GBT_PUT_DATA);
    *(t_size) = 0;
    error = GB_set_compression(gb_main,0);		/* no more compressions */
    error = GB_write_string(gb_ctree,ctree);
    error = GB_set_compression(gb_main,-1);		/* allow all types of compression */
    free(ctree);
    if (!error) size = gbt_write_tree_nodes(gb_tree,tree,0);
    if (error || (size<0)) {
        GB_print_error();
        return GB_get_error();
    }

    for (	gb_node = GB_find(gb_tree,"node",0,down_level);	/* delete all ghost nodes */
            gb_node;
            gb_node = gb_node_next){
        gb_node_next = GB_find(gb_node,"node",0,this_level|search_next);
        gbd = GB_find(gb_node,"id",0,down_level);
        if (!gbd || GB_read_usr_private(gb_node)) {
            error = GB_delete(gb_node);
            if (error) GB_print_error();
        }
    }
    gb_nnodes = GB_search(gb_tree,"nnodes",GB_INT);
    error = GB_write_int(gb_nnodes,size);
    if (error) return error;
    return 0;
}

GB_ERROR GBT_write_tree_rem(GBDATA *gb_main,const char *tree_name, const char *remark){
    GBDATA *ali_cont = GBT_get_tree(gb_main,tree_name);
    GBDATA *tree_rem =	GB_search(ali_cont,"remark",	GB_STRING);
    return GB_write_string(tree_rem,remark);
}
/********************************************************************************************
					some tree read functions
********************************************************************************************/

GBT_TREE *gbt_read_tree_rek(char **data, long *startid, GBDATA **gb_tree_nodes, long structure_size, int size_of_tree)
{
    GBT_TREE *node;
    GBDATA *gb_group_name;
    char	c;
    char	*p1;

    static char *membase;
    if (structure_size>0){
        node = (GBT_TREE *)GB_calloc(1,(size_t)structure_size);
    }else{
        if (!startid[0]){
            membase =(char *)GB_calloc(size_of_tree+1,(size_t)(-2*structure_size));	/* because of inner nodes */
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
            if (gb_group_name) node->name = GB_read_string(gb_group_name);
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

GBT_TREE *GBT_read_tree_and_size(GBDATA *gb_main,const char *tree_name, long structure_size, int *tree_size) /* read a tree */ {
    GBDATA *gb_node;
    GBDATA *gbd;
    GBDATA *gb_tree;
    GBDATA *gb_nnodes;
    GBDATA *gb_tree_data;
    GBDATA **gb_tree_nodes;
    GB_ERROR error;
    char *fbuf; long size,i;
    GBDATA *gb_ctree;
    GBT_TREE *node;
    long startid[1]; char *cptr[1];
    if (!tree_name) {
        GB_export_error("zero treename"); return 0;
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
    if (gb_ctree) {				/* new style tree */
        gb_tree_nodes = (GBDATA **)GB_calloc(sizeof(GBDATA *),(size_t)size);
        for (	gb_node = GB_find(gb_tree,"node",0,down_level);
                gb_node;
                gb_node = GB_find(gb_node,"node",0,this_level|search_next)){
            gbd = GB_find(gb_node,"id",0,down_level);
            if (!gbd) continue;
            /*{ GB_export_error("ERROR while reading tree '%s' 4634",tree_name);return 0;}*/
            i = GB_read_int(gbd);
            if ( i<0 || i>= size ){
                GB_internal_error("An inner node of the tree is corrupt");
            }else{
                gb_tree_nodes[i] = gb_node;
            }
        }
        startid[0] = 0;
        fbuf = cptr[0] = GB_read_string(gb_ctree);
        node = gbt_read_tree_rek(cptr, startid, gb_tree_nodes, structure_size,(int)size);
        free((char *)gb_tree_nodes);
        free (fbuf);
        if (tree_size) *tree_size = size; /* return size of tree */
        return node;
    }
    GB_export_error("Sorry old tree format not supported any more");
    return 0;
}

GBT_TREE *GBT_read_tree(GBDATA *gb_main,const char *tree_name, long structure_size) {
    return GBT_read_tree_and_size(gb_main, tree_name, structure_size, 0);
}

/********************************************************************************************
					link the tree tips to the database
********************************************************************************************/
long gbt_count_nodes(GBT_TREE *tree){
    if ( tree->is_leaf ) {
        return 1;
    }
    return gbt_count_nodes(tree->leftson) + gbt_count_nodes(tree->rightson);
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
    if (show_status) nodes = gbt_count_nodes(tree) + 1;
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    error = gbt_link_tree_to_hash_rek(tree,gb_species_data,nodes,&counter);
    return error;
}



/********************************************************************************************
					load a tree from file system
********************************************************************************************/
#define gbt_read_char(io,c)                                     \
for (c=' '; (c==' ') || (c=='\n') || (c=='\t')|| (c=='[') ;){   \
    c=getc(io);                                                 \
    if (c == '\n' ) gbt_line_cnt++;                             \
    if (c == '[') {                                             \
        for (;(c!=']')&&(c!=EOF);c=getc(io) );                  \
        c=' ';                                                  \
    }                                                           \
}                                                               \
gbt_last_character = c;

#define gbt_get_char(io,c)                      \
c = getc(io);                                   \
if (c == '\n' ) gbt_line_cnt++;                 \
gbt_last_character = c;

int gbt_last_character = 0;
int gbt_line_cnt = 0;

double
gbt_read_number(FILE * input)
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

/** Read in a quoted string, double quotes ('') are replaced by (') */
char *gbt_read_quoted_string(FILE *input){
    char buffer[1024];
    int	c;
    char *s;
    s = &(buffer[0]);
    c = gbt_last_character;
    if ( c == '\'' ) {
        gbt_get_char(input,c);
        while ( (c!= EOF) && (c!='\'') ) {
        gbt_lt_double_quot:
            *(s++) = c;
            if ( ( ((long)s)-(long)&(buffer[0])) > 1000 ) {
                *s = 0;
                GB_export_error("Error while reading tree: Name '%s' longer than 1000 bytes",buffer);
                return 0;
            }
            gbt_get_char(input,c);
        }
        if (c == '\'') {
            gbt_read_char(input,c);
            if (c == '\'') goto gbt_lt_double_quot;
        }
    }else{
        while ( c== '_') gbt_read_char(input,c);
        while ( c== ' ') gbt_read_char(input,c);
        while ( (c != ':') && (c!= EOF) && (c!=',') &&
                (c!=';') && (c!= ')') ) {
            *(s++) = c;
            if ( ( ((long)s)-(long)&(buffer[0])) > 1000 ) {
                *s = 0;
                GB_export_error("Error while reading tree: Name '%s' longer than 1000 bytes",buffer);
                return 0;
            }
            gbt_read_char(input,c);
        }
    }
    *s = 0;
    return GB_STRDUP(buffer);
}


GBT_TREE *gbt_load_tree_rek(FILE *input,int structuresize)
{
    int             c;
    GB_BOOL		loop_flag;
    GBT_TREE *nod,*left,*right;

    if ( gbt_last_character == '(' ) {	/* make node */

        nod = (GBT_TREE *)GB_calloc(structuresize,1);
        gbt_read_char(input,c);
        left = gbt_load_tree_rek(input,structuresize);
        if (!left ) return 0;
        nod->leftson = left;
        left->father = nod;
        if (	gbt_last_character != ':' &&
                gbt_last_character != ',' &&
                gbt_last_character != ';' &&
                gbt_last_character != ')'
                ) {
            char *str = gbt_read_quoted_string(input);
            if (!str) return 0;
            left->name = str;
        }
        if (gbt_last_character !=  ':') {
            nod->leftlen = 1.0;
        }else{
            gbt_read_char(input,c);
            nod->leftlen = gbt_read_number(input);
        }
        if ( gbt_last_character == ')' ) {	/* only a single node !!!!, skip this node */
            GB_FREE(nod);			/* delete superflous father node */
            left->father = NULL;
            return left;
        }
        if ( gbt_last_character != ',' ) {
            GB_export_error ( "error in *.tree file:  ',' expected '%c' found; line %i",
                              gbt_last_character, gbt_line_cnt);
            return NULL;
        }
        loop_flag = GB_FALSE;
        while (gbt_last_character == ',') {
            if (loop_flag==GB_TRUE) {		/* multi branch tree */
                left = nod;
                nod = (GBT_TREE *)GB_calloc(structuresize,1);
                nod->leftson = left;
                nod->leftson->father = nod;
            }else{
                loop_flag = GB_TRUE;
            }
            gbt_read_char(input, c);
            right = gbt_load_tree_rek(input,structuresize);
            if (right == NULL) return NULL;
            nod->rightson = right;
            right->father = nod;
            if (	gbt_last_character != ':' &&
                    gbt_last_character != ',' &&
                    gbt_last_character != ';' &&
                    gbt_last_character != ')'
                    ) {
                char *str = gbt_read_quoted_string(input);
                if (!str) return 0;
                right->name = str;
            }
            if (gbt_last_character != ':') {
                nod->rightlen = 0.1;
            }else{
                gbt_read_char(input, c);
                nod->rightlen = gbt_read_number(input);
            }
        }
        if ( gbt_last_character != ')' ) {
            GB_export_error ( "error in .tree file:  ')' expected '%c' found: line %i ",
                              gbt_last_character,gbt_line_cnt);
            return NULL;
        }
        gbt_read_char(input,c);		/* remove the ')' */

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

/* Load a newick compatible tree from file int GBT_TREE, structure size should be >0, see
 GBT_read_tree for more information */
GBT_TREE *GBT_load_tree(char *path, int structuresize)
{
    FILE		*input;
    GBT_TREE	*tree;
    int		c;
    if ((input = fopen(path, "r")) == NULL) {
        GB_export_error("Import tree: file '%s' not found", path);
        return 0;
    }
    gbt_read_char(input,c);
    gbt_line_cnt = 1;
    tree = gbt_load_tree_rek(input,structuresize);
    fclose(input);
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
    for (	gb_tree = GB_find(gb_treedata,0,0,down_level);
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
    long	size;
    memset(buffer,0,1024);

    gb_tree = GBT_get_tree(gb_main,tree_name);
    if (!gb_tree) {
        GB_export_error("tree '%s' not found",tree_name);
        return 0;
    }

    gb_nnodes = GB_find(gb_tree,"nnodes",0,down_level);
    if (!gb_nnodes) {
        GB_export_error("nnodes not found",tree_name);
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
    for (	gb_tree = GB_find(gb_treedata,0,0,down_level);
            gb_tree;
            gb_tree = GB_find(gb_tree,0,0,this_level|search_next) ){
        count ++;
    }
    erg = (char **)GB_calloc(sizeof(char *),(size_t)count+1);
    count = 0;
    for (	gb_tree = GB_find(gb_treedata,0,0,down_level);
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
    /* the same as GB_search(species, 'using/key', GB_CREATE) */
    GBDATA *gb_gb;
    GBDATA *gb_data;
    if (GB_check_key(ali_name)) {
        return 0;
    }
    if (GB_check_hkey(key)) {
        return 0;
    }
    gb_gb = GB_find(species,ali_name,0,down_level);
    if (!gb_gb) {
        gb_gb = GB_create_container(species,ali_name);
    };
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
            if (!strchr("-.nN",sequence[i])) break;		/* real base after end of alignment */
        }
        i++;							/* points to first 0 after alignment */
        if (i > ali_len){
            GBDATA *gb_main = GB_get_root(gb_data);
            ali_len = GBT_get_alignment_len(gb_main,ali_name);
            if (slen > ali_len){				/* check for modified alignment len */
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
/********************************************************************************************
					some simple find procedures
********************************************************************************************/
GBDATA *GBT_first_marked_species_rel_species_data(GBDATA *gb_species_data)
{
    return GB_first_marked(gb_species_data,"species");
}

GBDATA *GBT_first_marked_species(GBDATA *gb_main)
{
    GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    return GB_first_marked(gb_species_data,"species");
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
    GBDATA *gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    GBDATA *gb_species;
    gb_species = GB_find(gb_species_data,"species",0,down_level);
    return gb_species;
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
    for (	gb_extended = GB_find(gb_extended_data,"extended",0,down_level);
            gb_extended;
            gb_extended = GB_find(gb_extended,"extended",0,this_level|search_next)){
        if (GB_read_flag(gb_extended)) return gb_extended;
    }
    return 0;
}

GBDATA *GBT_next_marked_extended(GBDATA *gb_extended)
{
    while (	(gb_extended = GB_find(gb_extended,"extended",0,this_level|search_next))  ){
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
void
GBT_mark_all(GBDATA *gb_main, int flag)
{
    GBDATA *gb_species;
    GB_push_transaction(gb_main);

    for (	gb_species = GBT_first_species(gb_main);
            gb_species;
            gb_species = GBT_next_species(gb_species) ){
        GB_write_flag(gb_species,flag);
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
    long cnt = 0;
    GBDATA *gb_species_data;
    GB_push_transaction(gb_main);
    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);
    cnt = GB_rescan_number_of_subentries(gb_species_data);
    GB_pop_transaction(gb_main);
    return cnt;
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
    error = GB_write_int(gb_alignment_len,new_len);				/* write new len */
    if (!error)	error = GB_write_int(gb_alignment_aligned,0);		/* sequences will be unaligned */
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
            case 'a': if (strcmp(ali_type, "ami")==0) at = GB_AT_AMI; break;
            case 'p': if (strcmp(ali_type, "pro")==0) at = GB_AT_PRO; break;
            default: ad_assert(0); break;
        }
        free(ali_type);
    }
    return at;
}

GB_BOOL GBT_is_alignment_protein(GBDATA *gb_main,const char *alignment_name)
{
    /*     char *type = GBT_get_alignment_type(gb_main,use); */
    /*     GB_BOOL is_aa = GB_FALSE; */
    /*     if (!type) return GB_FALSE; */
    /*     if (!strcmp(type,"ami")) is_aa = GB_TRUE; */
    /*     if (!strcmp(type,"pro")) is_aa = GB_TRUE; */
    /*     free(type); */
    /*     return is_aa; */

    GB_alignment_type at = GBT_get_alignment_type(gb_main,alignment_name);
    return at==GB_AT_PRO || at==GB_AT_AMI;
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
        for (	gb2 = GB_find(gbd,0,0,down_level);	/* find everything */
                gb2;
                gb2 = GB_find(gb2,0,0,this_level|search_next)){
            if (deep){
                key = GB_read_key_pntr(gb2);
                sprintf(&prefix[len_of_prefix],"/%s",key);
                gbt_scan_db_rek(gb2,prefix,1);
            }else{
                prefix[len_of_prefix] = 0;
                gbt_scan_db_rek(gb2,prefix,1);
            }
        }
        prefix[len_of_prefix] = 0;
    }else{
        if (GB_check_hkey(prefix+1)) {
            prefix = prefix;		/* for debugging purpose */
        }else{
            prefix[0] = (char)type;
            GBS_incr_hash(	gbs_scan_db_data.hash_table,
                            prefix);
        }
    }
}

long gbs_scan_db_count(const char *key,long val)
{
    gbs_scan_db_data.count++;
    key = key;
    return val;
}

long gbs_scan_db_insert(const char *key,long val)
{
    gbs_scan_db_data.result[gbs_scan_db_data.count++]= GB_STRDUP(key);
    return val;
}

long gbs_scan_db_compare(const char *left,const char *right){
    return strcmp(&left[1],&right[1]);
}


char	**GBT_scan_db(GBDATA *gbd){
    /* returns a NULL terminated array of 'strings'
       each string is the path to a node beyond gbd;
       every string exists only once
       the first character of a string is the type of the entry
       the strings are sorted alphabetically !!!
	*/
    gbs_scan_db_data.hash_table = GBS_create_hash(1024,0);
    gbs_scan_db_data.buffer = (char *)malloc(GBT_SUM_LEN);
    strcpy(gbs_scan_db_data.buffer,"");
    gbt_scan_db_rek(gbd,gbs_scan_db_data.buffer,0);

    gbs_scan_db_data.count = 0;
    GBS_hash_do_loop(gbs_scan_db_data.hash_table,gbs_scan_db_count);

    gbs_scan_db_data.result = (char **)GB_calloc(sizeof(char *),gbs_scan_db_data.count+1);
    /* null terminated result */

    gbs_scan_db_data.count = 0;
    GBS_hash_do_loop(gbs_scan_db_data.hash_table,gbs_scan_db_insert);

    GBS_free_hash(gbs_scan_db_data.hash_table);

    GB_mergesort((void **)gbs_scan_db_data.result,0,gbs_scan_db_data.count, (gb_compare_two_items_type)gbs_scan_db_compare,0);

    free(gbs_scan_db_data.buffer);
    return gbs_scan_db_data.result;
}


/********************************************************************************************
				send a message to the db server to tmp/message
********************************************************************************************/

GB_ERROR GBT_message(GBDATA *gb_main, const char *msg)
{
    GBDATA *gbmesg;
    GB_push_transaction(gb_main);
    gb_main = GB_get_root(gb_main);
    gbmesg = GB_search(gb_main,"tmp/message",GB_STRING);
    GB_write_string(gbmesg,msg);
    GB_pop_transaction(gb_main);
    return 0;
}



GB_HASH *GBT_generate_species_hash(GBDATA *gb_main,int ncase)
{
    GB_HASH *hash = GBS_create_hash(10000,ncase);
    GBDATA *gb_species;
    GBDATA *gb_name;
    for (	gb_species = GBT_first_species(gb_main);
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
    GB_HASH *hash = GBS_create_hash(10000,0);
    GBDATA *gb_species;
    GBDATA *gb_name;
    for (	gb_species = GBT_first_marked_species(gb_main);
            gb_species;
            gb_species = GBT_next_marked_species(gb_species)){
        gb_name = GB_find(gb_species,"name",0,down_level);
        if (!gb_name) continue;
        GBS_write_hash(hash,GB_read_char_pntr(gb_name),(long)gb_species);
    }
    return hash;
}

/********************************************************************************************
				Rename one or many species (only one session at a time/ uses
				commit abort transaction)
********************************************************************************************/
struct gbt_renamed_struct {
    int		used_by;
    char	data[1];

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
    GBDATA *gb_species_data	= GBT_find_or_create(gb_main,"species_data",7);
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
    long	hash_size;

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
    {				/* install link followers */
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

GB_ERROR GBT_remote_action(GBDATA *gb_main, const char *application, const char *action_name){
    char awar_action[1024];
    char awar_result[1024];
    GBDATA *gb_action;
    GBDATA *gb_result;
    const char *result = 0;
    sprintf(awar_action,"tmp/remote/%s/action",application);
    sprintf(awar_result,"tmp/remote/%s/result",application);
    /* search action var */
    while (1){
        GB_begin_transaction(gb_main);
        gb_action = GB_search(gb_main,awar_action,GB_FIND);
        GB_commit_transaction(gb_main);
        if (gb_action) break;
        GB_usleep(20000);
    }
    /* write command */
    {
        GB_begin_transaction(gb_main);
        GB_write_string(gb_action,action_name);
        GB_commit_transaction(gb_main);
    }
    /* wait to end of action */
    while(1){
        char *ac;
        GB_begin_transaction(gb_main);
        ac = GB_read_string(gb_action);
        if (!strlen(ac)){
            gb_result = GB_search(gb_main,awar_result,GB_STRING);
            result = GB_read_char_pntr(gb_result);
            free(ac);
            GB_commit_transaction(gb_main);
            break;
        }
        free(ac);
        GB_commit_transaction(gb_main);
        GB_usleep(20000);
    }
    return result;
}

GB_ERROR GBT_remote_awar(GBDATA *gb_main, const char *application, const char *awar_name, const char *value){
    char awar_awar[1024];
    char awar_value[1024];
    char awar_result[1024];
    GBDATA *gb_awar;
    GBDATA *gb_value;
    GBDATA *gb_result;
    const char *result = 0;
    sprintf(awar_awar,"tmp/remote/%s/awar",application);
    sprintf(awar_value,"tmp/remote/%s/value",application);
    sprintf(awar_result,"tmp/remote/%s/result",application);
    /* search awar var */
    while (1){
        GB_begin_transaction(gb_main);
        gb_awar = GB_search(gb_main,awar_awar,GB_FIND);
        GB_commit_transaction(gb_main);
        if (gb_awar) break;
        GB_usleep(20000);
    }
    /* write command */
    {
        GB_begin_transaction(gb_main);
        gb_value = GB_search(gb_main,awar_value,GB_STRING);
        GB_write_string(gb_awar,awar_name);
        GB_write_string(gb_value,value);
        GB_commit_transaction(gb_main);
    }
    /* wait to end of action */
    while(1){
        char *ac;
        GB_begin_transaction(gb_main);
        ac = GB_read_string(gb_awar);
        if (!strlen(ac)){
            gb_result = GB_search(gb_main,awar_result,GB_STRING);
            result = GB_read_char_pntr(gb_result);
            GB_commit_transaction(gb_main);
            free(ac);
            break;
        }
        GB_commit_transaction(gb_main);
        free(ac);
        GB_usleep(20000);
    }
    return result;

}

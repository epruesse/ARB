#ifdef __cplusplus
extern "C" {
#endif
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"
#include "arbdb.h"
#include "arbdbt.h"
#include "ARB_ext.c"
#ifdef __cplusplus
}
#endif


MODULE = ARB		PACKAGE = ARB	 PREFIX = P2A_

PROTOTYPES: ENABLE

GBDATA *
P2A_find(gbd,key,str,search_mode)
	GBDATA *gbd
	char *key
	char *str
	char *search_mode

	CODE:
	if (str[0] == 0) str = 0;
	if (key[0] == 0) key = 0;
	RETVAL = GB_find(gbd,key,str,GBP_search_mode(search_mode));
	OUTPUT:
	RETVAL


GBDATA *
P2A_search(gbd,str,type_of_entry)
	GBDATA *gbd
	char *str
	char *type_of_entry

	CODE:
	if (str[0] == 0) str = 0;
	RETVAL = GB_search(gbd,str,GBP_gb_types(type_of_entry));
	OUTPUT:
	RETVAL
	
GBDATA *
P2A_create(father,key,type)
	GBDATA *father
	char *key
	char *type

	CODE:
	RETVAL = GB_create(father,key,GBP_gb_types(type));
	OUTPUT:
	RETVAL

GBDATA *
P2A_first_marked(gbd,keystring)
	GBDATA *gbd
	char *keystring

	CODE:
	if(keystring[0] == 0) keystring=0;
	RETVAL = GB_first_marked(gbd,keystring);
	OUTPUT:
	RETVAL

GBDATA *
P2A_next_marked(gbd,keystring)
	GBDATA *gbd
	char *keystring

	CODE:
	if(keystring[0] == 0) keystring=0;
	RETVAL = GB_next_marked(gbd,keystring);
	OUTPUT:
	RETVAL

char *
P2A_read_string(gbd)
	GBDATA *gbd

	CODE:
	RETVAL = GB_read_char_pntr(gbd);
	OUTPUT:
	RETVAL

char *
P2A_read_bits(gbd,c_0,c_1)
	GBDATA *gbd
	char c_0
	char c_1

	CODE:
	RETVAL = GB_read_bits_pntr(gbd,c_0,c_1);
	OUTPUT:
	RETVAL



char *
P2A_read_bytes(gbd)
	GBDATA *gbd

	CODE:
	RETVAL = GB_read_bytes_pntr(gbd);
	OUTPUT:
	RETVAL


long
P2A_read_ints_using_index(gbd,index)
	GBDATA *gbd
	int	index

	CODE:
	if( index >= GB_read_count(gbd)){
		croak("Error: ARB::read_ints_using_index index '%i' out of [0,%i[", index, GB_read_count(gbd));
	};
	RETVAL = GB_read_ints_pntr(gbd)[index];
	OUTPUT:
	RETVAL

double
P2A_read_floats_using_index(gbd,index)
	GBDATA *gbd
	int	index

	CODE:
	if( index >= GB_read_count(gbd)){
		croak("Error: ARB::read_ints_using_index index '%i' out of [0,%i[", index, GB_read_count(gbd));
	};
	RETVAL = GB_read_floats_pntr(gbd)[index];
	OUTPUT:
	RETVAL

char *
P2A_write_ints_using_index(gbd,val,index)
	GBDATA *gbd
	long val
	long index

	CODE:
	RETVAL = "Not Implemented"; /* GB_write_ints(gbd,i,size);*/
	OUTPUT:
	RETVAL

char *
P2A_write_floats_using_index(gbd,f,index)
	GBDATA *gbd
	double f
	long index

	CODE:
	RETVAL = "Not Implemented"; /*GB_write_floats(gbd,f,size);*/
	OUTPUT:
	RETVAL


char *
P2A_read_type(gbd)
	GBDATA *gbd

	CODE:
	RETVAL = GBP_type_to_string(GB_read_type(gbd));
	OUTPUT:
	RETVAL

char *
P2A_add_callback(gbd,func_name,clientdata)
	GBDATA *gbd
	char *func_name
	char *clientdata

	CODE:
	RETVAL = (char *)GBP_add_callback(gbd,func_name,clientdata);
	OUTPUT:
	RETVAL
	
char *
P2A_ensure_callback(gbd,func_name,clientdata)
	GBDATA *gbd
	char *func_name
	char *clientdata

	CODE:
	RETVAL = (char *)GBP_ensure_callback(gbd,func_name,clientdata);
	OUTPUT:
	RETVAL
	
void 
P2A_usleep(usec)
	long usec

	PPCODE:
	GB_usleep(usec);

	
char *
P2A_remove_callback(gbd,func_name,clientdata)
	GBDATA *gbd
	char *func_name
	char *clientdata

	CODE:
	RETVAL = (char *)GBP_remove_callback(gbd,func_name,clientdata);
	OUTPUT:
	RETVAL

char *
P2A_request_undo_type(gb_main,type)
	GBDATA *gb_main
	char *type

	CODE:
	RETVAL = (char *)GB_request_undo_type(gb_main,GBP_undo_types(type));
	OUTPUT:
	RETVAL

char *
P2A_undo(gb_main,type)
	GBDATA *gb_main
	char *type

	CODE:
	RETVAL = (char *)GB_undo(gb_main,GBP_undo_types(type));
	OUTPUT:
	RETVAL

char *
P2A_undo_info(gb_main,type)
	GBDATA *gb_main
	char *type

	CODE:
	if (static_pntr) free(static_pntr);
	static_pntr = GB_undo_info(gb_main,GBP_undo_types(type));
	RETVAL = static_pntr;
	OUTPUT:
	RETVAL

char *
P2A_get_requested_undo_type(gb_main)
	GBDATA *gb_main

	CODE:
	RETVAL = (char *)GBP_undo_type_2_string(GB_get_requested_undo_type(gb_main));
	OUTPUT:
	RETVAL
	
GBDATA *
P2AT_open_table_field(gb_table,key,type)
	GBDATA *gb_table
	char *key
	char *type

	CODE:
	RETVAL = GBT_open_table_field(gb_table,key,GBP_gb_types(type));
	OUTPUT:
	RETVAL

	

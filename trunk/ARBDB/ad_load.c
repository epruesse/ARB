#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>
#include <netinet/in.h>

/*#include "arbdb.h"*/
#include "adlocal.h"
#include "admap.h"


/********************************************************************************************
			Versions:
		ASCII

		V0	- 20.6.95
		V1	Full save
		V2	Differential save
********************************************************************************************/

long 
gb_read_ascii(const char *path, GBCONTAINER * gbd)
{
    char          **pdat, **p2;
    if (gbd == NULL)
		return -1;
    if (path == NULL)
		return -1;
    pdat = gb_read_file(path);
    if (pdat) {
		GB_search((GBDATA *)gbd,GB_SYSTEM_FOLDER,GB_CREATE_CONTAINER);		/* Switch to Version 3 */
		p2 = gb_read_rek(pdat + 1, gbd);
		if (p2)
			if (*p2) {
				fprintf(stderr, "error in file: %s\n", path);
				gb_file_loc_error(p2, "syntax error before in fi");
			};
		free(pdat[0]);
		free((char *)pdat);
    };
    return 0;
}

char **gb_read_file(const char *path)
{	/*	' ' '\t' '\n' werden entfernt und durch '\0' ersetzt
		"string" wird zu string\0
		\0 folgt short  bis zur naechsten '\0'
		Kommentare #  und  comment werden entfernt
	*/

    register char *t,*t2,*pb;
    char tab[256],tab2[256],x;
    register long i;
    char **pdat;
    FILE *input;
    char *buffer;
    long data_size,pdat_cnt = 0,pdat_size;

    for (i=0;i<256;i++)  { tab[i] = 1;tab2[i]=0;}
    tab[' '] = 0;
    tab['\t'] = 0;
    tab['\n'] = 0;
    tab['\0'] = 0;

    tab2[' '] = 1;
    tab2['\t'] = 1;
    tab2['\n'] = 1;
    t = tab;t2=tab2;
    pdat = NULL;data_size = 0;
    if ((input = fopen(path, "r")) == NULL) {
		printf(" file %s not found\n", path);
    }else{
		if (fseek(input,0,2)==-1){
			printf("file %s not seekable\n",path);
		}else{
			data_size = (long)ftell(input) + 1;
			rewind(input);
			buffer =  (char *)malloc((size_t)data_size+1);
			data_size = fread(buffer,1,(int)data_size,input);
			fclose(input);
			pdat_size = data_size/CROSS_BUFFER_DIFF+2;
			buffer[data_size] = 0;
			pdat_cnt = 0;
			pdat = (char **)malloc((size_t)(sizeof(char *) * pdat_size));
			pdat[pdat_cnt++] = buffer;
			pb = buffer;
			while ( 1 ) {
				if (pdat_cnt == pdat_size){
					pdat_size += data_size/CROSS_BUFFER_DIFF+2;
					pdat = (char **)realloc((char *)pdat,
											(size_t)(sizeof(char *) * pdat_size));
				}
				while ( t2[(int)(x = *pb)] ) pb++;
				if (! x) break;
				if (x == '"'){	/* string anfang */
					char *dest;
					if (pb[1] == 1) {	/* old string mode */
						pb += 2;
						pdat[pdat_cnt++] = pb;
						while ( (x = *pb++) ) {
							if( (x==(char)1) &&
								(pb[0]==(char)34) )break;
						}
						if (!x) break;
						pb[-1] = 0;
						pb++;
						continue;
					}
					pdat[pdat_cnt++] = ++pb;
					dest = pb;
					while ((x = *pb++) != '"' ){
						if (x == '\\'){
							x = *pb++;
							if (!x) break;
							if (x>='@' && x <='@'+ 25) {
								*dest++ = x-'@';continue;
							}
							if (x>='0' && x <='9') {
								*dest++ = x-('0'-25);continue;
							}
						}
						*dest++ = x;
					}
					dest[0] = 0;
					continue;
				}
				if ( (x == '/') && (pb[1] == '*') ) {	
					while ( (x = *pb++) && ((x != '*') || (*pb !='/')));
					if (!x) break;
					*pb++ = 0;
					continue;
				}
				if (x == '#') {
					while ( (x= *pb++) && (x != '\n') );	/*ende*/
					if (!x) break;
					continue;
				}	
				pdat[pdat_cnt++] = pb;
				while ( t[(int)(*pb++)] );
				pb[-1] = 0;
			}
		}
		pdat[pdat_cnt] = NULL;


		/* all data read */
		
    }
    return pdat;	
}

/********************************************************************************************
			print part of ascii file on error
********************************************************************************************/

void gb_file_loc_error(char **pdat,const char *s) 
{
    long i;
    printf("error in data_base: %s\n",s);
    printf("\t the error is followed by:\n");
    for(i=0; (i<10) && (pdat[i]);i++ ) {
		printf ("\t\t'%s'\n",pdat[i]);
    }
}

/********************************************************************************************
					Read Ascii File
********************************************************************************************/

char **gb_read_rek(char **pdat,GBCONTAINER *gbd)
{
    char *p;
    char *key;
    int	secr, secw, secd, lu;
    long	i;
    GBDATA *previous;
    GBCONTAINER *gbc;
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gbd);

    previous = NULL;
    while ( (p = *pdat++) && (*p != '%') ) {
		key = p;
		if ( !(p = *pdat++)) break;	/* eof */
		if (*p==':' && p[-1]!='"' ) {
			secd = p[1];A_TO_I(secd);
			secw = p[2];A_TO_I(secw);
			secr = p[3];A_TO_I(secr);
			lu = atoi(p+4);
			for (i=Main->last_updated;i<=lu;i++) 
			{
				Main->dates[i] = GB_STRDUP("unknown date");
				Main->last_updated = lu+1;
			};
			if ( !(p = *pdat++)) break;	/* eof */
		}else{
			secr = secw = secd = 0;
			lu = 0;
		}

		if (*p != '%' || p[-1]=='"' ) 
		{
			previous = gb_make_entry(gbd, key,-1,0,GB_STRING);
			GB_write_string(previous,p);
			previous->flags.security_delete = secd;
			previous->flags.security_write = secw;
			previous->flags.security_read = secr;
			previous->flags2.last_updated = lu;
		}else{
			if (p[2]) {
				gb_file_loc_error(pdat-2,
								  "illegal option");
				return NULL;
			}
			if (! *pdat) {	gb_file_loc_error(pdat-2,
											  "unexpected end of file, long expected");
			return NULL;
			};/* eof */


			switch (p[1]) {
				case 's':
					previous = gb_make_entry(gbd, key, -1,0,GB_STRING);
					GB_write_string(previous,*pdat++);
					break;
				case 'l':
					previous = gb_make_entry(gbd, key, -1,0,GB_LINK);
					GB_write_link(previous,*pdat++);
					break;
				case 'i':
					previous = gb_make_entry(gbd, key, -1,0,GB_INT);
					p = *pdat++;
					GB_write_int(previous,atoi(p));
					break;
				case 'y':
					previous = gb_make_entry(gbd, key, -1,0,GB_BYTE);
					p = *pdat++;
					GB_write_byte(previous,atoi(p));
					break;
				case 'f':
					previous = gb_make_entry(gbd, key, -1,0,GB_FLOAT);
					p = *pdat++;
					GB_write_float(previous,GB_atof(p));
					break;
				case 'I':
					previous = gb_make_entry(gbd, key, -1,0,GB_BITS);
					p = *(pdat++);
					GB_write_bits(previous,p,strlen(p),'-');
					break;
				case 'Y':
					previous = gb_make_entry(gbd, key, -1,0,GB_BYTES);
					if ( gb_ascii_2_bin(*(pdat++),previous)){
						gb_file_loc_error(pdat-2,"error in bytes");
						return 0;
					}
					break;
				case 'N':
					previous = gb_make_entry(gbd, key, -1,0,GB_INTS);
					if ( gb_ascii_2_bin(*(pdat++),previous)){
						gb_file_loc_error(pdat-2,"error in ints");
						return 0;
					}
					break;
				case 'F':
					previous = gb_make_entry(gbd, key, -1,0,GB_FLOATS);
					if ( gb_ascii_2_bin(*(pdat++),previous)){
						gb_file_loc_error(pdat-2,"error in floats");
						return 0;
					}
					break;
				case '%':
					p = *pdat;
					gbc = gb_make_container(gbd, key, -1,0);
					previous = (GBDATA *)gbc;
					if (*p != '(') {
						pdat ++;
						p = *pdat;
					}
					if ( (*p =='(')&&(p[1]=='%')){
						if (!(pdat = gb_read_rek(pdat+1,gbc))){
							return NULL;
						}
						if ( (!(p=*pdat)) || (*p !='%')||(p[1]!=')')){
							gb_file_loc_error(pdat-2,
											  "unexpected end of file, %%) expected");
							return NULL;
						}
						pdat++;
					}else{
						gb_file_loc_error(pdat-2,
										  "no '(' found, %");
						return NULL;
					}
					break;
				default: 	gb_file_loc_error(pdat-2,
											  "unknown option");
					return NULL;
			} /*switch */
			previous->flags.security_delete = secd;
			previous->flags.security_write = secw;
			previous->flags.security_read = secr;
			previous->flags2.last_updated = lu;
		} /* if */
    } /* while */
    return pdat-1;
}

long gb_read_bin_rek(FILE *in,GBCONTAINER *gbd,long nitems,long version,long reversed)
{
    long item;
    long type,type2;
    GBQUARK key;
    register char *p;
    register long i;
    register int c;
    long size;
    long memsize;
    GBDATA *gb2;
    GBCONTAINER *gbc =0 ;
    long security;
    char *buff;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);

    gb_create_header_array(gbd,(int)nitems);

    for (item = 0;item<nitems;item++) {
		type = getc(in);
		security = getc(in);
		type2 = (type>>4)&0xf;
		key = getc(in);
		if (!key){
			p = gb_local->buffer;
			for (i=0;i<256;i++) {
				c = getc(in);
				*(p++) = c;
				if (!c) break;
				if (c==EOF) {
					gb_read_bin_error(in,(GBDATA *)gbd,"Unexpected EOF found");
					return -1;
				}
			}
			if (i>GB_KEY_LEN_MAX*2) {
				gb_read_bin_error(in,(GBDATA *)gbd,"Key to long");
				return -1;
			}
			if (type2 == (long)GB_DB){
				gbc = gb_make_container(gbd,gb_local->buffer,-1,0);
				gb2 = (GBDATA *)gbc;
			}else{
				gb2 = gb_make_entry(gbd,gb_local->buffer,-1,0,(GB_TYPES)type2);
			}
		}else{
			if (type2 == (long)GB_DB){
				gbc = gb_make_container(gbd,NULL,-1,key);
				gb2 = (GBDATA *)gbc;
			}else{
				gb2 = gb_make_entry(gbd,NULL,-1,(GBQUARK)key,(GB_TYPES)type2);
			}
			if (!Main->keys[key].key) {
				GB_internal_error("Some database fields have no field indentifier -> setting to 'main'");
				gb_write_index_key(GB_FATHER(gbd),gbd->index,0);
			}
		}
		gb2->flags.security_delete	= type >> 1;
		gb2->flags.security_write	= ((type&1) << 2 ) + (security >>6);
		gb2->flags.security_read 	= security >> 3;
		gb2->flags.compressed_data	= security >> 2;
		GB_ARRAY_FLAGS(((GBCONTAINER*)gb2)).flags = (int)((security >> 1) & 1);
		gb2->flags.unused		= security >> 0;
		gb2->flags2.last_updated = getc(in);

		switch (type2) {
			case GB_INT:
				{
					GB_UINT4 buffer;
					if (!fread((char*)&buffer,sizeof(GB_UINT4),1,in) ) {
						GB_export_error("File too short, seems truncated");
						return -1;
					}
					gb2->info.i = ntohl(buffer);
					break;
				}
			case GB_FLOAT:
				gb2->info.i = 0;
				if (!fread((char*)&gb2->info.i,sizeof(float),1,in) ) {
					return -1;
				}
				break;
			case GB_STRING_SHRT:
				p = buff = GB_give_buffer(GBTUM_SHORT_STRING_SIZE+2);
				for (size=0;size<=GBTUM_SHORT_STRING_SIZE;size++) {
					if (!(*(p++) = getc(in) )) break;
				}
				*p=0;
				GB_SETSMDMALLOC(gb2,size,size+1,buff);
				break;
			case GB_STRING:
			case GB_LINK:
			case GB_BITS:
			case GB_BYTES:
			case GB_INTS:
			case GB_FLOATS:
				size = gb_read_in_long(in, reversed);
				memsize =  gb_read_in_long(in, reversed);
				if (GB_CHECKINTERN(size,memsize) ){
					GB_SETINTERN(gb2);
					p = &(gb2->info.istr.data[0]);
				}else{
					GB_SETEXTERN(gb2);
					p = GB_give_buffer(memsize);
				}
				i = fread(p,1,(size_t)memsize,in);
				if (i!=memsize) {
					gb_read_bin_error(in,gb2,"Unexpected EOF found");
					return -1;
				}
				GB_SETSMDMALLOC(gb2,size,memsize,p);
				break;
			case GB_DB:
				size = gb_read_in_long(in, reversed);
				/* gbc->d.size  is automatically incremented */
				memsize = gb_read_in_long(in, reversed);
				if (gb_read_bin_rek(in,gbc,size,version,reversed)) return -1;
				break;
			case GB_BYTE:
				gb2->info.i = getc(in);
				break;
			default:
				gb_read_bin_error(in,gb2,"Unknown type");
				return -1;
		}
    }
    return 0;
}


long gb_recover_corrupt_file(GBCONTAINER *gbd,FILE *in){
    /* search pattern dx xx xx xx string 0 */
    static FILE *old_in = 0;
    static unsigned char *file = 0;
    static long size = 0;
    long pos = ftell(in);
    if (!GBCONTAINER_MAIN(gbd)->allow_corrupt_file_recovery) {
		GB_export_error("Your data file is corrupt.\n"
						"	This may happen if \n"
						"	- there is a hard drive crash,\n"
						"	- data is corrupted by bad internet connections,\n"
						"	- or the data is destroyed by the program\n"
						"	- it is not an arb file\n"
						" 	You may recover part of your data by running\n"
						"		arb_2_ascii old_arb_file panic.arb\n"
						"	or send it to strunk@mikro.biologie.tu-muenchen.de\n");
		return -1;
    }
    pos = ftell(in);
    if (old_in != in) {
		file = (unsigned char *)GB_map_FILE(in,0);
		old_in = in;
		size = GB_size_of_FILE(in);
    }
    for (;pos<size-10;pos ++){
		if ( ( file[pos] & 0xf0) == (GB_STRING_SHRT<<4)) {
			long s;
			int c;
			for ( s= pos +4; s<size && file[s]; s++ ){
				c = file[s];
				if (! (isalnum(c) || isspace(c) || strchr("._;:,",c) ) ) break;
			}
			if ( s< size && s > pos+11 && !file[s]) {	/* we found something */
				gb_local->search_system_folder = 1;
				return fseek(in,pos,0);
			}
		}
    }
    return -1;			/* no short string found */
}



long gb_read_bin_rek_V2(FILE *in,GBCONTAINER *gbd,long nitems,long version,long reversed,long deep)
{
    long item;
    long type,type2;
    GBQUARK key;
    register char *p;
    register long i;
    long size;
    long memsize;
    int index;
    GBDATA *gb2;
    GBCONTAINER *gbc;
    long security;
    char *buff;
    GB_MAIN_TYPE *Main = GB_MAIN(gbd);
    struct gb_header_list_struct *header;

    gb_create_header_array(gbd,(int)nitems);
    header = GB_DATA_LIST_HEADER(gbd->d);
    if (deep == 0 && GBCONTAINER_MAIN(gbd)->allow_corrupt_file_recovery) {
		GB_warning("Read to end of file in recovery mode");
		nitems = 10000000; /* read forever at highest level */
    }
	
    for (item = 0;item<nitems;item++) 
    {		
		type = getc(in);
		if (type == EOF){
			GB_export_error("Unexpected end of file seen");
			return -1;
		}
		if (!type) {
			int func;
			if (version == 1) {	/* master file */
				if (gb_recover_corrupt_file(gbd,in)) return -1;
				continue;
			}
			func = getc(in);
			switch (func) {
				case 1:		/*	delete entry	*/
					index = (int)gb_read_number(in);
					if (index >= gbd->d.nheader ){
						gb_create_header_array(gbd,index+1);
						header = GB_DATA_LIST_HEADER(gbd->d);
					}
					if ((gb2 = GB_HEADER_LIST_GBD(header[index]))!=NULL) {
						gb_delete_entry(gb2);
						gb2 = NULL;
					}else{
						header[index].flags.ever_changed = 1;
						header[index].flags.changed = gb_deleted;
					}

					break;
				default:
					if (gb_recover_corrupt_file(gbd,in)) return -1;
					continue;
			}
			continue;
		}

		security = getc(in);
		type2 = (type>>4)&0xf;
		key = (GBQUARK)gb_read_number(in);

		if (key >= Main->keycnt || !Main->keys[key].key ){
			GB_export_error("Inconsistent Database: Changing field identifier to 'main'");
			key = 0;
		}

		gb2 = NULL;
		gbc = NULL;
		if (version == 2) {
			index = (int)gb_read_number(in);
			if (index >= gbd->d.nheader ) {
				gb_create_header_array(gbd,index+1);
				header = GB_DATA_LIST_HEADER(gbd->d);
			}
		
			if (index >= 0 && (gb2 = GB_HEADER_LIST_GBD(header[index]))!=NULL) {
				if ( 	(GB_TYPE(gb2) == GB_DB ) !=
						( type2 == GB_DB) ) {
					GB_internal_error("Type changed, you may loose data");
					gb_delete_entry(gb2);
					SET_GB_HEADER_LIST_GBD(header[index],NULL);
					gb2 = 0;	/* @@@ OLI */
				}else{
					if (type2 == GB_DB){
						gbc = (GBCONTAINER *)gb2;
					}else{
						GB_FREEDATA(gb2);
					}
				}				
			}
		}else{
			index = -1;
		}


		if (!gb2) {
			if (type2 == (long)GB_DB){
				gbc = gb_make_container(gbd,NULL,index, key);
				gb2 = (GBDATA *)gbc;
			}else{
				gb2 = gb_make_entry(gbd,NULL,index, key, (GB_TYPES)type2);
				GB_INDEX_CHECK_OUT(gb2);
			}
		}

		if (version == 2) {
			GB_CREATE_EXT(gb2);
			gb2->ext->update_date = gb2->ext->creation_date = Main->clock;
			header[gb2->index].flags.ever_changed = 1;
		}else{
			Main->keys[key].nref_last_saved++;
		}

		gb2->flags.security_delete	= type >> 1;
		gb2->flags.security_write	= ((type&1) << 2 ) + (security >>6);
		gb2->flags.security_read 	= security >> 3;
		gb2->flags.compressed_data	= security >> 2;
		header[gb2->index].flags.flags	= (int)((security >> 1) & 1);
		gb2->flags.unused		= security >> 0;
		gb2->flags2.last_updated = getc(in);

		switch (type2) {
			case GB_INT:
				{
					GB_UINT4 buffer;
					if (!fread((char*)&buffer,sizeof(GB_UINT4),1,in) ) {
						GB_export_error("File too short, seems truncated");
						return -1;
					}
					gb2->info.i = ntohl(buffer);
					break;
				}

			case GB_FLOAT:
				if (!fread((char*)&gb2->info.i,sizeof(float),1,in) ) {
					GB_export_error("File too short, seems truncated");
					return -1;
				}
				break;
			case GB_STRING_SHRT:
				i = GB_give_buffer_size();
				p = buff = GB_give_buffer(GBTUM_SHORT_STRING_SIZE+2);
				size = 0;
				while(1){
					for (;size<i;size++) {
						if (!(*(p++) = getc(in) )) goto shrtstring_fully_loaded;
					}
					i = i*3/2;
					buff = gb_increase_buffer(i);
					p = buff + size;
				}
		shrtstring_fully_loaded:
				GB_SETSMDMALLOC(gb2,size,size+1,buff);
				break;
			case GB_STRING:
			case GB_LINK:
			case GB_BITS:
			case GB_BYTES:
			case GB_INTS:
			case GB_FLOATS:
				size = gb_read_number(in);
				memsize =  gb_read_number(in);
				if (GB_CHECKINTERN(size,memsize) ){
					GB_SETINTERN(gb2);
					p = &(gb2->info.istr.data[0]);
				}else{
					GB_SETEXTERN(gb2);
					p = gbm_get_mem((size_t)memsize,GB_GBM_INDEX(gb2));
				}
				i = fread(p,1,(size_t)memsize,in);
				if (i!=memsize) {
					gb_read_bin_error(in,gb2,"Unexpected EOF found");
					return -1;
				}
				GB_SETSMD(gb2,size,memsize,p);
				break;
			case GB_DB:
				size = gb_read_number(in);
				/* gbc->d.size  is automatically incremented */
				if (gb_read_bin_rek_V2(in,gbc,size,version,reversed,deep+1)){
					if (!GBCONTAINER_MAIN(gbd)->allow_corrupt_file_recovery) {
						return -1;
					}
				}
				break;
			case GB_BYTE:
				gb2->info.i = getc(in);
				break;
			default:
				gb_read_bin_error(in,gb2,"Unknown type");
				if (gb_recover_corrupt_file(gbd,in)){
					if (GBCONTAINER_MAIN(gbd)->allow_corrupt_file_recovery){
						return 0;		/* loading stopped */
					}else{
						return -1;
					}
				}

				continue;
		}
    }
    return 0;
}

GBDATA *gb_search_system_folder_rek(GBDATA *gbd){


    GBDATA *gb2;
    GBDATA *gb_result = 0;
    for (gb2 = GB_find(gbd,0,0,down_level);
		 gb2;
		 gb2 = GB_find(gb2,0,0,this_level|search_next)){
		int type = GB_read_type(gb2);
		if (type != GB_DB) continue;
		if (!strcmp(GB_SYSTEM_FOLDER, GB_read_key_pntr(gb2))){
			gb_result = gb2;
			break;
		}
    }
    return gb_result;
}


void gb_search_system_folder(GBDATA *gb_main){
	/* Search a system folder within the database tree
	 *  and copy it to main level */
    GBDATA *gb_oldsystem;
    GB_ERROR error;
    GBDATA *gb_system = GB_find(gb_main,GB_SYSTEM_FOLDER,0,down_level);
    if (gb_system) return;
    
    GB_warning("Searching system information");
    gb_oldsystem = gb_search_system_folder_rek(gb_main);
    if (!gb_oldsystem){
		GB_warning("!!!!! not found (bad)");
		return;
    }
    gb_system = GB_search(gb_main,GB_SYSTEM_FOLDER,GB_CREATE_CONTAINER);
    error = GB_copy(gb_system,gb_oldsystem);
    if (!error) error = GB_delete(gb_oldsystem);
    if (error) GB_warning(error);
    GB_warning("***** found (good)");
}

long gb_read_bin(FILE *in,GBCONTAINER *gbd, int diff_file_allowed)
{
    register int c = 1;
    long i;
    long	error;
    long	j,k;
    long	version;
    long	reversed;
    long	nodecnt;
    long	first_free_key;
    char	*buffer,*p;
    GB_MAIN_TYPE *Main = GBCONTAINER_MAIN(gbd);

    while ( c && c!= EOF ) {
		c = getc(in);
    }
    if (c==EOF){
		gb_read_bin_error(in,(GBDATA *)gbd,"First zero not found");
		return 1;
    }

    i = gb_read_in_long(in,0);
    if ( strncmp((char *)&i,"vers",4) ) {
		gb_read_bin_error(in,(GBDATA *)gbd,"keyword 'vers' not found");
		return 1;
    }
    i = gb_read_in_long(in,0);
    switch (i) {
		case 0x01020304:
			reversed = 0;
			break;
		case 0x04030201:
			reversed = 1;
			break;
		default:
			gb_read_bin_error(in,(GBDATA *)gbd,"keyword '^A^B^C^D' not found");
			return 1;
    }
    version = gb_read_in_long(in,reversed);
    if (version >2 ) {
		gb_read_bin_error(in,(GBDATA *)gbd,"ARB Database version > '2'");
		return 1;
    }

    if (version == 2 && !diff_file_allowed) {
		GB_export_error("This is not a primary arb file, please select the master"
						" file xxx.arb");
		return 1;
    }

    buffer = GB_give_buffer(256);
    i = gb_read_in_long(in,0);
    if ( strncmp((char *)&i,"keys",4) ) {
		gb_read_bin_error(in,(GBDATA *)gbd,"keyword 'keys' not found");
		return 1;
    }

    if (!Main->key_2_index_hash) Main->key_2_index_hash = GBS_create_hash(30000,0);
	
    first_free_key = 0;
    gb_free_all_keys(Main);

    while(1) {			/* read keys */
		long nrefs = 0;
		if (version) {
			nrefs = gb_read_number(in);
		}
		p = buffer;
		for (k=0;k<GB_KEY_LEN_MAX;k++) {
			c = getc(in);
			if (!c) break;
			if (c==EOF) {
				gb_read_bin_error(in,(GBDATA *)gbd,"unexpected EOF while reading keys");
				return 1;
			}
			*(p++) = c;
		}
		*p=0;
		if (p == buffer) break;
		if (*buffer == 1) { 		/* empty key */
			long index = gb_create_key(Main,0,GB_FALSE);
			Main->keys[index].key = 0;
			Main->keys[index].nref = 0;
			Main->keys[index].next_free_key = first_free_key;
			first_free_key = index;
		}else{
			long index = gb_create_key(Main,buffer,GB_FALSE);
			Main->keys[index].nref = nrefs;
		}

    }
    Main->first_free_key = first_free_key;

    i = gb_read_in_long(in,0);
    if ( strncmp((char *)&i,"time",4) ) {
		gb_read_bin_error(in,(GBDATA *)gbd,"keyword 'time' not found");
		return 1;
    }
    for (j=0;j<255;j++) {			/* read times */
		p = buffer;
		for (k=0;k<256;k++) {
			c = getc(in);
			if (!c) break;
			if (c==EOF) {
				gb_read_bin_error(in,(GBDATA *)gbd,"unexpected EOF while reading times");
				return 1;
			}
			*(p++) = c;
		}
		*p=0;
		if (p == buffer) break;
		if (Main->dates[j]) free(Main->dates[j]);
		Main->dates[j] = GB_STRDUP(buffer);
    }
    if (j>=255) {
		gb_read_bin_error(in,(GBDATA *)gbd,"more then 255 dates are not allowed");
		return 1;
    }
    Main->last_updated = (unsigned int)j;

    i = gb_read_in_long(in,0);
    if ( strncmp((char *)&i,"data",4) ) {
		gb_read_bin_error(in,(GBDATA *)gbd,"keyword 'data' not found");
		return 0;
    }
    nodecnt = gb_read_in_long(in,reversed);
    GB_give_buffer(256);

    if (version==1)			/* teste auf map file falls version == 1 */
    {
		GB_CSTR map_path;
		int	merror;
		struct gb_map_header mheader;
		long	mode;
		int ok=0;
		mode = GB_mode_of_link(Main->path);	/* old master */
		if (S_ISLNK(mode)){
			char *path2 = GB_follow_unix_link(Main->path);	    
			map_path = gb_mapfile_name(path2);
			free(path2);
		}else{
			map_path = gb_mapfile_name(Main->path);
		}
		merror = gb_is_valid_mapfile(map_path,&mheader);
		if (merror>0)
		{
			if (gb_main_array[mheader.main_idx]==NULL)
			{
				GBCONTAINER *newGbd = (GBCONTAINER*)gb_map_mapfile(map_path);

				if (newGbd)
				{
					GBCONTAINER 	*father = GB_FATHER(gbd);
					GB_MAIN_IDX 	new_idx = mheader.main_idx,
						old_idx = father->main_idx;

					GB_commit_transaction((GBDATA*)gbd);

					ad_assert(newGbd->main_idx == new_idx);

					gb_main_array[new_idx] = Main;
					Main->data = newGbd;
					father->main_idx = new_idx;

					gbd = newGbd;
					SET_GB_FATHER(gbd,father);

					gb_main_array[old_idx]    = NULL;

					GB_begin_transaction((GBDATA*)gbd);
					ok=1;	
				}
			}
			else
			{
				GB_export_error("FastLoad-File index conflict -- please save one\n"
								"of the loaded databases as\n"
								"'Bin (with FastLoad File)' again");
			}
		}else{
			if (!merror){
				GB_warning("%s",GB_get_error());
			}else{
				printf("	no FastLoad File '%s' found: loading entire database\n",map_path);
			}
		}
		if (ok) return 0;
    }

    switch(version) {
		case 0:
			error = gb_read_bin_rek(in,gbd,nodecnt,version,reversed);
			break;
		case 2:
			for (i=1; i < Main->keycnt;i++) {
				if (Main->keys[i].key) {
					Main->keys[i].nref_last_saved = Main->keys[i].nref;
				}
			}

			if (Main->clock<=0) Main->clock++;
		case 1:
			error = gb_read_bin_rek_V2(in,gbd,nodecnt,version,reversed,0);
			break;
		default:
			GB_internal_error("Sorry: This ARB Version does not support database format V%i",version);
			error = 1;
    }
	
    if (gb_local->search_system_folder){
		gb_search_system_folder((GBDATA *)gbd);
    }
	
    switch(version) {
		case 2:
		case 1:
			for (i=1; i < Main->keycnt;i++) {
				if (Main->keys[i].key) {
					Main->keys[i].nref = Main->keys[i].nref_last_saved;
				}
			}
			break;
		default:
			break;
    }

    return error;
}

/********************************************************************************************
					OPEN DATABASE
********************************************************************************************/

long	gb_next_main_idx_for_mapfile;

void GB_set_next_main_idx(long idx){
    gb_next_main_idx_for_mapfile = idx;
}

GB_MAIN_IDX gb_make_main_idx(GB_MAIN_TYPE *Main)
{
    static int initialized = 0;
    GB_MAIN_IDX idx;

    if (!initialized)
    {
		for (idx=0; idx<GB_MAIN_ARRAY_SIZE; idx++)
			gb_main_array[idx] = NULL;
		initialized = 1;
    }
    if (gb_next_main_idx_for_mapfile<=0){
		while (1)	/* search for unused array index */
		{
			idx = (short)(time(NULL) % GB_MAIN_ARRAY_SIZE);
			if (gb_main_array[idx]==NULL)
				break;
		}
    }else{
		idx = (short)gb_next_main_idx_for_mapfile;
		gb_next_main_idx_for_mapfile = 0;
    }

    gb_main_array[idx] = Main;

    return idx;	
}

GB_ERROR 	gb_login_remote(struct gb_main_type *gb_main,const char *path,const char *opent){
    GBCONTAINER *gbd = gb_main->data;
    gb_main->local_mode = GB_FALSE;
    gb_main->c_link = gbcmc_open(path);

    if (!gb_main->c_link) {
		return GB_export_error("There is no ARBDB server '%s', please start one or add a filename",path);
    }
    
    gbd->server_id = 0;
    gb_main->remote_hash = GBS_create_hashi(GB_REMOTE_HASH_SIZE);

    if (gb_init_transaction(gbd))	{	/* login in server */
		return GB_get_error();
    }
    gbd->flags2.folded_container = 1;

    if (	strchr(opent, 't')	) gb_unfold(gbd,0,-2);	/* tiny */
    else if (strchr(opent, 'm')	) gb_unfold(gbd,1,-2);	/* medium (no sequence)*/
    else if (strchr(opent, 'b')	) gb_unfold(gbd,2,-2);	/* big (no tree)*/
    else if (strchr(opent, 'h')	) gb_unfold(gbd,-1,-2);	/* huge (all)*/
    else gb_unfold(gbd,0,-2);	/* tiny */
    return 0;
}

GBDATA *GB_login(const char *path,const char *opent,const char *user)
     /* opent	char	'r'	read
					'w'	write
					'c'	create
					'd'	look for default (if create)
						in $ARBHOME/lib
							(any leading '.' is removed )
					'D'	look for default (if create)
						in $ARBHOME/lib/arb_default
							(any leading '.' is removed )
					't'	small memory usage
					'm'	medium
					'h'	huge

					'R'	allow corrupt file recovery + opening quicks with no master
		*/
{
    GBCONTAINER         *gbd;
    FILE           *input;
    long             i;
    char		*free_path = 0;
    struct gb_main_type *Main;
    enum gb_open_types opentype;
    GB_CSTR quickFile = NULL;
    int ignoreMissingMaster = 0;
    int loadedQuickIndex = -1;
    GB_ERROR	error = 0;

    
    if (!opent) opentype = gb_open_all;
    else if (strchr(opent, 'w')) opentype = gb_open_all;
    else if (strchr(opent, 's')) opentype = gb_open_read_only_all;
    else opentype = gb_open_read_only_all;

    if (strchr(path,':')){
		; /* remote access */
    }else if (GBS_string_cmp(path,"*.quick?",0)==0){
		char *ext = gb_findExtension(path);
		ad_assert(ext!=0);
		if (isdigit(ext[6]))
		{
			loadedQuickIndex = atoi(ext+6);
			strcpy(ext, ".arb");
			quickFile = gb_oldQuicksaveName(path, loadedQuickIndex);
			if (strchr(opent,'R'))		ignoreMissingMaster = 1;
		}
    }else if (GBS_string_cmp(path,"*.a??",0)==0){
	
		char *extension = gb_findExtension(path);

		if (isdigit(extension[2]) && isdigit(extension[3]))
		{
			loadedQuickIndex = atoi(extension+2);
			strcpy(extension,".arb");
			quickFile = gb_quicksaveName(path, loadedQuickIndex);
			if (strchr(opent,'R'))		ignoreMissingMaster = 1;
		}else {
			char *base = GB_STRDUP(path);
			char *ext = gb_findExtension(base);
			{
				struct gb_scandir dir;
				ext[0]=0;
				gb_scan_directory(base,&dir);
				loadedQuickIndex = dir.highest_quick_index;

				if (dir.highest_quick_index!=dir.newest_quick_index)
				{
					GB_warning("The QuickSave-File with the highest index-number\n"
							   "is not the NEWEST of your QuickSave-Files.\n"
							   "If you didn't restore old QuickSave-File from a backup\n"
							   "please inform your system-administrator - \n"
							   "this may be a serious bug and you may loose your data.");
				}

				switch(dir.type)
				{
					case GB_SCAN_NO_QUICK:
						break;
					case GB_SCAN_NEW_QUICK:
						quickFile = gb_quicksaveName(path,dir.highest_quick_index);
						break;
					case GB_SCAN_OLD_QUICK:
						quickFile = gb_oldQuicksaveName(path,dir.newest_quick_index);
		    
						break;
				}
			}

			free(base);
		}
    }
    
    if (gb_verbose_mode){
		fprintf(stdout, "	ARB:	Loading '%s' ", path);
		if (quickFile) fprintf(stdout, "+ Changes-File '%s'", quickFile);
		fprintf(stdout,"\n");
    }
    
    gbm_init_mem();
    gb_init_gb();
     
    if (GB_install_pid(1)) return 0;

    Main = gb_make_gb_main_type(path);
    Main->local_mode = GB_TRUE;

    if (strchr(opent,'R')) Main->allow_corrupt_file_recovery = 1;

    gb_create_key(Main,"main",GB_FALSE);

    Main->dummy_father = gb_make_container(NULL, 0, -1,0);	/* create "main" */
    Main->dummy_father->main_idx = gb_make_main_idx(Main);
    Main->dummy_father->server_id = GBTUM_MAGIC_NUMBER;
    gbd = gb_make_container(Main->dummy_father, 0, -1,0 );	/* create "main" */

    Main->data = gbd;
    gbcm_login(gbd,user);
    Main->opentype = opentype;
    Main->security_level = 7;


    
    if (path && (strchr(opent, 'r')) ){
		if (strchr(path, ':')){
			error = gb_login_remote(Main,path,opent);
		}else{
			GB_ULONG time_of_main_file = 0;
			GB_ULONG time_of_quick_file = 0;
			Main->local_mode = GB_TRUE;
			GB_begin_transaction((GBDATA *)gbd);
			Main->clock = 0;		/* start clock */
			input = fopen(path, "r");
			if (!input && ignoreMissingMaster){
				goto load_quick_save_file_only;
			}
	    
			if ( !input ) 
			{
				GB_disable_quicksave((GBDATA *)gbd,"Database Created");

				if (strchr(opent, 'c') )
				{
					if (strchr(opent, 'd')||strchr(opent, 'D')){
						/* use default settings */
						const char *pre;
						if (strchr(opent, 'd')) pre = "";
						else pre = "arb_default/";
						free_path = GBS_find_lib_file(path,pre);
						if (!free_path) {
							fprintf(stderr,"file %s not found\n", path);
							fprintf(stderr,"Looking for default file %s, but not found in $ARBHOME/lib/%s\n",path,pre);
							fprintf(stderr," database %s created\n", path);
							GB_commit_transaction((GBDATA *)gbd);
							return (GBDATA *)gbd;
						}
						path = free_path;
						input = fopen(path, "r");
					}else{	
						printf(" database %s created\n", path);
						GB_commit_transaction((GBDATA *)gbd);
						return (GBDATA *)gbd;
					}
				}else{
					GB_export_error("ERROR Database '%s' not found",path);
					return 0;
				}
			}
			time_of_main_file = GB_time_of_file(path);
			i = gb_read_in_long(input, 0);
			if ((i== 0x56430176) || (i == GBTUM_MAGIC_NUMBER) || (i == GBTUM_MAGIC_REVERSED))    {
				i = gb_read_bin(input, gbd,0);		/* read or map whole db */
				gbd = Main->data;
				fclose(input);
		   
				/* gb_testDB((GBDATA*)gbd); */

				if (i ){
					if (Main->allow_corrupt_file_recovery) {
						GB_print_error();
					}else{
						return 0;
					}
				}
		
				if (quickFile){
					long err;
				load_quick_save_file_only:
					err = 0;
					input = fopen(quickFile,"r");

					if (input){
						time_of_quick_file = GB_time_of_file(quickFile);
						if (time_of_main_file && time_of_quick_file < time_of_main_file){
							GB_export_error("Your main database file '%s' is newer than\n"
											"	the changes file '%s'\n"
											"	That is very strange and happens only if files where\n"
											"	moved/copied by hand\n"
											"	Your file '%s' may be an old relict,\n"
											"	if you ran into problems now,delete it",
											path,quickFile,quickFile);
							GB_print_error();
							GB_warning(GB_get_error());
						}
						i = gb_read_in_long(input, 0);
						if ((i== 0x56430176) || (i == GBTUM_MAGIC_NUMBER) || (i == GBTUM_MAGIC_REVERSED)) 
						{
							err = gb_read_bin(input, gbd, 1);
							fclose (input);
						}
					}   else err = 1;

					if (err){
						GB_export_error("I cannot load your quick file '%s';\n"
										"    you may restore an older version by running arb with:\n"
										"    arb <name of quicksave-file>",
										quickFile);
						GB_print_error();
						if (Main->allow_corrupt_file_recovery){
							return (GBDATA *)(gbd);
						}
						return 0;
					}
				}
				Main->qs.last_index = loadedQuickIndex; /* determines which # will be saved next */
			} else {
				fclose(input);
				(void) gb_read_ascii(path, gbd);
				GB_disable_quicksave((GBDATA *)gbd,"Sorry, I cannot save differences to ascii files\n"
									 "	Save whole database in binary mode first");
			}
		}
    }else{
		GB_disable_quicksave((GBDATA *)gbd,"Database not part of this process");
		Main->local_mode = GB_TRUE;
		GB_begin_transaction((GBDATA *)gbd);
    }
    if (error) return 0;
    GB_commit_transaction((GBDATA *)gbd);
    {				/* New Transaction, should be quicksaveable */
		GB_begin_transaction((GBDATA *)gbd);
		if (!strchr(opent,'N')){ 	/* new format */
			gb_convert_V2_to_V3((GBDATA *)gbd);		/* Compression conversion */
		}
		error = gb_load_key_data_and_dictionaries((GBDATA *)Main->data);
		if (!error){
			error = GB_resort_system_folder_to_top((GBDATA *)Main->data);
		}
		GB_commit_transaction((GBDATA *)gbd);
    }
    Main->security_level = 0;
    gbl_install_standard_commands((GBDATA *)gbd);
    if (gb_verbose_mode)    fprintf(stdout, "	ARB:	Loading '%s' done\n", path);
    if (free_path) free(free_path);
    return (GBDATA *)gbd;
}

GBDATA *GB_open(const char *path, const char *opent)
{
    const char *user;
    user = GB_getenvUSER();
    return GB_login(path,opent,user);
}

int gb_verbose_mode = 0;

void GB_set_verbose(){
    gb_verbose_mode = 1;
}

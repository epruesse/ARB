/*
#######################################
#                                     #
#    ORS_CLIENT:  JAVA                #
#    communication with java applet   #
#                                     #
####################################### */
const int MAXIMUM_LINE_LENGTH = 1024;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
// #include <malloc.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <arbdb.h>
char *TRS_map_file(const char *path,int writeable);
#define GB_map_file TRS_map_file
#	include <cat_tree.hxx>
#undef GB_map_file
#include "tree_lib.hxx"
#include "trs_proto.h"

FILE *t2j_out;	// output file
CAT_tree *cat_tree = NULL;	// the tree
FILE *t2j_debug;

/********************************************************************************************
					error handling
********************************************************************************************/

char *TRS_error_buffer = 0;

char *TRS_export_error(const char *templat, ...)
{
    char buffer[10000];
    char *p = buffer;
    va_list	parg;
    sprintf (buffer,"ARB ERROR: ");
    p += strlen(p);
    va_start(parg,templat);

    vsprintf(p,templat,parg);

    if (TRS_error_buffer) free(TRS_error_buffer);
    TRS_error_buffer = strdup(buffer);
    return TRS_error_buffer;
}

char *TRS_get_error()
{
    return TRS_error_buffer;
}




struct TRS_strstruct {
    char *TRS_strcat_data;
    long TRS_strcat_data_size;
    long TRS_strcat_pos;
};

void *TRS_stropen(long init_size)	{		/* opens a memory file */
    struct TRS_strstruct *strstr;
    strstr = (struct TRS_strstruct *)malloc(sizeof(struct TRS_strstruct));
    strstr->TRS_strcat_data_size = init_size;
    strstr->TRS_strcat_pos = 0;
    strstr->TRS_strcat_data = (char *)malloc((size_t)strstr->TRS_strcat_data_size);
    strstr->TRS_strcat_data[0] = 0;
    return (void *)strstr;
}

char *TRS_strclose(void *strstruct, int optimize)	/* returns the memory file */
    /* if optimize == 1 then dont waste memory
				if optimize == 0 than fast */
{
    char *str;
    struct TRS_strstruct *strstr = (struct TRS_strstruct *)strstruct;
    if (optimize) {
	str = strdup(strstr->TRS_strcat_data);
	free(strstr->TRS_strcat_data);
    }else{
	str = strstr->TRS_strcat_data;
    }
    free((char *)strstr);
    return str;
}

void TRS_strcat(void *strstruct,const char *ptr)	/* this function adds many strings.
							   first create a strstruct with TRS_open */
{
    long	len;
    struct TRS_strstruct *strstr = (struct TRS_strstruct *)strstruct;

    len = strlen(ptr);
    if (strstr->TRS_strcat_pos + len + 2 >= strstr->TRS_strcat_data_size) {
	strstr->TRS_strcat_data_size = (strstr->TRS_strcat_pos+len+2)*3/2;
	strstr->TRS_strcat_data = (char *)realloc((char *)strstr->TRS_strcat_data,(size_t)strstr->TRS_strcat_data_size);
    }
    memcpy(strstr->TRS_strcat_data+strstr->TRS_strcat_pos,ptr,(int)len);
    strstr->TRS_strcat_pos += len;
    strstr->TRS_strcat_data[strstr->TRS_strcat_pos] = 0;
}

void TRS_chrcat(void *strstruct,char ch)	/* this function adds many strings.
						   The first call should be a null pointer */
{
    struct TRS_strstruct *strstr = (struct TRS_strstruct *)strstruct;
    if (strstr->TRS_strcat_pos + 3 >= strstr->TRS_strcat_data_size) {
	strstr->TRS_strcat_data_size = (strstr->TRS_strcat_pos+3)*3/2;
	strstr->TRS_strcat_data = (char *)realloc((char *)strstr->TRS_strcat_data,
						  (size_t)strstr->TRS_strcat_data_size);
    }
    strstr->TRS_strcat_data[strstr->TRS_strcat_pos++] = ch;
    strstr->TRS_strcat_data[strstr->TRS_strcat_pos] = 0;
}


char *TRS_mergesort(void **array,long start, long end, long (*compare)(void *,void *,char *cd), char *client_data)
{
    long size;
    long mid;
    long i, j;
    long	dest;
    void **buffer;
    void *ibuf[256];
    char *error;

    size = end - start;
    if (size <= 1) {
	return 0;
    }
    mid = (start+end) / 2;
    error = TRS_mergesort(array,start,mid,compare,client_data);
    error = TRS_mergesort(array,mid,end,compare,client_data);
    if (size <256) {
	buffer = ibuf;
    }else{
	buffer = (void **)malloc((size_t)(size * sizeof(void *)));
    }

    for ( dest = 0, i = start, j = mid ;
	  i< mid && j < end;){
	if (compare(array[i],array[j],client_data) < 0) {
	    buffer[dest++] = array[i++];
	}else{
	    buffer[dest++] = array[j++];
	}
    }
    while(i<mid)	buffer[dest++] = array[i++];
    while(j<end)	buffer[dest++] = array[j++];
    memcpy( (char *)(array+start),buffer,(int)size * sizeof(void *));
    if (size>=256) free((char *)buffer);
    return error;
}
/********************************************************************************************
					read a file to memory
********************************************************************************************/
char *TRS_read_file(const char *path)
{
    FILE *input;
    long data_size;
    char *buffer = 0;

    if (!strcmp(path,"-")) {
	/* stdin; */
	int c;
	void *str = TRS_stropen(1000);
	c = getc(stdin);
	while (c!= EOF) {
	    TRS_chrcat(str,c);
	    c = getc(stdin);
	}
	return TRS_strclose(str,0);
    }

    if ((input = fopen(path, "r")) == NULL) {
	TRS_export_error("File %s not found",path);
	return NULL;
    }else{
	data_size = TRS_size_of_file(path);
	buffer =  (char *)malloc((size_t)(data_size+1));
	data_size = fread(buffer,1,(size_t)data_size,input);
	buffer[data_size] = 0;
	fclose(input);
    }
    return buffer;
}

char *TRS_map_FILE(FILE *in,int writeable){
    int fi = fileno(in);
    long size = TRS_size_of_FILE(in);
    char *buffer;
    if (size<=0) {
	TRS_export_error("TRS_map_file: sorry file not found");
	return 0;
    }
    if (writeable){
	buffer = (char *)mmap(0, (int)size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fi, 0);/*@@@*/
    }else{
	buffer = (char *)mmap(0, (int)size, PROT_READ, MAP_SHARED, fi, 0);/* @@@ */
    }
    if (!buffer){
	TRS_export_error("TRS_map_file: Error Out of Memory: mmap failes ");
	return 0;
    }
    return buffer;
}

char *TRS_map_file(const char *path,int writeable){
    FILE *in;
    char *buffer;
    in = fopen(path,"r");
    if (!in) {
	TRS_export_error("TRS_map_file: sorry file '%s' not readable",path);
	return 0;
    }
    buffer = TRS_map_FILE(in,writeable);
    fclose(in);
    return buffer;
}
long TRS_size_of_file(const char *path)
{
    struct stat gb_global_stt;
    if (stat(path, &gb_global_stt)) return -1;
    return gb_global_stt.st_size;
}

long TRS_size_of_FILE(FILE *in){
    int fi = fileno(in);
    struct stat st;
    if (fstat(fi, &st)) {
	TRS_export_error("TRS_size_of_FILE: sorry file '%s' not readable");
	return -1;
    }
    return st.st_size;
}


T2J_transfer_struct::T2J_transfer_struct(long size, enum CAT_FIELDS key_indexi) {
    memset((char *)this,0,sizeof(T2J_transfer_struct));
    nitems = size;
    items = (T2J_item *)calloc(sizeof(T2J_item),(int)size);
    this->key_index = key_indexi;
}

/***********************************************************************
		To output stream: nibbles bytes and integers
***********************************************************************/
int t2j_last_nibble  = -1;

GB_INLINE void t2j_write_nibble(int value){
    if (t2j_last_nibble<0) {
	t2j_last_nibble = value;
    }else{
	int o = t2j_last_nibble<<4 | value;
	fputc(o,t2j_out);
	t2j_last_nibble = -1;
    }
}

GB_INLINE void t2j_flush_nibble(void){
    if (t2j_last_nibble>=0) {
	int o = t2j_last_nibble<<4 | 15;
	fputc(o,t2j_out);
	t2j_last_nibble = -1;
    }
}

int t2j_last_bit_string = 0;
int t2j_bits_already_in_c = 0;

GB_INLINE void t2j_flush_bits(void){
    fputc(t2j_last_bit_string,t2j_out);
    t2j_last_bit_string = 0;
    t2j_bits_already_in_c = 0;
}

GB_INLINE void t2j_write_bit(int bit){
    if (bit) {
	t2j_last_bit_string |=  0x80 >> t2j_bits_already_in_c;
    }
    if ((++t2j_bits_already_in_c) >=8) {
	t2j_flush_bits();
    }
}


void t2j_write_byte(int value){
    t2j_flush_nibble();
    fputc(value,t2j_out);
}

char *t2j_write_int2(unsigned int value){
    register int i;
    i = (value >>24) & 0xff; t2j_write_byte(i);
    i = (value >>16) & 0xff; t2j_write_byte(i);
    i = (value >>8) & 0xff; t2j_write_byte(i);
    i = (value >>0) & 0xff; t2j_write_byte(i);
    return 0;
}

char *t2j_write_uint(unsigned int value, int or_cmd){
    t2j_write_nibble(0);		// attention
    register int i;
    if (value >= 0x1000000) {
	t2j_write_nibble(4 | or_cmd);		// how many bytes !!!!
	i = (value >>24) & 0xff; t2j_write_byte(i);
	i = (value >>16) & 0xff; t2j_write_byte(i);
	i = (value >>8) & 0xff; t2j_write_byte(i);
	i = (value >>0) & 0xff; t2j_write_byte(i);
    }else if (value >= 0x1000) {
	t2j_write_nibble(3 | or_cmd);
	i = (value >>16) & 0xff; t2j_write_byte(i);
	i = (value >>8) & 0xff; t2j_write_byte(i);
	i = (value >>0) & 0xff; t2j_write_byte(i);
    }else if (value >= 0x100){
	t2j_write_nibble(2 | or_cmd);
	i = (value >>8) & 0xff; t2j_write_byte(i);
	i = (value >>0) & 0xff; t2j_write_byte(i);
    }else{
	t2j_write_nibble(1 | or_cmd);
	i = (value >>0) & 0xff; t2j_write_byte(i);
    }
    return 0;
}

/***********************************************************************
	Sends the data in nodes->user_data to the java programm
	Sideeffects:	writes to t2j_out
			uses cat_tree
***********************************************************************/

struct t2j_s1_item {
    CAT_node_id id;
    struct t2j_s1_item *next;
};

struct t2j_s1_item_header {
    struct t2j_s1_item *first;
    struct t2j_s1_item *last;
    char	*value;
    long	focus_members;
    long	members;
    long	estimated_transfer_size;		// very rough idea !!!
};

CAT_node_id t2j_get_focus_right_bound(CAT_node_id focus){
    while (cat_tree->nodes[focus].rightson) {
	focus = cat_tree->nodes[focus].rightson;
    }
    return focus;
}

/***********************************************************************
	Compare Function to sort the result list,
	The idea is that data within the focus is transferred first,
	followed by data with a lot of hits
	Sideeffects: sorts an void ** array
***********************************************************************/
long t2j_compare_two_hitems(struct t2j_s1_item_header *hi0, struct t2j_s1_item_header *hi1){
    double l = (hi0->focus_members + hi0->members/20) / hi0->estimated_transfer_size;
    double r = (hi1->focus_members + hi1->members/20) / hi1->estimated_transfer_size;
    if (l>r) return 1;
    return -1;
}

long t2j_compare_two_ids(long i1, long i2) {
    if (i1 < i2) return -1;
    return 1;
}

void t2j_write_range(int start_of_range, int end_of_range){
    int rsize = end_of_range - start_of_range;
    if (rsize == 1) {		// no range !!!
	t2j_write_nibble(rsize);
    }else{
	if (rsize >= 15) {
	    t2j_write_uint(rsize,8);
	}else{
	    t2j_write_nibble(15);
	    t2j_write_nibble(rsize);
	}
    }
}

void t2j_indexlist_2_OTF(long *buffer, long buf_size){
    int j;
    int start_of_range, end_of_range;
    start_of_range = end_of_range = -1;

    TRS_mergesort((void **)buffer,0,buf_size,
		  (long (* )(void *, void *, char *cd ))t2j_compare_two_ids,0);

    for (j = 0; j < buf_size; j++) {
	int id = (int) buffer[j];
	if (id == end_of_range + 1){		// resize range
	    end_of_range++;
	}else{
	    // write out range !!!
	    if (end_of_range > start_of_range){
		t2j_write_range(start_of_range,end_of_range);
	    }
	    int rsize = (int)id - end_of_range;
	    if (rsize >= 15) {
		t2j_write_uint(rsize,0);
	    }else{
		t2j_write_nibble(rsize);
	    }
	    start_of_range = end_of_range = (int)id;
	}
    }
    if (end_of_range > start_of_range){		// last range
	t2j_write_range(start_of_range,end_of_range);
    }

    t2j_write_nibble(0);
    t2j_write_nibble(0);
    t2j_flush_nibble();			// back to byte boundary !!!
}

/***********************************************************************
	Send all information stored in cat_tree->nodes[xx].user_data
***********************************************************************/
char *T2J_send_tree(CAT_node_id focus){
    struct t2j_s1_item_header **hitems =
	(struct t2j_s1_item_header **)calloc(sizeof(struct t2j_s1_item_header *),
					     (int)cat_tree->nnodes);	// nnodes is the very worst case !!!
    if (focus <0 || focus >= cat_tree->nnodes) focus = 0;
    int	header_in_use = 0;
    CAT_node_id	left_focus = focus;
    CAT_node_id	right_focus = t2j_get_focus_right_bound(focus);
    long	convert_hash = TRS_create_hash(cat_tree->nnodes);
    CAT_node_id i;

    // ********* collect the answer in a hash table
    for (i=0; i<cat_tree->nnodes; i++){
	CAT_node *node = & cat_tree->nodes[i];
	char *s = node->user_data;
	if (!s) continue;
	struct t2j_s1_item *item = (struct t2j_s1_item *)
	    calloc(sizeof(struct t2j_s1_item),1);
	item->id = i;
	struct t2j_s1_item_header *hitem =
	    (struct t2j_s1_item_header *)TRS_read_hash(convert_hash, s);
	if (!hitem) {
	    hitem = (struct t2j_s1_item_header *)
		calloc(sizeof(struct t2j_s1_item_header),1);
	    hitems[header_in_use++] = hitem;
	    hitem ->first = hitem->last = item;
	    hitem->estimated_transfer_size = strlen(s)+5;
	    hitem->value = s;
	    TRS_write_hash_no_strdup(convert_hash,s,(long)hitem);
	}else{
	    if (hitem->last->id != i-1) {
		hitem->estimated_transfer_size += 1;
	    }
	    hitem->last->next = item;
	    hitem->last = item;
	}
	hitem->members++;
	if (i >= left_focus && i <= right_focus) hitem->focus_members++;
    }
    // ***** sort it by quality
    TRS_mergesort((void **)hitems,0,header_in_use,	(long (* )(void *, void *, char *cd ))t2j_compare_two_hitems,0);

    CAT_NUMBERING numbering = CAT_NUMBERING_LEVELS;
    fputc(T2J_LEVEL_INDEXING,t2j_out);	// Write OTF tag
    t2j_write_int2(header_in_use);		// number of items


    for (i = 0; i< header_in_use; i++) {	// go through all values
	struct t2j_s1_item_header *hitem = hitems[i];
	int	redo_needed;
	struct t2j_s1_item *item;
	long *buffer = (long *)calloc(sizeof(long) , (int)hitem->members);
	char color_tab[256];				// already sent color info
	int j;
	for (j=0;j<256;j++) color_tab[j] = 0;		// nothing has been sent yet
	struct t2j_s1_item *ref_item;
	for (ref_item = hitem->first; ref_item; ref_item = ref_item->next) {
	    CAT_node *node = & cat_tree->nodes[ref_item->id];
	    int color = node->color;
	    int grouped = node->is_grouped;
	    int col_ref = grouped | (color<<4);
	    if (color_tab[col_ref]) continue;		// has already been sent
	    color_tab[col_ref] = 1;				// is to be sent
	    t2j_write_byte(col_ref);
	    redo_needed = 0;
	    fwrite(hitem->value, strlen(hitem->value)+1, 1, t2j_out);

	    for (j=0, item = ref_item; item; item = item->next) {
		CAT_node *nod = & cat_tree->nodes[item->id];
		if (nod->is_grouped != grouped || nod->color != color) {
		    redo_needed = 1;
		    continue;
		}
		buffer[j++] = cat_tree->nodes[item->id].numbering[numbering];
	    }
	    t2j_indexlist_2_OTF(buffer,j );
	    if (!redo_needed) break;
	}

	delete buffer;

    }
    return 0;
}

/***********************************************************************
	Transform any query list
	mode = 1 to OTF
	mode = 0 to SelectionString
	Sideeffects: no !!!!
***********************************************************************/
char *T2J_transform(int mode, char *path_of_tree, struct T2J_transfer_struct *data, CAT_node_id focus, FILE *out){
    if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);
    if (!cat_tree) return TRS_get_error();
    t2j_out = out;
    long index = TRS_create_hash(cat_tree->nnodes);
    int i;
    // create the index !!!!!!!!!!!!
    for (i=0;i< cat_tree->nnodes;i++) {
	if (cat_tree->nodes[i].field_offsets[data->key_index]) {
	    TRS_write_hash_no_strdup(index,cat_tree->data +	cat_tree->nodes[i].field_offsets[data->key_index],i+1);
	}else 	if (cat_tree->nodes[i].field_offsets[CAT_FIELD_GROUP_NAME]) {
	    TRS_write_hash_no_strdup(index,cat_tree->data +	cat_tree->nodes[i].field_offsets[CAT_FIELD_GROUP_NAME],i+1);
	}
    }


    for (i=0;i<data->nitems;i++) {
	long ind = TRS_read_hash(index, data->items[i].key);
	if (!ind) continue;
	ind -= 1;
	cat_tree->nodes[ind].color = data->items[i].color;
	cat_tree->nodes[ind].user_data = data->items[i].value;
    }
    if (mode == 1) {
	return T2J_send_tree(focus);
    }else{
	T2J_convert_colors_into_selection();
	return 0;
    }
}


/***********************************************************************
	Send the tree.   A '1' bit indicates a node '0' a leaf
	Sideeffects: writes to out
***********************************************************************/
char *T2J_send_bit_coded_tree(char *path_of_tree, FILE *out){
    if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);	// 3 lines that should never be deleted
    if (!cat_tree) return TRS_get_error();
    t2j_out = out;

    int i;
    t2j_write_uint(cat_tree->nnodes,0);
    for (i=0;i<cat_tree->nnodes;i++){
	CAT_node *node = & cat_tree->nodes[i];
	if (node->leftson) {
	    t2j_write_bit(1);
	}else{
	    t2j_write_bit(0);
	}
    }
    t2j_flush_bits();
    return 0;
}
/***********************************************************************
	Send the tree.   level numbering !!! branch lengths from 0-15
	Sideeffects: writes to out
***********************************************************************/
CAT_node_id *t2j_mcreate_level_indexing(void){
    static CAT_node_id *ids = NULL;
    if (ids) return ids;
    ids = (CAT_node_id *)calloc(sizeof(CAT_node_id), cat_tree->nnodes);
    int i;
    for (i=0;i< cat_tree->nnodes;i++) {
	ids[cat_tree->nodes[i].numbering[CAT_NUMBERING_LEVELS]] = i;
    }
    return ids;
}

char *T2J_send_branch_lengths(char *path_of_tree, FILE *out){
    if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);		// 3 lines that should never be deleted
    if (!cat_tree) return TRS_get_error();
    t2j_out = out;
    CAT_node_id *ids = t2j_mcreate_level_indexing();
    int i;
    for (i=0;i<cat_tree->nnodes;i++){
	CAT_node *node = & cat_tree->nodes[ids[i]];
	int bl = node->branch_length_byte;
	if (bl>=16) bl = 15;
	t2j_write_nibble(bl);
    }
    delete ids;
    t2j_flush_nibble();
    return 0;
}
/***********************************************************************
	Send tree in newick format
	File Syntax: id#value\nid#value\n....
	if node is grouped -> subtree is not exported
	if node.color == 0 -> prune node
	add addbl to branch length

***********************************************************************/
#define SEPERATOR '\''

void t2j_send_newick_rek(CAT_node *node, double addbl, FILE *out){
    const char *name = 0;

    if (!node->leftson){	// leaf
	{
	    int n_offset = node->field_offsets[CAT_FIELD_NAME];
	    if (n_offset){
		name = & cat_tree->data[n_offset];
		const char *full_name = "";
		int fn_offset = node->field_offsets[CAT_FIELD_FULL_NAME];
		if (fn_offset){
		    full_name = &cat_tree->data[fn_offset];
		}
		fprintf(out,"'%s#%s':%f",name,full_name,node->branch_length_float + addbl);
	    }
	}
	return;
    }
    {
	int n_offset = node->field_offsets[CAT_FIELD_GROUP_NAME];
	if (n_offset){
	    name = & cat_tree->data[n_offset];
	}
    }

    CAT_node *ln = &cat_tree->nodes[node->leftson];
    CAT_node *rn = &cat_tree->nodes[node->rightson];

    if ( (ln->color & rn->color ) == 0){	// prune tree, descend into only one branch
	if (ln->color != 0){
	    t2j_send_newick_rek(ln,addbl + node->branch_length_float ,out);
	}else{
	    t2j_send_newick_rek(rn,addbl + node->branch_length_float ,out);
	}
	return;
    }

    if (!node->is_grouped){
	fprintf(out,"(");
	t2j_send_newick_rek(ln,0,out);
	fprintf(out,",");
	t2j_send_newick_rek(rn,0,out);
	fprintf(out,")");
    }else{
	fprintf(out,"[GROUPED=%li]",node->nleafs);
    }
    if (name){
	fprintf(out,"'%s':%f\n",name,addbl + node->branch_length_float);
    }else{
	fprintf(out,":%f\n",addbl + node->branch_length_float);
    }
}

GB_ERROR t2j_patch_tree(CAT_node_id *levelindex,char *patch){
    int c;
    char *readp = strdup(patch);
    c = *(readp++);
    while (c != 0){
	int fac = 1;
	int s = 0;
	while ( c >='A' && c <='Z' ) {		// Read the start of the selection
	    s += fac * (c-'A');
	    fac *= 16;
	    c = *(readp++);
	}
	if (s<0 || s > cat_tree->nnodes){
	    return TRS_export_error("Node out of bound %i",s);
	}
	CAT_node *node = &cat_tree->nodes[levelindex[s]];
	if (c != SEPERATOR){
	    return TRS_export_error("Unknown character in patch '%c' '%c' expected in :'%s'",c,SEPERATOR,patch);
	}
	char *nread;
	nread = strchr(readp,SEPERATOR);
	if (!nread){
	    return TRS_export_error("Missing second '%c' int '%s'",SEPERATOR,patch);
	}

	*nread = 0;
	{			// patch node
	    char *new_name = strdup(readp);
	    if (node->leftson){	// internal group
		node->field_offsets[CAT_FIELD_GROUP_NAME] = new_name - &cat_tree->data[0];
	    }else{
		node->field_offsets[CAT_FIELD_FULL_NAME] = new_name - &cat_tree->data[0];
	    }
	}
	readp = nread+1;
	c = *(readp++);
    }
    return 0;
}

const char *t2j_group_tree(CAT_node_id *levelindex,const char *grouped){
    int c;
    const char *readp = grouped;
    c = *(readp++);
    while (c != 0){
	int fac = 1;
	int s = 0;
	while ( c >='A' && c <='Z' ) {		// Read the start of the selection
	    s += fac * (c-'A');
	    fac *= 16;
	    c = *(readp++);
	}
	if (s<0 || s >= cat_tree->nnodes){
	    return TRS_export_error("Node out of bound %i",s);
	}
	CAT_node *node = &cat_tree->nodes[levelindex[s]];
	if (c != '_'){
	    if (c){
		return TRS_export_error("Unknown character in groupstring '%c' '_' expected",c);
	    }else{
		readp--;
	    }
	}
	node->is_grouped = 1;
	c = *(readp++);
    }
    return 0;
}


char *T2J_send_newick_tree(const char *path_of_tree,
			   char *changedNodes,
			   char *selectedNodes,
			   const char *grouped_nodes,
			   FILE *out){
    if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);	// 3 lines that should never be deleted
    if (!cat_tree) return TRS_get_error();
    t2j_out = out;
    CAT_node_id *levelindex = t2j_mcreate_level_indexing();
    if (changedNodes && strlen(changedNodes)){
	const char *error = t2j_patch_tree(levelindex,changedNodes);
	if (error) {
	    fprintf(out,"%s\n",error);
	    return 0;
	}
    }

    T2J_set_color_of_selection(selectedNodes);

    {
	int i;
	for (i=0;i<cat_tree->nnodes;i++){
	    cat_tree->nodes[i].is_grouped = 0;
	}
	if (grouped_nodes && strlen(grouped_nodes)){
	    t2j_group_tree(levelindex,grouped_nodes);
	}
    }
    fprintf(out,"[%i]\n",cat_tree->nnodes);
    t2j_send_newick_rek(&cat_tree->nodes[0],0.0,out);
    fprintf(out,"\n");
    return 0;
}

/***********************************************************************
	Read a file into the transfer struct:
	File Syntax: id#value\nid#value\n....
***********************************************************************/
struct T2J_transfer_struct *T2J_read_query_result_from_data(char *data,CAT_FIELDS catfield){
    int nls = 0;
    char *p;
    for (p = data; p ; p = strchr(p+1,'\n')) nls++;
    T2J_transfer_struct *transfer = new T2J_transfer_struct(nls,catfield);
    nls = 0;
    char *np;
    char ascii_2_col[256];
    memset(ascii_2_col,T2J_COLOR_UNKNOWN,256);
    ascii_2_col[(unsigned char)'+'] = T2J_COLOR_YES;
    ascii_2_col[(unsigned char)'p'] = T2J_COLOR_YES;
    ascii_2_col[(unsigned char)'1'] = T2J_COLOR_YES;
    ascii_2_col[(unsigned char)'-'] = T2J_COLOR_NO;
    ascii_2_col[(unsigned char)'n'] = T2J_COLOR_NO;
    ascii_2_col[(unsigned char)'0'] = T2J_COLOR_NO;
    int i;
    for (i=0;i<10;i++) {
	ascii_2_col['0' + i] = i;
    }
    for (i=0;i<6;i++){
	ascii_2_col['a' + i] = i + 10;
	ascii_2_col['A' + i] = i + 10;
    }
    for (p = data; p ; p = np) {
	np = strchr(p,'\n');
	if (np) *(np++) = 0;

	char *col = strchr(p,'#');	// color
	if (!col) continue;
	*(col++) = 0;

	char *val = strchr(col,'#');	// value
	if (!val) continue;
	*(val++) = 0;
	while ( *p == ' ') p++;		// remove leading spaces
	while ( *val == ' ') val++;
	transfer->items[nls].key = p;
	transfer->items[nls].color = ascii_2_col[(unsigned char)col[0]];
	if (np - val > MAXIMUM_LINE_LENGTH) val[MAXIMUM_LINE_LENGTH] = 0;// maximum line length 1000
	transfer->items[nls].value = val;
	nls++;
    }
    transfer->nitems = nls;
    return transfer;
}
/***********************************************************************
	Read a string from pt_server into the transfer struct:
	File Syntax: dummy\1id\1value\1id\1value...\0
***********************************************************************/
struct T2J_transfer_struct *T2J_read_query_result_from_pts(char *data){
    int nls = 0;
    char *p;
    for (p = data; p ; p = strchr(p+1,1)) nls++;
    T2J_transfer_struct *transfer = new T2J_transfer_struct(nls/2+1,CAT_FIELD_NAME);
    nls = 0;
    char *start = strchr(data,1);
    if(!start) {
	TRS_export_error("No hits");
	return 0;
    }
    char *next;
    for ( start++; start; start=next ) {
	char *info = strchr(start,1);
	if (!info) break;
	*info=0;
	info++;
	next = strchr(info,1);
	if (next) *(next++) = 0;
	transfer->items[nls].key = start;
	transfer->items[nls].color = T2J_COLOR_YES;
	transfer->items[nls].value = info;
	nls++;
    }
    transfer->nitems = nls;
    return transfer;
}
/***********************************************************************
	Read a file into the transfer struct:
	File Syntax: id#value\nid#value\n....
***********************************************************************/
struct T2J_transfer_struct *T2J_read_query_result_from_file(char *path,CAT_FIELDS catfield){
    char *data = TRS_read_file(path);
    if (!data) {
	TRS_export_error("Cannot open datafile '%s'",path);
	return 0;
    }
    return T2J_read_query_result_from_data(data,catfield);
}
/***********************************************************************
	Transfer the first word of the full_name ...
***********************************************************************/
char *T2J_transfer_fullnames1(char *path_of_tree,FILE *out) {
    if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);		// 3 lines that should never be deleted
    if (!cat_tree) return TRS_get_error();
    t2j_out = out;

    int i;
    for (i=0;i<cat_tree->nnodes;i++){
	CAT_node *node = & cat_tree->nodes[i];
	if (node->field_offsets[CAT_FIELD_FULL_NAME]) {
	    node->user_data = cat_tree->data + node->field_offsets[CAT_FIELD_FULL_NAME];
	    char *sep = strchr(node->user_data,' ');
	    if (sep) *sep = 0;
	}
	node->color = (node->color_in + 14 - T2J_COLOR_YES) & 15;
    }
    return T2J_send_tree(0);
}


/***********************************************************************
	Transfer the rest word of the full_name ...
***********************************************************************/
char *T2J_transfer_fullnames2(char *path_of_tree,FILE *out) {
    if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);		// 3 lines that should never be deleted
    if (!cat_tree) return TRS_get_error();
    t2j_out = out;

    int i;
    for (i=0;i<cat_tree->nnodes;i++){
	CAT_node *node = & cat_tree->nodes[i];
	if (node->field_offsets[CAT_FIELD_FULL_NAME]) {
	    node->user_data = cat_tree->data + node->field_offsets[CAT_FIELD_FULL_NAME];
	    char *sep = strchr(node->user_data,' ');
	    if (sep){
		while (*sep == ' ') sep++;
	    }
	    node->user_data = sep;
	}
	node->color = 0;
    }
    return T2J_send_tree(0);
}

/***********************************************************************
	Transfer inner node labels ...
***********************************************************************/
char *T2J_transfer_group_names(char *path_of_tree,FILE *out) {
    if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);		// 3 lines that should never be deleted
    if (!cat_tree) return TRS_get_error();
    t2j_out = out;

    int i;
    for (i=0;i<cat_tree->nnodes;i++){
	CAT_node *node = & cat_tree->nodes[i];
	if (node->field_offsets[CAT_FIELD_GROUP_NAME]) {
	    node->user_data = cat_tree->data + node->field_offsets[CAT_FIELD_GROUP_NAME];
	}

    }
    return T2J_send_tree(0);
}

long	t2j_get_deepest_node_that_contains_all_selected(CAT_node_id nn,
							char *selected_ids,long nselected, CAT_node_id *focusout){
    CAT_node *node = & cat_tree->nodes[nn];
    long sum = 0;
    if (node->leftson > 0) {		// inner node
	sum += t2j_get_deepest_node_that_contains_all_selected( node->leftson,
								selected_ids, nselected, focusout);
	sum += t2j_get_deepest_node_that_contains_all_selected( node->rightson,
								selected_ids, nselected, focusout);
    }else{
	sum += selected_ids[nn];	// count the selected leafs
    }
    if (sum == nselected){			// there is a hit !!
	if (*focusout == 0) {		// and it is really the first one
	    *focusout = nn;		// export it
	}
    }
    return sum;
}

/***********************************************************************
*	Convert names into selection
************************************************************************/
void t2j_out_number( int number, char base){
    do {
	printf("%c", (number &0xf) + base);
	number = number>>4;
    } while(number != 0);
}

void T2J_convert_colors_into_selection(){
    int p = 0;
    int lastWritten = 0;		// level Numbering
    while ( p < cat_tree->nnodes ){
	// search a selected terminal
	int sel;
	for (sel = p; sel < cat_tree->nnodes; sel++){
	    if ( cat_tree->nodes[sel].leftson != 0) continue; // inner node
	    if ( cat_tree->nodes[sel].color) break;
	}
	if (sel == cat_tree->nnodes) break;	// no more selections
	// search a non selected terminal

	int nsel;
	for (nsel = sel; nsel < cat_tree->nnodes; nsel++){
	    if ( cat_tree->nodes[nsel].leftson != 0) continue; // inner node
	    if ( !cat_tree->nodes[nsel].color) break;
	}
	p = nsel;				// start of next search
	int first = cat_tree->nodes[sel].numbering[CAT_NUMBERING_LEVELS];		// convert to level numbering
	int last = nsel;
	if (nsel < cat_tree->nnodes) last = cat_tree->nodes[nsel].numbering[CAT_NUMBERING_LEVELS];

	int h = lastWritten;
	lastWritten = last;
	last -= first;	// calculate delta values
	first -= h;

	t2j_out_number( first,'A' );
	t2j_out_number( last,'a' );
    }
    printf("\n");
}


/***********************************************************************
	Convert a selection into names ...
***********************************************************************/
/**	input path_of_tree	path of the tree.otb file
	sel			what is send from the java as a selection
	varname			a string that is prepended to the output
	all_nodes		output = all nodes or just ranges
	field_name		which field should be placed in the output CAT_FIELD_NAME/CAT_FIELD_FULL_NAME ...

	if all_nodes >0 then the programm calculates:
	focusout		the internal id that contains all selected nodes
	maxnodeout		the name of the inner node that contains all selected nodes
	maxnodehits		relativ number of selected_nodes/tip beneath maxnodeout
*/

char *T2J_get_selection(char *path_of_tree, char *sel, const char *varname, int all_nodes,CAT_FIELDS field_name,
			CAT_node_id *focusout, char **maxnodeout, double *maxnodehits){
    if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);
    if (!cat_tree) return 0;
    CAT_node_id *levelindex = t2j_mcreate_level_indexing();

    char *readp = sel;
    int nselected = 0;
    char *selected_ids = (char *)calloc(sizeof(char), cat_tree->nnodes);
    void *memfile = TRS_stropen(1024);
    int s,e;
    int c;
    int fac;
    TRS_strcat(memfile,varname);
    readp = sel;
    c = *(readp++);
    int last = 0;
    for (; c >='A' && c <='Z'; ){
	fac = 1;s = last;
	while ( c >='A' && c <='Z' ) {		// Read the start of the selection
	    s += fac * (c-'A');
	    fac *= 16;
	    c = *(readp++);
	}
	fac = 1;e = s;
	while ( c >='a' && c <='z' ) {		// End of the selection
	    e += fac * (c-'a');
	    fac *= 16;
	    c = *(readp++);
	}
	if (e<=0) e=1;
	if (s<0) s= 0;
	if (e > cat_tree->nnodes) e = cat_tree->nnodes;
	if (s >= cat_tree->nnodes) e = cat_tree->nnodes-1;
	last = e;

	if (all_nodes) {
	    for (;s<e;s++) {
		TRS_strcat(memfile,	cat_tree->data +
			   cat_tree->nodes[levelindex[s]].
			   field_offsets[field_name]);
		nselected++;
		selected_ids[levelindex[s]] ++;
		TRS_chrcat(memfile,'#');
	    }
	}else{
	    TRS_strcat(memfile,cat_tree->data +
		       cat_tree->nodes[levelindex[s]].field_offsets[field_name]);
	    // thats one of my favourite statements
	    TRS_chrcat(memfile,'#');
	    TRS_strcat(memfile,cat_tree->data +
		       cat_tree->nodes[levelindex[e-1]].field_offsets[field_name]);
	    TRS_chrcat(memfile,' ');
	}
    }
    char *result = TRS_strclose(memfile,0);
    if (focusout) {
	*focusout = 0;
	if (maxnodeout ) *maxnodeout = 0;
	if (maxnodehits) *maxnodehits = 0.0;
	if (nselected == 1) {
	    t2j_get_deepest_node_that_contains_all_selected(
							    0,selected_ids,nselected,focusout);
	    if (maxnodeout ) *maxnodeout = cat_tree->data +
				 cat_tree->nodes[*focusout].
				 field_offsets[CAT_FIELD_NAME];
	    if (maxnodehits) *maxnodehits = 1.0;
	}else if (nselected) {
	    t2j_get_deepest_node_that_contains_all_selected(
							    0,selected_ids,nselected,focusout);
	    CAT_node_id nextuppderlabeldnode = *focusout;

	    while ( nextuppderlabeldnode > 0
		    && cat_tree->nodes[nextuppderlabeldnode].
		    field_offsets[CAT_FIELD_GROUP_NAME] == 0 ) {
		nextuppderlabeldnode = cat_tree->nodes[nextuppderlabeldnode].father;
	    }
	    if (nextuppderlabeldnode) {	// get the name of the node
		if (maxnodeout ) *maxnodeout = cat_tree->data +
				     cat_tree->nodes[nextuppderlabeldnode].
				     field_offsets[CAT_FIELD_GROUP_NAME];
		if (maxnodehits) *maxnodehits = (double)nselected/
				     (double) cat_tree->nodes[nextuppderlabeldnode].nleafs;
	    }
	}
    }
    delete selected_ids;
    return result;
}

/** set color of inner nodes:
    color = leftson.color | rightson.color
    */

int t2j_set_color_rek(int node){
    int ls = cat_tree->nodes[node].leftson;
    int rs = cat_tree->nodes[node].rightson;
    int color = 0;
    if (ls != 0) color |= t2j_set_color_rek(ls);
    else	color |= cat_tree->nodes[node].color;
    if (rs != 0) color |= t2j_set_color_rek(rs);
    else	color |= cat_tree->nodes[node].color;
    cat_tree->nodes[node].color = color;
    return color;
}

/***********************************************************************
	Convert a selection into names ...
***********************************************************************/
/**	input path_of_tree	path of the tree.otb file
	sel			what is send from the java as a selection
	sets the color of all selected nodes to one, others to 0
	inner node get one if at least one child is set to one
*/

void T2J_set_color_of_selection(char *sel ){
    if (!sel){
	/**   select all nodes */
	for (int s=  0; s < cat_tree->nnodes; s++){
	    cat_tree->nodes[s].color = 1;
	}
	return;
    }
    int s,e;
    int c;
    int fac;
    char *readp = sel;
    int last = 0;
    CAT_node_id *levelindex = t2j_mcreate_level_indexing();

    /** deselect all nodes */
    for ( s = 0; s < cat_tree->nnodes;s++){
	cat_tree->nodes[s].color = 0;
    }


    c = *(readp++);
    for (; c >='A' && c <='Z'; ){
	fac = 1;s = last;
	while ( c >='A' && c <='Z' ) {		// Read the start of the selection
	    s += fac * (c-'A');
	    fac *= 16;
	    c = *(readp++);
	}
	fac = 1;e = s;
	while ( c >='a' && c <='z' ) {		// End of the selection
	    e += fac * (c-'a');
	    fac *= 16;
	    c = *(readp++);
	}
	if (e<=0) e=1;
	if (s<0) s= 0;
	if (e > cat_tree->nnodes) e = cat_tree->nnodes;
	if (s >= cat_tree->nnodes) e = cat_tree->nnodes-1;
	last = e;

	for (;s<e;s++) {
	    cat_tree->nodes[levelindex[s]].color = 1;
	}
    }
    t2j_set_color_rek(0);
}


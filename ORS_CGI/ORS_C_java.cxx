/*
#######################################
#                                     #
#    ORS_CLIENT:  JAVA                #
#    communication with java applet   #
#                                     #
####################################### */

#include <stdio.h>
#include <string.h>
#include <memory.h>
// #include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>

#include <ors_client.h>
#include <client.h>

#include <cat_tree.hxx>
#include "ors_c_java.hxx"
#include "ors_c.hxx"
#include "ors_c_proto.hxx"

FILE *t2j_out;	// output file
CAT_tree *cat_tree = NULL;	// the tree
FILE *t2j_debug;

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
	}else if (value > 0x1000) {
		t2j_write_nibble(3 | or_cmd);
		i = (value >>16) & 0xff; t2j_write_byte(i);
		i = (value >>8) & 0xff; t2j_write_byte(i);
		i = (value >>0) & 0xff; t2j_write_byte(i);
	}else if (value > 0x100){
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
	Sends the data in nodes->user_data to the java program
	modifier_string is just send after the main header
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

	GB_mergesort((void **)buffer,0,buf_size,
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
GB_ERROR T2J_send_tree(CAT_node_id focus, char *modifier_string){
	struct t2j_s1_item_header **hitems =
		(struct t2j_s1_item_header **)calloc(sizeof(struct t2j_s1_item_header *),
			(int)cat_tree->nnodes);	// nnodes is the very worst case !!!
	if (focus <0 || focus >= cat_tree->nnodes) focus = 0;
	int	header_in_use = 0;
	CAT_node_id	left_focus = focus;
	CAT_node_id	right_focus = t2j_get_focus_right_bound(focus);
	GB_HASH	*convert_hash = GBS_create_hash(cat_tree->nnodes,0);
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
			(struct t2j_s1_item_header *)GBS_read_hash(convert_hash, s);
		if (!hitem) {
			hitem = (struct t2j_s1_item_header *)
				calloc(sizeof(struct t2j_s1_item_header),1);
			hitems[header_in_use++] = hitem;
			hitem ->first = hitem->last = item;
			hitem->estimated_transfer_size = strlen(s)+5;
			hitem->value = s;
			GBS_write_hash_no_strdup(convert_hash,s,(long)hitem);
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
	GB_mergesort((void **)hitems,0,header_in_use,
		(long (* )(void *, void *, char *cd ))t2j_compare_two_hitems,0);

	CAT_NUMBERING numbering = CAT_NUMBERING_LEVELS;
	fputc(T2J_LEVEL_INDEXING,t2j_out);	// Write OTF tag
	t2j_write_int2(header_in_use);		// number of items

	if (modifier_string) {
		if (!fwrite(modifier_string, strlen(modifier_string) +1, 1, t2j_out)){
			return GB_export_error("Cannot write to out");
		}
	}
	// TODO: HIER NOCH EIN UNBEDINGTES "begin" anhaengen!

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
				CAT_node *node = & cat_tree->nodes[item->id];
				if (node->is_grouped != grouped || node->color != color) {
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
	Transform any query list to OTF
	Sideeffects: no !!!!
***********************************************************************/
GB_ERROR T2J_transform(char *path_of_tree, char *modifier_string,
		struct T2J_transfer_struct *data,
		CAT_node_id focus, FILE *out){
	if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);
	if (!cat_tree) return GB_get_error();
	t2j_out = out;
	GB_HASH *index = GBS_create_hash(cat_tree->nnodes,0);
	int i;
			// create the index !!!!!!!!!!!!
	for (i=0;i< cat_tree->nnodes;i++) {
		if (cat_tree->nodes[i].field_offsets[data->key_index]) {
			GBS_write_hash_no_strdup(index,cat_tree->data +
				cat_tree->nodes[i].field_offsets[data->key_index],i+1);
		}else 	if (cat_tree->nodes[i].field_offsets[CAT_FIELD_GROUP_NAME]) {
			GBS_write_hash_no_strdup(index,cat_tree->data +
				cat_tree->nodes[i].field_offsets[CAT_FIELD_GROUP_NAME],i+1);
		}
	}


	for (i=0;i<data->nitems;i++) {
		long ind = GBS_read_hash(index, data->items[i].key);
		if (!ind) continue;
		ind -= 1;
		cat_tree->nodes[ind].color = data->items[i].color;
		cat_tree->nodes[ind].user_data = data->items[i].value;
	}
	return T2J_send_tree(focus, modifier_string);
}

/***********************************************************************
	Send the tree.   A '1' bit indicates a node '0' a leaf
	Sideeffects: writes to out
***********************************************************************/
GB_ERROR T2J_send_bit_coded_tree(char *path_of_tree, FILE *out){
	if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);	// 3 lines that should never be deleted
	if (!cat_tree) return GB_get_error();
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

void t2j_send_newick_rek(CAT_node *node, FILE *out){
    const char *name = 0;
    {
	int n_offset = node->field_offsets[CAT_FIELD_NAME];
	if (n_offset){
	    name = & cat_tree->data[n_offset];
	}
    }
    if (!node->leftson){	// leaf
	if (name){
	    fprintf(out,"'%s':%i",name,ode->branch_length);
	}
	return;
    }
    fprintf(out,"(");
    t2j_send_newick_rek(cat_tree->nodes[node->leftson],out);
    fprintf(out,",");
    t2j_send_newick_rek(cat_tree->nodes[node->rightson],out);
    if (name){
	fprintf(out,")'%s':%i",name,ode->branch_length);
    }else{
	fprintf(out,"):%i",ode->branch_length);
    }
}

GB_ERROR T2J_send_newick_tree(char *path_of_tree, FILE *out){
    if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);	// 3 lines that should never be deleted
    if (!cat_tree) return GB_get_error();
    t2j_out = out;
    fprintf(out,"[%i]\n",cat_tree->nnodes);
    t2j_send_newick_rek(&cat_tree->nodes[0],out);
    fprintf(out,"\n");
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

GB_ERROR T2J_send_branch_lengths(char *path_of_tree, FILE *out){
	if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);		// 3 lines that should never be deleted
	if (!cat_tree) return GB_get_error();
	t2j_out = out;
	CAT_node_id *ids = t2j_mcreate_level_indexing();
	int i;
	for (i=0;i<cat_tree->nnodes;i++){
		CAT_node *node = & cat_tree->nodes[ids[i]];
		int bl = node->branch_length;
		if (bl>=16) bl = 15;
		t2j_write_nibble(bl);
	}
	delete ids;
	t2j_flush_nibble();
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
	ascii_2_col['+'] = T2J_COLOR_YES;
	ascii_2_col['p'] = T2J_COLOR_YES;
	ascii_2_col['1'] = T2J_COLOR_YES;
	ascii_2_col['-'] = T2J_COLOR_NO;
	ascii_2_col['n'] = T2J_COLOR_NO;
	ascii_2_col['0'] = T2J_COLOR_NO;
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
		transfer->items[nls].color = ascii_2_col[col[0]];
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
		GB_export_error("No hits");
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
	char *data = GB_read_file(path);
	if (!data) {
		GB_export_error("Cannot open datafile '%s'",path);
		return 0;
	}
	return T2J_read_query_result_from_data(data,catfield);
}
/***********************************************************************
	Transfer the first word of the full_name ...
***********************************************************************/
GB_ERROR T2J_transfer_fullnames1(char *path_of_tree,FILE *out) {
	if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);		// 3 lines that should never be deleted
	if (!cat_tree) return GB_get_error();
	t2j_out = out;

	int i;
	for (i=0;i<cat_tree->nnodes;i++){
		CAT_node *node = & cat_tree->nodes[i];
		if (node->field_offsets[CAT_FIELD_FULL_NAME]) {
			node->user_data = cat_tree->data + node->field_offsets[CAT_FIELD_FULL_NAME];
			char *sep = strchr(node->user_data,' ');
			if (sep) *sep = 0;
		}
		node->color = node->color_in;
	}
	return T2J_send_tree(0,0);
}

/***********************************************************************
	Transfer the rest word of the full_name ...
***********************************************************************/
GB_ERROR T2J_transfer_fullnames2(char *path_of_tree,FILE *out) {
	if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);		// 3 lines that should never be deleted
	if (!cat_tree) return GB_get_error();
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
		node->color = node->color_in;
	}
	return T2J_send_tree(0,0);
}

/***********************************************************************
	Transfer inner node labels ...
***********************************************************************/
GB_ERROR T2J_transfer_group_names(char *path_of_tree,FILE *out) {
	if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);		// 3 lines that should never be deleted
	if (!cat_tree) return GB_get_error();
	t2j_out = out;

	int i;
	for (i=0;i<cat_tree->nnodes;i++){
		CAT_node *node = & cat_tree->nodes[i];
		if (node->field_offsets[CAT_FIELD_GROUP_NAME]) {
			node->user_data = cat_tree->data + node->field_offsets[CAT_FIELD_GROUP_NAME];
		}

	}
	return T2J_send_tree(0,0);
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
	Convert a selection into names ...
***********************************************************************/
/**	input path_of_tree	path of the tree.otb file
	sel			what is send from the java as a selection
	varname			a string that is prepended to the output
	all_nodes		output = all nodes or just ranges

	if all_nodes >0 then the program calculates:
	focusout		the internal id that contains all selected nodes
	maxnodeout		the name of the inner node that contains all selected nodes
	maxnodehits		relativ number of selected_nodes/tip beneath maxnodeout
*/

char *T2J_get_selection(char *path_of_tree, char *sel, const char *varname, int all_nodes,
			CAT_node_id *focusout, char **maxnodeout, double *maxnodehits){
	if (!cat_tree)	cat_tree = load_CAT_tree(path_of_tree);
	if (!cat_tree) return 0;
	CAT_node_id *levelindex = t2j_mcreate_level_indexing();

	char *readp = sel;
	int nselected = 0;
	char *selected_ids = (char *)calloc(sizeof(char), cat_tree->nnodes);
	void *memfile = GBS_stropen(1024);
	int s,e;
	int c;
	int fac;
	GBS_strcat(memfile,varname);
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
				GBS_strcat(memfile,	cat_tree->data +
					cat_tree->nodes[levelindex[s]].
					field_offsets[CAT_FIELD_NAME]);
				nselected++;
				selected_ids[levelindex[s]] ++;
				GBS_chrcat(memfile,'#');
			}
		}else{
			GBS_strcat(memfile,cat_tree->data +
				cat_tree->nodes[levelindex[s]].field_offsets[CAT_FIELD_NAME]);
					// thats one of my favourites statements
			GBS_chrcat(memfile,'#');
			GBS_strcat(memfile,cat_tree->data +
				cat_tree->nodes[levelindex[e-1]].field_offsets[CAT_FIELD_NAME]);
			GBS_chrcat(memfile,' ');
		}
	}
	char *result = GBS_strclose(memfile);
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

/***********************************************************************
	TEST TEST TEST ...
***********************************************************************/
#if 0
int main(int argc, char **argv) {
	if (argc != 3) {
		fprintf(stderr,"Syntax: %s treefilename datafile > outfile\n",argv[0]);
		fprintf(stderr,"        %s -tree treefilename     > outfile\n",argv[0]);
		fprintf(stderr,"        %s -bl treefilename     > outfile\n",argv[0]);
		return -1;
	}
	char *error = 0;
	if (!strcmp(argv[1],"-tree")){
		error = T2J_send_bit_coded_tree(argv[2],stdout);
	}else if (!strcmp (argv[1], "-bl")){
		error = T2J_send_branch_lengths(argv[2],stdout);
	}else{
		T2J_transfer_struct *transfer = T2J_read_query_result_from_file(argv[2]);
		if (!transfer){
			error = GB_get_error();
		}
		if (!error) error = T2J_transform(argv[1],0, transfer, 0, stdout);
	}
	if (error) {
		fprintf(stderr,"%s\n",error);
		return -1;
	}
	return 0;
}
#endif

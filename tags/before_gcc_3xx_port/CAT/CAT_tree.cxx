#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <malloc.h>
#include <memory.h>

#include <arbdb.h>
#include <arbdbt.h>

#include "cat_tree.hxx"


const char *CAT_field_names[CAT_FIELD_MAX];	// Fields which are extracted from the arb database
			// syntax:	field[:word/(range of words]
			// ex	full_name

double CATL_tree_max_deep(GBT_TREE *tree){
	if (tree->is_leaf) return 0.0;
	double l,r;
	l = CATL_tree_max_deep(tree->leftson);
	r = CATL_tree_max_deep(tree->rightson);
	if (tree->leftlen<0) tree->leftlen = 0;
	if (tree->rightlen<0) tree->rightlen = 0;
	l += tree->leftlen;
	r += tree->rightlen;
	if (l>r) return l;
	return r;
}

int CATL_size_of_tree(GBT_TREE *tree){
	if (tree->is_leaf) return 1;
	return CATL_size_of_tree(tree->leftson) +
		CATL_size_of_tree(tree->rightson) +1;
}


CAT_tree *new_CAT_tree(int size)
 {
	CAT_tree *THIS = (CAT_tree *)calloc(sizeof(CAT_tree) + sizeof(CAT_node) * (size-1),1);

	CAT_field_names[CAT_FIELD_NAME] = "name";
	CAT_field_names[CAT_FIELD_FULL_NAME] = "full_name";
	CAT_field_names[CAT_FIELD_GROUP_NAME] = "group_name";
	CAT_field_names[CAT_FIELD_ACC] = "acc";
	CAT_field_names[CAT_FIELD_STRAIN] = "strain";
	return THIS;
}


void *cat_mem_files[CAT_FIELD_MAX];


GB_ERROR cat_write_cat_tree(CAT_tree *cat_tree, FILE *out){
	char *cat_strings[CAT_FIELD_MAX];
	long cat_offsets[CAT_FIELD_MAX];
	long cat_sizes[CAT_FIELD_MAX];
	int i,j,node;
	for (i=0;i<CAT_FIELD_MAX;i++) cat_strings[i] = GBS_strclose(cat_mem_files[i]);
	long sizeoftree = sizeof(CAT_tree) + sizeof(CAT_node) * (cat_tree->nnodes-1);
	long offset = sizeoftree;
	for (i=0;i<CAT_FIELD_MAX;i++){
		int len = strlen(cat_strings[i])+1;
		cat_sizes[i] = len;
		// change '^A' seperated strings to \0 sep. strings
		for (j=0;j<len;j++) if (cat_strings[i][j] == 1) cat_strings[i][j] = 0;
		cat_offsets[i] = offset;
		offset += len;
	}

	for (node = 0; node < cat_tree->nnodes; node++) {
		for (i=0;i<CAT_FIELD_MAX;i++) {
			if ( cat_tree->nodes[node].field_offsets[i] == 0) continue;
				// no field
			cat_tree->nodes[node].field_offsets[i] += cat_offsets[i];
		}
	}

	if (	!fwrite( (char *)cat_tree,(int)(sizeoftree), 1, out) ) {
		return GB_export_error("Disk Full");
	}


	for (i=0;i<CAT_FIELD_MAX;i++){
		if (!fwrite( cat_strings[i], (int)cat_sizes[i], 1, out) ) {
			return GB_export_error("Disk Full");
		}
	}
	return 0;
}

long cat_convert_tree_2_cat_rek(GBT_TREE *tree, CAT_tree *cat_tree, int deep, double scale,
		int *level_counter, int& leaf_counter, int& node_counter){
	int nodec = node_counter++;
	CAT_node *node = & cat_tree->nodes[nodec];
	node->deep = deep;
	if (tree->is_leaf) {
		node->numbering[CAT_NUMBERING_LEAFS_ONLY] = leaf_counter++;
		node->numbering[CAT_NUMBERING_LEVELS] = node->numbering[CAT_NUMBERING_LEAFS_ONLY] + cat_tree->nnodes/2;
	}else{
		node->numbering[CAT_NUMBERING_LEVELS] = level_counter[deep];
		level_counter[deep]++;

		node->leftson = cat_convert_tree_2_cat_rek(tree->leftson,cat_tree,deep+1,scale,
				level_counter,leaf_counter,node_counter);
		node->rightson = cat_convert_tree_2_cat_rek(tree->rightson,cat_tree,deep+1,scale,
				level_counter,leaf_counter,node_counter);
		cat_tree->nodes[node->leftson].father = nodec;
		cat_tree->nodes[node->rightson].father = nodec;

		double l = tree->leftlen;
		double r = tree->rightlen;

		if (l<0) l = 0.0;
		if (r<0) r = 0.0;

		cat_tree->nodes[node->leftson].branch_length_float = l;
		cat_tree->nodes[node->rightson].branch_length_float = r;

		l *= scale;
		r *= scale;
		if (l>0 && l<1) l = 1;
		if (r>0 && r<1) r = 1;

		if (l> 255) l = 255;
		if (r > 255) l = 255;

		cat_tree->nodes[node->leftson].branch_length_byte = (unsigned char)l;
		cat_tree->nodes[node->rightson].branch_length_byte = (unsigned char)r;
	}
	if (tree->gb_node) {
		int i;
		const char *f;
		GBDATA *gbd;
		int	start = 0;
		int	end = 1000000;
		for (i=0;i<CAT_FIELD_MAX;i++) {
			start = 0;
			end = 1000000;
			f = CAT_field_names[i];
			gbd = GB_find(tree->gb_node,f,0,down_level);
			if (gbd){
				char * s= GB_read_as_string(gbd);
				if (s && strlen(s)){		// append text to output stream
					node->field_offsets[i] = GBS_memoffset(cat_mem_files[i]);
					GBS_strcat(cat_mem_files[i],s);
					GBS_chrcat(cat_mem_files[i],1);	// seperated by '^A'
					delete s;
				}
			}
		}
		gbd = GB_find(tree->gb_node,"grouped",0,down_level);
		if (gbd) {
			node->is_grouped_in = GB_read_byte(gbd);
		}
		node->color_in = T2J_COLOR_NO - GB_read_flag(tree->gb_node);
	}else{
		if (tree->name){
		    if (tree->is_leaf){
			char *name = strdup(tree->name);
			char *fullname = strchr(name,'#');
			if (fullname){
			    *(fullname++) = 0;
			}else{
			    fullname = name;
			}

			node->field_offsets[CAT_FIELD_NAME] = GBS_memoffset(cat_mem_files[CAT_FIELD_NAME]);
			GBS_strcat(cat_mem_files[CAT_FIELD_NAME],name);
			GBS_chrcat(cat_mem_files[CAT_FIELD_NAME],1);	// seperated by '^A'

			node->field_offsets[CAT_FIELD_FULL_NAME] = GBS_memoffset(cat_mem_files[CAT_FIELD_FULL_NAME]);
			GBS_strcat(cat_mem_files[CAT_FIELD_FULL_NAME],fullname);
			GBS_chrcat(cat_mem_files[CAT_FIELD_FULL_NAME],1);// seperated by '^A'
			delete name;
		    }else{
			node->field_offsets[CAT_FIELD_GROUP_NAME] = GBS_memoffset(cat_mem_files[CAT_FIELD_GROUP_NAME]);
			GBS_strcat(cat_mem_files[CAT_FIELD_GROUP_NAME],tree->name);
			GBS_chrcat(cat_mem_files[CAT_FIELD_GROUP_NAME],1);	// seperated by '^A'
		    }
		}else{
			node->color_in = T2J_COLOR_UNKNOWN;
		}
	}
	return nodec;
}

/** calculates the leafes for all inner nodes */
long calcnleafs(CAT_tree *cat_tree, int nodenr){
	if (cat_tree->nodes[nodenr].leftson) {
		int sum =	calcnleafs(cat_tree, cat_tree->nodes[nodenr].leftson) +
				calcnleafs(cat_tree, cat_tree->nodes[nodenr].rightson);
		cat_tree->nodes[nodenr].nleafs = sum;
		return sum;
	}
	cat_tree->nodes[nodenr].nleafs = 1;
	return 1;
}

CAT_tree *convert_GBT_tree_2_CAT_tree(GBT_TREE *tree) {
	CAT_tree *cat_tree;
	int size_of_tree = CATL_size_of_tree(tree);
	cat_tree = new_CAT_tree(size_of_tree);
	cat_tree->nnodes = size_of_tree;
	int leaf_counter = 0;
	int node_counter = 0;
	int *level_counter = (int *)calloc(sizeof(int),size_of_tree);

	double scale = 255/CATL_tree_max_deep(tree);
	cat_convert_tree_2_cat_rek(tree, cat_tree, 0,scale,
				level_counter, leaf_counter, node_counter);
	int i;
	for (i = 1; i < size_of_tree; i++) level_counter[i] += level_counter[i-1];
	for (i = 0; i < size_of_tree; i++) {
		CAT_node *node = & cat_tree->nodes[i];
		if (node->deep && node->leftson){
			node->numbering[CAT_NUMBERING_LEVELS] += level_counter[node->deep-1];
		}
	}
	calcnleafs(cat_tree,0);

	delete level_counter;
	return cat_tree;
}

void cat_debug_tree(CAT_tree *cat_tree){
	int i;
	for (i=0;i< cat_tree->nnodes; i++){
		CAT_node *node = & cat_tree->nodes[i];
		printf("%4i f%4li l%4li r%4li lfn%4li lvl%4li d%4li bl%4i    ",
			i,
			node->father,
			node->leftson,
			node->rightson,
			node->numbering[CAT_NUMBERING_LEAFS_ONLY],
			node->numbering[CAT_NUMBERING_LEVELS],
			node->deep,
			(int)node->branch_length_byte);
		int j;
		for (j=0;j<CAT_FIELD_MAX;j++){
			if (node->field_offsets[j]) {
				printf("%15s ",cat_tree->data+node->field_offsets[j]);
			}
		}
		printf("\n");
	}
}

GB_ERROR create_and_save_CAT_tree(GBT_TREE *tree, const char *path){
    GB_ERROR error = 0;
    int i;
    for (i = 0; i < CAT_FIELD_MAX; i++){
	cat_mem_files[i] = GBS_stropen(CAT_INIT_STRING_FILE_SIZE);
	GBS_chrcat(cat_mem_files[i],1);		// write 1 byte so that all further
	// entries get a non zero offset
    }
    CAT_tree *cat_tree = convert_GBT_tree_2_CAT_tree(tree);
    if (!cat_tree) return GB_get_error();
    FILE *out = fopen(path,"w");
    if (!out) return GB_export_error("Sorry cannot write to file '%s'",path);
    error = cat_write_cat_tree(cat_tree,out);
    fclose(out);
    return error;

}

GB_ERROR create_and_save_CAT_tree(GBDATA *gb_main, const char *tree_name, const char *path){
    GB_transaction dummy(gb_main);

    GBT_TREE *tree = GBT_read_tree(gb_main,tree_name, sizeof(GBT_TREE) );
    if (!tree) return GB_get_error();
    GB_ERROR error = GBT_link_tree(tree,gb_main,GB_FALSE);
    if (error) return error;
    error = create_and_save_CAT_tree(tree,path);
    return error;
}

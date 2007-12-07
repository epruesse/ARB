#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <arbdb.h>
#include <arbdbt.h>

#include <ors_client.h>

#include <cat_tree.hxx>
#include "ors_lib.h"
#include "ors_c_java.hxx"
#include "ors_c.hxx"
#include "ors_c_proto.hxx"

void ORS_C_make_clean(char *data){			// removes all non ascii characters
							// from the filename
	char *source = data;
	char *dest = data;
	int c;
	int len = 0;
	while (  (c=*(source++)) != 0) {
		if (isalnum(c)) *(dest++) = c;
		if (len++ > 256) break;
	}
	*dest = 0;
}

GB_ERROR ORS_C_try_to_do_java_function(void){
	//////// BIN_TREE (JAVA) /////////
	char *bin_tree_file = OC_cgi_var_2_filename("bt","tree_names","BIN_TREE");
	char *bin_tree_path = GBS_global_string_copy(ORS_LIB_PATH "%s",bin_tree_file);
	GB_ERROR error = 0;
			// look for predefined datas
	if (!ORS_strncase_tail_cmp(ors_gl.path_info,"/GET_TMP_FILE")) {
		char tmpfile[1025];
		char *ffile = strdup(cgi_var("dailypw"));
		ORS_C_make_clean(ffile);
		sprintf(tmpfile,ORS_TMP_PATH "ovp_%s",ffile);
		delete ffile;
		char *data = GB_read_file(tmpfile);
		if (!data) {
			return GB_export_error("This button is inactive now. You will be informed"
				" when you should use it");
		}
		long data_size = GB_size_of_file(tmpfile);
		fwrite(data,data_size,1,stdout);
		GB_unlink(tmpfile);
		return NULL;
	}

	char *ors_tmp_html = cgi_var("ors_tmp_html");
	if (*ors_tmp_html) {			// old file is requested
		char tmpfile[1025];
		int i = atoi(ors_tmp_html+4);	// the old pid id
		sprintf(tmpfile,ORS_TMP_PATH "ors_%i",i);
		char *f = GB_read_file(tmpfile);
		if (!f) return GB_get_error();
		fwrite(f,1,strlen(f),stdout);
		GB_unlink(tmpfile);
		return NULL;
	}


	char *orsexec = GBS_find_string(ors_gl.path_info,"orsexec/",1);
	if (orsexec) {
		orsexec+=strlen("orsexec/");
		if (!strcasecmp(orsexec,"BIN_TREE")) {
			error=T2J_send_bit_coded_tree(bin_tree_path,stdout);
		}else if (!strcasecmp(orsexec,"BT_BL")) {
			error=T2J_send_branch_lengths(bin_tree_path,stdout);
		}
		//////// BT_NAME_1 (JAVA) : base names /////////
		else if (!strcasecmp(orsexec,"BT_NAME_1")) {
			error = T2J_transfer_fullnames1(bin_tree_path,stdout);
		}
		//////// BT_NAME_2 (JAVA) : base names /////////
		else if (!strcasecmp(orsexec,"BT_NAME_2")) {
			error = T2J_transfer_fullnames2(bin_tree_path,stdout);
		}
		//////// BT_NODE_1 (JAVA) : inner node names /////////
		else if (!strcasecmp(orsexec,"BT_NODE_1")) {
			error = T2J_transfer_group_names(bin_tree_path,stdout);
		}else{
			return GB_export_error("Unknown Java Function orsexec='%s'",orsexec);
		}
		return error;
	}
	char *ors_script = GBS_find_string(ors_gl.path_info,"/" ORS_SCRIPT_PATH,1);
	if (ors_script) {
		ors_script += strlen(ORS_SCRIPT_PATH) + 1;
		return ORS_C_exec_script(bin_tree_path,ors_script);
	}

	return ORS_export_error("Unknown Java Function '%s'\n",
				ors_gl.path_info);
}

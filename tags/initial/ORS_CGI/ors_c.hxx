
#define JAVA ors_gl.java_mode
#define PASSWORD_MIN_LENGTH 16

typedef long T_ORS_LOCAL;
typedef long T_ORS_MAIN;

extern struct OC_pdb_list {
	OC_pdb_list *next;
	OC_pdb_list *prev;
	char *section;
	char *name;
	char *db_name;
	char *type;
	int  width;
	int  height;
	char *rights;
	char *init;
	char *label;
	char *help_page;
	char *features;
	char *content;
} OC_pdblist;

extern struct gl_struct {
	aisc_com *link;
	T_ORS_LOCAL locs;
	T_ORS_MAIN com;
	char **cgi_vars;    	// imported CGI variables
	char *userpath;     	// username with all parent users (as path)
	char *username;     	// user's fullname 
	char *password;     	// user's password 
	int  is_superuser;	// user's state
	int  is_author;		// user's state
	char *remote_host;  	// hostname according to browser
	char *remote_user;  	// userid according to browser
	char *path_info;    	// appendix to cgi-bin path
	char *user_info;	// user's info (set by father)
	char *user_own_info;	// user's own info
	char *mail_addr;	// user's mail addresses
	char *www_home;		// user's www home page
	char *pub_exist_max; 	// user's maximum exist pl
	char *pub_content_max;	// user's maximum exist pl
	char *pub_exist_def; 	// user's default exist pl
	char *pub_content_def;	// user's defaults exist pl
	int  max_users;	// user's maximum num of subusers
	int  curr_users;	// user's current num of subusers
	int  max_user_depth;	// user's maximum depth of subusers
	int  user_ta_id;	// user's current transaction id
	CAT_node_id focus;

	char *sel_user;      	// user (still without path, just being created)
	char *sel_par_userpath; // user parent path (without name)
	char *sel_userpath;  	// selected userpath
	char *sel_userpath2;  	// second selected userpath
	char *sel_username;  	// selected user's fullname
	char *sel_password;  	// selected user's password
	int  sel_is_superuser;	// sel_user's state
	int  sel_is_author;	// sel_user's state
	char *sel_mail_addr; 	// selected user's mail addresses
	char *sel_www_home; 	// selected user's www home page
	char *sel_user_info;	// selected user's info (set by father)
	char *sel_user_own_info;// selected user's own info
	char *sel_pub_exist_max;// selected user's maximum exist pl
	char *sel_pub_content_max; // selected user's maximum exist pl
	int  sel_max_users; 	// selected user's maximum num of subusers
	int  sel_curr_users;	// selected user's current num of subusers
	int  sel_max_user_depth;// selected user's maximum depth of s.u.
	int  sel_user_ta_id;	// selected user's current transaction id

// lists may be needed more than once - so they are cached
// to avoid multiple transfer from server
	char *list_of_subusers;
	char *list_of_sel_parents;
	char *list_of_all_users;
	char *list_of_who;	// who is logged in currently
	char *list_of_who_my;	// who is logged in currently (my users only)
	char *list_of_probes;	// user's probes

	char *sel_species;  	// selected species short name

	char *action;       	// action of a html page (if not hard coded)
	char *search;       	// search mode (simple, std, full)
	char *search_any_field;	// search field to search through all fields

	char *dailypw;      	// temporarily generated password
	char *bt;		// name of base tree
	char *message;      	// output message for html-page ##MESSAGE##
	int  debug;         	// behave in debug mode
	int  java_mode;         // behave in binary (not html) mode

	OC_pdb_list *pdb_list; // list of probedb fields
	int  probe_ta_id;	// probe's current transaction id

	char *sequence;		// probe sequence
	char *target_seq;	// target sequence of probe sequence
	char *allowed_bases;	// these bases are allowed in sequence
	char *probe_id;		// id of current probe
	char *cloned_probe_id;	// id of cloned probe
	
} ors_gl;



const int OC_MAX_HTML_TABLE_WIDTH = 20;
const int TABLE_ALIGN_LEFT   = 1;
const int TABLE_ALIGN_RIGHT  = 2;
const int TABLE_ALIGN_CENTER = 3;

typedef void (*OC_html_create_func_type)(char *content, int col_mode, char *col_param, int row, int col) ;

class OC_output_html_table {
	public:
	OC_html_create_func_type f[OC_MAX_HTML_TABLE_WIDTH];	// function for each column
	int	f_set[OC_MAX_HTML_TABLE_WIDTH];			// does this function exist?
	int 	virtual_field[OC_MAX_HTML_TABLE_WIDTH];		// virtual field = function with input from another field
	char * 	content[OC_MAX_HTML_TABLE_WIDTH];		// field storage for each line
	int 	col_mode[OC_MAX_HTML_TABLE_WIDTH];		// mode for each col_function
	char * 	col_param[OC_MAX_HTML_TABLE_WIDTH];		// param for each col_function
	int 	other_col[OC_MAX_HTML_TABLE_WIDTH];		// read data from col for each col_function
	int 	is_header[OC_MAX_HTML_TABLE_WIDTH];		// 1 = <hd>, 0 = <td>

	int 	exclude_field[OC_MAX_HTML_TABLE_WIDTH];		// 1 = exclude this field from input list
	int 	invisible_field[OC_MAX_HTML_TABLE_WIDTH];	// 1 = this field content is not printed (but available for col functions)
	int 	invisible[OC_MAX_HTML_TABLE_WIDTH];		// 1 = this column is invisible
	int	from_field;
	int	ignore_fields;

	int	is_map_public;
	int	table_command;		// output table command?
	int	border_width;
	int	align_mode;

	//                   	output is 	function 			content
	//                   	written 	to start			is taken
	//                   	here		in each column			from here 
	void	set_col_func(	int output_col,	OC_html_create_func_type fin, int other_column, int mode) { 
						  f[output_col] = fin; if (output_col != other_column) virtual_field[output_col] = 1; 
									f_set[output_col] = 1; col_mode[output_col] = mode;
									other_col[output_col] = other_column; };
	void	set_col_func(	int output_col,	OC_html_create_func_type fin, int other_column) { 
						  f[output_col] = fin; if (output_col != other_column) virtual_field[output_col] = 1; 
									f_set[output_col] = 1; other_col[output_col] = other_column; };
	void	set_col_func(	int output_col,	OC_html_create_func_type fin) { 
						  f[output_col] = fin; f_set[output_col] = 1; other_col[output_col] = output_col; };
	void	set_col_mode(	int col, int mode) 	{   col_mode[col] = mode; };
	void	set_col_param(	int col, char *param) 	{   col_param[col] = param; };

	void	from_list_field(int from) 	{ this->from_field = from; };
	void	ignore_list_fields(int ignore) 	{ this->ignore_fields = ignore; };
	void	map_public() 			{ this->is_map_public = 1; };
	void	no_table_command() 		{ this->table_command = 0; };
	void	border(int border) 		{ this->border_width = border; };
	void	border(void) 			{ this->border_width = 1; };
	void	no_border(void) 		{ this->border_width = 0; };
	void	exclude_list_field(int pos) 	{ this->exclude_field[pos] = 1; };
	void	invisible_list_field(int pos) 	{ this->invisible_field[pos] = 1; };
	void	header_column(int pos) 		{ this->is_header[pos] = 1; };
	void	align(int align) 		{ this->align_mode = align; };
	void create();
	void output(char *list, int col, char *header, char *headline);
};

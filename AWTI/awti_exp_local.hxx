#define AWAR_EXPORT_FILE "tmp/export_db/file"
#define AWAR_EXPORT_FORM "tmp/export/form"
#define AWAR_EXPORT_ALI "tmp/export/alignment"
#define AWAR_EXPORT_MULTIPLE_FILES "tmp/export/multiple_files"

struct export_format_struct {
	char *system;
    char *internal_command;
	char *new_format;
	char *suffix;

	export_format_struct(void);
	~export_format_struct(void);
};

AW_BOOL  awtc_read_string_pair(FILE *in, char *&s1,char *&s2);

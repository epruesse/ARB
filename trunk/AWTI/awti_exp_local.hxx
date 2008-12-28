#define AWAR_EXPORT_FILE           "tmp/export_db/file"
#define AWAR_EXPORT_FORM           "export/form"
#define AWAR_EXPORT_ALI            "tmp/export/alignment"
#define AWAR_EXPORT_MULTIPLE_FILES "tmp/export/multiple_files"
#define AWAR_EXPORT_MARKED         "export/marked"
#define AWAR_EXPORT_COMPRESS       "export/compress"
#define AWAR_EXPORT_FILTER_PREFIX  "export/filter"
#define AWAR_EXPORT_FILTER_NAME    AWAR_EXPORT_FILTER_PREFIX "/name"
#define AWAR_EXPORT_FILTER_FILTER  AWAR_EXPORT_FILTER_PREFIX "/filter"
#define AWAR_EXPORT_FILTER_ALI     AWAR_EXPORT_FILTER_PREFIX "/alignment"
#define AWAR_EXPORT_CUTSTOP        "export/cutstop"

AW_BOOL awtc_read_string_pair(FILE *in, char *&s1, char *&s2, size_t& lineNr);

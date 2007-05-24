//#include <xview/cms.h>

#if defined(__STDC__) || defined(__cplusplus)
# define P_(s) s
#else
# define P_(s) ()
#endif
#define GDEMAXMENU 100

/* NTREE/GDE_event.cxx */
/*void GDE_startaction_cb P_((AW_window *aw,AWwindowinfo *AWinfo,AW_CL cd));*/
char *GDE_makeawarname P_((AWwindowinfo *AWinfo,long i));
char *GBS_string_to_key P_((char *s));

/* NTREE/GDE_ParseMenu.cxx */
void ParseMenu P_((void));
int Find P_((const char *target, const char *key));
int Find2 P_((const char *target, const char *key));
void Error P_((const char *msg)) __attribute__((noreturn));
int getline P_((FILE *file, char *string));
void crop P_((char *input, char *head, char *tail));

/* GDE_arbdb_io.cxx */
int Arbdb_get_curelem P_((NA_Alignment *dataset));
void ReadArbdb_plain P_((char *filename, NA_Alignment *dataset, int type));
class AP_filter;
int ReadArbdb P_((NA_Alignment *dataset,long marked,AP_filter *filter,long compress, bool cutoff_stop_codon));
int ReadArbdb2 P_((NA_Alignment *dataset,AP_filter *filter,long compress, bool cutoff_stop_codon));
int WriteArbdb P_((NA_Alignment *aln, char *filename, int method, int maskable));
//void Updata_Arbdb P_((Panel_item item, Event *event));
int getelem P_((NA_Sequence *a,int b));
void putelem P_((NA_Sequence *a,int b,NA_Base c));
void putcmask P_((NA_Sequence *a,int b,int c));

int isagap P_((NA_Sequence *a,int b));

/* GDE_FileIO.cxx */
int MAX P_((int a,int b));
int MIN P_((int a,int b));
void ErrorOut5 P_((int code, const char *string));
char *Calloc P_((int count, int size));
char *Realloc P_((char *block, int size));
void Cfree P_((char *block));
char *String P_((char *str));
void LoadData P_((char *filen));
void LoadFile P_((char *filename, NA_Alignment *dataset, int type, int format));
int FindType P_((char *name, int *dtype, int *ftype));
void AppendNA P_((NA_Base *buffer, int len, NA_Sequence *seq));
void Ascii2NA P_((char *buffer, int len, int matrix[16 ]));
int WriteNA_Flat P_((NA_Alignment *aln, char *filename, int method, int maskable));
void Warning P_((const char *s));
void InitNASeq P_((NA_Sequence *seq, int type));
void ReadCMask P_((const char *filename));
void ReadNA_Flat P_((char *filename, char *dataset, int type));
int WriteStatus P_((NA_Alignment *aln, char *filename, int method));
void ReadStatus P_((char *filename));
void NormalizeOffset P_((NA_Alignment *aln));
int WriteCMask P_((NA_Alignment *aln, char *filename, int method, int maskable));
void Regroup P_((NA_Alignment *alignment));

/* HGLfile.cxx */
int OverWrite P_((NA_Sequence *thiss,NA_Alignment *aln));
char *uniqueID P_(());
void SeqNorm P_((NA_Sequence *seq));
void RemoveQuotes P_((char *string));
void StripSpecial P_((char *string));
int WriteGDE P_((NA_Alignment *aln,char *filename,int method,int maskable));
void ReadGDE P_((char *filename,NA_Alignment *dataset,int type));
void AdjustGroups(NA_Alignment *aln); /*from BuiltIn.c */

/* Genbank.cxx */
void ReadGen P_((char *filename, NA_Alignment *dataset, int type));
void AsciiTime P_((void *a, char *asciitime));
void SetTime P_((void *s));
int WriteGen P_((NA_Alignment *aln, char *filename, int method, int maskable));

#undef P_
extern Gmenu menu[GDEMAXMENU];
extern int num_menus;
extern GBDATA *gb_main;

/* global.h */
extern int DataType;
extern int FileFormat,first_select;
/*int Dirty,OldEditMode,EditMode = INSERT, EditDir = RIGHT;*/
extern int DisplayAttr,OVERWRITE;
extern int SCALE;
extern int BlockInput;
#ifdef SeeAlloc
int TotalCalloc = 0;
int TotalRealloc = 0;
#endif
extern char FileName[80];
extern char current_dir[1024];

/*
*	Months of the year
*/
extern const char *GDEmonth[12];
/*
*       Tables for DNA/RNA <--> ASCII translation
*/

extern int Default_RNA_Trans[];

extern int Default_DNA_Trans[];

extern int Default_NA_RTrans[];


/*
*	RGB values for the simple palette
*/


/*
*	Character->color lookup table
*/

extern int Default_NAColor_LKUP[];

extern int Default_PROColor_LKUP[];

extern const char *vert_mito[512];
extern const char *mycoplasma[512];
extern const char *universal[512];
extern const char *yeast[512];

extern const char *three_to_one[23];
#if 0

extern unsigned char grey0;
extern unsigned char grey1;
extern unsigned char grey2;
extern unsigned char grey3;
extern unsigned char grey4;
extern unsigned char grey5;
extern unsigned char grey6;
extern unsigned char grey7;

extern unsigned char **greys;
extern char **grey_pm; /*Pixmap instead of char !?!*/
#endif

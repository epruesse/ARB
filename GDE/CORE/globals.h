#define OWTOOLKIT_WARNING_DISABLED
#include <xview/cms.h>
int DataType;
int FileFormat,first_select = FALSE;
int Dirty,OldEditMode,EditMode = INSERT, EditDir = RIGHT;
int DisplayAttr = 0,OVERWRITE = FALSE;
int SCALE = 1;
int BlockInput = FALSE;
#ifdef SeeAlloc
int TotalCalloc = 0;
int TotalRealloc = 0;
#endif
char FileName[80];
char current_dir[1024];

/*
*	Months of the year
*/
char month[12][6] =
{
	"-JAN-","-FEB-","-MAR-","-APR-","-MAY-","-JUN-",
	"-JUL-","-AUG-","-SEP-","-OCT-","-NOV-","-DEC-"
};

/*
*       Tables for DNA/RNA <--> ASCII translation
*/

int Default_RNA_Trans[128] =
{
'-','A','C','M','G','R','S','V','U','W','Y','H','K','D','B','N',/*Upper*/
'.','a','c','m','g','r','s','v','u','w','y','h','k','d','b','n',/*lower*/
'-','A','C','M','G','R','S','V','U','W','Y','H','K','D','B','N',/*Upper select*/
'.','a','c','m','g','r','s','v','u','w','y','h','k','d','b','n',/*lwr select*/
'-','A','C','M','G','R','S','V','U','W','Y','H','K','D','B','N',/*extended*/
'~','a','c','m','g','r','s','v','u','w','y','h','k','d','b','n',/*extended*/
'-','A','C','M','G','R','S','V','U','W','Y','H','K','D','B','N',/*extended*/
'~','a','c','m','g','r','s','v','u','w','y','h','k','d','b','n',/*extended*/
};

int Default_DNA_Trans[128]={
'-','A','C','M','G','R','S','V','T','W','Y','H','K','D','B','N',/*Upper*/
'.','a','c','m','g','r','s','v','t','w','y','h','k','d','b','n',/*lower*/
'-','A','C','M','G','R','S','V','T','W','Y','H','K','D','B','N',/*Upper select*/
'.','a','c','m','g','r','s','v','t','w','y','h','k','d','b','n',/*lwr select*/
'-','A','C','M','G','R','S','V','T','W','Y','H','K','D','B','N',/*extended*/
'~','a','c','m','g','r','s','v','t','w','y','h','k','d','b','n',/*extended*/
'-','A','C','M','G','R','S','V','T','W','Y','H','K','D','B','N',/*extended*/
'~','a','c','m','g','r','s','v','t','w','y','h','k','d','b','n',/*extended*/
};

int Default_NA_RTrans[128]={
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x10,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
		/* Upper case alpha */
0x01,0xe,0x02,0x0d,0,0,0x04,0x0b,0,0,0x0c,0,0x03,0x0f,0,0,0,0x05,0x06,0x08,
0x08,0x07,0x09,0x0f,0xa,0,0,0,0,0,0,0,
		/* Lower case alpha */
0x11,0x1e,0x12,0x1d,0,0,0x14,0x1b,0,0,0x1c,0,0x13,0x1f,0,0,0,0x15,0x16,0x18,
0x18,0x17,0x19,0x1f,0x1a,0,0,0,0,0x10,0
};


/*
*	RGB values for the simple palette
*/

Xv_singlecolor Default_Colors[16]= {
	{0,128,0},
	{255,192,0},
	{255,0,255},
	{225,0,0},
	{0,192,192},
	{0,192,0},
	{0,0,255},
	{128,0,255},
	{0,0,0},
	{36,36,36},
	{72,72,72},
	{109,109,109},
	{145,145,145},
	{182,182,182},
	{218,218,218},
	{255,255,255}
};


/*
*	Character->color lookup table
*/

int Default_NAColor_LKUP[128] =
{
13,3,6,13,8,13,13,13,5,13,13,13,13,13,13,13,
13,3,6,13,8,13,13,13,5,13,13,13,13,13,13,13,
13,3,6,13,8,13,13,13,5,13,13,13,13,13,13,13,
13,3,6,13,8,13,13,13,5,13,13,13,13,13,13,13,
13,3,6,13,8,13,13,13,5,13,13,13,13,13,13,13,
13,3,6,13,8,13,13,13,5,13,13,13,13,13,13,13,
13,3,6,13,8,13,13,13,5,13,13,13,13,13,13,13,
13,3,6,13,8,13,13,13,5,13,13,13,13,13,13,13
};

int Default_PROColor_LKUP[128] =
{
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,2,8,3,8,8,6,2,4,5,12,4,5,5,8,12,2,
8,4,2,2,12,5,6,12,6,8,12,12,12,12,12,12,
2,8,3,8,8,6,2,4,5,12,4,5,5,8,12,2,
8,4,2,2,12,5,6,12,6,8,12,12,12,12,12
};

char vert_mito[512][4] =
{
"AAA","Lys", "AAC","Asn", "AAG","Lys", "AAT","Asn", "ACA","Thr",
"ACC","Thr", "ACG","Thr", "ACT","Thr", "AGA","Ter", "AGC","Ser",
"AGG","Ter", "AGT","Ser", "ATA","Met", "ATC","Ile", "ATG","Met",
"ATT","Ile", "CAA","Gln", "CAC","His", "CAG","Gln", "CAT","His",
"CCA","Pro", "CCC","Pro", "CCG","Pro", "CCT","Pro", "CGA","Arg",
"CGC","Arg", "CGG","Arg", "CGT","Arg", "CTA","Leu", "CTC","Leu",
"CTG","Leu", "CTT","Leu", "GAA","Glu", "GAC","Asp", "GAG","Glu",
"GAT","Asp", "GCA","Ala", "GCC","Ala", "GCG","Ala", "GCT","Ala",
"GGA","Gly", "GGC","Gly", "GGG","Gly", "GGT","Gly", "GTA","Val",
"GTC","Val", "GTG","Val", "GTT","Val", "TAA","Ter", "TAC","Tyr",
"TAG","Ter", "TAT","Tyr", "TCA","Ser", "TCC","Ser", "TCG","Ser",
"TCT","Ser", "TGA","Trp", "TGC","Cys", "TGG","Trp", "TGT","Cys",
"TTA","Leu", "TTC","Phe", "TTG","Leu", "TTT","Phe"
 },
mycoplasma[512][4] =
{
"AAA","Lys", "AAC","Asn", "AAG","Lys", "AAT","Asn", "ACA","Thr",
"ACC","Thr", "ACG","Thr", "ACT","Thr", "AGA","Arg", "AGC","Ser",
"AGG","Arg", "AGT","Ser", "ATA","Ile", "ATC","Ile", "ATG","Met",
"ATT","Ile", "CAA","Gln", "CAC","His", "CAG","Gln", "CAT","His",
"CCA","Pro", "CCC","Pro", "CCG","Pro", "CCT","Pro", "CGA","Arg",
"CGC","Arg", "CGG","Arg", "CGT","Arg", "CTA","Leu", "CTC","Leu",
"CTG","Leu", "CTT","Leu", "GAA","Glu", "GAC","Asp", "GAG","Glu",
"GAT","Asp", "GCA","Ala", "GCC","Ala", "GCG","Ala", "GCT","Ala",
"GGA","Gly", "GGC","Gly", "GGG","Gly", "GGT","Gly", "GTA","Val",
"GTC","Val", "GTG","Val", "GTT","Val", "TAA","Ter", "TAC","Tyr",
"TAG","Ter", "TAT","Tyr", "TCA","Ser", "TCC","Ser", "TCG","Ser",
"TCT","Ser", "TGA","Trp", "TGC","Cys", "TGG","Trp", "TGT","Cys",
"TTA","Leu", "TTC","Phe", "TTG","Leu", "TTT","Phe" },
universal[512][4] =
{
"AAA","Lys", "AAC","Asn", "AAG","Lys", "AAT","Asn", "ACA","Thr",
"ACC","Thr", "ACG","Thr", "ACT","Thr", "AGA","Arg", "AGC","Ser",
"AGG","Arg", "AGT","Ser", "ATA","Ile", "ATC","Ile", "ATG","Met",
"ATT","Ile", "CAA","Gln", "CAC","His", "CAG","Gln", "CAT","His",
"CCA","Pro", "CCC","Pro", "CCG","Pro", "CCT","Pro", "CGA","Arg",
"CGC","Arg", "CGG","Arg", "CGT","Arg", "CTA","Leu", "CTC","Leu",
"CTG","Leu", "CTT","Leu", "GAA","Glu", "GAC","Asp", "GAG","Glu",
"GAT","Asp", "GCA","Ala", "GCC","Ala", "GCG","Ala", "GCT","Ala",
"GGA","Gly", "GGC","Gly", "GGG","Gly", "GGT","Gly", "GTA","Val",
"GTC","Val", "GTG","Val", "GTT","Val", "TAA","Ter", "TAC","Tyr",
"TAG","Ter", "TAT","Tyr", "TCA","Ser", "TCC","Ser", "TCG","Ser",
"TCT","Ser", "TGA","Ter", "TGC","Cys", "TGG","Trp", "TGT","Cys",
"TTA","Leu", "TTC","Phe", "TTG","Leu", "TTT","Phe" },
yeast[512][4] =
{
"AAA","Lys", "AAC","Asn", "AAG","Lys", "AAT","Asn", "ACA","Thr",
"ACC","Thr", "ACG","Thr", "ACT","Thr", "AGA","Arg", "AGC","Ser",
"AGG","Arg", "AGT","Ser", "ATA","Met", "ATC","Ile", "ATG","Met",
"ATT","Ile", "CAA","Gln", "CAC","His", "CAG","Gln", "CAT","His",
"CCA","Pro", "CCC","Pro", "CCG","Pro", "CCT","Pro", "CGA","Arg",
"CGC","Arg", "CGG","Arg", "CGT","Arg", "CTA","Thr", "CTC","Thr",
"CTG","Thr", "CTT","Thr", "GAA","Glu", "GAC","Asp", "GAG","Glu",
"GAT","Asp", "GCA","Ala", "GCC","Ala", "GCG","Ala", "GCT","Ala",
"GGA","Gly", "GGC","Gly", "GGG","Gly", "GGT","Gly", "GTA","Val",
"GTC","Val", "GTG","Val", "GTT","Val", "TAA","Ter", "TAC","Tyr",
"TAG","Ter", "TAT","Tyr", "TCA","Ser", "TCC","Ser", "TCG","Ser",
"TCT","Ser", "TGA","Trp", "TGC","Cys", "TGG","Trp", "TGT","Cys",
"TTA","Leu", "TTC","Phe", "TTG","Leu", "TTT","Phe"
};


char three_to_one[23][5] = {
"AlaA", "ArgR", "AsnN", "AspD",
"AsxB", "CysC", "GlnQ", "GluE",
"GlxZ", "GlyG", "HisH", "IleI",
"LeuL", "LysK", "MetM", "PheF",
"ProP", "SerS", "ThrT", "TrpW",
"TyrY", "ValV", "Ter*"
};



static unsigned char grey0[] = {0,0,0,0,0,0,0,0};
static unsigned char grey1[] = {138,0,0,0,138,0,0,0};
static unsigned char grey2[] = {138,0,34,0,138,0,34,0};
static unsigned char grey3[] = {138,85,34,85,138,85,34,85};
static unsigned char grey4[] = {117,170,221,170,117,170,221,170};
static unsigned char grey5[] = {117,255,221,255,117,255,221,255};
static unsigned char grey6[] = {117,255,255,255,117,255,255,255};
static unsigned char grey7[] = {255,255,255,255,255,255,255,255};

unsigned char *greys[] = {grey1,grey3,grey3,grey1,grey2,grey3,grey0,grey3,
			grey0,grey1,grey2,grey3,grey4,grey5,grey6,grey7};
Pixmap *grey_pm[16];

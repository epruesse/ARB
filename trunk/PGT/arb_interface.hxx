// COPYRIGHT (C) 2004 - 2005 KAI BADER <BADERK@IN.TUM.DE>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// CVS REVISION TAG  --  $Revision$

#ifndef ARB_INTERFACE_H
#define ARB_INTERFACE_H

// UNCOMMENT THE APPROPRIATE DEFINE TO CONTROL THE ARB DEBUG MODE
// #define NDEBUG
#define DEBUG

#include <Xm/XmAll.h>
#include <arbdb.h>
#include <arbdbt.h>

// ARB AWARS USED BY PGT
#define AWAR_EXPERIMENT_NAME "tmp/exp/name"
#define AWAR_PROTEOM_NAME    "tmp/exp/proteom_name"
#define AWAR_PROTEIN_NAME    "tmp/exp/protein_name"
#define AWAR_CONFIG_CHANGED  "tmp/exp/config_changed"
#define AWAR_GENE_NAME       "tmp/gene/name"
#define AWAR_SPECIES_NAME    "tmp/focus/species_name"
#define AWAR_ORGANISM_NAME   "tmp/focus/organism_name"

// PATH TO THE PGT CONFIG FILE
#define PGT_CONFIG_FILE            ".arb_prop/pgt.arb"

// CONFIG DATABASE ENTRIES
#define CONFIG_PGT_COLOR_CROSSHAIR "colors/crosshair"
#define CONFIG_PGT_COLOR_UNMARKED  "colors/unmarked"
#define CONFIG_PGT_COLOR_MARKED    "colors/marked"
#define CONFIG_PGT_COLOR_SELECTED  "colors/selected"
#define CONFIG_PGT_COLOR_TEXT      "colors/text"
#define CONFIG_PGT_ID_PROTEIN      "id_protein"
#define CONFIG_PGT_ID_GENE         "id_gene"
#define CONFIG_PGT_INFO_PROTEIN    "info_protein"
#define CONFIG_PGT_INFO_GENE       "info_gene"

// DEFAULT CONFIG SETTINGS
#define DEFAULT_COLOR_CROSSHAIR "#FF0000"
#define DEFAULT_COLOR_UNMARKED  "#FFFF00"
#define DEFAULT_COLOR_MARKED    "#FF00FF"
#define DEFAULT_COLOR_SELECTED  "#00FFFF"
#define DEFAULT_COLOR_TEXT      "#000000"
#define DEFAULT_ID_PROTEIN      "name"
#define DEFAULT_ID_GENE         "locus_tag"
#define DEFAULT_INFO_PROTEIN    "name"
#define DEFAULT_INFO_GENE       "name"

int ARB_connect(char *);
int ARB_disconnect();

int CONFIG_connect();
int CONFIG_disconnect();

bool ARB_begin_transaction();
bool ARB_commit_transaction();

void ARB_dump(GBDATA *);
void ARB_dump_helper(GBDATA *, int);
bool ARB_connected();
GBDATA *get_gbData();
//
GBDATA *find_species(char *);
GBDATA *find_genome(char *);
GBDATA *find_genome(GBDATA *);
GBDATA *find_experiment(char *, char *);
GBDATA *find_experiment(GBDATA *, char *);
GBDATA *find_proteome(char *, char *, char *);
GBDATA *find_proteome(GBDATA *, char *);
//
void getSpeciesList(Widget, bool);
void getExperimentList(Widget, char *, bool);
void getProteomeList(Widget, char *, char *, bool);
void getEntryNamesList(Widget, bool);
//
extern void addLogEntry(char *, ...);
//
bool check_create_AWAR(GBDATA *, char *AWAR_path, bool);
void set_AWAR(char *AWAR_path, char *content);
char *get_AWAR(char *AWAR_path);
void set_CONFIG(char *CONFIG_path, char *content);
char *get_CONFIG(char *CONFIG_path);

void set_species_AWAR(char *content);
void set_experiment_AWAR(char *content);
void set_proteom_AWAR(char *content);
void set_protein_AWAR(char *content);
void set_gene_AWAR(char *content);
void set_config_AWAR(char *content);
//
char *get_species_AWAR();
char *get_experiment_AWAR();
char *get_proteom_AWAR();
char *get_protein_AWAR();
char *get_gene_AWAR();
char *get_config_AWAR();
//
void add_callback(char *, GB_CB, void *);
void add_species_callback(GB_CB, void *);
void add_experiment_callback(GB_CB, void *);
void add_proteom_callback(GB_CB, void *);
void add_protein_callback(GB_CB, void *);
void add_gene_callback(GB_CB, void *);
void add_config_callback(GB_CB, void *);
//
void checkCreateAWARS();


#endif // ARB_INTERFACE_H

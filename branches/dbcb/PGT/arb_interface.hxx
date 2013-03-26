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

#include <Xm/XmAll.h>
#include <arbdb.h>
#include <arbdbt.h>

#define pgt_assert(cond) arb_assert(cond)

// ARB AWARS USED BY PGT
#define AWAR_EXPERIMENT_NAME "tmp/exp/name"
#define AWAR_PROTEOM_NAME    "tmp/exp/proteom_name"
#define AWAR_PROTEIN_NAME    "tmp/exp/protein_name"
#define AWAR_CONFIG_CHANGED  "tmp/exp/config_changed"
#define AWAR_GENE_NAME       "tmp/gene/name"
#define AWAR_SPECIES_NAME    "tmp/focus/species_name"
#define AWAR_ORGANISM_NAME   "tmp/focus/organism_name"

// NAME OF THE PGT CONFIG FILE
#define PGT_CONFIG_NAME            "pgt.arb"

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

void CONFIG_connect();
void CONFIG_disconnect();

bool ARB_begin_transaction();
bool ARB_commit_transaction();

GBDATA *get_gbData();

GBDATA *find_genome(char *);
GBDATA *find_experiment(char *, char *);
GBDATA *find_proteome(char *, char *, char *);
GBDATA *find_proteome(GBDATA *, char *);
GBDATA *find_proteine_data(char *, char *, char *);

void getSpeciesList(Widget, bool);
void getExperimentList(Widget, char *, bool);
void getProteomeList(Widget, char *, char *, bool);
void getEntryNamesList(Widget, bool);

void set_CONFIG(const char *CONFIG_path, const char *content);
char *get_CONFIG(const char *CONFIG_path);

void set_species_AWAR(char *content);
void set_experiment_AWAR(char *content);
void set_proteom_AWAR(char *content);
void set_protein_AWAR(char *content);
void set_gene_AWAR(char *content);
void set_config_AWAR(char *content);

char *get_species_AWAR();
char *get_experiment_AWAR();
char *get_proteom_AWAR();
char *get_protein_AWAR();
char *get_gene_AWAR();

class mainDialog;
class imageDialog;

void add_mainDialog_callback (const char *awar, void(*cb)( GBDATA *, mainDialog *,  GB_CB_TYPE), mainDialog *md);
void del_mainDialog_callback (const char *awar, void(*cb)( GBDATA *, mainDialog *,  GB_CB_TYPE), mainDialog *md);
void add_imageDialog_callback(const char *awar, void(*cb)( GBDATA *, imageDialog *, GB_CB_TYPE), imageDialog *id);
void del_imageDialog_callback(const char *awar, void(*cb)( GBDATA *, imageDialog *, GB_CB_TYPE), imageDialog *id);

//
void checkCreateAWARS();


#endif // ARB_INTERFACE_H

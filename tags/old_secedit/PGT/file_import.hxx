// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$

#ifndef FILE_IMPORT_H
#define FILE_IMPORT_H

#include "arb_interface.hxx"
#include "file_import.hxx"


// Interessant wäre es vielleicht dies alles hier in eine Klasse zu verpacken
// Zusätzlich sollte vielleicht noch eine Funktion zur Analyse des Spaltentyps
// hinzugefügt werden. !?


#define DATATYPE_UNKNOWN 0
#define DATATYPE_INT     1
#define DATATYPE_FLOAT   2
#define DATATYPE_STRING  4


typedef struct IMPORTDATA
{
    char *species;      // SPECIES NAME
    char *experiment;   // EXPERIMENT NAME
    char *proteome;     // PROTEOME NAME
} importData;


typedef struct IMPORTTABLE
{
    int rows;           // TABLE ROWS
    int columns;        // TABLE COLUMNS
    char **cell;        // TABLE CELL ARRAY = TABLE ENTRIES
    char **header;      // TABLE HEADER DATA
    bool hasHeader;     // FIRST LINE = TABLE HEADER (TRUE/FALSE)
    int *columnType;    // ARB COLUMN TYPE (IF IDENTIFIABLE)
    bool hasTypes;      // IS COLUMN TYPE DATA AVAILABLE
} importTable;


typedef struct _XSLTimporter
{
    char *path;
    char **importer;
    int number;
} XSLTimporter;


importTable *fileopenCSV(char *, int);
int importCSV(importTable *, importData *);
importTable *createImportTable(int, int);
XSLTimporter *findXSLTFiles(char *);
int identifyType(char *);
void identifyColumns(importTable *);

#endif // FILE_IMPORT_H

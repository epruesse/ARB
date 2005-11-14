// Copyright (c) 2004 - 2005 Kai Bader <baderk@in.tum.de>
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
// AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// CVS REVISION TAG  --  $Revision$


/****************************************************************************
*  TODO
*
*  - THE CSV-IMPORT SHOULD DIFFER BETWEEN THE DATA TYPES
*  - CR/LF MUST BE REMOVED FROM THE LAST COLUMN
****************************************************************************/


#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
//
#include <sys/types.h>
#include <dirent.h>

using namespace std;

#include "file_import.hxx"


/****************************************************************************
*  IMPORT FUNCTION FOR A CSV DATA FILE (OPENS THE FILE AND RETURNS THE DATA)
****************************************************************************/
importTable *fileopenCSV(char *filename, int delimiter)
{
    // DEFINE VARIABLES
    int inconsistencies= 0;
    int rows=            0;
    int total_columns=   0;
    int local_columns=   0;
    int cell_index;
    int cell_size;
    char **cells;
    char *cell;
    char *start_ptr;
    char *end_ptr;

    // ALLOCATE MEM FOR IMPORTTABLE STRUCTURE
    importTable *table= NULL;

//     // ALLOCATE MEM FOR IMPORTTABLE STRUCTURE
//     importTable *table= (importTable *)malloc(sizeof(importTable));
//     if(!table) return NULL;

    // ALLOCATE MEMORY FOR READ BUFFER
    char *buffer= (char *)malloc(sizeof(char) * 10241);
    buffer[10240]=0;

    // DEFINE INPUT STREAM AND OPEN FILE
    ifstream iS;
    iS.open(filename);

    if(iS)
    {
        // PASS 1: GET CSV TABLE SIZE AND FIND INCONSISTENCIES

        while(iS.getline(buffer, 10240))
        {
            rows++;
            local_columns= 0;
            start_ptr= buffer;

            if(*start_ptr)
            {
                local_columns++;
            }

            while(*start_ptr && (end_ptr= strchr(start_ptr, delimiter)))
            {
                local_columns++;
                start_ptr= end_ptr + 1;
            }

            if((local_columns != total_columns) && (rows > 1))
            {
                inconsistencies++;

                printf("CSV-IMPORTER: found inconsistent data in row %d (%d instead of %d cells).\n",
                       rows, local_columns, total_columns);
            }
            else
            {
                total_columns= local_columns;
            }

        }

        // CHECK IF INCONSISTENCIES HAVE OCCURED AND ABORT IF NECESSARY
        if(inconsistencies)
        {
            iS.close();
            free(table);
            free(buffer);
            printf("CSV-IMPORTER: aborted due to inconsistent data (%d violations).\n", inconsistencies);
            return NULL;
        }

        // PASS 2: READ CSV-DATA FROM FILE INTO THE MEM-ARRAY

        // RESET FILE INPUT STREAM
        iS.clear();
        iS.seekg(0, ios::beg);



        // ALLOCATE MEMORY FOR THE TABLE DATA
        table= createImportTable(rows, total_columns);
        if(!table) return NULL;

//         cells= (char **)malloc(rows * total_columns * sizeof(char *));
//         if(!cells)
//         {
//             iS.close();
//             free(table);
//             free(buffer);
//             printf("CSV-IMPORTER: unable to allocate memory for the table data.\n");
//             return NULL;
//         }

        cells= table->cell;
//         table->rows= rows;
//         table->columns= total_columns;

        cell_index= 0;

        while(iS.getline(buffer, 10240))
        {
            start_ptr= buffer;

            while(*start_ptr && (end_ptr= strchr(start_ptr, delimiter)))
            {
                // REMOVE QUOTATION MARKS, IF NECESSARY
                if(*start_ptr == '"' && *(end_ptr-1) == '"' && (start_ptr < (end_ptr-1)))
                {
                    start_ptr++;
                    *(end_ptr-1)= 0;
                }

                cell_size= (int)(end_ptr - start_ptr) + 1;
                cell= (char *)malloc(cell_size * sizeof(char));

                *end_ptr= 0;
                strncpy(cell, start_ptr, cell_size);

                cells[cell_index]= cell;
                cell_index++;

                start_ptr= end_ptr + 1;
            }
            // if(*start_ptr)
            //{
                end_ptr= start_ptr;
                while((*end_ptr) && (*end_ptr != 0x0D) && (*end_ptr != 0x0A)) end_ptr++;

                cell_size= (int)(end_ptr - start_ptr) + 1;
                cell= (char *)malloc(cell_size * sizeof(char));

                *end_ptr= 0;
                strncpy(cell, start_ptr, cell_size);

                cells[cell_index]= cell;
                cell_index++;
            //}
        }
    }
    else
    {
        // SOMETHING WENT WRONG. FREE ALLOCATED MEM AND RETURN
        free(table);
        free(buffer);
        printf("CSV-IMPORTER: unable to open file: %s\n", filename);
        return NULL;
    }

    iS.close();
    free(buffer);

    // IDENTIFY THE COLUMNS BEST FITTING DATATYPES
    identifyColumns(table);

    printf("CSV-IMPORTER: successfully imported file: %s (rows: %d, columns: %d)\n", filename, rows, total_columns);
    return table;
}


/****************************************************************************
*  IMPORT FUNCTION FOR CSV DATA & HEADER (IMPORTS DIRECTLY INTO ARB)
****************************************************************************/
int importCSV(importTable *table, importData *data)
{
    // FETCH ARB DATABASE HANDLE
    GBDATA *gb_main= get_gbData();
    GBDATA *gb_experiment;
    GBDATA *gb_proteom, *gb_proteom_data, *gb_proteom_name;
    GBDATA *gb_protein, *gb_protein_data, *gb_protein_entry;
    char *head, *content;
    int rows= table->rows;
    int columns= table->columns;

//     GBDATA *gb_main= get_gbData();
//     GBDATA *gb_prot, *gb_exp, *gb_prot_data, *gb_prot_name;
//     GBDATA *gb_protein, *gb_protein_entry;
//     char *head, *content;
//     int rows= table->rows;
//     int columns= table->columns;
    // CHECK IF AN ARB CONNECTION IS GIVEN
    if(!gb_main)
    {
        printf("CSV import failed - no ARB connection.");
        return -1;
    }

    // FIND EXPERIMENT ENTRY
    gb_experiment= find_experiment(data->species, data->experiment);

    // IF SO, EXIT THE FILE IMPORT (EVERYTHING OTHER WOULD LEAD TO INCONSISTENT DATA)
    if(!gb_experiment)
    {
        printf("CSV import failed - the given species or experiment name could not be resolved.");
        return -2;
    }

    // IS THERE ALREADY A PROTEOME WITH THE NEW FILENAME?
    gb_proteom= find_proteome(gb_experiment, data->proteome);

    // IF SO, EXIT THE FILE IMPORT (EVERYTHING OTHER WOULD LEAD TO INCONSISTENT DATA)
    if(gb_proteom)
    {
        printf("CSV import failed - the given proteome name already exists (must be unique).");
        return -3;
    }

    // BEGIN ARB TRANSACTION
    ARB_begin_transaction();

    // ENTER EXPERIMENT DATA ENTRY
    gb_proteom_data= GB_find(gb_experiment, "proteome_data", 0, down_level);

    // IF THERE IS NO PROETOME_DATA ENTRY, CREATE A NEW ONE
    if(!gb_proteom_data) gb_proteom_data=
        GB_create_container(gb_experiment, "proteome_data");

    // CREATE NEW PROTEOME ENTRY
    gb_proteom= GB_create_container(gb_proteom_data, "proteome");

    // ADD THE NAME TO THE NEW PROTEOME ENTRY
    gb_proteom_name= GB_create(gb_proteom, "name", GB_STRING);
    GB_write_string(gb_proteom_name, data->proteome);

    // CREATE PROTEINE DATA ENTRY
    gb_protein_data= GB_create_container(gb_proteom, "proteine_data");

    // IMPORT CELL DATA
    for(int r= 1; r < rows; r++)
    {
        // EACH ROW REPRESENTS A PROTEIN -> NEW CONTAINER
        gb_protein= GB_create_container(gb_protein_data, "protein");

        // TRAVERSE COLUMNS FOR EACH ROW AND CREATE ENTRIES
        for(int c= 0; c < columns; c++)
        {
            // CLEAN HEADER FOR USAGE AS ARB KEY
            head= GBS_string_2_key(table->header[c]);

            // FETCH CONTENT FOR THE NEW ENTRY
            content= table->cell[(r * columns) + c];

            // CHECK IF WE HAVE AN ENTRY
            if(content)
            {
                // CHECK THE CORRECT COLUMN TYPE
                if(table->hasTypes)
                {
                    switch(table->columnType[c])
                    {
                        case DATATYPE_INT:
                            gb_protein_entry= GB_create(gb_protein, head, GB_INT);
                            if(gb_protein_entry) GB_write_int(gb_protein_entry, atol(content));
                            break;
                        case DATATYPE_FLOAT:
                            gb_protein_entry= GB_create(gb_protein, head, GB_FLOAT);
                            if(gb_protein_entry) GB_write_float(gb_protein_entry, atof(content));
                            break;
                        default:
                            gb_protein_entry= GB_create(gb_protein, head, GB_STRING);
                            if(gb_protein_entry) GB_write_string(gb_protein_entry, content);
                            break;
                    }
                }
                else
                {
                    gb_protein_entry= GB_create(gb_protein, head, GB_STRING);
                    GB_write_string(gb_protein_entry, content);
                }
            }
        }
    }

    // END ARB TRANSACTION
    ARB_commit_transaction();

    return 0;
}


/****************************************************************************
*  CREATE AN IMPORT TABLE
****************************************************************************/
importTable *createImportTable(int rows, int columns)
{
    // ALLOCATE MEMORY FOR THE TABLE DATA
    importTable *table= (importTable *)malloc(sizeof(importTable));
    if(!table) return NULL;

    // ALLOCATE MEMORY FOR THE CELL DATA
    char **cells= (char **)malloc(rows * columns * sizeof(char *));
    if(!cells) return NULL;

    // ALLOCATE MEMORY FOR THE HEADER DATA
    char **header= (char **)malloc(columns * sizeof(char *));
    if(!header) return NULL;

    // INIT ALL CELL VALUES WITH NULL
    int i;
    for(i= 0; i < (rows * columns); i++) cells[i]= NULL;
    for(i= 0; i < columns; i++) header[i]= NULL;

    // ALLOCATE MEMORY FOR THE COLUMN TYPE DATA
    int *columnType= (int *)malloc(columns * sizeof(int));
    if(!columnType) return NULL;

    // ENTER VALID PREDEFINED VALUES (SHOULD BE CHANGED LATER)
    table->rows= rows;
    table->columns= columns;
    table->cell= cells;
    table->header= header;
    table->hasHeader= false;
    table->columnType= columnType;

    // RETURN POINTER TO TABLE
    return table;
}


/****************************************************************************
*  FIND XSLT FILES (*.XSL) AS IMPORT FILTERS
****************************************************************************/
XSLTimporter *findXSLTFiles(char *path)
{
    DIR *dir;
    struct dirent *dir_entry;
    int count= 0;
    char *name;

    // *** FIRST RUN, COUNT ENTRIES...

    // EXIT, IF THE GIVEN PATH WAS INCORRECT
    if((dir= opendir(path)) == NULL) return NULL;

    // TRAVERSE ALL FILES...
    while((dir_entry= readdir(dir)) != NULL)
        if(strstr(dir_entry->d_name, ".xsl") || strstr(dir_entry->d_name, ".XSL"))
            count++;

    // CLOSE DIRECTORY HANDLE
    closedir(dir);

    // *** SECOND RUN, ALLOCATE SPACE AND READ ALL NAMES

    // CREATE NEW STRUCT
    XSLTimporter *xslt= (XSLTimporter *)malloc(sizeof(XSLTimporter));

    // CREATE PATH ENTRY
    xslt->path= (char *)malloc(sizeof(char) * (strlen(path) + 1));
    strcpy(xslt->path, path);

    // ALLOCATE MEM FOR NAME ARRAY POINTER
    xslt->importer= (char **)malloc(sizeof(char *) * count);

    // EXIT, IF THE GIVEN PATH WAS INCORRECT
    if((dir= opendir(path)) == NULL)
    {
        free(xslt->importer);
        free(xslt->path);
        free(xslt);

        return NULL;
    }

    // TRAVERSE ALL FILES...
    count= 0;
    while((dir_entry= readdir(dir)) != NULL)
    {
        if(strstr(dir_entry->d_name, ".xsl") || strstr(dir_entry->d_name, ".XSL"))
        {
            name= (char *)malloc(sizeof(char) * (strlen(dir_entry->d_name) + 1));
            strcpy(name, dir_entry->d_name);

            xslt->importer[count]= name;
            count++;
        }
    }

    xslt->number= count;

    return xslt;
}


/****************************************************************************
*  IDENTIFY ENTRY TYPE
*  THIS FUNCTION TRIES TO IDENTIFY THE TYPE OF AN ENTRY (STRING, NUMBER)
****************************************************************************/
int identifyType(char *entry)
{
    bool has_dot= false;
    bool has_numeric= true;
    char *ptr= entry;

    if(!ptr || (*ptr == 0)) return DATATYPE_UNKNOWN;

    while(*ptr)
    {
        if((*ptr == '.') || (*ptr == ',')) has_dot= true;
        else if((*ptr < '0') || (*ptr > '9')) has_numeric= false;

        ptr++;
    }

    if(has_dot && has_numeric) return DATATYPE_FLOAT;
    else if(has_numeric) return DATATYPE_INT;

    return DATATYPE_STRING;
}


/****************************************************************************
*  TRY TO IDENTIFY THE COLUMN TYPE USING THEIR ENTRIES
****************************************************************************/
void identifyColumns(importTable *table)
{
    // FUNCTION VARIABLES
    int rows= table->rows;
    int columns= table->columns;
    char **cell= table->cell;
    int *columnType= table->columnType;
    int c, r, colType, cellType;

    // TRAVERSE EVERY COLUMN
    for(c= 0; c < columns; c++)
    {
        colType= DATATYPE_UNKNOWN;

        // VIEW ALL ENTRIES AND IDENTIFY THE BEST FITTING TYPE
        for(r= 1; r < rows; r++)
        {
            // GET THE CELLS DATATYPE
            cellType= identifyType(cell[(r * columns) + c]);

            // CHANGE COLUMN TYPE IF A HIGHER DATATYPE IS FOUND
            if(cellType > colType) colType= cellType;

//            // DEBUG DEBUG DEBUG
//             if(cellType > colType)
//             {
//                 printf("COLUMN %d: ENTRY \'%s\' SWITCHED FROM %d TO %d\n",
//                        c, cell[(r * columns) + c], colType, cellType);
//                 colType= cellType;
//             }
//            // DEBUG DEBUG DEBUG
}

        columnType[c]= colType;
    }
}





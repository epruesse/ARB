/*****************************************************************************
*
* ca_mark (part of the chipanalyser project)
*
*     Kai Bader (baderk@in.tum.de)
*     Wolfgang Thomas (thomasw@in.tum.de)
*     2003 TU Munich
*****************************************************************************/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <fstream>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <PT_com.h>
#include <client.h>
#include <servercntrl.h>

#include "ca_mark.hxx"

using namespace std;


/****************************************************************************/

// just display the usage of this program
void print_usage(const char *name)
{
    printf("\nUsage: %s SPECIES_FILE\n\n"
	   "This program is used to (un)mark all listed species in the given\n"
	   "SPECIES_FILE and add comments to them. It is a link between the\n"
	   "chipanalyser and ARB. All entries in the SPECIES_FILE are of\n"
	   "this structure (divided by ':'):\n\n"
	   "\tSPECIES_SHORT_NAME:MARK_FLAG:COMMENT\n\n"
	   "\tSPECIES_SHORT_NAME == the name used by ARB to identify the species.\n"
	   "\tMARK_FLAG == boolean flag (0 or 1), species marking condition.\n"
	   "\tCOMMENT == comment, may contain up to 128 chars\n\n", name);
}


// all the file reading and parsing, also the highlighting of the species
// is done here
int parse_species_file(char *filename)
{
    // first, try to open a connection to ARB
    GBDATA *gb_main = GB_open(":","r");
    if (!gb_main)
    {
	printf("error: you must have a running arbdb server.\n");
	return -1;
    }

    GB_begin_transaction(gb_main);

    char buffer[256];
    buffer[255]= 0; // last byte NULL, avoids overflows

    char *name, *flag, *comment; // pointers to the substrings in the buffer

    ifstream iS;
    iS.open(filename);

    if(iS) // file.open succeeded
    {
       while(iS.getline(buffer, 255)) // while there is unread data, do...
        {
	    //buffer[0]= 0;

	    // read a line to:  char *buffer
	    //iS.getline(buffer, 255);

	    // the following lines split the buffer into 3 substrings by
	    // exchanging the delimiter ':' with NULL.

	    name= buffer; // first entry equals name
	    flag= 0;
	    comment= 0;

	    if(name)
	    {
		flag= strchr(buffer, ':'); // find dividing caracter

		if(flag)
		{
		    flag[0]= 0;
		    flag++; // set position of our flat string

		    comment= strchr(flag, ':'); // find next entry

		    if(comment)
		    {
			comment[0]= 0;
			comment++; // set position of comment string
		    }
		}
	    }

	    //
	    if(name && flag) // if the parsing worked, do...
	    {

		GBDATA *species = GB_find(GBT_get_species_data(gb_main), "species", 0, down_level);

		while(species)
		{
		    GBDATA *gb_name = GB_find(species, "name", 0, down_level);
		    char *species_name = GB_read_string(gb_name);

		    if(!strcmp(species_name, name))
		    {
			if (flag[0] == '1')
			{
			    GB_write_flag(species, 1);

			    if(comment[0])
			    {
				GBDATA *ca_comment= GB_find(species, "ca_comment", 0, down_level);
				if(!ca_comment) ca_comment= GB_create(species,"ca_comment",GB_STRING);
			        GB_write_string(ca_comment, comment);
			    }
			}
			else
			{
			    GB_write_flag(species, 0);

			    if(comment[0])
			    {
				GBDATA *ca_comment= GB_find(species, "ca_comment", 0, down_level);
				if(!ca_comment) ca_comment= GB_create(species,"ca_comment",GB_STRING);
			        GB_write_string(ca_comment, comment);
			    }
			}
		    }
		    species = GB_find(species, "species", 0, this_level|search_next);
		}
	    }
	}
    }
    else // failed to open file
    {
	printf("error: unable to open file: %s\n", filename);
	print_usage("ca_mark");
	return -1;
    }

    iS.close();

    GB_commit_transaction(gb_main);
    GB_close(gb_main);

    return 0;
}


int main(int argc,char **argv)
{
    // check, if the correct number of args (one) is given
    if(argc!=2)
    {
	print_usage(argv[0]);
	return -1;
    }

    // check, if the given arg is something like '--help' or '-h'
    if(argv[1][0] == '-')
    {
	print_usage(argv[0]);
	return -1;
    }

    // call return parse_species_file here...
    int retval= parse_species_file(argv[1]);

    return retval;
}

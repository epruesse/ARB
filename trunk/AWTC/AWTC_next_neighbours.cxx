#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#include <arbdb.h>
#include <arbdbt.h>


#include <servercntrl.h>
#include <PT_com.h>
#include <client.h>

#include <awtc_next_neighbours.hxx>


AWTC_FIND_FAMILY_MEMBER::AWTC_FIND_FAMILY_MEMBER(){
    matches = 0;
    name = 0;
    next = 0;
}

AWTC_FIND_FAMILY_MEMBER::~AWTC_FIND_FAMILY_MEMBER(){
    free(name);
}

void awtc_ff_message(const char *msg){
    GB_warning("%s",msg);
}

void AWTC_FIND_FAMILY::delete_family_list(){
    AWTC_FIND_FAMILY_MEMBER *fl,*fln;
    for (fl = family_list; fl ; fl = fln){
	 fln = fl->next;
	 delete fl;
    }
    family_list = 0;
}

GB_ERROR AWTC_FIND_FAMILY::init_communication(void)
{
    const char *user = "Find Family";

/*** create and init local com structure ***/
    if( aisc_create(link, PT_MAIN, com,
		    MAIN_LOCS, PT_LOCS, &locs,
		    LOCS_USER, user,
		    NULL)){
	return GB_export_error("Cannot initialize communication");
    }
    return 0;
}


GB_ERROR AWTC_FIND_FAMILY::open(char *servername)
{
   char *socketid;

   if (arb_look_and_start_server(AISC_MAGIC_NUMBER,servername,gb_main)){
      return GB_export_error ("Cannot contact PT  server");
   }

   socketid = GBS_read_arb_tcp(servername);

   if (!socketid) {
      return GB_export_error ("Cannot find entry '%s' in $ARBHOME/arb_tcp.dat",servername);
   }

   link = (aisc_com *)aisc_open(socketid,&com,AISC_MAGIC_NUMBER);

   if (!link) {
      return GB_export_error ("Cannot contact Probe bank server ");
   }

   if (init_communication()) {
      return GB_export_error  ("Cannot contact Probe bank server (2)");
   }

   free(socketid);
   return 0;
}

void AWTC_FIND_FAMILY::close(void)
{
   if (link) aisc_close(link);
	link = 0;
}

AWTC_FIND_FAMILY::AWTC_FIND_FAMILY(GBDATA *gb_maini)
{
    memset((char *)this,0,sizeof(*this));
    this->gb_main = gb_maini;
}

AWTC_FIND_FAMILY::~AWTC_FIND_FAMILY(void)
{
    delete_family_list();
    close();
}

GB_ERROR AWTC_FIND_FAMILY::find_family(char *sequence, int find_type, int max_hits)
{
    T_PT_FAMILYLIST f_list;
    char *compressed_sequence = GB_command_interpreter(gb_main, sequence, "|keep(acgtunACGTUN)", 0, 0);

    bytestring bs;
    bs.data = compressed_sequence;
    bs.size = strlen(bs.data)+1;

    delete_family_list();
    /*
     * Start find_family() at the PT_SERVER
     *
     * Here we have to make a loop, until the match count of the
     * first member is big enough
     *
     */

    if (aisc_put(link, PT_LOCS, locs,
		 LOCS_FIND_TYPE, find_type,
		 LOCS_FIND_FAMILY, &bs,0)){
	return GB_export_error  ("Communication Error (2)");
    }

    /*
     * Read family list
     */
    aisc_get(link,PT_LOCS, locs,LOCS_FAMILY_LIST, &f_list, 0);

    while (f_list){
	if (max_hits== 0) break;
	max_hits--;
	AWTC_FIND_FAMILY_MEMBER *fl = new AWTC_FIND_FAMILY_MEMBER();
	fl->next = family_list;
	family_list = fl;

	aisc_get(link, PT_FAMILYLIST, f_list,
		 FAMILYLIST_NAME,&fl->name,
		 FAMILYLIST_MATCHES,&fl->matches,
		 FAMILYLIST_NEXT, &f_list,0);
    }

    free(compressed_sequence);

    return 0;
}

void AWTC_FIND_FAMILY::print(){
    AWTC_FIND_FAMILY_MEMBER *fl;
    for (fl = family_list; fl ; fl = fl->next){
	printf("%s %li\n",fl->name,fl->matches);
    }
}

GB_ERROR AWTC_FIND_FAMILY::go(int server_id,char *sequence,GB_BOOL fast,int max_hits){
    char buffer[256];
    sprintf(buffer,"ARB_PT_SERVER%i",server_id);
    GB_ERROR error;
    error = this->open(buffer);
    if (error) return error;
    error = this->init_communication();
    if (error) return error;
    error = find_family(sequence,fast,max_hits);
    if (error){
	close();
	return error;
    }
    close();
    return 0;
}

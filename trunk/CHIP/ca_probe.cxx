#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <fstream>
#include <vector>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <aw_root.hxx>
#include <PT_com.h>
#include <client.h>
#include <servercntrl.h>

#include "ca_probe.hxx"


#define MAX_SPECIES 999999 // max species returned by pt-server (probe match)
using namespace std;


struct _probe_match_result
{
	char name[16];
	char fullname[255];
	int mis;
	int nmis;
	float wmis;
	int pos;
	int ecoli;
	int rev;
	char string[255];
} probe_match_result;
//

#warning Hallo Kai bitte verwende ../PROBE_DESIGN/probe_match_parser.hxx zum Parsen der Probe-Match-Ausgabe

#if 0
static void my_print(const char *, ...) {
    chip_assert(0); // CHIP_init_pt_server should install another handler
}
#endif
// ================================================================================
// KAI  -  added 30.04.2003

void correctIllegalChars(char *str)
{
    char *s= str;
    while(*s)
    {
	if((*s < 32) || (*s > 126)) *s= '?';
	s++;
    }
}


// ================================================================================

static bool  server_initialized  = false;;
static char *current_server_name = 0;

#if 0
static void 	 (*print)(const char *format, ...) = my_print;
#endif

static GB_ERROR connection_lost      = "connection to pt-server lost";
static GB_ERROR cant_contact_unknown = "can't contact pt-server (unknown reason)";
static GB_ERROR cant_contact_refused = "can't contact pt-server (connection refused)";

static GB_ERROR CHIP_init_pt_server_not_called 	 = "programmers error: pt-server not contacted (CHIP_init_pt_server not called)";
// static GB_ERROR CHIP_init_find_probes_called_twice = "programmers error: CHIP_init_find_probes called twice";

// Hier werden die eingelesenen Daten des input-probefiles gehalten
vector<probe_data> probeData;
FILE *pFile;

using namespace std;


// ================================================================================

//  -------------------------
//      struct gl_struct
//  -------------------------
struct gl_struct {
    aisc_com  *link;
    T_PT_LOCS  locs;
    T_PT_MAIN  com;
};



// ================================================================================

//  -----------------------------------------------------------------
//      static int init_local_com_struct(struct gl_struct& pd_gl)
//  -----------------------------------------------------------------
static int init_local_com_struct(struct gl_struct& pd_gl)
{
    const char *user;
    if (!(user = getenv("USER"))) user = "unknown user";

    /* @@@ use finger, date and whoami */
    if( aisc_create(pd_gl.link, PT_MAIN, pd_gl.com,
                    MAIN_LOCS, PT_LOCS, &pd_gl.locs,
                    LOCS_USER, user,
                    NULL)){
        return 1;
    }
    return 0;
}

void error_msg(char **argv){
    printf("syntax: %s [-consense #ofTrees] tree_name treefile [comment]\n",argv[0]);
    exit(-1);
}

void show_error(GBDATA *gb_main) {
    GBT_message(gb_main, GB_get_error());
    GB_print_error();
}


// -------------------------------------------------
//      double max_branchlength(GBT_TREE *node)
// -------------------------------------------------
double max_branchlength(GBT_TREE *node) {
    if (node->is_leaf) return 0.0;

    double max_val                      = node->leftlen;
    if (node->rightlen>max_val) max_val = node->rightlen;

    double left_max  = max_branchlength(node->leftson);
    double right_max = max_branchlength(node->rightson);

    if (left_max>max_val) max_val = left_max;
    if (right_max>max_val) max_val = right_max;

    return max_val;
}


// ---------------------------------------
//      int main(int argc,char **argv)
// ---------------------------------------
int main(int argc,char **argv)
{
  GB_ERROR error;
   error = 0;
  int args_err= 0;
  int args_help= 0;
  char *arg_input_file= NULL;
  char *arg_result_file= NULL;
  char *arg_ptserver= NULL;
  char *arg_numMismatches= NULL;
  int numMismatches= 0;
  int weightedMismatches= 0;
  float maxWeightedMismatches= -1;


  if (argc < 4)
    args_help = 1;

  for(int c= 1; c < argc; c++) {
    if((argv[c][0]== '-') && (argv[c][1]!= '-')) {
      switch(argv[c][1]) {
      case 'w':
	weightedMismatches= 1; // 0= do not use weighted, 1= use weighted mm.
	if((c+1) < argc)
	{
		c++;
		maxWeightedMismatches= atof(argv[c]);
	}
	break;
      case 'h':
	args_help= 1;
	break;
      default:
	args_err= 1;
	break;
      }
    }
  }

   if(!args_help && !args_err && argc>3)
     {
     if( argv[1][0]!='-' && argv[2][0]!='-' && argv[3][0]!='-')
     {
        arg_input_file = argv[1];
        arg_result_file = argv[2];
        arg_ptserver = argv[3];
      }
    else
      args_err=1;
     if (argc>4)
       {
	 arg_numMismatches = argv[4];
         numMismatches = atoi(arg_numMismatches);
       }
     else
       numMismatches = 0;
     }



  if (args_help && !args_err)
    {
      printf("Usage: %s input.c_p result.c_p 'pt-path pt-server' [numberOfMismatches] [Options]\n\n", argv[0]);
      printf("Example: %s probelist_input.txt probelist_result.txt 'localhost: user7.arb' 0 -w 0.3\n\n", argv[0]);
      printf("The default for numberOfMismatches is 0.\n");
      printf("-w #\tuse weighted mismatches for the probematch (# = upper limit, if set).\n");
      printf("-h\tprint this help and exit.\n\n");
      return -1;
    }

  if (args_err)
    {
      printf("%s: Error while processing input. Try %s -h for help.\n", argv[0], argv[0]);
      return -1;
    }


  //  printf("vor einlesen\n");
  // Einlesen des input-probefiles
     error = read_input_file(arg_input_file);
         if (error)
	   {
	     printf(error);
	     return -1;
	   }

	 //	 printf("nach einlesen \n");
  // Ist ein pt-server gestartet?
	GBDATA *gb_main = GB_open(":","r");
	//printf("nach GB_open\n");
	if (!gb_main){
		printf("%s: Error: you have to start an arbdb server first\n", argv[0]);
		return -1;
	}


 // Versuche, den pt-server zu initialisieren
      error =  CHIP_init_pt_server(gb_main, arg_ptserver);
         if (error)
	   {
	     printf("%s\n", error);
	     return -1;
	   }
	 // printf("nach init_pt_server \n");
      GBT_message(gb_main, "Reading tree...");

	GB_begin_transaction(gb_main);

	struct CHIP_probe_match_para my_para;
	my_para.split = 0.5;
	my_para.dtedge = 0.5;
	my_para.dt = 0.5;
	my_para.bondval[0] = 0.0;
	my_para.bondval[1] = 0.0;
	my_para.bondval[2] = 0.5;
	my_para.bondval[3] = 1.1;
	my_para.bondval[4] = 0.0;
	my_para.bondval[5] = 0.0;
	my_para.bondval[6] = 1.5;
	my_para.bondval[7] = 0.0;
	my_para.bondval[8] = 0.5;
	my_para.bondval[9] = 1.5;
	my_para.bondval[10] = 0.4;
	my_para.bondval[11] = 0.9;
	my_para.bondval[12] = 1.1;
	my_para.bondval[13] = 0.0;
	my_para.bondval[14] = 0.9;
	my_para.bondval[15] = 0.0;

// 	my_para.bondval = {
// 		0.0, 0.0, 0.5, 1.1,
// 		0.0, 0.0, 1.5, 0.0,
// 		0.5, 1.5, 0.4, 0.9,
// 		1.1, 0.0, 0.9, 0.0 };

	pFile = fopen(arg_result_file, "w");
	if (pFile!=NULL)
	  {
	    fputs("# Probe File for Chipanalyser\n# This File was created by ca_probe (arb probe-match)\n# Number of Mismatches considered: ", pFile);
	    fputs(arg_numMismatches, pFile);
	    fputs("\n\n", pFile);
	    fclose(pFile);
	  }
	else
	  error = "Error: could not open reult file for writing.\n";
	if (error)
	  {
		  printf(error);
		  return -1;
	  }

	// Gehe probeData durch, und rufe fuer jeden Eintrag CHIP_probe_match auf
	for (unsigned int j=0; j<probeData.size(); j++)
	{
	  //printf("j=%d and probename=%s\n", j, probeData[j].name);
	  error =  CHIP_probe_match(probeData[j] , my_para,  arg_result_file, numMismatches, weightedMismatches, maxWeightedMismatches);
	}

	if (error)
	  {
		  printf(error);
		  return -1;
	  }

	GB_commit_transaction(gb_main);

	GB_close(gb_main);
	printf("ok\n");
	return 0;


}
//   ------------------------------------
//   GB_ERROR read_input_file(char *fn)
//   ------------------------------------

GB_ERROR read_input_file(char *fn)
{
    GB_ERROR 	error = 0;

    char line[256];
    line[255]= 0; // no segfault

    char tmpname[255];
    char tmplongname[255];
    char tmpsequence[255];
    ifstream iS;
    bool gotProbe = false;

    iS.open(fn);
    if(iS)
    {
       while(iS.getline(line, 255))
       {
	 if (line[0]=='#')
	   {
	     //do nothing
	   }
	 else if(strstr(line,"probe")  && !strstr(line, "name")) {
	     tmpname[0] = 0;
	     tmplongname[0] = 0;
	     tmpsequence[0] = 0;
	     gotProbe = true;
	}

	else if(strstr(line, "name") && !strstr(line, "longname"))
	{
       	  char *tmpstr = strstr(line,"name");
	  int i = 0;
	  int buf_count = 0;
	  char buf[255];
	  bool useIt = 0;
	  bool gotWord = 0;
	  while(tmpstr[i] && i<255)
	  {
	    if (!useIt && tmpstr[i] =='=')
	    {
	      useIt = 1;
	      i++;
	    }
	    if (useIt)
	    {
	      while(tmpstr[i]==' ')
	      {
		if (gotWord)
		{
		  buf[buf_count] = tmpstr[i];
		  i++;
		  buf_count++;
		}
		else
		 i++;
	      }
	      while(tmpstr[i]!=' ' && tmpstr[i]!='\t' && tmpstr[i])
	      {
		gotWord=1;
	        buf[buf_count] = tmpstr[i];
	        i++;
	        buf_count++;
	      }
	    }
	    else
	    {
	      i++;
	    }
	  }
          buf[buf_count]=0;

	  if (strlen(buf)>0) {
	    strcpy(tmpname, buf);
	  }
	} // end name

	else if(strstr(line, "longname"))
	{
	  int i = 0;
	  int buf_count = 0;
	  char buf[255];
	  bool useIt = 0;
	  bool gotWord = 0;
	  char *tmpstr = strstr(line, "longname");
	  while(tmpstr[i] && i<255)
	  {
	    if (!useIt && tmpstr[i] =='=')
	    {
	      useIt = 1;
	      i++;
	    }
	    if (useIt)
	    {
  	      while(tmpstr[i]==' ')
	      {
		if (gotWord)
		{
		  buf[buf_count] = tmpstr[i];
		  i++;
		  buf_count++;
		}
		else
		  i++;
	      }

	      while(tmpstr[i]!=' ' && tmpstr[i]!='\t' && tmpstr[i])
	      {
		gotWord=1;
	        buf[buf_count] = tmpstr[i];
	        i++;
	        buf_count++;
	      }
	    }
	    else
	    {
	     i++;
	    }

	  }
	     buf[buf_count]=0;
	  if (strlen(buf)>0)
	    strcpy(tmplongname, buf);
	} // end longname

	else if(char *tmpstr = strstr(line, "sequence"))
	{
	  int i = 0;
	  int buf_count = 0;
	  char buf[255];
	  bool useIt = 0;
	  bool gotWord = 0;
	  while(tmpstr[i] && i<255)
	  {
	    if (!useIt && tmpstr[i] =='=')
	    {
	      useIt = 1;
	      i++;
	    }
	    if (useIt)
	    {
	       while(tmpstr[i]==' ')
	       {
		 if (gotWord)
		 {
		   buf[buf_count] = tmpstr[i];
		   i++;
		   buf_count++;
		 }
		 else
	      	   i++;
	       }
	       while(tmpstr[i]!=' ' && tmpstr[i]!='\t' && tmpstr[i])
	      {
		gotWord=1;
	        buf[buf_count] = tmpstr[i];
	        i++;
	        buf_count++;
	      }
	    }

	    else
	    {
	      i++;
	    }
	  }
	   buf[buf_count]=0;
	  if (strlen(buf)>0)
	    strcpy(tmpsequence, buf);
	} // end sequence

	else if(!line[0])
	  {
	    if(gotProbe)
	    {
	      probe_data pD;
	      strcpy(pD.name, tmpname);
	      strcpy(pD.longname, tmplongname);
	      strcpy(pD.sequence, tmpsequence);
	      probeData.push_back(pD);
	      gotProbe = false;
	    }
   	  }

        } // end while(!eof)

       // last entry:
       if(gotProbe)
       {
         probe_data pD;
         strcpy(pD.name, tmpname);
         strcpy(pD.longname, tmplongname);
         strcpy(pD.sequence, tmpsequence);
         probeData.push_back(pD);
       }



      iS.close();
    }
    else
      error = ("Could not open input-file.\n");


    return error;
}



//  ---------------------------------------------
//      static char *pd_ptid_to_choice(int i)
//  ---------------------------------------------
static char *pd_ptid_to_choice(int i){
    char search_for[256];
    char choice[256];
    char	*fr;
    char *file;
    char *server;
    char empty[] = "";
    sprintf(search_for,"ARB_PT_SERVER%i",i);

    server = GBS_read_arb_tcp(search_for);
    if (!server) return 0;
    fr = server;
    file = server;				/* i got the machine name of the server */
    if (*file) file += strlen(file)+1;	/* now i got the command string */
    if (*file) file += strlen(file)+1;	/* now i got the file */
    if (strrchr(file,'/')) file = strrchr(file,'/')-1;
    if (!(server = strtok(server,":"))) server = empty;
    sprintf(choice,"%-8s: %s",server,file+2);
    delete fr;


    return strdup(choice);

    //    return error;

}

//  --------------------------------------------------------------------------------------
//      static char *probe_pt_look_for_server(const char *servername, GB_ERROR& error)
//  --------------------------------------------------------------------------------------
static char *probe_pt_look_for_server(GBDATA *gb_main, const char *servername, GB_ERROR& error)
{
    int serverid = -1;

    for (int i=0;i<1000; ++i) {
      // Folgende Zeile gibt beim Kompilieren (auf der Trance) Fehlermeldung
      //  char *aServer = GBS_ptserver_id_to_choice(i);
      // deshalb wird weiterhin alte Fassung verwendet:
        char *aServer = pd_ptid_to_choice(i);
        if (aServer) {
	  //printf("server='%s'\n",aServer);
            if (strcmp(aServer, servername)==0) {
                serverid = i;
		free(aServer);
                break;
            }
	    free(aServer);
        }
    }
    if (serverid==-1) {
        error = GB_export_error("'%s' is not a known pt-server.", servername);
        return 0;
    }
    char choice[256];
    sprintf(choice, "ARB_PT_SERVER%li", (long)serverid);
    error = arb_look_and_start_server(AISC_MAGIC_NUMBER, choice, gb_main);
    if (error) {
        printf("error: cannot start pt-server (%s)", error);
        return 0;
    }
    return GBS_read_arb_tcp(choice);
}


// ================================================================================

// --------------------------------------------------------------------------------
//     class PT_server_connection
// --------------------------------------------------------------------------------
class PT_server_connection {
private:
    T_PT_PDC  pdc;
    GB_ERROR error;
    struct gl_struct my_pd_gl;

public:
    PT_server_connection() {
        error = 0;
        memset(&my_pd_gl, 0, sizeof(my_pd_gl));

	if (!server_initialized) {
	    error = CHIP_init_pt_server_not_called;
	}
	else {
	    my_pd_gl.link = (aisc_com *)aisc_open(current_server_name, &my_pd_gl.com,AISC_MAGIC_NUMBER);

	    if (!my_pd_gl.link) 			error = cant_contact_unknown;
	    else if (init_local_com_struct(my_pd_gl)) 	error = cant_contact_refused;
	    else {
		aisc_create(my_pd_gl.link,PT_LOCS, my_pd_gl.locs,
			    LOCS_PROBE_DESIGN_CONFIG, PT_PDC, &pdc,
			    0);
	    }
	}
    }
    virtual ~PT_server_connection() {
        if (my_pd_gl.link) aisc_close(my_pd_gl.link);
        delete error;
    }

    struct gl_struct& get_pd_gl() { return my_pd_gl; }
    GB_ERROR get_error() const { return error; }
    T_PT_PDC& get_pdc() { return pdc; }
};



// ================================================================================

//  -----------------------------------------------------------------------------------------------------------------------------
//      GB_ERROR CHIP_init_pt_server(GBDATA *gb_main, const char *servername, void (*print_function)(const char *format, ...))
//  -----------------------------------------------------------------------------------------------------------------------------
GB_ERROR CHIP_init_pt_server(GBDATA *gb_main, const char *servername) {
    if (server_initialized) return "pt-server is already initialized";

    GB_ERROR 	error = 0;

   current_server_name = (char *)probe_pt_look_for_server(gb_main, servername, error);
    // chip_assert(error || current_server_name);
    server_initialized 	= true;;
    return error;
}

//  -------------------------------------
//      void CHIP_exit_pt_server(void)
//  -------------------------------------
void CHIP_exit_pt_server(void) {
    free(current_server_name);
    current_server_name = 0;
    server_initialized 	= false;
}



// ================================================================================

//  ----------------------------------------------------------------------------------------------------------------
//      static bool chip_init_probe_match(T_PT_PDC pdc, struct gl_struct& pd_gl, const CHIP_probe_match_para& para)
//  ----------------------------------------------------------------------------------------------------------------
static bool chip_init_probe_match(T_PT_PDC pdc, struct gl_struct& pd_gl, const CHIP_probe_match_para& para) {
    int 	i;
    char 	buffer[256];

    if (aisc_put(pd_gl.link, PT_PDC, pdc,
                 PDC_DTEDGE, 		para.dtedge,
                 PDC_DT, 		para.dt,
                 PDC_SPLIT, 		para.split,
                 0)) return false;

    for (i=0;i<16;i++) {
        sprintf(buffer,"probe_design/bonds/pos%i",i);
        if (aisc_put(pd_gl.link,PT_PDC, pdc,
                     PT_INDEX,		i,
                     PDC_BONDVAL, 	para.bondval[i],
                     0) ) return false;
    }

    return true;
}


char *skipToNextWord(char *ptr)
{
	while(*ptr && (*ptr != ' ')) ptr++;
	while(*ptr && (*ptr == ' ')) ptr++;
	return ptr;
}


void cleanEndingSpaces(char *ptr)
{
	while(*ptr)ptr++;
	while(*(--ptr) == ' ');
	*(++ptr)= 0;
}


//  -----------------------------------------------------------------------------------------------------
//      GB_ERROR CHIP_probe_match(probe_data &pD, const CHIP_probe_match_para& para,  char *fn, int numMismatches)
//  -----------------------------------------------------------------------------------------------------
//  calls probe-match for the sequence contained in pD  and appends all matching species to the result-probefile fn

GB_ERROR CHIP_probe_match(probe_data &pD, const CHIP_probe_match_para& para,  char *fn, int numMismatches, int weightedMismatches, float maxWeightedMismatches)
{

  //  printf("CHIP_probe_match\n");

  static PT_server_connection *my_server = 0;
    GB_ERROR 			 error 	   = 0;

    if (!my_server) {
      // GB_ERROR tmp_error =  CHIP_init_pt_server(gb_main, "localhost: SSU_rRNA.arb");
        my_server = new PT_server_connection();
	//printf("new PT-connection\n");
        error 	  = my_server->get_error();
	if (error)
	  printf("Error: %s\n", error);

        if (!error &&
            !chip_init_probe_match(my_server->get_pdc(), my_server->get_pd_gl(), para)) {
            error = connection_lost;
	    printf("connection lost\n");
        }

        if (error) {
            delete my_server;
            my_server = 0;
            return error;
        }
    }

    //     char 		*match_info, *match_name;
    //     GBDATA 		*gb_species_data = 0;
    //     GBDATA 		*gb_species;
    //     int 		 show_status 	 = 0;
    //     GBT_TREE* 		 p	 = 0;
//     T_PT_PDC& 		 pdc   = my_server->get_pdc();

    struct gl_struct& 	 pd_gl = my_server->get_pd_gl();

    // @@@ soll eigentlich auch reverse-complement gesucht werden??
    // printf("sequenz: %s\n", pD.sequence);

    if (aisc_nput(pd_gl.link,
		  PT_LOCS, 			pd_gl.locs,
                  LOCS_MATCH_REVERSED,		1,
                  LOCS_MATCH_SORT_BY,		weightedMismatches,
                  LOCS_MATCH_COMPLEMENT, 	0,
                  LOCS_MATCH_MAX_MISMATCHES,	numMismatches,
                  LOCS_MATCH_MAX_SPECIES, 	MAX_SPECIES,
                  LOCS_SEARCHMATCH,		pD.sequence,
                  0))
    {
        error = connection_lost;
    }
    if (!error) {
        // read_results:
        T_PT_MATCHLIST 	 match_list;
        long 		 match_list_cnt;
        char		*locs_error = 0;
        bytestring 	 bs;

        bs.data = 0;

        if (aisc_get( pd_gl.link, PT_LOCS, pd_gl.locs,
                      LOCS_MATCH_LIST,		&match_list,
                      LOCS_MATCH_LIST_CNT,	&match_list_cnt,
                      LOCS_MATCH_STRING,	&bs,
                      LOCS_ERROR,		&locs_error,
                      0))
        {
            error = connection_lost;
        }

        if (!error && locs_error && locs_error[0]) {
            static char *err;

            if (err) free(err);
            err   = locs_error;
            error = err;
        }

        if (!error) {
	  // Oeffne result-probefile im append-modus
	   pFile = fopen(fn, "a");
	 if (pFile!=NULL)

	  {
	    char tmp[256];

	    fputs("\nprobe\n", pFile);

	    char probe_name[255];
	    strcpy(probe_name, "\tname= ");
	    strcpy(tmp, pD.name);
	    correctIllegalChars(tmp);
	    strcat(probe_name, tmp);
	    strcat(probe_name, "\n");
	    fputs(probe_name, pFile);

	    char probe_longname[255];
	    strcpy(probe_longname, "\tlongname= ");
	    strcpy(tmp, pD.longname);
	    correctIllegalChars(tmp);
	    strcat(probe_longname, tmp);
	    strcat(probe_longname, "\n");
	    fputs(probe_longname, pFile);

	    char probe_sequence[255];
	    strcpy(probe_sequence, "\tsequence= ");
	    strcat(probe_sequence, pD.sequence);
	    strcat(probe_sequence, "\n");
	    fputs(probe_sequence, pFile);

	    char toksep[2] = { 1, 0 };
	    char *hinfo = strtok(bs.data, toksep);

	    if(weightedMismatches) // weighted mismatches are used ...
	    {
		if(hinfo)
		{
			while(1)
			{
				struct _probe_match_result pMR;
				//char buf[64];

				// fetch tokens, break if unavailable
				char *token1= strtok(NULL, toksep);
				if(!token1) break;
				char *token2= strtok(NULL, toksep);
				if(!token2) break;

				// extract organism name from token
				char *ptr= skipToNextWord(token2);

				strncpy(tmp, ptr, 8);
				tmp[8]=0;
				correctIllegalChars(tmp);
				cleanEndingSpaces(tmp);
				strcpy(pMR.name, tmp);

				// calc length of fullname in the hinfo string
				char *hinfo_ptr= skipToNextWord(hinfo);
				hinfo_ptr= skipToNextWord(hinfo_ptr);
				int fullnamelength= (int)(skipToNextWord(hinfo_ptr) - hinfo_ptr);

				// extract organism fullname from token
				ptr= skipToNextWord(ptr);
				strncpy(tmp, ptr, fullnamelength);
				tmp[fullnamelength]= 0;
				correctIllegalChars(tmp);
				cleanEndingSpaces(tmp);
				strcpy(pMR.fullname, tmp);

				// wmis
				ptr= skipToNextWord(ptr + fullnamelength);
				ptr= skipToNextWord(ptr);
				ptr= skipToNextWord(ptr);
				strncpy(tmp, ptr, 4);
				tmp[4]=0;
				pMR.wmis= atof(tmp);

				// other settings can be separated similar, but not needed here...

				// add matched organism data to probe file (output)
				if((maxWeightedMismatches == -1) || (maxWeightedMismatches >= pMR.wmis))
				{
					strcpy(tmp, "\tmatch= ");
					strcat(tmp, pMR.name);
					strcat(tmp, ", ");
					strcat(tmp, pMR.fullname);
					strcat(tmp, "\n");
					fputs(tmp, pFile);
					//
					// printf("<<%s>>  <<%s>>  wmis=%f\n", pMR.name, pMR.fullname, pMR.wmis);
				}
			}
		}
	    }
	    else // no weighted mismatches ...
	    {
		if (hinfo)
		{
			while(1)
			{
				char *match_name = strtok(0, toksep);
				if (!match_name) break;
				char *match_info = strtok(0, toksep);
				if (!match_info) break;
				char *match_longname = parse_match_info(match_info);
				char probe_match[255];
				strcpy(probe_match, "\tmatch= ");
				correctIllegalChars(match_name);
				strcat(probe_match, match_name);

				strcat(probe_match, ", ");
				correctIllegalChars(match_longname);

				strcat(probe_match, match_longname);
				strcat(probe_match, "\n");
				fputs(probe_match, pFile);
			}
		} // end while(1)
	    }


	      fclose(pFile);
	    } // end if(pFile!=NULL)
	    else
	     error = "Error: Could not open result file for appending.\n";
        }
	free(bs.data);
    }

    return error;
}

char *parse_match_info(const char *match_info)
{
  int i,j;
  // hardgecoded: die ersten 12 bits von match_info enthalten den Namen, die naechsten 32
  // bits enthalten den Langnamen
  i = 11;
  j = 0;
  char buf[255];
  while(match_info[i] && j < 32)
    {
      buf[j] = match_info[i];
      i++;
      j++;
    }

  return GB_strdup(buf);
}


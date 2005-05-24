#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <malloc.h> */
#include <names_client.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <servercntrl.h>
#include <client.h>

struct ang_struct {
    aisc_com *link;
    T_AN_LOCAL locs;
    T_AN_MAIN com;
} ang = { 0,0,0} ;

int init_local_com_names()
{
    /*** create and init local com structure ***/
    if( aisc_create(ang.link, AN_MAIN, ang.com,
		    MAIN_LOCAL, AN_LOCAL, &ang.locs,
		    LOCAL_WHOAMI, "i bin der arb_tree",
		    NULL)){
	return 1;
    }
    return 0;
}

GB_ERROR connect_2_an(GBDATA *gb_main)
{
    char *servername;
    char *name_server = "ARB_NAME_SERVER";

    if (arb_look_and_start_server(AISC_MAGIC_NUMBER,name_server,gb_main)){
	return "Sorry I can't start the NAME SERVER";
    }
    servername = (char *)GBS_read_arb_tcp(name_server);
    if (!servername) exit (-1);

    ang.link = (aisc_com *)aisc_open(servername, &ang.com,AISC_MAGIC_NUMBER);
    if (init_local_com_names()){
	free(servername);
	return "Sorry I can't start the NAME SERVER";
    }
    free(servername);
    return 0;
}

void disconnect_an(void)
{
    if (ang.link) aisc_close(&ang.link);
}


GB_ERROR pars_names(GBDATA *gb_main, int use_advice, int save_data)
{
    GBDATA *gb_species_data;
    GBDATA *gb_species;
    GBDATA *gb_full_name;
    GBDATA *gb_name;
    GBDATA *gb_acc;
    long	hash;
    static 	char *shrt;

    char	*full_name;
    char	*name;
    char	*advice;
    char	*acc;

    GB_ERROR err;
    GB_ERROR err2;
    char	buffer[256];

    err = connect_2_an(gb_main);
    if (err) return err;

    err = GBT_begin_rename_session(gb_main,1);
    if (err) return err;


    gb_species_data = GB_search(gb_main,"species_data",GB_CREATE_CONTAINER);

    hash = GBS_create_hash(1024);
    err2 = 0;

    if (aisc_nput(ang.link, AN_LOCAL, ang.locs,
		  LOCAL_USEADVICE,		use_advice,
		  0)){
	err = "Connection Problems with the NAME_SERVER";
    }

    for ( gb_species = GBT_first_species_rel_species_data(gb_species_data);
          gb_species&&!err;
          gb_species = GBT_next_species(gb_species))
    {
        gb_full_name = GB_find(gb_species,"full_name",0,down_level);
        gb_name = GB_find(gb_species,"name",0,down_level);
        gb_acc = GB_find(gb_species,"acc",0,down_level);

        if (gb_acc)	acc = GB_read_string(gb_acc);
        else		acc = strdup("");
        if (gb_full_name) full_name = GB_read_string(gb_full_name);
        else		  full_name = strdup("");
        name = GB_read_string(gb_name);
        if (strlen(acc) + strlen(full_name) ) {
            if (aisc_nput(ang.link, AN_LOCAL, ang.locs,
                          LOCAL_FULL_NAME,	full_name,
                          LOCAL_ACCESSION,	acc,
                          LOCAL_ADVICE,		name,
                          0)){
                err = "Connection Problems with the NAME_SERVER";
            }
            if (aisc_get(ang.link, AN_LOCAL, ang.locs,
                         LOCAL_GET_SHORT,	&shrt,
                         0)){
                err = "Connection Problems with the NAME_SERVER";
            }
        }else{
            shrt = strdup(name);
        }
        if (!err) {
            if (GBS_read_hash(hash,shrt)) {
                char *newshrt;
                int i;
                newshrt = (char *)calloc(sizeof(char),strlen(shrt)+10);
                for (i= 1 ; ; i++) {
                    sprintf(newshrt,"%s.%i",shrt,i);
                    if (!GBS_read_hash(hash,newshrt))break;
                }
                err2 = "There are duplicated entries!!.\nThe duplicated entries contain a '.' character !!!";
                free(shrt); shrt = newshrt;
            }
            GBS_incr_hash(hash,shrt);
            err = GBT_rename_species(name,shrt);
        }
        if (name)	free(name);	name = 0;
        if (acc)	free(acc);	acc = 0;
        if (full_name)	free(full_name);full_name = 0;
        if (shrt)	free(shrt);	shrt = 0;
    }


    GBS_free_hash(hash);
    hash = 0;
    if (err) {
        GBT_abort_rename_session();
        return err;
    }else{
        //		if (save_data) {
        //			if (aisc_put(ang.link, AN_MAIN, ang.com,
        //				MAIN_SAVEALL,	0,0)){
        //				err = "Nameserver cannot save the data";
        //			}
        //		}
        GBT_commit_rename_session(0, 0);
    }
    disconnect_an();
    return err2;
}

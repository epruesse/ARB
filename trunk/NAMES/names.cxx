#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <names_server.h>
#include <names_client.h>
#include "names.h"
#include <arbdb.h>
#include <names_prototypes.h>
#include <server.h>
#include <client.h>
#include <servercntrl.h>
#include <struct_man.h>


#define UPPERCASE(c) if ( (c>='a') && (c<='z')) c+= 'A'-'a'

struct AN_gl_struct AN_global;
AN_main *aisc_main; /* muss so heissen */

void an_add_short(AN_local *locs,char *new_name, char *parsed_name,char *parsed_sym,char *shrt,char *acc)
{
    AN_shorts *an_shorts;
    AN_revers *an_revers;
    char	*full_name;
    locs = locs;

    if (strlen(parsed_sym)){
        full_name = (char *)calloc(sizeof(char),
                                   strlen(parsed_name) + strlen(" sym")+1);
        sprintf(full_name,"%s sym",parsed_name);
    }else{
        full_name = strdup(parsed_name);
    }

    an_shorts = create_AN_shorts();
    an_revers = create_AN_revers();

    an_shorts->mh.ident = strdup(new_name);
    an_shorts->shrt = strdup(shrt);
    an_shorts->full_name = strdup(full_name);
    an_shorts->acc = strdup(acc);
    aisc_link((struct_dllpublic_ext*)&(aisc_main->pnames),(struct_dllheader_ext*)an_shorts);

    an_revers->mh.ident = strdup(shrt);
    an_revers->full_name = full_name;
    an_revers->acc = strdup(acc);
    aisc_link((struct_dllpublic_ext*)&(aisc_main->prevers),(struct_dllheader_ext*)an_revers);
    aisc_main->touched = 1;
}


AN_shorts *an_find_shrt(AN_shorts *sin,char *search)
{
    while (sin) {
        if (!strncmp(sin->shrt,search,3)) {
            return sin;
        }
        sin = sin->next;
    }
    return 0;
}

/*	searches for an already defined short,
	if found then return found
	else if fullname == 0 return advice
	else if |advice| >=3 try advice first
	else try first 3 characters of full
	else try 
*/
char *nas_string_2_key(const char *str)	/* converts any string to a valid key */
{
    char buf[GB_KEY_LEN_MAX+1];
    int i;
    int c;
    for (i=0;i<GB_KEY_LEN_MAX;) {
        c= *(str++);
        if (!c) break;
        if (isalpha(c)) buf[i++] = c;
        else if (c==' ' || c=='_') buf[i++] = '_';
    }
    for (;i<GB_KEY_LEN_MIN;i++) buf[i] = '_';
    buf[i] = 0;
#if defined(DUMP_NAME_CREATION)
    printf("nas_string_2_key('%s') = '%s'\n", str, buf);
#endif // DUMP_NAME_CREATION    
    return strdup(buf);
}

char *nas_remove_small_vocals(const char *str) {
    char buf[GB_KEY_LEN_MAX+1];
    int i;
    int c;
    
    for (i=0; i<GB_KEY_LEN_MAX; ) {
        c = *str++;
        if (!c) break;
        if (strchr("aeiouy", c)==0) {
            buf[i++] = c;
        } 
    }
    for (; i<GB_KEY_LEN_MIN; i++) buf[i] = '_';
    buf[i] = 0;
#if defined(DUMP_NAME_CREATION)
    printf("nas_remove_small_vocals('%s') = '%s'\n", str, buf);
#endif // DUMP_NAME_CREATION
    return strdup(buf);
}

void an_complete_shrt(char *shrt, const char *rest_of_full) {
    int len = strlen(shrt);
    
    while (len<5) {
        char c = *rest_of_full++;
	
        if (!c) break;
        shrt[len++] = c;
    }
    
    while (len<GB_KEY_LEN_MIN) {
        shrt[len++] = '_';
    }
    
    shrt[len] = 0; 
}

char *an_get_short(AN_shorts *shorts,dll_public *parent,char *full){
    AN_shorts *look;
    //     int	search_pos;
    //     int	c_pos;
    //     int	end_pos;
    //     char	*p;

    if (strlen(full) == 0) {
        return strdup("Xxx");
    }

    look = (AN_shorts *)aisc_find_lib((struct_dllpublic_ext*)parent,full);
    if (look) {		/* hurra hurra , schon gefunden */
        return strdup(look->shrt);
    }

    char *full2 = nas_string_2_key(full);
    char *full3 = 0;
    char shrt[10];
    int len2, len3;
    int p1, p2;
    
    // try first three letters:
    
    strncpy(shrt,full2,3);
    UPPERCASE(shrt[0]);
    shrt[3] = 0;
    
    look = an_find_shrt(shorts,shrt);    
    if (!look) {
        an_complete_shrt(shrt, full2+3);
        goto insert_shrt;
    }

    // generate names from consonants: 

    full3 = nas_remove_small_vocals(full2); 
    len3 = strlen(full3);
    
    for (p1=3; p1<(len3-1); p1++) {
        shrt[1] = full3[p1];
        for (p2=p1+1; p2<len3; p2++) {
            shrt[2] = full3[p2];
            look = an_find_shrt(shorts, shrt);
            if (!look) {
                an_complete_shrt(shrt, full3+p2+1);
                goto insert_shrt;
            }
        }
    }
    
    // generate names from all characters:
    
    len2 = strlen(full2); 
    for (p1=3; p1<(len2-1); p1++) {
        shrt[1] = full2[p1];
        for (p2=p1+1; p2<len2; p2++) {
            shrt[2] = full2[p2];
            look = an_find_shrt(shorts, shrt);
            if (!look) {
                an_complete_shrt(shrt, full2+p2+1);
                goto insert_shrt;
            }
        }
    }
    
    // generate names containing two characters and one digit:
    
    for (p1=2; p1<len2; p1++) {
        shrt[1] = full2[p1];
        for (p2=0; p2<=9; p2++) {
            shrt[2] = '0'+p2;
            look = an_find_shrt(shorts, shrt);
            if (!look) {
                an_complete_shrt(shrt, full2+p1+1);
                goto insert_shrt;
            }
        }
    }
    
    // generate names containing one characters and two digits:
    
    for (p1=0; p1<=99; p1++) {
        shrt[1] = '0'+(p1/10);
        shrt[2] = '0'+(p1%10);
        look = an_find_shrt(shorts, shrt);
        if (!look) {
            an_complete_shrt(shrt, full2+1);
            goto insert_shrt;
        }
    }

    free(full3);
    free(full2);
    
    return 0;

    /* ---------------------------------------- */
    
 insert_shrt:
    
#if defined(DUMP_NAME_CREATION)
    if (isdigit(shrt[0]) || isdigit(shrt[1])) {
        printf("generated new short-name '%s' for full-name '%s' full2='%s' full3='%s'\n", shrt, full, full2, full3);
    }
#endif    // DUMP_NAME_CREATION
    
    look = create_AN_shorts();
    look->mh.ident = strdup(full2);
    look->shrt = strdup(shrt);
    aisc_link((struct_dllpublic_ext*)parent,(struct_dllheader_ext*)look);
    aisc_main->touched = 1;
    
    free(full3);
    free(full2);
    
    return strdup(shrt); 
}

extern "C" aisc_string get_short(AN_local *locs)
    /* get the short name from the previously set
			names */
{
    char *parsed_name;
    char *parsed_sym;
    char *parsed_acc;
    char	*new_name;
    char *first;
    char *second;
    char *p;
    static char *shrt=0;
    AN_shorts *an_shorts;
    int	shortlen;

    if(shrt) free(shrt);
    shrt = 0;

    parsed_name = GBS_string_eval(locs->full_name,
                                  "\t= :\"=:'=:* * *=*1 *2:sp.=species:spec.=species:.=",0);
    /* delete ' " \t and more than two words */
    parsed_sym = GBS_string_eval(locs->full_name,
                                 "\t= :* * *sym*=S",0);
    if (strlen(parsed_sym)>1) {
        free(parsed_sym);
        parsed_sym = strdup("");
    }
    parsed_acc = GBS_string_eval(locs->acc,";=:\t=: =:\"=:,=",0);
    /* delete spaces and more */
    first = GBS_string_eval(parsed_name,"* *=*1",0);
    p = strdup(parsed_name+strlen(first));
    second = GBS_string_eval(p," =:\t=",0);
    UPPERCASE(second[0]);
    free(p);

    new_name = (char *)calloc(sizeof(char),
                              strlen(first)+strlen(second)+strlen(parsed_acc)+4);

    if (strlen(parsed_acc)){
        sprintf(new_name,"*%s",parsed_acc);
    }else{
        sprintf(new_name,"%s*%s*%s",first,second,parsed_sym);
    }

    an_shorts = (AN_shorts *)aisc_find_lib((struct_dllpublic_ext*)&(aisc_main->pnames),new_name);
    if (!an_shorts){		/* now there is no short name */
        char *first_short,*second_short;
        char *first_advice,*second_advice;
        int	count;
        char 	test_short[256];

        if (strlen(locs->advice) ) {
            first_advice = GBS_string_eval(locs->advice,
                                           "???*=?1?2?3:0=:1=:2=:3=:4=:5=:6=:7=:8=:9=",0);
        }else{
            first_advice = strdup("Xxx");
        }

        if (strlen(locs->advice) > 3) {
            second_advice = GBS_string_eval(locs->advice+3,
                                            " =:\t=:,=:;=",0);
        }else{
            second_advice = strdup("Yyyyy");
        }

        if (strlen(first)) {
            first_short = an_get_short(	aisc_main->shorts1,
                                        &(aisc_main->pshorts1),first);
        }else{
            first_short = strdup(first_advice);
        }
        first_short = (char *)realloc(first_short,strlen(first_short)+3);

        if (!strlen(first_short)) {
            sprintf(first_short,"Xxx");
        }


        shortlen = 5;
        second_short = (char *)calloc(sizeof(char), 10);
        if (strlen(second)) {
            strncpy(second_short,second,shortlen);
        }else if (strlen(first_short) > 3) {
            strncpy(second_short,first_short+3,shortlen);
        }else if (strlen(second_advice) ) {
            strncpy(second_short,second_advice,shortlen);
            UPPERCASE(second_short[0]);
        }else{
            strncpy(second_short,"Yyyyy",shortlen);
        }

        first_short[3] = 0;		/* 3 characters for the first word */

        sprintf(test_short,"%s%s",
                first_short,second_short);

        if (aisc_find_lib((struct_dllpublic_ext*)&(aisc_main->prevers),test_short)) {
            second_short[shortlen-1] = 0;
            for (count = 2; count <100000; count++) {
                if (count>=10){
                    second_short[shortlen-2] = 0;
                }
                if (count>=100){
                    second_short[shortlen-3] = 0;
                }
                sprintf(test_short,"%s%s%i",first_short,second_short,count);
                if (!aisc_find_lib((struct_dllpublic_ext*)&(aisc_main->prevers),test_short)) break;
            }
        }
        shrt = strdup(test_short);
        if (strlen(parsed_name) >3) {
            an_add_short(locs,new_name,parsed_name,parsed_sym,shrt,parsed_acc);
            /* add the newly created short to list */
        }
        free(first_short);
        free(second_short);
        free(first_advice);
        free(second_advice);
    }else{
        shrt = strdup(an_shorts->shrt);
    }
    free(first);free(second);free(parsed_name);free(parsed_sym);
    free(parsed_acc); free(new_name);
    return shrt;
}

extern "C" int server_save(AN_main *main, int dummy)
{
    FILE	*file;
    int	error;
    char	*sec_name;
    dummy = dummy;
    if (main->touched) {

        sec_name = (char *)calloc(sizeof(char),strlen(aisc_main->server_file)+2);
        sprintf(sec_name,"%s%%",aisc_main->server_file);
        file = fopen(sec_name,"w");
        if (!file) {
            fprintf(stderr,"ERROR cannot save file %s\n",sec_name);
        }else{
            error = save_AN_main(aisc_main,file);
            fclose(file);
            if (!error) {
                if (GB_rename(sec_name,aisc_main->server_file)) {
                    GB_print_error();
                    fprintf(stderr,"ERROR cannot rename file %s %s\n",
                            sec_name,aisc_main->server_file);
                }else{
                    main->touched = 0;
                }
            }
        }
        free(sec_name);
    }
    return 0;
}

int server_load(AN_main *main)
{
    FILE	*file;
    int	error=0;
    char	*sec_name = main->server_file;
    AN_shorts	*shrt;
    AN_revers	*revers;

    file = fopen(sec_name,"r");
    if (!file) {
        fprintf(stderr,"ERROR cannot load file %s\n",sec_name);
    }else{
        error = load_AN_main(main,file);
    }
    for (shrt = main->names; shrt; shrt = shrt->next) {
        revers = create_AN_revers();
        revers->mh.ident = strdup(shrt->shrt);
        revers->full_name = strdup(shrt->full_name);
        revers->acc  = strdup(shrt->acc);
        aisc_link((struct_dllpublic_ext*)&(main->prevers),(struct_dllheader_ext*)revers);
    }
    return error;
}

int names_server_shutdown(void)
{
    /* 	server_save(aisc_main,0);	*/
    aisc_server_shutdown(AN_global.server_communication);
    return 0;
}


extern "C" int server_shutdown(AN_main *pm,aisc_string passwd){
    /** passwdcheck **/
    if( strcmp(passwd, "ldfiojkherjkh") ) return 1;
    printf("\nI got the shutdown message.\n");

    /** shoot clients **/
    aisc_broadcast(AN_global.server_communication, 0,
		   "SERVER SHUTDOWN BY ADMINISTRATOR!\n");

    /** shutdown **/
    names_server_shutdown();
    exit(0);
    pm = pm;
    return 0;	/* Never Reached */
}

int main(int argc,char **argv)
{
    char *name;
    int	i;
    struct Hs_struct *so;
    struct arb_params *params;

    params = arb_trace_argv(&argc,argv);
    if (!params->default_file) {
        printf("No file: Syntax: %s -boot/-kill/-look -dfile\n",argv[0]);
        exit(-1);
    }

    if (argc ==1) {
        char flag[]="-look";
        argv[1] = flag;
        argc = 2;
    }

    if (argc!=2) {
        printf("Wrong Parameters %i Syntax: %s -boot/-kill/-look -dfile\n",argc,argv[0]);
        exit(-1);
    }

    aisc_main = create_AN_main();
    /***** try to open com with any other pb server ******/
    if (params->tcp) {
        name = params->tcp;
    }else{
        if( !(name=(char *)GBS_read_arb_tcp("ARB_NAME_SERVER")) ){
            GB_print_error();
            exit(-1);
        }else{
            name=strdup(name);
        }
    }

    AN_global.cl_link = (aisc_com *)aisc_open(name,
                                              (long *)&AN_global.cl_main,AISC_MAGIC_NUMBER);

    if (AN_global.cl_link) {
        if( !strcmp(argv[1],"-look")) {
            aisc_close(AN_global.cl_link);AN_global.cl_link=0;
            exit(0);
        }

        printf("There is another activ server. I try to kill him...\n");
        aisc_nput(AN_global.cl_link, AN_MAIN, AN_global.cl_main,
                  MAIN_SHUTDOWN, "ldfiojkherjkh",
                  NULL);
        aisc_close(AN_global.cl_link);AN_global.cl_link=0;
        sleep(1);
    }
    if( ((strcmp(argv[1],"-kill")==0)) ||
        ((argc==3) && (strcmp(argv[2],"-kill")==0))){
        printf("Now I kill myself!\n");
        exit(0);
    }
    for(i=0, so=0; (i<MAX_TRY) && (!so); i++){
        so = open_aisc_server(name, TIME_OUT,0);
        if(!so) sleep(10);
    }
    if(!so){
        printf("AN_SERVER: Gave up on opening the communication socket!\n");
        exit(0);
    }
    AN_global.server_communication = so;

    aisc_main->server_file = strdup(params->default_file);
    server_load(aisc_main);	

    while (i == i) {
        aisc_accept_calls(so);
        if (aisc_main->ploc_st.cnt <=0) {
            server_save(aisc_main,0);
            names_server_shutdown();
            exit(0);
        }
    }

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>


int main(int argc, char **argv)
{

    char	   *data;
    char	   *ndata;
    FILE	   *out;
    int	        arg;
    char	   *eval;
    const char *fname;
    int	        linemode           = GB_FALSE;
    int	        delete_empty_lines = GB_FALSE;
    int	        startarg;
    int	        patchmode          = GB_FALSE;

    if (argc <=1 || (argc >= 2 && strcmp(argv[1], "--help") == 0)) {
        printf("syntax: arb_replace [-l/L/p] \"old=newdata\" [filepattern]\n");
        //         printf("	%s %s %s %s",argv[0],argv[1],argv[2],argv[3]);
        printf("	-l	linemode, parse each line seperatedly\n");
        printf("	-L	linemode, parse each line seperatedly, delete empty lines\n");
        printf("	-p	patchmode, (no wildcards allowed, rightside<leftside)\n");
        return -1;
    }
    if (!strcmp(argv[1],"-l")) {
        eval               = argv[2];
        linemode           = GB_TRUE;
        startarg           = 3;
    }
    else if (!strcmp(argv[1],"-L")) {
        eval               = argv[2];
        linemode           = GB_TRUE;
        delete_empty_lines = GB_TRUE;
        startarg           = 3;
    }
    else if (!strcmp(argv[1],"-p")) {
        eval      = argv[2];
        patchmode = GB_TRUE;
        startarg  = 3;
    }
    else {
        eval     = argv[1];
        startarg = 2;
    }
    int	usestdin = GB_FALSE;
    if (startarg==argc) {
        usestdin = GB_TRUE;		// stdin stdout
        argc++;
    }

    for (arg = startarg; arg<argc;arg++){
        if (usestdin) {
            fname = "-";
        }else{
            fname = argv[arg];
        }
        data = GB_read_file(fname);
        if (data) {
            if (patchmode) {
                unsigned long size = GB_size_of_file(fname);
                char *right = strchr(eval,'=');
                int patched = GB_FALSE;
                if (!right) {
                    fprintf(stderr,"'=' not found in replace string\n");
                    return -1;
                }
                if (strlen(right) > strlen(eval)) {
                    fprintf(stderr,"You cannot replace a shorter string by a longer one!!!\n");
                    return -1;
                }

                *(right++) = 0;
                unsigned long i;
                int leftsize = strlen(eval);
                size -= leftsize;
                for (i=0;i<size;i++){
                    if (!strncmp(data+i,eval,leftsize)){
                        strcpy(data+i,right);
                        patched = GB_TRUE;
                    }
                }
                if (patched) {
                    out = fopen(fname,"w");
                    if (out) {
                        if (fwrite(data,(unsigned int)size,1,out) != 1 ){
                            fprintf(stderr,"Write failed %s\n",fname);
                            return -1;
                        }
                        fprintf(out,"%s",data);
                        fclose(out);
                        printf("File %s parsed\n",fname);
                    }else{
                        fprintf(stderr,"Write failed %s\n",fname);
                        return -1;
                    }
                }
                return 0;
            }

            if (linemode) {
                char *p = data;
                char *nextp;
                char *h;
                void *strstruct = GBS_stropen(1024);
                while ((nextp = strchr(p,'\n'))) {
                    nextp[0] = 0;			// remove '\n'
                    h = GBS_string_eval(p,eval,0);
                    if (!h){
                        h = strdup (p);
                        fprintf(stderr,"%s\n",GB_get_error());
                    }

                    if (usestdin) {
                        fprintf(stdout,"%s",h);
                        if (h[0] || !delete_empty_lines) {
                            fprintf(stdout,"\n");
                        }
                    }else{
                        GBS_strcat(strstruct,h);
                        if (h[0] || !delete_empty_lines) {	// delete empty lines
                            GBS_chrcat(strstruct,'\n');		// insert '\n'
                        }
                    }
                    p = nextp+1;
                    nextp[0] = '\n';		// reinsert '\n'
                    free(h);
                }
                h = GBS_string_eval(p,eval,0);
                GBS_strcat(strstruct,h);
                ndata = GBS_strclose(strstruct);
                free(h);

            }else{
                ndata = GBS_string_eval(data,eval,0);
            }

            if (!ndata){
                fprintf(stderr,"%s\n",GB_get_error());
                exit(-1);
            }
            if (strcmp(data,ndata)){
                if (usestdin) {
                    fprintf(stdout,"%s",ndata);
                }else{
                    out = fopen(fname,"w");
                    if (out) {
                        fprintf(out,"%s",ndata);
                        fclose(out);
                        printf("File %s parsed\n",fname);
                    }else{
                        printf("cannot write %s\n",fname);
                    }
                }
            }
            free(ndata);
            free(data);
        }
    }
    return 0;
}

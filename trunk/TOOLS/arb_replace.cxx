#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>


int main(int argc, char **argv)
{

    char       *data;
    char       *ndata;
    FILE       *out;
    int         arg;
    char       *eval;
    const char *fname;
    int         linemode           = false;
    int         delete_empty_lines = false;
    int         startarg;
    int         patchmode          = false;

    if (argc <=1 || (argc >= 2 && strcmp(argv[1], "--help") == 0)) {
        printf("syntax: arb_replace [-l/L/p] \"old=newdata\" [filepattern]\n");
        printf("        -l      linemode, parse each line separately\n");
        printf("        -L      linemode, parse each line separately, delete empty lines\n");
        printf("        -p      patchmode, (no wildcards allowed, rightside<leftside)\n");
        return -1;
    }
    if (!strcmp(argv[1],"-l")) {
        eval               = argv[2];
        linemode           = true;
        startarg           = 3;
    }
    else if (!strcmp(argv[1],"-L")) {
        eval               = argv[2];
        linemode           = true;
        delete_empty_lines = true;
        startarg           = 3;
    }
    else if (!strcmp(argv[1],"-p")) {
        eval      = argv[2];
        patchmode = true;
        startarg  = 3;
    }
    else {
        eval     = argv[1];
        startarg = 2;
    }
    int usestdin = false;
    if (startarg==argc) {
        usestdin = true;                            // stdin stdout
        argc++;
    }

    for (arg = startarg; arg<argc;arg++){
        if (usestdin) {
            fname = "-";
        }
        else {
            fname = argv[arg];
        }
        data = GB_read_file(fname);
        if (data) {
            if (patchmode) {
                unsigned long size = GB_size_of_file(fname);
                char *right = strchr(eval,'=');
                int patched = false;
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
                        patched = true;
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
                    }
                    else {
                        fprintf(stderr,"Write failed %s\n",fname);
                        return -1;
                    }
                }
                return 0;
            }

            if (linemode) {
                char          *p         = data;
                char          *nextp;
                char          *h;
                GBS_strstruct *strstruct = GBS_stropen(1024);

                while ((nextp = strchr(p,'\n'))) {
                    nextp[0] = 0;                       // remove '\n'
                    h = GBS_string_eval(p,eval,0);
                    if (!h){
                        h = strdup (p);
                        fprintf(stderr,"%s\n",GB_await_error());
                    }

                    if (usestdin) {
                        fprintf(stdout,"%s",h);
                        if (h[0] || !delete_empty_lines) {
                            fprintf(stdout,"\n");
                        }
                    }
                    else {
                        GBS_strcat(strstruct,h);
                        if (h[0] || !delete_empty_lines) {      // delete empty lines
                            GBS_chrcat(strstruct,'\n');         // insert '\n'
                        }
                    }
                    p = nextp+1;
                    nextp[0] = '\n';            // reinsert '\n'
                    free(h);
                }
                h = GBS_string_eval(p,eval,0);
                GBS_strcat(strstruct,h);
                ndata = GBS_strclose(strstruct);
                free(h);

            }
            else {
                ndata = GBS_string_eval(data,eval,0);
            }

            if (!ndata){
                fprintf(stderr,"%s\n",GB_await_error());
                exit(-1);
            }
            if (strcmp(data,ndata)){
                if (usestdin) {
                    fprintf(stdout,"%s",ndata);
                }
                else {
                    out = fopen(fname,"w");
                    if (out) {
                        fprintf(out,"%s",ndata);
                        fclose(out);
                        printf("File %s parsed\n",fname);
                    }
                    else {
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>

#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include "awt.hxx"
#include "awtlocal.hxx"

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

char *AWT_fold_path(char *path, const char *pwd)
{
    const char *p;
    if (!strcmp(pwd,"PWD"))	p = GB_getcwd();
    else p = GB_getenv(pwd);

    if (!p) return path;
    char *cwd;
    int cwdlen = strlen(p) + 1;
    cwd = (char *)GB_calloc(sizeof(char),cwdlen+1);
    sprintf(cwd,"%s/",p);

    char *source = path;
    char *dest = path;
    while (*source) {
        if (!strncmp(source,"/../",4)) {
            *dest = 0;
            char *slash = strrchr(path,'/');
            if (slash) dest = slash;
            source+=3;
        }
        if (!strncmp(source,"/./",3)) {
            source+=2;
        }
        if (!strcmp(source,"/..")){
            if  (source!=path) {
                *dest = 0;
                char *slash = strrchr(path,'/');
                if (slash) dest = slash;
                if (dest == path) *(dest++) = '/';	// last '/' symbol
            }else{
                *(dest++) = '/';			// /.. -> /
            }
            source += 3;
        }
        if (!strcmp(source,"/.")  && (source!=path)) source += 2;
        *(dest++) = *source;
        if (*source) source++;
    }
    if (!strncmp(cwd,path,cwdlen)){
        delete cwd;
        return path + cwdlen;
    }

    delete cwd;

    if (!strcmp(p,path)) return strcpy(path,""), path;
    if (!strncmp(path,"./",2)) return path + 2;
    if (!strncmp(path,"//",2)) return path + 1;

    return path;
}

/* create a full path */

char *AWT_unfold_path(const char *path, const char *pwd) {
    const char *cwd;

    if (path[0] == '/') return strdup(path);

    if (!strcmp(pwd,"PWD"))	cwd = GB_getcwd();
    else cwd = GB_getenv(pwd);

    if (path[0] == 0) return strdup(cwd);
    char *res = (char*)GB_calloc(strlen(cwd)+strlen(path)+2,sizeof(char));
    sprintf(res,"%s/%s",cwd,path);

    return res;
}

const char *AWT_valid_path(const char *path) {
    if (strlen(path)) return path;
    return ".";
}

int AWT_is_dir(const char *path) {
    struct stat stt;
    if (stat(AWT_valid_path(path), &stt)) return 0;
    if (S_ISDIR(stt.st_mode)) return 1;
    return 0;
}

void awt_create_selection_box_cb(void *dummy, struct adawcbstruct *cbs) {
    AW_root *aw_root = cbs->aws->get_root();
    AWUSE(dummy);
    cbs->aws->clear_selection_list(cbs->id);

    char       *diru    = aw_root->awar(cbs->def_dir)->read_string();
    char       *dir     = AWT_fold_path(diru,cbs->pwd);
    char       *fulldir = AWT_unfold_path(dir,cbs->pwd);
    char       *filter  = aw_root->awar(cbs->def_filter)->read_string();
    const char *cwd;

    if (!strcmp(cbs->pwd,"PWD"))	cwd = GB_getcwd();
    else cwd                            = GB_getenv(cbs->pwd);

    const char *home            = GB_getenvHOME();
    const char *arbhome         = GB_getenvARBHOME();
    const char *workingdir      = GB_getenv("ARB_WORKDIR");
    if (!workingdir) workingdir = home; // if no working dir -> use home dir

    char buffer[GB_PATH_MAX];	memset(buffer,0,GB_PATH_MAX);
    char buffer2[GB_PATH_MAX];	memset(buffer2,0,GB_PATH_MAX);
    char directory[GB_PATH_MAX];	memset(directory,0,GB_PATH_MAX);
    char fulldirectory[GB_PATH_MAX];memset(fulldirectory,0,GB_PATH_MAX);

    if (cbs->show_dir) {
        cbs->aws->insert_selection( cbs->id,
                                    (char *)GBS_global_string("CONTENTS OF '%s'",fulldir),
                                    "." );
    }

    if (cbs->show_dir) {
        if (strcmp(cwd,"/")){
            cbs->aws->insert_selection( cbs->id, "D \' PARENT DIR      (..)\'", ".." );
        }
        if (strcmp(cwd,fulldir)){
            sprintf(buffer,"D \'$%s\'            (%s)",cbs->pwd,cwd);
            cbs->aws->insert_selection( cbs->id, buffer, cwd );
        }
        if (strcmp(home,fulldir)){
            sprintf(buffer,"D \'$HOME\'           (%s)",home);
            cbs->aws->insert_selection( cbs->id, buffer, home );
        }
        if (strcmp(workingdir,fulldir)){
            sprintf(buffer,"D \'$ARB_WORKDIR\'    (%s)",workingdir);
            cbs->aws->insert_selection( cbs->id, buffer, workingdir );
        }
        sprintf(buffer2,"%s/lib/pts",arbhome);
        if (strcmp(buffer2,fulldir)){
            sprintf(buffer,"D \'$PT_SERVER_HOME\' (%s)",buffer2);
            cbs->aws->insert_selection( cbs->id, buffer, buffer2 );
        }
    }

    DIR *dirp = opendir(fulldir);
    struct dirent *dp;
    struct stat stt;

    if (!dirp) {
        cbs->aws->insert_selection( cbs->id, "Your directory path is invalid", "?" );
    }else{
        for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)){
            if (strlen(fulldir)){
                if (fulldir[strlen(fulldir)-1]=='/') {
                    sprintf(fulldirectory,"%s%s",fulldir,dp->d_name);
                }else{
                    sprintf(fulldirectory,"%s/%s",fulldir,dp->d_name);
                }
            } else {
                sprintf(fulldirectory,"%s",dp->d_name);
            }

            if (strlen(dir)){
                if (dir[strlen(dir)-1]=='/') {
                    sprintf(directory,"%s%s",dir,dp->d_name);
                }else{
                    sprintf(directory,"%s/%s",dir,dp->d_name);
                }
            }else{
                sprintf(directory,"%s",dp->d_name);
            }

            if (AWT_is_dir(fulldirectory)){
                if (!cbs->show_dir) continue;
                if (!strcmp (directory,".") ) continue;
                if (!strcmp (directory,"..") ) continue;
                if (!GBS_string_cmp (directory,"*/.",1) ) continue;
                if (!GBS_string_cmp (directory,"*/..",1) ) continue;
                sprintf(buffer,"D %s",directory);
            }else{
                int fl;
                if ( (fl = strlen(filter)) ){
                    char *nf = filter;
                    if (nf[0] == '.'){
                        nf++;
                        fl--;
                    }
                    int nl = strlen(dp->d_name);
                    if (nl <= fl ) continue;
                    char *ss = dp->d_name + nl - fl;
                    if (ss[-1] != '.') continue;
                    if (GBS_string_cmp(ss, nf, 0))
                        continue;
                }
                if (stat(fulldirectory, &stt)) continue;
                if (!S_ISREG(stt.st_mode)) continue;	// list only regular files
                char atime[256];
                struct tm *tms = localtime(&stt.st_mtime);
                strftime( atime, 255,"%b %d %k:%M %Y",tms);
                long ksize = stt.st_size/1024;
                sprintf(buffer,"f %-30s   '%5lik %s'",directory,ksize,atime);
            }
            cbs->aws->insert_selection( cbs->id, buffer, directory );
        }
        closedir(dirp);
    }

    cbs->aws->insert_default_selection( cbs->id, "", "" );
    cbs->aws->sort_selection_list(cbs->id, 0);
    cbs->aws->update_selection_list( cbs->id );

    delete fulldir;
    delete diru;
    delete filter;
}

void awt_create_selection_box_changed_filename(void *dummy, struct adawcbstruct *cbs)
{
    AWUSE(dummy);
    char buffer[1024];
    AW_root *aw_root = cbs->aws->get_root();
    char *fname = aw_root->awar(cbs->def_name)->read_string();
    if (!strlen(fname)) {
        delete fname;
        return;
    }

    char *filter = aw_root->awar(cbs->def_filter)->read_string();
    char *dir = aw_root->awar(cbs->def_dir)->read_string();
    char *fulldir = AWT_unfold_path(dir,cbs->pwd);
    char *sdir = AWT_fold_path(fulldir,cbs->pwd);

    char *newname;
    char *slash = strrchr(fname,'/');
    if (slash) {
        newname = strdup(fname);
    }else{
        // add directory to the fname
        if (strlen(sdir)){
            newname = strdup(GBS_global_string("%s/%s",sdir,fname));
        }else{
            newname = strdup(fname);
        }
    }


    char *newfullname = AWT_unfold_path(newname,cbs->pwd);

    if (AWT_is_dir(newfullname)){
        aw_root->awar(cbs->def_name)->write_string("");	// may start a rekursion if noname is a dir
        aw_root->awar(cbs->def_dir)->write_string(AWT_fold_path(newfullname,cbs->pwd));
    }else{			// no directory
        if (strlen(filter)){			// now check the correct suffix
            char *pfilter = strrchr(filter,'.');
            if (pfilter) pfilter++;
            else	pfilter = filter;
            char *path = strdup(newname);
            char *point = strrchr(path,'.');
            if (point){
                char *slash2 = strrchr(path,'/');
                if (!slash2 || slash2<point) *point = 0;
            }
            sprintf(buffer,"%s.%s",path,pfilter);
            delete path;
        }else{
            sprintf(buffer,"%s",newname);
        }
        if (strchr(buffer,'/')) {
            char *s = strrchr(buffer,'/');
            *s = 0;
            if ( strcmp(dir,buffer) ) {		// adjust path
                char *fullfname = AWT_unfold_path(buffer,cbs->pwd);
                aw_root->awar(cbs->def_dir)
                    ->write_string(AWT_fold_path(fullfname,cbs->pwd));
                delete fullfname;
            }
            *s = '/';
        }
        if (strcmp(fname,buffer)) {
            aw_root->awar(cbs->def_name)->write_string(buffer);	// loops back if changed !!!
        }
    }

    delete newname;
    delete newfullname;
    delete fulldir;
    delete filter;
    delete fname;
    delete dir;
}

void awt_create_selection_box(AW_window *aws, const char *awar_prefix,const char *at_prefix,const  char *pwd, AW_BOOL show_dir )
{
    AW_root             *aw_root = aws->get_root();
    struct adawcbstruct *acbs    = new adawcbstruct;

    acbs->aws      = (AW_window *)aws;
    acbs->pwd      = strdup(pwd);
    acbs->show_dir = show_dir;
    acbs->def_name = GBS_string_eval(awar_prefix,"*=*/file_name",0);

    aw_root->awar(acbs->def_name)->add_callback((AW_RCB1)awt_create_selection_box_changed_filename,(AW_CL)acbs);

    acbs->def_dir = GBS_string_eval(awar_prefix,"*=*/directory",0);
    aw_root->awar(acbs->def_dir)->add_callback((AW_RCB1)awt_create_selection_box_cb,(AW_CL)acbs);

    acbs->def_filter = GBS_string_eval(awar_prefix,"*=*/filter",0);
    aw_root->awar(acbs->def_filter)->add_callback((AW_RCB1)awt_create_selection_box_changed_filename,(AW_CL)acbs);
    aw_root->awar(acbs->def_filter)->add_callback((AW_RCB1)awt_create_selection_box_cb,(AW_CL)acbs);

    char buffer[1024];
    sprintf(buffer,"%sfilter",at_prefix);
    if (aws->at_ifdef(buffer)){
        aws->at(buffer);
        aws->create_input_field(acbs->def_filter,5);
    }

    sprintf(buffer,"%sfile_name",at_prefix);
    if (aws->at_ifdef(buffer)){
        aws->at(buffer);
        aws->create_input_field(acbs->def_name,20);
    }

    sprintf(buffer,"%sbox",at_prefix);
    aws->at(buffer);
    acbs->id = aws->create_selection_list(acbs->def_name,0,"",2,2);
    awt_create_selection_box_cb(0,acbs);
}




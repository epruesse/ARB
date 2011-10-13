// =============================================================== //
//                                                                 //
//   File      : arb_file.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in October 2011   //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "arb_file.h"

#include <unistd.h>
#include <sys/stat.h>
#include <cstdio>
#include <cerrno>

#include "arb_msg.h"

// AISC_MKPT_PROMOTE:#ifndef ARB_CORE_H
// AISC_MKPT_PROMOTE:#include "arb_core.h"
// AISC_MKPT_PROMOTE:#endif


long GB_size_of_file(const char *path) {
    struct stat stt;
    if (!path || stat(path, &stt)) return -1;
    return stt.st_size;
}

long GB_size_of_FILE(FILE *in) {
    int         fi = fileno(in);
    struct stat st;
    if (fstat(fi, &st)) {
        GB_export_error("GB_size_of_FILE: sorry file is not readable");
        return -1;
    }
    return st.st_size;
}

unsigned long GB_time_of_file(const char *path) {
    struct stat stt;
    if (!path || stat(path, &stt)) return 0; // return epoch for missing files
    return stt.st_mtime;
}

long GB_mode_of_file(const char *path) {
    struct stat stt;
    if (path) if (stat(path, &stt)) return -1;
    return stt.st_mode;
}

long GB_mode_of_link(const char *path) {
    struct stat stt;
    if (path) if (lstat(path, &stt)) return -1;
    return stt.st_mode;
}

bool GB_is_regularfile(const char *path) {
    // Warning : returns true for symbolic links to files (use GB_is_link() to test)
    struct stat stt;
    return stat(path, &stt) == 0 && S_ISREG(stt.st_mode);
}

bool GB_is_link(const char *path) {
    struct stat stt;
    return lstat(path, &stt) == 0 && S_ISLNK(stt.st_mode);
}

bool GB_is_executablefile(const char *path) {
    struct stat stt;
    bool        executable = false;

    if (stat(path, &stt) == 0) {
        uid_t my_userid = geteuid(); // effective user id
        if (stt.st_uid == my_userid) { // I am the owner of the file
            executable = !!(stt.st_mode&S_IXUSR); // owner execution permission
        }
        else {
            gid_t my_groupid = getegid(); // effective group id
            if (stt.st_gid == my_groupid) { // I am member of the file's group
                executable = !!(stt.st_mode&S_IXGRP); // group execution permission
            }
            else {
                executable = !!(stt.st_mode&S_IXOTH); // others execution permission
            }
        }
    }

    return executable;
}

bool GB_is_privatefile(const char *path, bool read_private) {
    // return true, if nobody but user has write permission
    // if 'read_private' is true, only return true if nobody but user has read permission
    //
    // Note: Always returns true for missing files!
    //
    // GB_is_privatefile is mainly used to assert that files generated in /tmp have secure permissions

    struct stat stt;
    bool        isprivate = true;

    if (stat(path, &stt) == 0) {
        if (read_private) {
            isprivate = (stt.st_mode & (S_IWGRP|S_IWOTH|S_IRGRP|S_IROTH)) == 0;
        }
        else {
            isprivate = (stt.st_mode & (S_IWGRP|S_IWOTH)) == 0;
        }
    }
    return isprivate;
}

bool GB_is_readablefile(const char *filename) {
    FILE *in = fopen(filename, "r");

    if (in) {
        fclose(in);
        return true;
    }
    return false;
}

bool GB_is_directory(const char *path) {
    // Warning : returns true for symbolic links to directories (use GB_is_link())
    struct stat stt;
    return stat(path, &stt) == 0 && S_ISDIR(stt.st_mode);
}

long GB_getuid_of_file(const char *path) {
    struct stat stt;
    if (stat(path, &stt)) return -1;
    return stt.st_uid;
}

int GB_unlink(const char *path)
{   /*! unlink a file
     * @return
     *  0   success
     *  1   File did not exist
     * -1   Error (use GB_await_error() to retrieve message)
     */

    if (unlink(path) != 0) {
        if (errno == ENOENT) {
            return 1;
        }
        GB_export_error(GB_IO_error("removing", path));
        return -1;
    }
    return 0;
}

void GB_unlink_or_warn(const char *path, GB_ERROR *error) {
    /* Unlinks 'path'
     *
     * In case of a real unlink failure:
     * - if 'error' is given -> set error if not already set
     * - otherwise only warn
     */

    if (GB_unlink(path)<0) {
        GB_ERROR unlink_error = GB_await_error();
        if (error && *error == NULL) *error = unlink_error;
        else GB_warning(unlink_error);
    }
}

GB_ERROR GB_symlink(const char *target, const char *link) {
    if (symlink(target, link)<0) {
        return GBS_global_string("Cannot create symlink '%s' to file '%s'", link, target);
    }
    return 0;
}

GB_ERROR GB_set_mode_of_file(const char *path, long mode) {
    if (chmod(path, (int)mode)) return GBS_global_string("Cannot change mode of '%s'", path);
    return 0;
}

char *GB_follow_unix_link(const char *path) {   // returns the real path of a file
    char buffer[1000];
    char *path2;
    char *pos;
    char *res;
    int len = readlink(path, buffer, 999);
    if (len<0) return 0;
    buffer[len] = 0;
    if (path[0] == '/') return strdup(buffer);

    path2 = strdup(path);
    pos = strrchr(path2, '/');
    if (!pos) {
        free(path2);
        return strdup(buffer);
    }
    *pos = 0;
    res  = GBS_global_string_copy("%s/%s", path2, buffer);
    free(path2);
    return res;
}

GB_ERROR GB_rename_file(const char *oldpath, const char *newpath) {
    long old_mod               = GB_mode_of_file(newpath); // keep filemode for existing files
    if (old_mod == -1) old_mod = GB_mode_of_file(oldpath);

    GB_ERROR error = NULL;
    if (rename(oldpath, newpath) != 0) {
        error = GB_IO_error("renaming", GBS_global_string("%s into %s", oldpath, newpath));
    }
    else {
        error = GB_set_mode_of_file(newpath, old_mod);
    }
    
    sync();                                         // why ?
    return error;
}


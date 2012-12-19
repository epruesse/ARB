// ================================================================ //
//                                                                  //
//   File      : common.c                                           //
//   Purpose   : Common code for server and client                  //
//                                                                  //
//   Institute of Microbiology (Technical University Munich)        //
//   http://www.arb-home.de/                                        //
//                                                                  //
// ================================================================ //

#include "common.h"

#include <dupstr.h>
#include <arb_cs.h>

#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#if defined(WARN_TODO)
#warning DRY code below
/* e.g. aisc_get_m_id and aisc_client_get_m_id
 * aisc_open_socket() / aisc_client_open_socket()
 * etc.
 */
#endif

const char *aisc_client_get_m_id(const char *path, char **m_name, int *id) {
    // Warning: duplicated in server.c@aisc_get_m_id
    char *p;
    char *mn;
    int   i;
    if (!path) {
        return "AISC_CLIENT_OPEN ERROR: missing hostname:socketid";
    }
    if (!strcmp(path, ":")) {
        path = getenv("SOCKET");
        if (!path) return "environment socket not found";
    }
    p = (char *)strchr(path, ':');
    if (path[0] == '*' || path[0] == ':') {     // UNIX MODE
        char buffer[128];
        if (!p) {
            return "AISC_CLIENT_OPEN ERROR: missing ':' in *:socketid";
        }
        if (p[1] == '~') {
            sprintf(buffer, "%s%s", getenv("HOME"), p+2);
            *m_name = (char *)strdup(buffer);
        }
        else {
            *m_name = (char *)strdup(p+1);
        }
        *id = -1;
        return 0;
    }
    if (!p) {
        return "OPEN_ARB_DB_CLIENT ERROR: missing ':' in netname:socketid";
    }

    mn = (char *) calloc(sizeof(char), p - path + 1);
    strncpy(mn, path, p - path);

    /* @@@ falls hier in mn ein der Bereich von path bis p stehen soll, fehlt eine abschliesende 0 am String-Ende
       auf jeden Fall erzeugt der folgende strcmp einen (rui) */

    if (strcmp(mn, "localhost") == 0) freedup(mn, arb_gethostname());

    *m_name = mn;
    i       = atoi(p + 1);
    if ((i < 1024) || (i > 32000)) {
        return "OPEN_ARB_DB_CLIENT ERROR: socketnumber not in [1024..32000]";
    }
    *id = i;
    return 0;
}

const char *aisc_get_m_id(const char *path, char **m_name, int *id) {
    // Warning: duplicated in client.c@aisc_client_get_m_id
    if (!path) {
        return "OPEN_ARB_DB_CLIENT ERROR: missing hostname:socketid";
    }
    if (!strcmp(path, ":")) {
        path = (char *)getenv("SOCKET");
        if (!path) return "environment socket not found";
    }

    const char *p = strchr(path, ':');
    if (path[0] == '*' || path[0] == ':') {     // UNIX MODE
        char buffer[128];
        if (!p) {
            return "OPEN_ARB_DB_CLIENT ERROR: missing ':' in *:socketid";
        }
        if (p[1] == '~') {
            sprintf(buffer, "%s%s", getenv("HOME"), p+2);
            *m_name = (char *)strdup(buffer);
        }
        else {
            *m_name = (char *)strdup(p+1);
        }
        *id = -1;
        return 0;
    }
    if (!p) {
        return "OPEN_ARB_DB_CLIENT ERROR: missing ':' in netname:socketid";
    }

    char *mn = (char *) calloc(sizeof(char), p - path + 1);
    strncpy(mn, path, p - path);

    if (strcmp(mn, "localhost") == 0) freedup(mn, arb_gethostname());
    *m_name = mn;

    int i = atoi(p + 1);
    if ((i < 1024) || (i > 4096)) {
        return "OPEN_ARB_DB_CLIENT ERROR: socketnumber not in [1024..4095]";
    }
    *id = i;
    return 0;
}

const char *aisc_client_open_socket(const char *path, int delay, int do_connect, int *psocket, char **unix_name) {
    struct in_addr  addr;                                     // union -> u_long
    struct hostent *he;
    const char     *err;
    int             socket_id;
    char           *mach_name = 0;
    FILE           *test;

    err = aisc_client_get_m_id(path, &mach_name, &socket_id);

    // @@@ mem assigned to mach_name is leaked often
    // @@@ refactor aisc_client_open_socket -> one exit point
    // @@@ note that the code is nearly duplicated in server.c@aisc_open_socket

    if (err) {
        if (mach_name) free(mach_name);
        return err;
    }
    if (socket_id >= 0) {       // UNIX
        sockaddr_in so_ad;
        memset((char *)&so_ad, 0, sizeof(sockaddr_in));
        *psocket = socket(PF_INET, SOCK_STREAM, 0);
        if (*psocket <= 0) {
            return "CANNOT CREATE SOCKET";
        }
        arb_gethostbyname(mach_name, he, err);
        if (err) {
            free(mach_name);
            return err;
        }
        // simply take first address
        free(mach_name); mach_name = 0;
        addr.s_addr = *(int *) (he->h_addr);
        so_ad.sin_addr = addr;
        so_ad.sin_family = AF_INET;
        so_ad.sin_port = htons(socket_id);      // @@@ = pb_socket
        if (do_connect) {
            if (connect(*psocket, (struct sockaddr *)&so_ad, 16)) {
                return "";
            }
        }
        else {
            char one = 1;
            setsockopt(*psocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
            if (bind(*psocket, (struct sockaddr *)&so_ad, 16)) {
                return "Could not open socket on Server";
            }
        }
        if (delay == TCP_NODELAY) {
            static int      optval;
            optval = 1;
            setsockopt(*psocket, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, 4);
        }
        *unix_name = 0;
        return 0;
    }
    else {
        struct sockaddr_un so_ad;
        *psocket = socket(PF_UNIX, SOCK_STREAM, 0);
        if (*psocket <= 0) {
            return "CANNOT CREATE SOCKET";
        }
        so_ad.sun_family = AF_UNIX;
        strcpy(so_ad.sun_path, mach_name);
        if (do_connect) {
            if (connect(*psocket, (struct sockaddr*)&so_ad, strlen(mach_name)+2)) {
                return "";
            }
        }
        else {
            test = fopen(mach_name, "r");
            if (test) {
                struct stat stt;
                fclose(test);
                if (!stat(path, &stt)) {
                    if (S_ISREG(stt.st_mode)) {
                        fprintf(stderr, "%X\n", (unsigned int)stt.st_mode);
                        return "Socket already exists as a file";
                    }
                }
            }
            if (unlink(mach_name)) {
                ;
            }
            else {
                printf("old socket found\n");
            }
            if (bind(*psocket, (struct sockaddr*)&so_ad, strlen(mach_name)+2)) {
                return "Could not open socket on Server";
            }
        }
        *unix_name = mach_name;
        return 0;
    }
}

const char *aisc_open_socket(const char *path, int delay, int do_connect, int *psocket, char **unix_name) {
    struct in_addr  addr;       // union -> u_long
    struct hostent *he;
    const char     *err;
    static int      socket_id;
    char           *mach_name = NULL;
    FILE           *test;

    err = aisc_get_m_id(path, &mach_name, &socket_id);

    // @@@ mem assigned to mach_name is leaked often
    // @@@ refactor aisc_open_socket -> one exit point
    // @@@ note that the code is nearly duplicated in client.c@aisc_client_open_socket
    // @@@ a good place for DRYed code would be ../../CORE/arb_cs.cxx

    if (err) {
        return err;
    }
    if (socket_id >= 0) {       // UNIX
        sockaddr_in so_ad;
        memset((char *)&so_ad, 0, sizeof(sockaddr_in));
        *psocket = socket(PF_INET, SOCK_STREAM, 0);
        if (*psocket <= 0) {
            return "CANNOT CREATE SOCKET";
        }

        arb_gethostbyname(mach_name, he, err);
        if (err) return err;

        // simply take first address
        addr.s_addr = *(int *) (he->h_addr);
        so_ad.sin_addr = addr;
        so_ad.sin_family = AF_INET;
        so_ad.sin_port = htons(socket_id);      // @@@ = pb_socket
        if (do_connect) {
            if (connect(*psocket, (struct sockaddr*)&so_ad, 16)) {
                return "";
            }
        }
        else {
            static int one = 1;
            setsockopt(*psocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
            if (bind(*psocket, (struct sockaddr*)&so_ad, 16)) {
                return "Could not open socket on Server (1)";
            }
        }
        if (delay == TCP_NODELAY) {
            static int      optval;
            optval = 1;
            setsockopt(*psocket, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, 4);
        }
        *unix_name = 0;
    }
    else {
        struct sockaddr_un so_ad;
        *psocket = socket(PF_UNIX, SOCK_STREAM, 0);
        if (*psocket <= 0) {
            return "CANNOT CREATE SOCKET";
        }
        so_ad.sun_family = AF_UNIX;
        strcpy(so_ad.sun_path, mach_name);
        if (do_connect) {
            if (connect(*psocket, (struct sockaddr*)&so_ad, strlen(mach_name)+2)) {
                return "";
            }
        }
        else {
            static int one = 1;
            test = fopen(mach_name, "r");
            if (test) {
                struct stat stt;
                fclose(test);
                if (!stat(path, &stt)) {
                    if (S_ISREG(stt.st_mode)) {
                        fprintf(stderr, "%X\n", stt.st_mode);
                        return "Socket already exists as a file";
                    }
                }
            }
            if (unlink(mach_name)) {
                ;
            }
            else {
                printf("old socket found\n");
            }
            setsockopt(*psocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&one, sizeof(one));
            if (bind(*psocket, (struct sockaddr*)&so_ad, strlen(mach_name)+2)) {
                return "Could not open socket on Server (2)";
            }
            if (chmod(mach_name, 0777)) return "Cannot change mode of socket";
        }

        reassign(*unix_name, mach_name);
    }
    free(mach_name);
    return 0;
}



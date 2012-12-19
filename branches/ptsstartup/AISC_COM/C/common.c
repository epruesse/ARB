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
#include <arb_msg.h>
#include <arb_assert.h>

#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

enum ClientOrServer { AISC_SERVER, AISC_CLIENT };

inline const char *who(ClientOrServer cos) {
    switch (cos) {
        case AISC_SERVER: return "AISC_SERVER";
        case AISC_CLIENT: return "AISC_CLIENT";
    }
}

static GB_ERROR common_get_m_id(ClientOrServer cos, const char *path, char **m_name, int *id) {
    // @@@ one exit point; prepare error message there!

    if (!path) {
        return GBS_global_string("%s ERROR: missing hostname:socketid", who(cos)); // @@@ 
    }
    if (!strcmp(path, ":")) {
        path = getenv("SOCKET");
        if (!path) return "environment socket not found"; // @@@ 
    }

    const char *p = strchr(path, ':');
    if (path[0] == '*' || path[0] == ':') {     // UNIX MODE
        char buffer[128];
        if (!p) {
            return GBS_global_string("%s ERROR: missing ':' in *:socketid", who(cos)); // @@@ 
        }
        if (p[1] == '~') {
            sprintf(buffer, "%s%s", getenv("HOME"), p+2);
            *m_name = (char *)strdup(buffer);
        }
        else {
            *m_name = (char *)strdup(p+1);
        }
        *id = -1;
        return 0; // @@@ 
    }
    if (!p) {
        return "OPEN_ARB_DB_CLIENT ERROR: missing ':' in netname:socketid"; // @@@ 
    }

    char *mn = (char *) calloc(sizeof(char), p - path + 1);
    strncpy(mn, path, p - path);

    /* @@@ falls hier in mn ein der Bereich von path bis p stehen soll, fehlt eine abschliesende 0 am String-Ende
       auf jeden Fall erzeugt der folgende strcmp einen (rui) */

    if (strcmp(mn, "localhost") == 0) freedup(mn, arb_gethostname());

    *m_name = mn;
    int i   = atoi(p + 1);
    if ((i < 1024) || (i > 32000)) {
        return "OPEN_ARB_DB_CLIENT ERROR: socketnumber not in [1024..32000]"; // @@@ 
    }
    *id = i;
    return 0;
}

static GB_ERROR common_open_socket(ClientOrServer cos, const char *path, int delay, int do_connect, int *psocket, char **unix_name) {
    struct in_addr  addr; // union -> u_long
    struct hostent *he;
    int             socket_id;
    char           *mach_name = NULL;
    FILE           *test;

    GB_ERROR error = common_get_m_id(cos, path, &mach_name, &socket_id);

    // @@@ mem assigned to mach_name is leaked often
    // @@@ refactor aisc_client_open_socket -> one exit point

    if (!error) {
        const char one = 1;
        if (socket_id >= 0) {       // UNIX
            sockaddr_in so_ad;
            memset((char *)&so_ad, 0, sizeof(sockaddr_in));
            *psocket = socket(PF_INET, SOCK_STREAM, 0);
            if (*psocket <= 0) {
                return "CANNOT CREATE SOCKET"; // @@@ 
            }

            arb_gethostbyname(mach_name, he, error);
            if (error) {
                free(mach_name);
                return error; // @@@ 
            }
            // simply take first address
            addr.s_addr = *(int *) (he->h_addr);
            so_ad.sin_addr = addr;
            so_ad.sin_family = AF_INET;
            so_ad.sin_port = htons(socket_id);      // @@@ = pb_socket
            if (do_connect) {
                if (connect(*psocket, (struct sockaddr*)&so_ad, 16)) {
                    return ""; // @@@ 
                }
            }
            else {
                setsockopt(*psocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
                if (bind(*psocket, (struct sockaddr*)&so_ad, 16)) {
                    return "Could not open socket on Server (1)"; // @@@ 
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
                return "CANNOT CREATE SOCKET"; // @@@ 
            }
            so_ad.sun_family = AF_UNIX;
            strcpy(so_ad.sun_path, mach_name);
            if (do_connect) {
                if (connect(*psocket, (struct sockaddr*)&so_ad, strlen(mach_name)+2)) {
                    return ""; // @@@ 
                }
            }
            else {
                test = fopen(mach_name, "r");
                if (test) {
                    struct stat stt;
                    fclose(test);
                    if (!stat(path, &stt)) {
                        if (S_ISREG(stt.st_mode)) {
                            fprintf(stderr, "%X\n", stt.st_mode);
                            return "Socket already exists as a file"; // @@@ 
                        }
                    }
                }
                if (unlink(mach_name)) {
                    ;
                }
                else {
                    printf("old socket found\n");
                }
                if (cos == AISC_SERVER) {
                    setsockopt(*psocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
                }
                if (bind(*psocket, (struct sockaddr*)&so_ad, strlen(mach_name)+2)) {
                    return "Could not open socket on Server (2)"; // @@@ 
                }
                if (cos == AISC_SERVER) {
                    if (chmod(mach_name, 0777)) return "Cannot change mode of socket"; // @@@ 
                }
            }

            reassign(*unix_name, mach_name);
        }
        arb_assert(!error);
    }
    free(mach_name);
    return error;
}

const char *aisc_client_open_socket(const char *path, int delay, int do_connect, int *psocket, char **unix_name) {
    return common_open_socket(AISC_CLIENT, path, delay, do_connect, psocket, unix_name);
}
const char *aisc_server_open_socket(const char *path, int delay, int do_connect, int *psocket, char **unix_name) { // @@@ rename into aisc_server_open_socket
    return common_open_socket(AISC_SERVER, path, delay, do_connect, psocket, unix_name);
}


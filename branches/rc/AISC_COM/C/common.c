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
    arb_assert(0);
    return "<unknown>";
}

static GB_ERROR common_get_m_id(const char *path, char **m_name, int *id) {
    GB_ERROR error = NULL;
    if (!path) {
        error = "missing hostname:socketid";
    }
    else {
        if (strcmp(path, ":") == 0) {
            path = getenv("SOCKET");
            if (!path) {
                error = "expected environment variable 'SOCKET' (needed to connect to ':')";
            }
        }

        if (!error) {
            const char *p = strchr(path, ':');
            if (path[0] == '*' || path[0] == ':') {     // UNIX MODE
                char buffer[128];
                if (!p) {
                    error = "missing ':' in *:socketid";
                }
                else {
                    if (p[1] == '~') {
                        sprintf(buffer, "%s%s", getenv("HOME"), p+2);
                        *m_name = (char *)strdup(buffer);
                    }
                    else {
                        *m_name = (char *)strdup(p+1);
                    }
                    *id = -1;
                }
            }
            else {
                if (!p) {
                    error = "missing ':' in netname:socketid";
                }
                else {
                    char *mn = (char *) calloc(sizeof(char), p - path + 1);
                    strncpy(mn, path, p - path);

                    /* @@@ falls hier in mn ein der Bereich von path bis p stehen soll, fehlt eine abschliesende 0 am String-Ende
                       auf jeden Fall erzeugt der folgende strcmp einen (rui) */

                    if (strcmp(mn, "localhost") == 0) freedup(mn, arb_gethostname());

                    *m_name = mn;
                    int i   = atoi(p + 1);
                    if ((i < 1024) || (i > 32000)) {
                        error = "socketnumber is not in range [1024..32000]";
                    }
                    else {
                        *id = i;
                    }
                }
            }
        }
    }
    return error;
}

#define SOCKET_EXISTS_AND_CONNECT_WORKED ""

static GB_ERROR common_open_socket(ClientOrServer cos, const char *path, int delay, int do_connect, int *psocket, char **unix_name) {
    char     *mach_name = NULL;
    int       socket_id;
    GB_ERROR  error     = common_get_m_id(path, &mach_name, &socket_id);

    if (!error) {
        const char one = 1;
        if (socket_id >= 0) {       // UNIX
            sockaddr_in so_ad;
            memset((char *)&so_ad, 0, sizeof(sockaddr_in));
            *psocket = socket(PF_INET, SOCK_STREAM, 0);
            if (*psocket <= 0) {
                error = "failed to create socket"; // @@@ reason why
            }
            else {
                struct hostent *he;
                arb_gethostbyname(mach_name, he, error);
                if (!error) {
                    // simply take first address
                    struct in_addr addr;
                    addr.s_addr      = *(int *) (he->h_addr);
                    so_ad.sin_addr   = addr;
                    so_ad.sin_family = AF_INET;
                    so_ad.sin_port   = htons(socket_id);    // @@@ = pb_socket
                    if (do_connect) {
                        if (connect(*psocket, (struct sockaddr*)&so_ad, 16)) {
                            error = SOCKET_EXISTS_AND_CONNECT_WORKED;
                        }
                    }
                    else {
                        setsockopt(*psocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
                        if (bind(*psocket, (struct sockaddr*)&so_ad, 16)) {
                            error = "Could not open socket on Server (1)";
                        }
                    }
                    if (!error) {
                        if (delay == TCP_NODELAY) {
                            static int optval;
                            optval = 1;
                            setsockopt(*psocket, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, 4);
                        }
                        *unix_name = 0;
                    }
                }
            }
        }
        else {
            struct sockaddr_un so_ad;
            *psocket = socket(PF_UNIX, SOCK_STREAM, 0);
            if (*psocket <= 0) {
                error = "cannot create socket"; // @@@ reason why
            }
            else {
                so_ad.sun_family = AF_UNIX;
                strcpy(so_ad.sun_path, mach_name);
                if (do_connect) {
                    if (connect(*psocket, (struct sockaddr*)&so_ad, strlen(mach_name)+2)) {
                        error = SOCKET_EXISTS_AND_CONNECT_WORKED;
                    }
                }
                else {
                    FILE *test = fopen(mach_name, "r");
                    if (test) {
                        struct stat stt;
                        if (!stat(path, &stt)) {
                            if (S_ISREG(stt.st_mode)) {
                                fprintf(stderr, "%X\n", stt.st_mode);
                                error = "Socket already exists as a file";
                            }
                        }
                        fclose(test);
                    }

                    if (!error) {
                        if (unlink(mach_name) != 0) {
                            printf("Warning: old socket file '%s' failed to unlink\n", mach_name);
                        }
                        if (cos == AISC_SERVER) {
                            setsockopt(*psocket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
                        }
                        if (bind(*psocket, (struct sockaddr*)&so_ad, strlen(mach_name)+2)) {
                            error = "Could not open socket on Server (2)"; // @@@ reasons ?
                        }
                        if (!error && cos == AISC_SERVER) {
                            if (chmod(mach_name, 0777)) {
                                error = "Cannot change mode of socket"; // @@@ reasons!
                            }
                        }
                    }
                }

                if (!error) reassign(*unix_name, mach_name);
            }
        }
    }

    free(mach_name);

    if (error && *error) { // real error (see SOCKET_EXISTS_AND_CONNECT_WORKED)
        error = GBS_global_string("%s-Error: %s", who(cos), error);
    }
    return error;
}

const char *aisc_client_open_socket(const char *path, int delay, int do_connect, int *psocket, char **unix_name) {
    return common_open_socket(AISC_CLIENT, path, delay, do_connect, psocket, unix_name);
}
const char *aisc_server_open_socket(const char *path, int delay, int do_connect, int *psocket, char **unix_name) { // @@@ rename into aisc_server_open_socket
    return common_open_socket(AISC_SERVER, path, delay, do_connect, psocket, unix_name);
}


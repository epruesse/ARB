/* =============================================================== */
/*                                                                 */
/*   File      : client.c                                          */
/*   Purpose   :                                                   */
/*                                                                 */
/*   Institute of Microbiology (Technical University Munich)       */
/*   http://www.arb-home.de/                                       */
/*                                                                 */
/* =============================================================== */

#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <unistd.h>
#include <cstdarg>

#include "client_privat.h"
#include "client.h"

#include "trace.h"

#define aisc_assert(cond) arb_assert(cond)

/* AISC_MKPT_PROMOTE:#include <client_types.h> */

#define AISC_MAGIC_NUMBER_FILTER 0xffffff00

static const char *err_connection_problems = "CONNECTION PROBLEMS";

int   aisc_core_on_error = 1;
char *aisc_error;

#define CORE()                                                  \
    do {                                                        \
        if (aisc_core_on_error) {                               \
            ARB_SIGSEGV(true);                                  \
        }                                                       \
    } while (0)

aisc_com *aisc_client_link;

int aisc_print_error_to_stderr = 1;
static char errbuf[300];

#define PRTERR(msg) if (aisc_print_error_to_stderr) fprintf(stderr, "%s: %s\n", msg, link->error);


int aisc_c_read(int socket, char *ptr, long size)
{
    long        leftsize, readsize;
    leftsize = size;
    readsize = 0;
    while (leftsize) {
        readsize = read(socket, ptr, (size_t)leftsize);
        if (readsize<=0) return 0;
        ptr += readsize;
        leftsize -= readsize;
    }
    return size;
}

int aisc_c_write(int socket, char *ptr, int size)
{
    int leftsize, writesize;
    leftsize = size;
    writesize = 0;
    while (leftsize) {
        writesize = write(socket, ptr, leftsize);
        if (writesize<=0) return 0;
        ptr += writesize;
        leftsize -= writesize;
    }
    return size;
}

/* ----------------------------- */
/*      bytestring handling      */


static void aisc_c_add_to_bytes_queue(aisc_com *link, char *data, int size)
{
    aisc_bytes_list *bl = (aisc_bytes_list *)calloc(sizeof(*bl), 1);
#ifndef NDEBUG
    memset(bl, 0, sizeof(*bl)); /* @@@ clear mem needed to avoid (rui's) */
#endif
    bl->data = data;
    bl->size = size;

    if (link->aisc_client_bytes_first) {
        link->aisc_client_bytes_last->next = bl;
        link->aisc_client_bytes_last = bl;
    }
    else {
        link->aisc_client_bytes_first = bl;
        link->aisc_client_bytes_last = bl;
    }
}

int aisc_c_send_bytes_queue(aisc_com    *link)
{
    int len;
    aisc_bytes_list *bl, *bl_next;
    for (bl = link->aisc_client_bytes_first; bl; bl=bl_next) {
        bl_next = bl->next;
        len = aisc_c_write(link->socket, (char *)bl->data, bl->size);
        free(bl);
        if (len<0)return 1;
    };
    link->aisc_client_bytes_first = link->aisc_client_bytes_last = NULL;
    return 0;
}

/* -------------------------- */
/*      message handling      */

struct client_msg_queue {
    client_msg_queue *next;
    int               message_type;
    char             *message;
};

int aisc_add_message_queue(aisc_com *link, long size)
{
    client_msg_queue *msg    = (client_msg_queue *) calloc(sizeof(client_msg_queue), 1);
    char             *buffer = (char *)calloc(sizeof(char), (size_t)size);
    long              len    = aisc_c_read(link->socket, buffer, size);

    if (len != size) {
        link->error = err_connection_problems;
        PRTERR("AISC_ERROR");
        return 1;
    }
    msg->message = strdup(buffer+sizeof(long));
    msg->message_type = (int)*(long *)buffer;
    free(buffer);

    if (link->message_queue == 0) {
        link->message_queue = (int *)msg;
    }
    else {
        client_msg_queue *mp;
        for (mp = (client_msg_queue *) link->message_queue; mp->next; mp=mp->next) ;
        mp->next = msg;
    }
    return 0;
}

int aisc_check_error(aisc_com * link)
{
    int         len;
    long        magic_number;
    long        size;

 aisc_check_next :

    link->error = 0; /* @@@ avoid (rui) */
    len = aisc_c_read(link->socket, (char *)(link->aisc_mes_buffer), 2*sizeof(long));
    if (len != 2*sizeof(long)) {
        link->error = err_connection_problems;
        PRTERR("AISC_ERROR");
        return 1;
    }
    if (link->aisc_mes_buffer[0] >= AISC_MESSAGE_BUFFER_LEN) {
        link->error = err_connection_problems;
        PRTERR("AISC_ERROR");
        return 1;
    }
    magic_number = link->aisc_mes_buffer[1];
    if ((unsigned long)(magic_number & AISC_MAGIC_NUMBER_FILTER) != (unsigned long)(link->magic & AISC_MAGIC_NUMBER_FILTER)) {
        link->error = err_connection_problems;
        PRTERR("AISC_ERROR");
        return 1;
    }
    size = link->aisc_mes_buffer[0];
    if (size) {
        if (magic_number-link->magic == AISC_CCOM_MESSAGE) {
            if (aisc_add_message_queue(link, size*sizeof(long))) return 1;
            goto aisc_check_next;

        }
        len = aisc_c_read(link->socket, (char *)(link->aisc_mes_buffer), size * sizeof(long));
        if (len != (long)(size*sizeof(long))) {
            link->error = err_connection_problems;
            PRTERR("AISC_ERROR");
            return 1;
        }
        switch (magic_number-link->magic) {
            case AISC_CCOM_OK:
                return 0;
            case AISC_CCOM_ERROR:
                sprintf(errbuf, "SERVER_ERROR %s", (char *)(link->aisc_mes_buffer));
                link->error = errbuf;
                PRTERR("AISC_ERROR");
                CORE();
                return 1;
            default:
                return 0;
        }
    }
    return 0;
}

char *aisc_client_get_hostname() {
    static char *hn = 0;
    if (!hn) {
        char buffer[4096];
        gethostname(buffer, 4095);
        hn = strdup(buffer);
    }
    return hn;
}

const char *aisc_client_get_m_id(const char *path, char **m_name, int *id) {
    char           *p;
    char           *mn;
    int             i;
    if (!path) {
        return "AISC_CLIENT_OPEN ERROR: missing hostname:socketid";
    }
    if (!strcmp(path, ":")) {
        path = (char *)getenv("SOCKET");
        if (!path) return "environment socket not found";
    }
    p = (char *) strchr(path, ':');
    if (path[0] == '*' || path[0] == ':') {     /* UNIX MODE */
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

    if (!strcmp(mn, "localhost")) {
        free(mn);
        mn = strdup(aisc_client_get_hostname());
    }
    *m_name = mn;
    i = atoi(p + 1);
    if ((i < 1024) || (i > 32000)) {
        return "OPEN_ARB_DB_CLIENT ERROR: socketnumber not in [1024..32000]";
    }
    *id = i;
    return 0;
}

static const char *aisc_client_open_socket(const char *path, int delay, int do_connect, int *psocket, char **unix_name) {
    char            buffer[128];
    struct in_addr  addr;                                     /* union -> u_long  */
    struct hostent *he;
    const char     *err;
    int             socket_id;
    char           *mach_name = 0;
    FILE           *test;

    err = aisc_client_get_m_id(path, &mach_name, &socket_id);
    if (err) {
        if (mach_name) free(mach_name);
        return err;
    }
    if (socket_id >= 0) {       /* UNIX */
        sockaddr_in so_ad;
        memset((char *)&so_ad, 0, sizeof(sockaddr_in));
        *psocket = socket(PF_INET, SOCK_STREAM, 0);
        if (*psocket <= 0) {
            return "CANNOT CREATE SOCKET";
        }
        if (!(he = gethostbyname(mach_name))) {
            sprintf(buffer, "Unknown host: %s", mach_name);
            free(mach_name);
            return (char *)strdup(buffer);
        }
        /* simply take first address  */
        free(mach_name); mach_name = 0;
        addr.s_addr = *(int *) (he->h_addr);
        so_ad.sin_addr = addr;
        so_ad.sin_family = AF_INET;
        so_ad.sin_port = htons(socket_id);      /* @@@ = pb_socket  */
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

void *aisc_init_client(aisc_com *link)
{
    int len, mes_cnt;
    mes_cnt = 2;
    link->aisc_mes_buffer[0] = mes_cnt-2;
    link->aisc_mes_buffer[1] = AISC_INIT+link->magic;
    len = aisc_c_write(link->socket, (char *)(link->aisc_mes_buffer), mes_cnt * sizeof(long));
    if (!len) {
        link->error = err_connection_problems;
        PRTERR("AISC_CONN_ERROR");
        return 0;
    }
    aisc_check_error(link);
    return (void *)(link->aisc_mes_buffer[0]);
}

static void ignore_sigpipe(int) {
}

static void aisc_free_link(aisc_com *link) {
    if (link->old_sigpipe_handler != SIG_ERR) { // failed to install -> do not uninstall
        UNINSTALL_SIGHANDLER(SIGPIPE, ignore_sigpipe, link->old_sigpipe_handler, "aisc_free_link");
    }
    free(link);
}

extern "C" aisc_com *aisc_open(const char *path, long *mgr, long magic) {
    aisc_com *link;
    const char *err;

    link = (aisc_com *) calloc(sizeof(aisc_com), 1);
    link->aisc_client_bytes_first = link->aisc_client_bytes_last = NULL;
    link->magic = magic;
    {
        static char *unix_name = 0;
        err = aisc_client_open_socket(path, TCP_NODELAY, 1, &link->socket, &unix_name);
        freenull(unix_name);
    }
    if (err) {
        free(link);
        if (*err) PRTERR("ARB_DB_CLIENT_OPEN");
        return 0;
    }

    link->old_sigpipe_handler = INSTALL_SIGHANDLER(SIGPIPE, ignore_sigpipe, "aisc_open");

    *mgr = 0;
    *mgr = (long)aisc_init_client(link);
    if (!*mgr || link->error) {
        aisc_free_link(link);
        return 0;
    }
    aisc_client_link = link;
    return link;
}

extern "C" int aisc_close(aisc_com *link) {
    if (link) {
        if (link->socket) {
            link->aisc_mes_buffer[0] = 0;
            link->aisc_mes_buffer[1] = 0;
            link->aisc_mes_buffer[2] = 0;
            aisc_c_write(link->socket, (char *)(link->aisc_mes_buffer), 3 * sizeof(long));
            shutdown(link->socket, 2);
            close(link->socket);
            link->socket = 0;
        }
        aisc_free_link(link);
    }
    return 0;
}


int aisc_get_message(aisc_com *link)
{
    int anz, len;
    long        size, magic_number;
    struct timeval timeout;
    fd_set set;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    FD_ZERO (&set);
    FD_SET (link->socket, &set);

    if (link->message) free(link->message);
    link->message_type = 0;
    link->message = 0;

    anz = select(FD_SETSIZE, &set, NULL, NULL, &timeout);

    if (anz) {
        len = aisc_c_read(link->socket, (char *)(link->aisc_mes_buffer), 2*sizeof(long));
        if (len != 2*sizeof(long)) {
            link->error = err_connection_problems;
            PRTERR("AISC_ERROR");
            return -1;
        }
        if (!link->aisc_mes_buffer[0]) {
            link->error = err_connection_problems;
            PRTERR("AISC_ERROR");
            return -1;
        }
        magic_number = link->aisc_mes_buffer[1]-link->magic;
        if (magic_number != AISC_CCOM_MESSAGE) {
            link->error = err_connection_problems;
            PRTERR("AISC_ERROR");
            return -1;
        }
        size = link->aisc_mes_buffer[0];
        if (aisc_add_message_queue(link, size*sizeof(long))) return -1;
    }
    if (link->message_queue) {
        client_msg_queue *msg = (client_msg_queue *)link->message_queue;
        link->message_queue = (int *)msg->next;
        link->message = msg->message;
        link->message_type = msg->message_type;
        free(msg);
        return link->message_type;
    }
    return 0;
}



int aisc_get(aisc_com *link, int o_type, long object, ...)
{
    /* goes to header: __ATTR__SENTINEL  */
    long    *arg_pntr[MAX_AISC_SET_GET];
    long     arg_types[MAX_AISC_SET_GET];
    long     mes_cnt;
    long     arg_cnt;
    va_list  parg;
    long     type, o_t;
    long     attribute, code;
    long     count;
    long     i, len;
    long     size;

    AISC_DUMP_SEP();

    mes_cnt = 2;
    arg_cnt = 0;
    count = 4;
    va_start(parg, object);
    link->aisc_mes_buffer[mes_cnt++] = object;
    while ((code=va_arg(parg, long))) {
        attribute       = code & AISC_ATTR_MASK;
        type            = code & AISC_VAR_TYPE_MASK;
        o_t             = code & AISC_OBJ_TYPE_MASK;

        if ((o_t != (int)o_type)) {
            sprintf(errbuf, "ARG NR %li DON'T FIT OBJECT", count);
            link->error = errbuf;
            PRTERR("AISC_GET_ERROR");
            CORE();
            return      1;
        };

        if ((attribute > AISC_MAX_ATTR)) {
            sprintf(errbuf, "ARG %li IS NOT AN ATTRIBUTE_TYPE", count);
            link->error = errbuf;
            PRTERR("AISC_GET_ERROR");
            CORE();
            return      1;
        };
        link->aisc_mes_buffer[mes_cnt++] = code;
        arg_pntr[arg_cnt] = va_arg(parg, long *);
        arg_types[arg_cnt++] = type;
        count += 2;
        if (arg_cnt>=MAX_AISC_SET_GET) {
            sprintf(errbuf, "TOO MANY ARGS (>%i)", MAX_AISC_SET_GET);
            link->error = errbuf;
            PRTERR("AISC_GET_ERROR");
            CORE();
            return      1;
        }
    }
    va_end(parg);
    if (mes_cnt > 3) {
        link->aisc_mes_buffer[0] = mes_cnt - 2;
        link->aisc_mes_buffer[1] = AISC_GET+link->magic;
        len = aisc_c_write(link->socket, (char *)(link->aisc_mes_buffer),
                           (size_t)(mes_cnt * sizeof(long)));
        if (!len) {
            link->error = err_connection_problems;
            PRTERR("AISC_GET_ERROR");
            return 1;
        }

        if (aisc_check_error(link)) return 1;
        mes_cnt = 0;
        for (i=0; i<arg_cnt; i++) {
            switch (arg_types[i]) {
                case AISC_TYPE_INT:
                case AISC_TYPE_COMMON:
                    AISC_DUMP(aisc_get, int, link->aisc_mes_buffer[mes_cnt]);
                    arg_pntr[i][0] = link->aisc_mes_buffer[mes_cnt++];
                    break;
                case AISC_TYPE_DOUBLE:
                    AISC_DUMP(aisc_get, double, *(double*)&(link->aisc_mes_buffer[mes_cnt]));
                    ((int*)arg_pntr[i])[0] = (int)(link->aisc_mes_buffer[mes_cnt++]);
                    ((int*)arg_pntr[i])[1] = (int)(link->aisc_mes_buffer[mes_cnt++]);
                    break;
                case AISC_TYPE_STRING: {
                    char *str       = strdup((char *)(&(link->aisc_mes_buffer[mes_cnt+1])));
                    AISC_DUMP(aisc_get, charPtr, str);
                    arg_pntr[i][0]  = (long)str;
                    mes_cnt        += link->aisc_mes_buffer[mes_cnt] + 1;
                    break;
                }
                case AISC_TYPE_BYTES:
                    size = arg_pntr[i][1] = link->aisc_mes_buffer[mes_cnt++];
                    AISC_DUMP(aisc_get, int, size);
                    if (size) {
                        arg_pntr[i][0] = (long)calloc(sizeof(char), (size_t)size);
                        len = aisc_c_read(link->socket, (char *)(arg_pntr[i][0]), size);
                        if (size!=len) {
                            link->error = err_connection_problems;
                            PRTERR("AISC_GET_ERROR");
                        }
#if defined(DUMP_COMMUNICATION)
                        aisc_dump_hex("aisc_get bytestring: ", (char *)(arg_pntr[i][0]), size);
#endif /* DUMP_COMMUNICATION */
                    }
                    else {
                        arg_pntr[i][0] = 0;
                    }
                    break;

                default:
                    link->error = "UNKNOWN TYPE";
                    PRTERR("AISC_GET_ERROR");
                    CORE();
                    return 1;
            }
        }

    }
    return 0;
}

long *aisc_debug_info(aisc_com *link, int o_type, long object, int attribute)
{
    int mes_cnt;
    int o_t;
    int len;

    mes_cnt = 2;
    o_t     = attribute & AISC_OBJ_TYPE_MASK;
    if ((o_t != (int)o_type)) {
        link->error = "ATTRIBUTE DON'T FIT OBJECT";
        PRTERR("AISC_DEBUG_ERROR");
        CORE();
        return 0;
    };
    attribute = attribute&0xffff;
    if ((attribute > AISC_MAX_ATTR)) {
        link->error = "CLIENT DEBUG NOT CORRECT TYPE";
        PRTERR("AISC_DEBUG_ERROR");
        CORE();
        return 0;
    };
    link->aisc_mes_buffer[mes_cnt++] = object;
    link->aisc_mes_buffer[mes_cnt++] = attribute;
    link->aisc_mes_buffer[0] = mes_cnt - 2;
    link->aisc_mes_buffer[1] = AISC_DEBUG_INFO+link->magic;
    len = aisc_c_write(link->socket, (char *)(link->aisc_mes_buffer), mes_cnt * sizeof(long));
    if (!len) {
        link->error = err_connection_problems;
        PRTERR("AISC_GET_ERROR");
        return 0;
    }
    if (aisc_check_error(link)) return 0;
    return &(link->aisc_mes_buffer[0]);
}


inline char *part_of(const char *str, size_t max_len, size_t str_len) {
    aisc_assert(strlen(str) == str_len);
    char *part;
    if (str_len <= max_len) {
        part = strdup(str);
    }
    else {
        const int DOTS = 3;
        int       copy = max_len-DOTS;

        part = (char*)malloc(max_len+1);
        memcpy(part, str, copy);
        memset(part+copy, '.', DOTS);
        
        part[max_len] = 0;
    }
    return part;
}

static int aisc_collect_sets(aisc_com *link, int mes_cnt, va_list parg, int o_type, int count) {
    double dummy;
    int type, o_t;
    int attribute, code;
    int len, ilen;
    char        *str;
    int arg_cnt = 0;

    AISC_DUMP_SEP();

    while ((code=va_arg(parg, int))) {
        attribute       = code & AISC_ATTR_MASK;
        type            = code & AISC_VAR_TYPE_MASK;
        o_t             = code & AISC_OBJ_TYPE_MASK;

        if (code != AISC_INDEX) {
            if ((o_t != (int)o_type)) {
                sprintf(errbuf, "ATTRIBUTE ARG NR %i DON'T FIT OBJECT", count);
                link->error = errbuf;
                PRTERR("AISC_SET_ERROR");
                CORE();
                return 0;
            }
            if ((attribute > AISC_MAX_ATTR)) {
                sprintf(errbuf, "ARG %i IS NOT AN ATTRIBUTE_TYPE", count);
                link->error = errbuf;
                PRTERR("AISC_SET_ERROR");
                CORE();
                return 0;
            }
        }
        link->aisc_mes_buffer[mes_cnt++] = code;
        switch (type) {
            case AISC_TYPE_INT:
            case AISC_TYPE_COMMON:
                link->aisc_mes_buffer[mes_cnt++] = va_arg(parg, long);
                AISC_DUMP(aisc_collect_sets, int, link->aisc_mes_buffer[mes_cnt-1]);
                break;
            case AISC_TYPE_DOUBLE: {
                int *ptr;
                dummy    = va_arg(parg, double);
                AISC_DUMP(aisc_collect_sets, double, dummy);
                ptr = (int*)&dummy;
                link->aisc_mes_buffer[mes_cnt++] = *ptr++;
                link->aisc_mes_buffer[mes_cnt++] = *ptr;
                break;
            }
            case AISC_TYPE_STRING:
                str = va_arg(parg, char *);
                AISC_DUMP(aisc_collect_sets, charPtr, str);
                len = strlen(str)+1;
                if (len > AISC_MAX_STRING_LEN) {
                    char *strpart = part_of(str, AISC_MAX_STRING_LEN-40, len-1);
                    sprintf(errbuf, "ARG %i: STRING \'%s\' TOO LONG", count+2, strpart);
                    free(strpart);

                    link->error = errbuf;
                    PRTERR("AISC_SET_ERROR");
                    CORE();

                    return 0;
                }
                ilen = (len)/sizeof(long) + 1;
                link->aisc_mes_buffer[mes_cnt++] = ilen;
                memcpy((char *)(&(link->aisc_mes_buffer[mes_cnt])), str, len);
                mes_cnt += ilen;
                break;

            case AISC_TYPE_BYTES:
                {
                    bytestring *bs;
                    bs = va_arg(parg, bytestring *);
                    AISC_DUMP(aisc_collect_sets, int, bs->size);
                    if (bs->data && bs->size) {
                        aisc_c_add_to_bytes_queue(link, bs->data, bs->size);
#if defined(DUMP_COMMUNICATION)
                        aisc_dump_hex("aisc_collect_sets bytestring: ", bs->data, bs->size);
#endif /* DUMP_COMMUNICATION */
                    }
                    link->aisc_mes_buffer[mes_cnt++] = bs->size;              /* size */
                    break;
                }
            default:
                link->error = "UNKNOWN TYPE";
                PRTERR("AISC_SET_ERROR");
                CORE();
                return 0;
        }

        count += 2;
        if ((arg_cnt++) >= MAX_AISC_SET_GET) {
            sprintf(errbuf, "TOO MANY ARGS (>%i)", MAX_AISC_SET_GET);
            link->error = errbuf;
            PRTERR("AISC_SET_ERROR");
            CORE();
            return      0;
        }
    }
    return mes_cnt;
}

int     aisc_put(aisc_com        *link, int o_type, long object, ...)
{
    /* goes to header: __ATTR__SENTINEL  */
    int mes_cnt, arg_cnt;
    va_list     parg;
    int len;
    arg_cnt = mes_cnt = 2;
    link->aisc_mes_buffer[mes_cnt++] = object;
    link->aisc_mes_buffer[mes_cnt++] = o_type;

    va_start(parg, object);
    if (!(mes_cnt = aisc_collect_sets(link, mes_cnt, parg, o_type, 4))) return 1;

    if (mes_cnt > 3) {
        link->aisc_mes_buffer[0] = mes_cnt - 2;
        link->aisc_mes_buffer[1] = AISC_SET+link->magic;
        len = aisc_c_write(link->socket, (char *)(link->aisc_mes_buffer), mes_cnt * sizeof(long));
        if (!len) {
            link->error = err_connection_problems;
            PRTERR("AISC_SET_ERROR");
            return 1;
        }
        if (aisc_c_send_bytes_queue(link)) return 1;
        if (aisc_check_error(link)) return 1;
    }
    return 0;
}

int     aisc_nput(aisc_com        *link, int o_type, long object, ...)
{
    /* goes to header: __ATTR__SENTINEL  */
    int mes_cnt, arg_cnt;
    va_list     parg;
    int len;
    arg_cnt = mes_cnt = 2;
    link->aisc_mes_buffer[mes_cnt++] = object;
    link->aisc_mes_buffer[mes_cnt++] = o_type;

    va_start(parg, object);
    if (!(mes_cnt = aisc_collect_sets(link, mes_cnt, parg, o_type, 4))) {
        return 1;
    }

    if (mes_cnt > 3) {
        link->aisc_mes_buffer[0] = mes_cnt - 2;
        link->aisc_mes_buffer[1] = AISC_NSET+link->magic;
        len = aisc_c_write(link->socket, (char *)(link->aisc_mes_buffer), mes_cnt * sizeof(long));
        if (!len) {
            link->error = err_connection_problems;
            PRTERR("AISC_SET_ERROR");
            return 1;
        }
        if (aisc_c_send_bytes_queue(link)) {
            return 1;
        }
    }
    return 0;
}

int aisc_create(aisc_com *link, int father_type, long father,
                int attribute,  int object_type, long *object, ...)
{
    /* goes to header: __ATTR__SENTINEL  */
    int mes_cnt;
    int len;
    va_list parg;
    *object = 0;
    mes_cnt = 2;
    if ((father_type&0xff00ffff)) {
        link->error = "FATHER_TYPE UNKNOWN";
        PRTERR("AISC_CREATE_ERROR");
        CORE();
        return 1;
    }
    if ((object_type&0xff00ffff)) {
        link->error = "OBJECT_TYPE UNKNOWN";
        PRTERR("AISC_CREATE_ERROR");
        CORE();
        return 1;
    }
    link->aisc_mes_buffer[mes_cnt++] = father_type;
    link->aisc_mes_buffer[mes_cnt++] = father;
    link->aisc_mes_buffer[mes_cnt++] = attribute;
    link->aisc_mes_buffer[mes_cnt++] = object_type;
    if (father_type != (attribute & AISC_OBJ_TYPE_MASK)) {
        link->error = "ATTRIBUTE TYPE DON'T FIT OBJECT";
        PRTERR("AISC_CREATE_ERROR");
        CORE();
        return 1;
    }
    va_start(parg, object);
    if (!(mes_cnt = aisc_collect_sets(link, mes_cnt, parg, object_type, 7))) return 1;
    link->aisc_mes_buffer[0] = mes_cnt - 2;
    link->aisc_mes_buffer[1] = AISC_CREATE+link->magic;
    len = aisc_c_write(link->socket, (char *)(link->aisc_mes_buffer), mes_cnt * sizeof(long));
    if (!len) {
        link->error = err_connection_problems;
        PRTERR("AISC_CREATE_ERROR");
        return 1;
    }
    if (aisc_c_send_bytes_queue(link)) return 1;
    if (aisc_check_error(link)) return 1;
    *object = link->aisc_mes_buffer[0];
    return 0;
}

int aisc_copy(aisc_com *link, int s_type, long source, int father_type,
                long father, int attribute, int object_type, long *object, ...)
{
    /* goes to header: __ATTR__SENTINEL  */
    int mes_cnt;
    int len;
    va_list parg;
    *object = 0;
    if (s_type != object_type) {
        link->error = "OBJECT_TYPE IS DIFFERENT FROM SOURCE_TYPE";
        PRTERR("AISC_COPY_ERROR");
        CORE();
        return 1;
    }

    mes_cnt = 2;
    if ((father_type&0xff00ffff)) {
        link->error = "FATHER UNKNOWN";
        PRTERR("AISC_COPY_ERROR");
        CORE();
        return 1;
    }
    link->aisc_mes_buffer[mes_cnt++] = source;
    link->aisc_mes_buffer[mes_cnt++] = father_type;
    link->aisc_mes_buffer[mes_cnt++] = father;
    link->aisc_mes_buffer[mes_cnt++] = attribute;
    link->aisc_mes_buffer[mes_cnt++] = object_type;
    if (father_type != (attribute & AISC_OBJ_TYPE_MASK)) {
        link->error = "ATTRIBUTE TYPE DON'T FIT OBJECT";
        PRTERR("AISC_COPY_ERROR");
        CORE();
        return 1;
    }
    va_start(parg, object);
    if (!(mes_cnt = aisc_collect_sets(link, mes_cnt, parg, object_type, 9))) return 1;
    link->aisc_mes_buffer[0] = mes_cnt - 2;
    link->aisc_mes_buffer[1] = AISC_COPY+link->magic;
    len = aisc_c_write(link->socket, (char *)(link->aisc_mes_buffer), mes_cnt * sizeof(long));
    if (!len) {
        link->error = err_connection_problems;
        PRTERR("AISC_COPY_ERROR");
        return 1;
    }
    if (aisc_c_send_bytes_queue(link)) return 1;
    if (aisc_check_error(link)) return 1;
    *object = link->aisc_mes_buffer[0];
    return 0;
}



int aisc_delete(aisc_com *link, int object_type, long source)
{
    int len, mes_cnt;

    mes_cnt = 2;
    link->aisc_mes_buffer[mes_cnt++] = object_type;
    link->aisc_mes_buffer[mes_cnt++] = source;
    link->aisc_mes_buffer[0] = mes_cnt - 2;
    link->aisc_mes_buffer[1] = AISC_DELETE+link->magic;
    len = aisc_c_write(link->socket, (char *)(link->aisc_mes_buffer), mes_cnt * sizeof(long));
    if (!len) {
        link->error = err_connection_problems;
        PRTERR("AISC_DELETE_ERROR");
        return 1;
    }
    return aisc_check_error(link);
}


int aisc_find(aisc_com *link,
              int father_type,
              long      father,
              int       attribute,
              int       object_type,
              long      *object,
              char      *ident)
{
    int mes_cnt;
    int i, len;
    char        *to_free = 0;
    object_type = object_type;
    *object = 0;
    mes_cnt = 2;
    if ((father_type&0xff00ffff)) {
        link->error = "FATHER_TYPE UNKNOWN";
        PRTERR("AISC_FIND_ERROR");
        CORE();
        return 1;
    }
    if (father_type != (attribute & AISC_OBJ_TYPE_MASK)) {
        link->error = "ATTRIBUTE TYPE DON'T MACH FATHER";
        PRTERR("AISC_FIND_ERROR");
        CORE();
    }
    link->aisc_mes_buffer[mes_cnt++] = father_type;
    link->aisc_mes_buffer[mes_cnt++] = father;
    link->aisc_mes_buffer[mes_cnt++] = attribute;
    if (!ident) {
        link->error = "IDENT == NULL";
        PRTERR("AISC_FIND_ERROR");
        CORE();
        return 1;
    }
    len = strlen(ident);
    if (len >= 32) {
        sprintf(errbuf, "len(\'%s\') > 32)", ident);
        link->error = errbuf;
        PRTERR("AISC_FIND_ERROR");
        return 1;
    }
    if (((long)ident) & 3) {
        to_free = ident = strdup(ident);
    }
    len = (len+1)/sizeof(long)+1;
    link->aisc_mes_buffer[mes_cnt++] = len;
    for (i=0; i<len; i++) {
        link->aisc_mes_buffer[mes_cnt++] = ((long *)ident)[i];
    }
    link->aisc_mes_buffer[0] = mes_cnt - 2;
    link->aisc_mes_buffer[1] = AISC_FIND+link->magic;
    len = aisc_c_write(link->socket, (char *)(link->aisc_mes_buffer), mes_cnt * sizeof(long));
    if (!len) {
        link->error = err_connection_problems;
        PRTERR("AISC_FIND_ERROR");
        return 1;
    }
    if (aisc_check_error(link)) return 1;
    *object = link->aisc_mes_buffer[0];
    if (to_free) free(to_free);
    return 0;
}

/* --------------------------------------------------------------------------------
 * Note: it's not possible to define unit tests here - they won't execute
 * Instead put your tests into ../../SERVERCNTRL/servercntrl.cxx@UNIT_TESTS
 */



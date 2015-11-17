// ===============================================================
/*                                                                 */
//   File      : client.c
//   Purpose   :
/*                                                                 */
//   Institute of Microbiology (Technical University Munich)
//   http://www.arb-home.de/
/*                                                                 */
// ===============================================================

#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
#include <sys/un.h>

#include <unistd.h>
#include <cstdarg>

#include <arb_cs.h>

#include "client_privat.h"
#include "client.h"

#include "trace.h"

#define aisc_assert(cond) arb_assert(cond)

// AISC_MKPT_PROMOTE:#include <client_types.h>

#define AISC_MAGIC_NUMBER_FILTER 0xffffff00

static const char *err_connection_problems = "CONNECTION PROBLEMS";

int aisc_core_on_error = 1;

#define CORE()                                                  \
    do {                                                        \
        if (aisc_core_on_error) {                               \
            ARB_SIGSEGV(true);                                  \
        }                                                       \
    } while (0)

aisc_com *aisc_client_link;

static int aisc_print_error_to_stderr = 1;
static char errbuf[300];

#define PRTERR(msg) if (aisc_print_error_to_stderr) fprintf(stderr, "%s: %s\n", msg, link->error);

// -----------------------------
//      bytestring handling


static void aisc_c_add_to_bytes_queue(aisc_com *link, char *data, int size)
{
    aisc_bytes_list *bl = (aisc_bytes_list *)calloc(sizeof(*bl), 1);
#ifndef NDEBUG
    memset(bl, 0, sizeof(*bl)); // @@@ clear mem needed to avoid (rui's)
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

static int aisc_c_send_bytes_queue(aisc_com *link) {
    int len;
    aisc_bytes_list *bl, *bl_next;
    for (bl = link->aisc_client_bytes_first; bl; bl=bl_next) {
        bl_next = bl->next;
        len = arb_socket_write(link->socket, bl->data, bl->size);
        free(bl);
        if (len<0)return 1;
    };
    link->aisc_client_bytes_first = link->aisc_client_bytes_last = NULL;
    return 0;
}

// --------------------------
//      message handling

struct client_msg_queue {
    client_msg_queue *next;
    int               message_type;
    char             *message;
};

static int aisc_add_message_queue(aisc_com *link, long size)
{
    client_msg_queue *msg    = (client_msg_queue *) calloc(sizeof(client_msg_queue), 1);
    char             *buffer = (char *)calloc(sizeof(char), (size_t)size);
    long              len    = arb_socket_read(link->socket, buffer, size);

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
        for (mp = (client_msg_queue *)link->message_queue; mp->next; mp=mp->next) ;
        mp->next = msg;
    }
    return 0;
}

static int aisc_check_error(aisc_com * link)
{
    int         len;
    long        magic_number;
    long        size;

 aisc_check_next :

    link->error = 0; // @@@ avoid (rui)
    len = arb_socket_read(link->socket, (char *)(link->aisc_mes_buffer), 2*sizeof(long));
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
        len = arb_socket_read(link->socket, (char *)(link->aisc_mes_buffer), size * sizeof(long));
        if (len != (long)(size*sizeof(long))) {
            link->error = err_connection_problems;
            PRTERR("AISC_ERROR");
            return 1;
        }
        switch (magic_number-link->magic) {
            case AISC_CCOM_OK:
                return 0;
            case AISC_CCOM_ERROR:
                sprintf(errbuf, "SERVER_ERROR: %s", (char *)(link->aisc_mes_buffer));
                link->error = errbuf;
                PRTERR("AISC_ERROR");
                return 1;
            default:
                return 0;
        }
    }
    return 0;
}



static long aisc_init_client(aisc_com *link)
{
    int mes_cnt;
    mes_cnt = 2;
    link->aisc_mes_buffer[0] = mes_cnt-2;
    link->aisc_mes_buffer[1] = AISC_INIT+link->magic;
    if (arb_socket_write(link->socket, (const char *)link->aisc_mes_buffer, mes_cnt * sizeof(long))) {
        link->error = err_connection_problems;
        PRTERR("AISC_CONN_ERROR");
        return 0;
    }
    aisc_check_error(link);
    return link->aisc_mes_buffer[0];
}

static void aisc_free_link(aisc_com *link) {
    free(link);
}

aisc_com *aisc_open(const char *path, AISC_Object& main_obj, long magic, GB_ERROR *error) {
    aisc_com   *link;
    const char *err;

    aisc_assert(error && !*error);
    aisc_assert(!main_obj.exists()); // already initialized

    link = (aisc_com *) calloc(sizeof(aisc_com), 1);
    link->aisc_client_bytes_first = link->aisc_client_bytes_last = NULL;
    link->magic = magic;
    {
        char *unix_name = 0;
        err = arb_open_socket(path, true, &link->socket, &unix_name);
        aisc_assert(link->socket>=0);
        free(unix_name);
    }
    if (err) {
        if (*err) {
            link->error = err;
            PRTERR("AISC");
        }
        if (link->socket) {
            shutdown(link->socket, SHUT_RDWR);
            close(link->socket);
        }
        *error = link->error;
        free(link);
        aisc_assert(!(*error && main_obj.exists()));
        return 0;
    }

    main_obj.init(aisc_init_client(link));
    if (!main_obj.exists() || link->error) {
        *error = link->error;
        main_obj.clear();
        aisc_free_link(link);
        aisc_assert(!(*error && main_obj.exists()));
        return 0;
    }
    aisc_client_link = link;
    aisc_assert(!*error);
    return link;
}

int aisc_close(aisc_com *link, AISC_Object& object) {
    if (link) {
        if (link->socket) {
            link->aisc_mes_buffer[0] = 0;
            link->aisc_mes_buffer[1] = 0;
            link->aisc_mes_buffer[2] = 0;
            arb_socket_write(link->socket, (const char *)link->aisc_mes_buffer, 3 * sizeof(long));
            shutdown(link->socket, SHUT_RDWR);
            close(link->socket);
            link->socket = 0;
        }
        aisc_free_link(link);
    }
    object.clear();
    return 0;
}

int aisc_get(aisc_com *link, int o_type, const AISC_Object& object, ...)
{
    // goes to header: __ATTR__SENTINEL
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
    count   = 4;

    link->aisc_mes_buffer[mes_cnt++] = object.get();
    aisc_assert(object.type() == o_type);

    va_start(parg, object);
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
        if (arb_socket_write(link->socket, (const char *)(link->aisc_mes_buffer), 
                             (size_t)(mes_cnt * sizeof(long)))) {
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
                    AISC_DUMP(aisc_get, double, *(double*)(char*)/*avoid aliasing problems*/&(link->aisc_mes_buffer[mes_cnt]));
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
                        len = arb_socket_read(link->socket, (char *)(arg_pntr[i][0]), size);
                        if (size!=len) {
                            link->error = err_connection_problems;
                            PRTERR("AISC_GET_ERROR");
                        }
#if defined(DUMP_COMMUNICATION)
                        aisc_dump_hex("aisc_get bytestring: ", (char *)(arg_pntr[i][0]), size);
#endif // DUMP_COMMUNICATION
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

long *aisc_debug_info(aisc_com *link, int o_type, const AISC_Object& object, int attribute)
{
    int mes_cnt;
    int o_t;

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
    aisc_assert(object.type() == o_type);
    link->aisc_mes_buffer[mes_cnt++] = object.get();
    link->aisc_mes_buffer[mes_cnt++] = attribute;
    link->aisc_mes_buffer[0] = mes_cnt - 2;
    link->aisc_mes_buffer[1] = AISC_DEBUG_INFO+link->magic;
    if (arb_socket_write(link->socket, (const char *)(link->aisc_mes_buffer), mes_cnt * sizeof(long))) {
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
    int type, o_t; // @@@ fix locals
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
                double_xfer darg;

                darg.as_double = va_arg(parg, double);
                AISC_DUMP(aisc_collect_sets, double, darg.as_double);

                link->aisc_mes_buffer[mes_cnt++] = darg.as_int[0];
                link->aisc_mes_buffer[mes_cnt++] = darg.as_int[1];
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
#endif // DUMP_COMMUNICATION
                    }
                    link->aisc_mes_buffer[mes_cnt++] = bs->size;              // size
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

 // @@@ DRY aisc_put vs aisc_nput
 // @@@ the difference between aisc_put and aisc_nput is: aisc_put may return an error from server

int aisc_put(aisc_com *link, int o_type, const AISC_Object& object, ...) { // goes to header: __ATTR__SENTINEL
    aisc_assert(object.type() == o_type);

    int mes_cnt = 2;
    link->aisc_mes_buffer[mes_cnt++] = object.get();
    link->aisc_mes_buffer[mes_cnt++] = o_type;

    va_list parg;
    va_start(parg, object);
    if (!(mes_cnt = aisc_collect_sets(link, mes_cnt, parg, o_type, 4))) return 1;

    if (mes_cnt > 3) {
        link->aisc_mes_buffer[0] = mes_cnt - 2;
        link->aisc_mes_buffer[1] = AISC_SET+link->magic;
        if (arb_socket_write(link->socket, (const char *)(link->aisc_mes_buffer), mes_cnt * sizeof(long))) {
            link->error = err_connection_problems;
            PRTERR("AISC_SET_ERROR");
            return 1;
        }
        if (aisc_c_send_bytes_queue(link)) return 1;
        if (aisc_check_error(link)) return 1;
    }
    return 0;
}

int aisc_nput(aisc_com *link, int o_type, const AISC_Object& object, ...) { // goes to header: __ATTR__SENTINEL
    aisc_assert(object.type() == o_type);
    
    int mes_cnt = 2;
    link->aisc_mes_buffer[mes_cnt++] = object.get();
    link->aisc_mes_buffer[mes_cnt++] = o_type;

    va_list parg;
    va_start(parg, object);
    if (!(mes_cnt = aisc_collect_sets(link, mes_cnt, parg, o_type, 4))) {
        return 1;
    }

    if (mes_cnt > 3) {
        link->aisc_mes_buffer[0] = mes_cnt - 2;
        link->aisc_mes_buffer[1] = AISC_NSET+link->magic;
        if (arb_socket_write(link->socket, (const char *)(link->aisc_mes_buffer), mes_cnt * sizeof(long))) {
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

int aisc_create(aisc_com *link, int father_type, const AISC_Object& father,
                int attribute,  int object_type, AISC_Object& object, ...)
{
    // goes to header: __ATTR__SENTINEL
    // arguments in '...' set elements of CREATED object (not of father)
    int mes_cnt;
    va_list parg;
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
    aisc_assert(father.type() == father_type);
    aisc_assert(object.type() == object_type);
    link->aisc_mes_buffer[mes_cnt++] = father_type;
    link->aisc_mes_buffer[mes_cnt++] = father.get();
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
    if (arb_socket_write(link->socket, (const char *)(link->aisc_mes_buffer), mes_cnt * sizeof(long))) {
        link->error = err_connection_problems;
        PRTERR("AISC_CREATE_ERROR");
        return 1;
    }
    if (aisc_c_send_bytes_queue(link)) return 1;
    if (aisc_check_error(link)) return 1;
    object.init(link->aisc_mes_buffer[0]);
    return 0;
}

/* --------------------------------------------------------------------------------
 * Note: it's not possible to define unit tests here - they won't execute
 * Instead put your tests into ../../SERVERCNTRL/servercntrl.cxx@UNIT_TESTS
 */



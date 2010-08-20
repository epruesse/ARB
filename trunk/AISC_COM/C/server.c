#include <stdio.h>
#include <stdlib.h>
/* #include <malloc.h> */
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <limits.h>

#include "trace.h"

#define FD_SET_TYPE

#if defined(DEBUG)
/* #define SERVER_TERMINATE_ON_CONNECTION_CLOSE */
#endif /* DEBUG */

#include <signal.h>
#include <sys/time.h>
#include <netdb.h>
#include <setjmp.h>

#include "aisc_com.h"
/* AISC_MKPT_PROMOTE:#include <aisc_func_types.h> */
#include "server.h"
#include "aisc_global.h"

#include <arb_assert.h>
#include <SigHandler.h>

#define aisc_assert(cond) arb_assert(cond)

#define AISC_SERVER_OK 1
#define AISC_SERVER_FAULT 0
#define MAX_QUEUE_LEN 5

#define AISC_MAGIC_NUMBER_FILTER 0xffffff00

/* ------------------------- */
/*      some structures      */

#ifdef __cplusplus
extern "C" {
#endif

    struct Socinf {
        Socinf             *next;
        int                 socket;
        aisc_callback_func  destroy_callback;
        long                destroy_clientdata;
        int                 lasttime;
    };

#ifdef __cplusplus
}
#endif

struct pollfd;
struct Hs_struct {
    int            hso;
    Socinf        *soci;
    struct pollfd *fds;
    unsigned long  nfds;
    int            nsoc;
    int            timeout;
    int            fork;
    char          *unix_name;
};

struct aisc_bytes_list {
    char            *data;
    int              size;
    aisc_bytes_list *next;
} *aisc_server_bytes_first, *aisc_server_bytes_last;


extern char  *aisc_object_names[];
extern char **aisc_attribute_names_list[];

extern aisc_talking_func_long  *aisc_talking_functions_get[];
extern aisc_talking_func_long  *aisc_talking_functions_set[];
extern aisc_talking_func_longp *aisc_talking_functions_copy[];
extern aisc_talking_func_longp *aisc_talking_functions_find[];
extern aisc_talking_func_longp *aisc_talking_functions_create[];
extern aisc_talking_func_long   aisc_talking_functions_delete[];

const char *aisc_server_error;
int         mdba_make_core = 1;

static char       error_buf[256];
static int        aisc_server_con;
static Hs_struct *aisc_server_hs;

/* ----------------------------- */
/*      valid memory tester      */

static bool    sigsegv_occurred = false;
static bool    catch_sigsegv    = 0;
static jmp_buf return_after_segv;

static const char *test_address_valid(void *address, long key)
{
    /* tests whether 'address' is a valid readable address
     * if 'key' != 0 -> check if 'address' contains 'key'
     *
     * returns NULL or error string
     */

    static char  buf[256];
    char        *result = buf;

    sigsegv_occurred = false;
    catch_sigsegv    = true;

    // ----------------------------------------
    // start of critical section
    // (need volatile for modified local auto variables, see man longjump)
    volatile long  i;
    volatile int trapped = sigsetjmp(return_after_segv, 1);

    if (trapped == 0) { // normal execution
        i = *(long *)address; // here a SIGSEGV may happen. Execution will continue in else-branch
    }
    else {                      // return after SEGV
        arb_assert(trapped == 666); // oops - SEGV did not occur in mem access above!
        arb_assert(sigsegv_occurred); // oops - wrong handler installed ?
    }
    // end of critical section
    // ----------------------------------------

    catch_sigsegv = false;

    if (sigsegv_occurred) {
        sprintf(buf, "AISC memory manager error: can't access memory at address %p", address);
    }
    else {
        if (key && i != key) {
            sprintf(buf, "AISC memory manager error: object at address %p has wrong type (found: 0x%lx, expected: 0x%lx)",
                    address, i, key);
        }
        else {
            result = NULL;  // ok, address (and key) valid
        }
    }

    return result;
}

static void aisc_server_sigsegv(int sig) {
    sigsegv_occurred = true;

    if (catch_sigsegv) {
        siglongjmp(return_after_segv, 666); // never returns
    }

    UNINSTALL_SIGHANDLER(SIGSEGV, aisc_server_sigsegv, SIG_DFL, "aisc_server_sigsegv");
}

/* ----------------------------- */
/*      broken pipe handler      */

static int pipe_broken;

static void aisc_server_sigpipe(int)
{
    fputs("AISC server: pipe broken\n", stderr);
    pipe_broken = 1;
}

/* -------------------------- */
/*      new read command      */

static int aisc_s_read(int socket, char *ptr, int size)
{
    int leftsize, readsize;
    leftsize = size;
    readsize = 0;
    while (leftsize) {
        readsize = read(socket, ptr, leftsize);
        if (readsize<=0) return 0;
        ptr += readsize;
        leftsize -= readsize;
    }

#if defined(DUMP_COMMUNICATION)
    aisc_dump_hex("aisc_s_read: ", ptr-size, size);
#endif /* DUMP_COMMUNICATION */

    return size;
}

static int aisc_s_write(int socket, char *ptr, int size)
{
    int leftsize, writesize;
    leftsize = size;
    writesize = 0;
    pipe_broken = 0;
    while (leftsize) {
        writesize = write(socket, ptr, leftsize);
        if (pipe_broken) return -1;
        if (writesize<0) return -1;
        ptr += writesize;
        leftsize -= writesize;
        if (leftsize) usleep(10000);
    }

#if defined(DUMP_COMMUNICATION)
    aisc_dump_hex("aisc_s_write: ", ptr-size, size);
#endif /* DUMP_COMMUNICATION */

    return 0;
}


/* ---------------------------------------------- */
/*      object+attr_names for error messages      */


const char *aisc_get_object_names(long i)
{
    if ((i<0) || (i>=AISC_MAX_OBJECT) || (!aisc_object_names[i])) {
        return "<unknown object>";
    }
    return aisc_object_names[i];
}

static const char *aisc_get_object_attribute(long i, long j)
{
    if ((i<0) || (i>=AISC_MAX_OBJECT) || (!aisc_attribute_names_list[i])) {
        return "<null>";
    }
    if ((j<0) || (j>=AISC_MAX_ATTR) || (!aisc_attribute_names_list[i][j])) {
        return "<unknown attribute>";
    }
    return aisc_attribute_names_list[i][j];
}


static char *aisc_get_hostname() {
    static char *hn = 0;
    if (!hn) {
        char buffer[4096];
        gethostname(buffer, 4095);
        hn = strdup(buffer);
    }
    return hn;
}

static const char *aisc_get_m_id(const char *path, char **m_name, int *id)
{
    char           *p;
    char           *mn;
    int             i;
    if (!path) {
        return "OPEN_ARB_DB_CLIENT ERROR: missing hostname:socketid";
    }
    if (!strcmp(path, ":")) {
        path = (char *)getenv("SOCKET");
        if (!path) return "environment socket not found";
    }
    p = (char *) strchr(path, ':');
    if (path[0] == '*' || path[0] == ':') {     /* UNIX MODE */
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
    mn = (char *) calloc(sizeof(char), p - path + 1);
    strncpy(mn, path, p - path);
    if (!strcmp(mn, "localhost")) {
        free(mn);
        mn = strdup(aisc_get_hostname());
    }

    *m_name = mn;
    i = atoi(p + 1);
    if ((i < 1024) || (i > 4096)) {
        return "OPEN_ARB_DB_CLIENT ERROR: socketnumber not in [1024..4095]";
    }
    *id = i;
    return 0;
}


static const char *aisc_open_socket(const char *path, int delay, int do_connect, int *psocket, char **unix_name) {

    char buffer[128];
    struct in_addr addr;        /* union -> u_long  */
    struct hostent *he;
    const char *err;
    static int socket_id;
    static char *mach_name;
    FILE *test;

    err = aisc_get_m_id(path, &mach_name, &socket_id);
    if (err) {
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
            return (char *)strdup(buffer);
        }
        /* simply take first address */
        addr.s_addr = *(int *) (he->h_addr);
        so_ad.sin_addr = addr;
        so_ad.sin_family = AF_INET;
        so_ad.sin_port = htons(socket_id);      /* @@@ = pb_socket  */
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
        *unix_name = mach_name;
        return 0;
    }
}

Hs_struct *open_aisc_server(const char *path, int timeout, int fork) {
    Hs_struct  *hs;
    static int  so;
    static int  i;
    const char *err;

    hs = (Hs_struct *)calloc(sizeof(Hs_struct), 1);
    if (!hs) return 0;
    hs->timeout = timeout;
    hs->fork = fork;
    err = aisc_open_socket(path, TCP_NODELAY, 0, &so, &hs->unix_name);
    if (err) {
        if (*err)
            printf("%s\n", err);
        return 0;
    }

    // install signal handlers (asserting none have been installed yet!)
    ASSERT_RESULT(SigHandler, INSTALL_SIGHANDLER(SIGSEGV, aisc_server_sigsegv, "open_aisc_server"), SIG_DFL);
    ASSERT_RESULT_PREDICATE(INSTALL_SIGHANDLER(SIGPIPE, aisc_server_sigpipe, "open_aisc_server"), is_default_or_ignore_sighandler);

    aisc_server_bytes_first = 0;
    aisc_server_bytes_last  = 0;
    /* simply take first address */
    if (listen(so, MAX_QUEUE_LEN) < 0) {
        printf("AISC_SERVER_ERROR could not listen (server) %i\n", errno);
        return NULL;
    }
    i = 0;
    hs->hso = so;
    return hs;
}

static void aisc_s_add_to_bytes_queue(char *data, int size) {
    aisc_bytes_list *bl;
    bl = (aisc_bytes_list *)calloc(sizeof(aisc_bytes_list), 1);
    bl->data = data;
    bl->size = size;

    if (aisc_server_bytes_first) {
        aisc_server_bytes_last->next = bl;
        aisc_server_bytes_last = bl;
    }
    else {
        aisc_server_bytes_first = bl;
        aisc_server_bytes_last = bl;
    }
}

static int aisc_s_send_bytes_queue(int socket) {
    aisc_bytes_list *bl, *bl_next;
    for (bl = aisc_server_bytes_first; bl; bl=bl_next) {
        bl_next = bl->next;
        if (aisc_s_write(socket, (char *)bl->data, bl->size)) return 1;
        free(bl);
    };
    aisc_server_bytes_first = aisc_server_bytes_last = NULL;
    return 0;
}

#if defined(DEVEL_RALF)
static void test_test_address_valid() {
    // code to test if test_address_valid() works
#if 0
    /* test test_address_valid with illegal memory */
    aisc_server_error = test_address_valid((void*)7, 13);
    printf("aisc_server_error='%s'\n", aisc_server_error);
#endif
#if 0
    {
        char mem[]        = "some_object";
        /* test test_address_valid with illegal obj key */
        aisc_server_error = test_address_valid((void*)mem, 13);
        printf("aisc_server_error='%s'\n", aisc_server_error);
    }
#endif
}
#endif /* DEVEL_RALF */

static long aisc_talking_get(long *in_buf, int size, long *out_buf, int max_size) {
    long in_pos, out_pos;
    long code, object_type, attribute, type;

    aisc_talking_func_long    function;
    aisc_talking_func_long   *functions;
    aisc_talking_func_double  dfunction;

    long          len;
    long          erg = 0;
    static double derg;
    long          object;

    in_pos = out_pos = 0;
    aisc_server_error = NULL;
    object = in_buf[in_pos++];
    object_type = (in_buf[in_pos] & 0x00ff0000);
    attribute = 0;
    max_size = 0;

    if (object_type > (AISC_MAX_OBJECT*0x10000)) {
        aisc_server_error = "UNKNOWN OBJECT";
        object = 0;
    }
    else {
        aisc_server_error = test_address_valid((void *)object, object_type);
    }
    object_type = object_type >> (16);

#if defined(DEVEL_RALF) && 0
    test_test_address_valid();
#endif /* DEVEL_RALF */

    AISC_DUMP_SEP();
    AISC_DUMP(aisc_talking_get, int, object_type);

    while (!aisc_server_error && (in_pos < size)) {
        code = in_buf[in_pos];
        attribute = code & 0x0000ffff;
        type = code & 0xff000000;
        functions = aisc_talking_functions_get[object_type];
        if (!functions) {
            aisc_server_error = "OBJECT HAS NO ATTRIBUTES";
            attribute = 0;
            break;
        }
        if (attribute > AISC_MAX_ATTR) {
            sprintf(error_buf, "ATTRIBUTE %lx OUT of RANGE", attribute);
            aisc_server_error = error_buf;
            attribute = 0;
            break;
        }
        function = functions[attribute];
        if (!function) {
            sprintf(error_buf, "DON'T KNOW ATTRIBUTE %li",
                    attribute);
            aisc_server_error = error_buf;
            break;
        }

        AISC_DUMP(aisc_talking_get, int, attribute);
        AISC_DUMP(aisc_talking_get, int, type);

        if (type == AISC_ATTR_DOUBLE) {
            dfunction = (aisc_talking_func_double) function;
            derg = dfunction(object);
        }
        else {
            erg = function(object);
        }
        if (aisc_server_error) {
            break;
        }
        switch (type) {
            case AISC_ATTR_INT:
            case AISC_ATTR_COMMON:
                AISC_DUMP(aisc_talking_get, int, erg);
                out_buf[out_pos++] = erg;
                break;

            case AISC_ATTR_DOUBLE:
                AISC_DUMP(aisc_talking_get, double, derg);
                out_buf[out_pos++] = ((int *) &derg)[0];
                out_buf[out_pos++] = ((int *) &derg)[1];
                break;

            case AISC_ATTR_STRING:
                if (!erg) erg = (long) "(null)";
                len           = strlen((char *)erg);
                if (len > AISC_MAX_STRING_LEN) {
                    erg = (long) "(string too long)";
                    len = strlen((char *)erg);
                }

                AISC_DUMP(aisc_talking_get, charPtr, (char*)erg);

                len += 1;
                len /= sizeof(long);
                len++;
                out_buf[out_pos++] = len;
                strcpy((char *)&out_buf[out_pos], (char *)erg);
                out_pos += len;
                break;
            case AISC_ATTR_BYTES:
                {
                    bytestring *bs = (bytestring *)erg;

                    AISC_DUMP(aisc_talking_get, int, bs->size);
#if defined(DUMP_COMMUNICATION)
                    aisc_dump_hex("aisc_talking_get bytestring: ", bs->data, bs->size);
#endif /* DUMP_COMMUNICATION */

                    if (bs->data && bs->size)
                        aisc_s_add_to_bytes_queue(bs->data, bs->size);
                    out_buf[out_pos++] = bs->size;              /* size */
                    break;
                }
            default:
                aisc_server_error = "UNKNOWN TYPE";
                break;
        }
        in_pos++;
    }
    if (aisc_server_error) {
        sprintf((char *) out_buf, "AISC_GET_SERVER_ERROR %s: OBJECT:%s   ATTRIBUTE:%s",
                aisc_server_error,
                aisc_get_object_names(object_type),
                aisc_get_object_attribute(object_type, attribute));
        return -((strlen((char *)out_buf) + 1) / sizeof(long) + 1);
    }
    return out_pos;
}

int aisc_server_index = -1;

static void aisc_talking_set_index(int *obj, int i) {
    obj = obj;
    aisc_server_index = i;
}

int aisc_talking_get_index(int u, int o)
{
    if (aisc_server_index==-1) {
        aisc_server_error = "AISC_SERVER_ERROR MISSING AN AISC_INDEX";
        return -1;
    }
    if ((aisc_server_index<u) || (aisc_server_index>=o)) {
        sprintf(error_buf, "AISC_SET_SERVER_ERROR: INDEX %i IS OUT OF RANGE [%i,%i]",
                aisc_server_index, u, o);
        aisc_server_error = error_buf;
    }

    AISC_DUMP(aisc_talking_get_index, int, aisc_server_index);

    return aisc_server_index;
}

static long aisc_talking_sets(long *in_buf, int size, long *out_buf, long *object, int object_type) {
    int   blen, bsize;
    long  in_pos, out_pos;
    long  code, attribute, type;

    aisc_talking_func_long function;
    aisc_talking_func_long *functions;
    in_pos = out_pos = 0;
    aisc_server_index = -1;
    aisc_server_error   = NULL;
    object_type         = (object_type &0x00ff0000);

    attribute = 0;
    if (object_type > (AISC_MAX_OBJECT*0x10000)) {
        object_type = 0;
        aisc_server_error = "UNKNOWN OBJECT";
    }
    else {
        aisc_server_error = test_address_valid((void *)object, object_type);
    }
    object_type = object_type>>(16);
    functions   = aisc_talking_functions_set[object_type];
    if (!functions) {
        sprintf(error_buf, "OBJECT %x HAS NO ATTRIBUTES",
                object_type);
        aisc_server_error = error_buf;
    }

    AISC_DUMP_SEP();
    AISC_DUMP(aisc_talking_sets, int, object_type);

    while (!aisc_server_error && (in_pos<size)) {
        code            = in_buf[in_pos++];
        attribute       = code &0x0000ffff;
        type            = code &0xff000000;
        if (attribute > AISC_MAX_ATTR) {
            sprintf(error_buf, "ATTRIBUTE %li DOESN'T EXIST",
                    attribute);
            aisc_server_error = error_buf;
            attribute = 0;
            break;
        }
        if (code == AISC_INDEX) {
            function = (aisc_talking_func_long)aisc_talking_set_index;
        }
        else {
            function = functions[attribute];
        }
        if (!function) {
            sprintf(error_buf, "ATTRIBUTE %li DOESN'T EXIST",
                    attribute);
            aisc_server_error = error_buf;
            break;
        }

        AISC_DUMP(aisc_talking_sets, int, attribute);
        AISC_DUMP(aisc_talking_sets, int, type);

        switch (type) {
            case        AISC_ATTR_INT:
            case        AISC_ATTR_COMMON:

                AISC_DUMP(aisc_talking_sets, long, in_buf[in_pos]);

                function((long)object, in_buf[in_pos++]);
                break;
            case        AISC_ATTR_DOUBLE:
                {
                    double dummy;
                    int *ptr;
                    ptr = (int*)&dummy;
                    *ptr++ = (int)in_buf[in_pos++];
                    *ptr++ = (int)in_buf[in_pos++];

                    AISC_DUMP(aisc_talking_sets, double, dummy);

                    function((long)object, dummy);
                    break;
                }
            case        AISC_ATTR_STRING:
                {
                    char *str = strdup((char *)&(in_buf[in_pos+1]));

                    AISC_DUMP(aisc_talking_sets, charPtr, str);

                    function((long)object, str);
                    in_pos    += in_buf[in_pos]+1;
                    break;
                }
            case        AISC_ATTR_BYTES:
                bsize = (int)in_buf[in_pos++];

                AISC_DUMP(aisc_talking_sets, int, bsize);

                if (bsize) {
                    long *ptr;
                    ptr = (long*)calloc(sizeof(char), bsize);
                    blen = aisc_s_read(aisc_server_con, (char *)ptr, bsize);
                    if (bsize!=blen) {
                        aisc_server_error = "CONNECTION PROBLEMS IN BYTESTRING";
                    }
                    else {
                        bytestring bs;
                        bs.data = (char *)ptr;
                        bs.size = bsize;

#if defined(DUMP_COMMUNICATION)
                        aisc_dump_hex("aisc_talking_sets bytestring: ", (char*)ptr, bsize);
#endif /* DUMP_COMMUNICATION */

                        function((long)object, &bs);
                    }
                }
                else {
                    function((long)object, 0);
                }
                break;
            default:
                aisc_server_error = "UNKNOWN TYPE";
                break;
        }
    }
    if (aisc_server_error) {
        sprintf((char *) out_buf, "AISC_SET_SERVER_ERROR %s: OBJECT:%s   ATTRIBUTE:%s",
                aisc_server_error,
                aisc_get_object_names(object_type),
                aisc_get_object_attribute(object_type, attribute));
        return -((strlen((char *)out_buf) + 1) / sizeof(long) + 1);
    }
    return 0;
}

static long aisc_talking_set(long *in_buf, int size, long *out_buf, int max_size) {
    int in_pos, out_pos;
    int    object_type;
    long   object;
    in_pos = out_pos = 0;
    aisc_server_error      = NULL;
    max_size = 0;
    object = in_buf[in_pos++];
    object_type    = ((int)in_buf[in_pos++])& 0x00ff0000;
    return aisc_talking_sets(&(in_buf[in_pos]),
                             size-in_pos, out_buf, (long *)object, object_type);
}

static long aisc_talking_nset(long *in_buf, int size, long *out_buf, int max_size) {
    int in_pos, out_pos;
    long   error;
    int    object_type;
    long   object;
    in_pos = out_pos = 0;
    aisc_server_error      = NULL;
    max_size = 0;
    object = in_buf[in_pos++];
    object_type    = (int)(in_buf[in_pos++]& 0x00ff0000);
    error =  aisc_talking_sets(&(in_buf[in_pos]),
                               size-in_pos, out_buf, (long *)object, object_type);
    return AISC_NO_ANSWER;
}

static struct aisc_static_set_mem {
    long *ibuf, *obuf;
    int size, type;
} md;

long aisc_make_sets(long *obj)
{
    if (md.size>0) {
        return aisc_talking_sets(md.ibuf, md.size, md.obuf, obj, md.type);
    }
    else {
        return 0;
    }
}

static long aisc_talking_create(long *in_buf, int size, long *out_buf, int max_size) {
    int  in_pos, out_pos;
    long code, father_type, object_type, attribute, type;

    aisc_talking_func_longp function;
    aisc_talking_func_longp *functions;

    int   i;
    long *erg = 0;
    long  father;

    in_pos            = out_pos = 0;
    aisc_server_error = NULL;
    father_type       = in_buf[in_pos++];
    father            = in_buf[in_pos++];
    max_size          = 0;

    for (i=0; i<1; i++) {
        if ((father_type&0xff00ffff) ||
             (((unsigned int)father_type& 0xff0000) >= (AISC_MAX_OBJECT*0x10000))) {
            aisc_server_error = "AISC_CREATE_SERVER_ERROR: FATHER UNKNOWN";
            break;
        }
        aisc_server_error = test_address_valid((void *)father, father_type);
        if (aisc_server_error) break;

        father_type = father_type>>16;
        functions = aisc_talking_functions_create[father_type];
        code = in_buf[in_pos++];
        attribute = code & 0x0000ffff;
        type = code & 0xff000000;
        object_type = in_buf[in_pos++];
        if (!functions) {
            sprintf(error_buf, "AISC_CREATE_SERVER_ERROR: FATHER %s DOESN'T HAVE TARGET-ATTRIBUTE %s",
                    aisc_get_object_names(father_type), aisc_get_object_attribute(father_type, attribute));
            aisc_server_error = error_buf;
            break;
        }
        if (attribute > AISC_MAX_ATTR) {
            aisc_server_error = "AISC_CREATE_SERVER_ERROR: UNKNOWN ATTRIBUTE";
            break;
        }
        function = functions[attribute];
        if (!function) {
            sprintf(error_buf, "AISC_CREATE_SERVER_ERROR: FATHER %s FATHER DOESN'T HAVE TARGET-ATTRIBUTE %s",
                    aisc_get_object_names(father_type), aisc_get_object_attribute(father_type, attribute));
            aisc_server_error = error_buf;
            break;
        }
        md.ibuf = &(in_buf[in_pos]);
        md.obuf = out_buf;
        md.size = size - in_pos;
        md.type = (int)object_type;
        erg = function(father);
    }
    if (aisc_server_error) {
        sprintf((char *) out_buf, "%s", aisc_server_error);
        return -((strlen(aisc_server_error) + 1) / sizeof(long) + 1);
    }
    else {
        out_buf[0] = (long)erg;
        return 1;
    }
}

static long aisc_talking_copy(long *in_buf, int size, long *out_buf, int max_size) {
    int in_pos, out_pos;
    int code, father_type, object_type, attribute, type;

    aisc_talking_func_longp function;
    aisc_talking_func_longp *functions;

    int   i;
    long *erg = 0;
    long  father;
    long  object;

    in_pos            = out_pos = 0;
    aisc_server_error = NULL;
    object            = in_buf[in_pos++];
    father_type       = (int)in_buf[in_pos++];
    father            = in_buf[in_pos++];

    for (i=0; i<1; i++) {
        if ((father_type&0xff00ffff) ||
             (((unsigned int)father_type& 0xff0000) >= (AISC_MAX_OBJECT*0x10000))) {
            aisc_server_error = "AISC_COPY_SERVER_ERROR: FATHER UNKNOWN";
            break;
        }
        aisc_server_error = test_address_valid((void *)father, father_type);
        if (aisc_server_error) break;

        father_type = father_type>>16;
        functions = aisc_talking_functions_copy[father_type];
        code = (int)in_buf[in_pos++];
        object_type = (int)in_buf[in_pos++];
        attribute = code & 0x0000ffff;
        type = code & 0xff000000;
        if (!functions) {
            aisc_server_error = "AISC_COPY_SERVER_ERROR: FATHER DOESN'T HAVE TARGET-ATTRIBUTES";
            break;
        }
        if (attribute > AISC_MAX_ATTR) {
            aisc_server_error = "AISC_COPY_SERVER_ERROR: UNKNOWN ATTRIBUTE";
            break;
        }
        function = functions[attribute];
        if (!function) {
            sprintf(error_buf, "AISC_COPY_SERVER_ERROR: FATHER %s DOESN'T HAVE TARGET-ATTRIBUTE %s",
                    aisc_get_object_names(father_type), aisc_get_object_attribute(father_type, attribute));
            aisc_server_error = error_buf;
            break;
        }
        aisc_server_error = test_address_valid((void *)object, object_type);
        if (aisc_server_error) break;

        md.ibuf = &(in_buf[in_pos]);
        md.obuf = out_buf;
        md.size = size - in_pos;
        md.type = object_type;
        erg = function(father, object);
    }
    max_size = max_size;
    if (aisc_server_error) {
        sprintf((char *) out_buf, "%s", aisc_server_error);
        return -((strlen(aisc_server_error) + 1) / sizeof(long) + 1);
    }
    else {
        out_buf[0] = (long)erg;
        return 1;
    }
}

static long aisc_talking_find(long *in_buf, int size, long *out_buf, int max_size) {
    int  in_pos, out_pos;
    long code, father_type, attribute, type;

    aisc_talking_func_longp function;
    aisc_talking_func_longp *functions;

    int   i;
    long *erg = 0;
    long  father;

    in_pos            = out_pos = 0;
    aisc_server_error = NULL;
    father_type       = in_buf[in_pos++];
    father            = in_buf[in_pos++];
    size              = size;
    max_size          = max_size;

    for (i = 0; i < 1; i++) {
        if ((father_type & 0xff00ffff) ||
            (((unsigned int) father_type & 0xff0000) >= (AISC_MAX_OBJECT*0x10000))) {
            aisc_server_error = "AISC_FIND_SERVER_ERROR: FATHER UNKNOWN";
            break;
        }
        aisc_server_error = test_address_valid((void *)father, father_type);
        if (aisc_server_error)
            break;
        father_type = father_type>>16;
        functions = aisc_talking_functions_find[father_type];
        code = in_buf[in_pos++];
        attribute = code & 0x0000ffff;
        type = code & 0xff000000;
        if (!functions) {
            aisc_server_error = "AISC_FIND_SERVER_ERROR: FATHER DON'T KNOW ATTRIBUTES FOR SEARCH";
            break;
        }
        if (attribute > AISC_MAX_ATTR) {
            aisc_server_error = "AISC_FIND_SERVER_ERROR: UNKNOWN ATTRIBUTE";
            break;
        }
        function = functions[attribute];
        if (!function) {
            sprintf(error_buf, "AISC_FIND_SERVER_ERROR: FATHER %s DON'T KNOW ATTRIBUTE %s FOR SEARCH",
                    aisc_get_object_names(father_type), aisc_get_object_attribute(father_type, attribute));
            aisc_server_error = error_buf;
            break;
        }
        if (in_buf[in_pos++]<=0) {
            aisc_server_error = " AISC_FIND_SERVER_ERROR: CANNOT FIND EMPTY IDENT";
            break;
        }
        erg = function(father, &(in_buf[in_pos]));
    }
    if (aisc_server_error) {
        sprintf((char *) out_buf, "%s", aisc_server_error);
        return -((strlen(aisc_server_error) + 1) / sizeof(long) + 1);
    }
    else {
        out_buf[0] = (long) erg;
        return 1;
    }
}

extern int *aisc_main;

static long aisc_talking_init(long *in_buf, int size, long *out_buf, int max_size) {
    in_buf            = in_buf;
    size              = size;
    max_size          = max_size;
    aisc_server_error = NULL;
    out_buf[0]        = (long)aisc_main;
    return 1;
}

static long aisc_fork_server(long *in_buf, int size, long *out_buf, int max_size) {
    pid_t pid;

    in_buf   = in_buf;
    size     = size;
    out_buf  = out_buf;
    max_size = max_size;
    pid      = fork();

    if (pid<0) return 0;                            /* return OK because fork does not work */
    return pid;
}

static long aisc_talking_delete(long *in_buf, int size, long *out_buf, int max_size) {
    int             in_pos, out_pos;
    long             object_type;

    aisc_talking_func_long function;

    int             i;
    long             object;
    in_pos = out_pos = 0;
    aisc_server_error = NULL;
    object_type = in_buf[in_pos++];
    object_type = (object_type & 0x00ff0000);
    object = in_buf[in_pos++];
    for (i = 0; i < 1; i++) {
        if (object_type > (AISC_MAX_OBJECT*0x10000)) {
            aisc_server_error = "AISC_GET_SERVER_ERROR: UNKNOWN OBJECT";
        }
        else {
            aisc_server_error = test_address_valid((void *)object, object_type);
        }
        if (aisc_server_error)
            break;
        object_type = object_type >> (16);
        function = aisc_talking_functions_delete[object_type];
        if (!function) {
            sprintf(error_buf, "AISC_SET_SERVER_ERROR: OBJECT %s cannot be deleted",
                    aisc_object_names[object_type]);
            aisc_server_error = error_buf;
            break;
        }
        else {
            function(object);
        }
    }
    if (aisc_server_error) {
        size = size; max_size = max_size;
        sprintf((char *) out_buf, "%s", aisc_server_error);
        return -((strlen(aisc_server_error) + 1) / sizeof(long) + 1);
    }
    return 0;
}

static long aisc_talking_debug_info(long *in_buf, int size, long *out_buf, int max_size) {
    int  in_pos, out_pos;
    long object_type, attribute;

    aisc_talking_func_long *functionsg;
    aisc_talking_func_long *functionss;
    aisc_talking_func_longp *functions;

    int   i;
    long *object;

    size              = size;
    max_size          = max_size;
    in_pos            = out_pos = 0;
    aisc_server_error = NULL;

    for (i=0; i<256; i++) out_buf[i] = 0;
    for (i = 0; i < 1; i++) {
        object = (long *)in_buf[in_pos++];
        attribute = in_buf[in_pos++];
        aisc_server_error = test_address_valid((void *)object, 0);
        if (aisc_server_error)
            break;
        object_type = *object;
        if ((object_type > (AISC_MAX_OBJECT*0x10000)) || (object_type&0xff00ffff) || (object_type<0x00010000)) {
            aisc_server_error = "AISC_DEBUGINFO_SERVER_ERROR: UNKNOWN OBJECT";
            break;
        }
        attribute &= 0x0000ffff;
        object_type = object_type>>16;
        if (!aisc_talking_functions_delete[object_type]) { out_buf[0] = 1; };

        if (!(functionsg=aisc_talking_functions_get[object_type])) {
            out_buf[1] = 2;
        }
        else {
            if (!functionsg[attribute])         out_buf[1] = 1;
        };

        if (!(functionss=aisc_talking_functions_set[object_type])) {
            out_buf[2] = 2;
        }
        else {
            if (!functionss[attribute])         out_buf[2] = 1;
        };

        if (!(functions=aisc_talking_functions_find[object_type])) {
            out_buf[3] = 2;
        }
        else {
            if (!functions[attribute])  out_buf[3] = 1;
        };

        if (!(functions=aisc_talking_functions_create[object_type])) {
            out_buf[4] = 2;
        }
        else {
            if (!functions[attribute])  out_buf[4] = 1;
        };

        if (!(functions=aisc_talking_functions_copy[object_type])) {
            out_buf[5] = 2;
        }
        else {
            if (!functions[attribute])  out_buf[5] = 1;
        };

    }
    if (aisc_server_error) {
        sprintf((char *) out_buf, "%s", aisc_server_error);
        return -((strlen(aisc_server_error) + 1) / sizeof(long) + 1);
    }
    else {
        return 20;
    }
}

int aisc_broadcast(Hs_struct *hs, int message_type, const char *message)
{
    Socinf *si;
    int     size    = message ? strlen(message) : 0;
    int     sizeL   = (size+1+sizeof(long)-1) / sizeof(long); // number of longs needed to safely store string
    long   *out_buf = (long *)calloc(sizeL+3, sizeof(long));

    if (!message) {
        out_buf[3] = 0;
    }
    else {
        char *strStart = (char*)(out_buf+3);
        int   pad      = sizeL*sizeof(long)-(size+1);

        arb_assert(pad >= 0);

        memcpy(strStart, message, size+1);
        if (pad) memset(strStart+size+1, 0, pad); // avoid to send uninitialized bytes
    }

    arb_assert(sizeL >= 1);

    out_buf[0] = sizeL+1;
    out_buf[1] = AISC_CCOM_MESSAGE;
    out_buf[2] = message_type;

    for (si=hs->soci; si; si=si->next) {
        aisc_s_write(si->socket, (char *)out_buf, (sizeL + 3) * sizeof(long));
    }
    free(out_buf);
    return 0;
}

static int aisc_private_message(int socket, int message_type, char *message) {
    int   len;
    int   size;
    long *out_buf;

    len = 1;

    if (!message) size = 0;
    else size          = strlen(message);

    out_buf = (long *)malloc(size+64);
    if (!message) {
        out_buf[3] = 0;
        len += 1;
    }
    else {
        sprintf((char *) (out_buf+3), "%s", message);
        len += (size + 1) / sizeof(long) + 1;
    }
    out_buf[0] = len;
    out_buf[1] = AISC_CCOM_MESSAGE;
    out_buf[2] = message_type;

    if (aisc_s_write(socket, (char *)out_buf, (len + 2) * sizeof(long))) {
        aisc_server_error = "Pipe broken";
        return 0;
    }
    free(out_buf);
    return 0;
}



int aisc_talking_count;

#ifdef __cplusplus
extern "C" {
#endif

    typedef long (*aisc_talking_function_type)(long*, int, long*, int);

#ifdef __cplusplus
}
#endif

static aisc_talking_function_type aisc_talking_functions[] = {
    aisc_talking_get,
    aisc_talking_set,
    aisc_talking_nset,
    aisc_talking_create,
    aisc_talking_find,
    aisc_talking_copy,
    aisc_talking_delete,
    aisc_talking_init,
    aisc_talking_debug_info,
    aisc_fork_server
};

static int aisc_talking(int con) {
    static long      buf[AISC_MESSAGE_BUFFER_LEN];
    static long      out_buf[AISC_MESSAGE_BUFFER_LEN];
    unsigned long    len;
    static long      size;
    long             magic_number;
    len = aisc_s_read(con, (char *)buf, 2* sizeof(long));
    if (len == 2*sizeof(long)) {
        aisc_server_con = con;
        if (buf[0] >= AISC_MESSAGE_BUFFER_LEN)
            return AISC_SERVER_FAULT;
        magic_number = buf[1];
        if ((unsigned long)(magic_number & AISC_MAGIC_NUMBER_FILTER) != (unsigned long)(AISC_MAGIC_NUMBER & AISC_MAGIC_NUMBER_FILTER)) {
            return AISC_SERVER_FAULT;
        }
        size = buf[0];

        {
            long expect = size*sizeof(long);
            aisc_assert(expect >= 0);
            aisc_assert(expect <= INT_MAX);

            len = aisc_s_read(con, (char *)buf, (int)expect);
            aisc_assert(len <= LONG_MAX);

            if ((long)len != expect) {
                printf(" ERROR in AISC_SERVER: Expected to get %li bytes from client (got %lu)\n", expect, len);
                return AISC_SERVER_OK;
            }
        }
        magic_number &= ~AISC_MAGIC_NUMBER_FILTER;
        size          = (aisc_talking_functions[magic_number])
            (buf, (int)size, out_buf + 2, AISC_MESSAGE_BUFFER_LEN - 2);
        if (size >= 0) {
            out_buf[1] = AISC_CCOM_OK;
        }
        else {
            if (size == (long)AISC_NO_ANSWER) {
                return AISC_SERVER_OK;
            }
            out_buf[1] = AISC_CCOM_ERROR;
            size *= -1;
        }
        out_buf[0] = size;
        if (aisc_s_write(con, (char *)out_buf, (int)(size + 2) * sizeof(long))) {
            return AISC_SERVER_FAULT;
        }
        if (aisc_server_bytes_first) {
            if (aisc_s_send_bytes_queue(con)) {
                return AISC_SERVER_FAULT;
            }
        }
        return AISC_SERVER_OK;
    }
    else {
        return AISC_SERVER_FAULT;
    }
}

Hs_struct *aisc_accept_calls(Hs_struct *hs)
{
    int             con;
    int             anz, i;
    Socinf         *si, *si_last = NULL, *sinext, *sptr;
    fd_set          set, setex;
    struct timeval  timeout;

    if (!hs) {
        fprintf(stderr, "AISC_SERVER_ERROR socket error (==0)\n");
    }

    timeout.tv_sec  = hs->timeout / 1000;
    timeout.tv_usec = (hs->timeout % 1000) * 1000;

    aisc_server_hs = hs;

    while (hs) {
        FD_ZERO(&set);
        FD_ZERO(&setex);
        FD_SET(hs->hso, &set);
        FD_SET(hs->hso, &setex);

        for (si=hs->soci, i=1; si; si=si->next, i++)
        {
            FD_SET(si->socket, &set);
            FD_SET(si->socket, &setex);
        }
        if (hs->timeout >= 0) {
            anz = select(FD_SETSIZE, FD_SET_TYPE &set, NULL, FD_SET_TYPE &setex, &timeout);
        }
        else {
            anz = select(FD_SETSIZE, FD_SET_TYPE &set, NULL, FD_SET_TYPE &setex, 0);
        }

        if (anz==-1) {
            printf("ERROR: poll in aisc_accept_calls\n");
            return 0;
        }
        if (!anz) { /* timed out */
            return hs;
        }
        /* an event has occurred */
        if ((timeout.tv_usec>=0)&&(timeout.tv_usec<100000)) timeout.tv_usec = 100000;

        if (FD_ISSET(hs->hso, &set)) {
            con = accept(hs->hso, NULL, 0);
            if (hs->fork) {
                long id = fork();
                if (!id) {
                    return hs;
                }
            }

            if (con>0) {
                static int optval;
                sptr = (Socinf *)calloc(sizeof(Socinf), 1);
                if (!sptr) return 0;
                sptr->next = hs->soci;
                sptr->socket = con;
                hs->soci=sptr;
                hs->nsoc++;
                optval = 1;
                setsockopt(con, IPPROTO_TCP, TCP_NODELAY, (char *)&optval, 4);
            }
        }
        else {
            si_last = 0;

            for (si=hs->soci; si; si_last=si, si=sinext) {
                sinext = si->next;

                if (FD_ISSET(si->socket, &set)) {
                    if (AISC_SERVER_OK == aisc_talking(si->socket)) continue;
                } else if (!FD_ISSET(si->socket, &setex)) continue;

                if (close(si->socket) != 0) {
                    printf("aisc_accept_calls: ");
                    printf("couldn't close socket!\n");
                }

                hs->nsoc--;
                if (si == hs->soci) {   /* first one */
                    hs->soci = si->next;
                }
                else {
                    si_last->next = si->next;
                }
                if (si->destroy_callback) {
                    si->destroy_callback(si->destroy_clientdata);
                }
                free(si);
#ifdef SERVER_TERMINATE_ON_CONNECTION_CLOSE
                if (hs->nsoc == 0) { /* no clients left */
                    if (hs->fork) exit(0); /* child exits */
                    return hs; /* parent exits */
                }
                break;
#else
                /* normal behavior */
                if (hs->nsoc == 0 && hs->fork) exit(0);
                break;
#endif
            }
        }
    } /* while main loop */

    return hs;
}

void aisc_server_shutdown_and_exit(Hs_struct *hs, int exitcode) {
    /* goes to header: __ATTR__NORETURN  */
    Socinf *si;

    for (si=hs->soci; si; si=si->next) {
        shutdown(si->socket, 2); /* 2 = both dir */
        close(si->socket);
    }
    shutdown(hs->hso, 2);
    close(hs->hso);
    if (hs->unix_name) unlink(hs->unix_name);

    printf("Server terminates with code %i.\n", exitcode);

    exit(exitcode);
}


/* --------------------------- */
/*      special functions      */


static int aisc_get_key(int *obj) {
    return *obj;
}

extern "C" int aisc_add_destroy_callback(aisc_callback_func callback, long clientdata) {        /* call from server function */
    Socinf    *si;
    int        socket = aisc_server_con;
    Hs_struct *hs     = aisc_server_hs;
    if (!hs)
        return socket;
    for (si = hs->soci; si; si = si->next) {
        if (si->socket == socket) {
            si->destroy_callback = callback;
            si->destroy_clientdata = clientdata;
        }
    }
    return socket;
}

void aisc_remove_destroy_callback() {   /* call from server function */
    Socinf    *si;
    int        socket = aisc_server_con;
    Hs_struct *hs     = aisc_server_hs;
    if (!hs)
        return;
    for (si = hs->soci; si; si = si->next) {
        if (si->socket == socket) {
            si->destroy_callback = 0;
            si->destroy_clientdata = 0;
        }
    }
}

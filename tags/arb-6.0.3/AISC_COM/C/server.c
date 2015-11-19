#include <stdio.h>
#include <stdlib.h>
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
// #define SERVER_TERMINATE_ON_CONNECTION_CLOSE
#endif // DEBUG

#include <signal.h>
#include <sys/time.h>
#include <netdb.h>
#include <setjmp.h>

#include <aisc_com.h>
// AISC_MKPT_PROMOTE:#include <aisc_func_types.h>
#include "server.h"

#include <SigHandler.h>
#include <arb_cs.h>
#include <static_assert.h>
#include "common.h"

// AISC_MKPT_PROMOTE:#ifndef _STDIO_H
// AISC_MKPT_PROMOTE:#include <stdio.h>
// AISC_MKPT_PROMOTE:#endif

#define aisc_assert(cond) arb_assert(cond)

#define AISC_SERVER_OK 1
#define AISC_SERVER_FAULT 0
#define MAX_QUEUE_LEN 5

#define AISC_MAGIC_NUMBER_FILTER 0xffffff00

// -------------------------
//      some structures

#ifdef __cplusplus
extern "C" {
#endif

    struct Socinf {
        Socinf                *next;
        int                    socket;
        aisc_destroy_callback  destroy_callback;
        long                   destroy_clientdata;
        int                    lasttime;
    };

#ifdef __cplusplus
}
#endif

struct pollfd;
struct Hs_struct : virtual Noncopyable {
    int            hso;
    Socinf        *soci;
    struct pollfd *fds;
    unsigned long  nfds;
    int            nsoc;
    int            timeout;
    int            fork;
    char          *unix_name;

    Hs_struct()
        : hso(0),
          soci(NULL),
          fds(NULL),
          nfds(0),
          nsoc(0),
          timeout(0),
          fork(0),
          unix_name(NULL)
    {}
    ~Hs_struct() { freenull(unix_name); }
};

struct aisc_bytes_list {
    char            *data;
    int              size;
    aisc_bytes_list *next;
};

static aisc_bytes_list *aisc_server_bytes_first;
static aisc_bytes_list *aisc_server_bytes_last;


extern char  *aisc_object_names[];
extern char **aisc_attribute_names_list[];

extern aisc_talking_func_long  *aisc_talking_functions_get[];
extern aisc_talking_func_long  *aisc_talking_functions_set[];
extern aisc_talking_func_longp *aisc_talking_functions_copy[];
extern aisc_talking_func_longp *aisc_talking_functions_find[];
extern aisc_talking_func_longp *aisc_talking_functions_create[];
extern aisc_talking_func_long   aisc_talking_functions_delete[];

const char *aisc_server_error;

const int   ERRORBUFSIZE = 256;
static char error_buf[ERRORBUFSIZE];

static int        aisc_server_con;
static Hs_struct *aisc_server_hs;

// -----------------------
//      error handling

void aisc_server_errorf(const char *templat, ...) {
    // goes to header: __ATTR__FORMAT(1)
    va_list parg;

    va_start(parg, templat);
    int printed = vsprintf(error_buf, templat, parg);

    if (printed >= ERRORBUFSIZE) {
        fprintf(stderr,
                "Fatal: buffer overflow in aisc_server_errorf\n"
                "Error was: ");
        vfprintf(stderr, templat, parg);
        fputs("\nTerminating..\n", stderr);
        fflush(stderr);
        exit(EXIT_FAILURE);
    }
    va_end(parg);

    aisc_server_error = error_buf;
}


// -----------------------------
//      valid memory tester

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
    volatile long i       = 0;
    volatile int  trapped = sigsetjmp(return_after_segv, 1);

    if (trapped == 0) {       // normal execution
        i = *(long *)address; // here a SIGSEGV may happen. Execution will continue in else-branch
    }
    else {                             // return after SEGV
        aisc_assert(trapped == 666);   // oops - SEGV did not occur in mem access above!
        aisc_assert(sigsegv_occurred); // oops - wrong handler installed ?
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

static SigHandler old_sigsegv_handler;

__ATTR__NORETURN static void aisc_server_sigsegv(int sig) {
    sigsegv_occurred = true;
    if (catch_sigsegv) {
        siglongjmp(return_after_segv, 666); // never returns
    }
    // unexpected SEGV

    UNINSTALL_SIGHANDLER(SIGSEGV, aisc_server_sigsegv, old_sigsegv_handler, "aisc_server_sigsegv");
    old_sigsegv_handler(sig);
    aisc_assert(0); // oops - old handler returned
    abort();
}

// -----------------------------
//      broken pipe handler

static int pipe_broken;

static void aisc_server_sigpipe(int) {
    pipe_broken = 1;
}

// --------------------------
//      new read command

static int aisc_s_read(int socket, char *ptr, int size) {
    int leftsize = size;
    while (leftsize) {
        int readsize = read(socket, ptr, leftsize);
        if (readsize<=0) return 0;
        ptr += readsize;
        leftsize -= readsize;
    }

#if defined(DUMP_COMMUNICATION)
    aisc_dump_hex("aisc_s_read: ", ptr-size, size);
#endif // DUMP_COMMUNICATION

    return size;
}

static int aisc_s_write(int socket, char *ptr, int size) {
    int leftsize = size;
    pipe_broken = 0;
    while (leftsize) {
        int writesize = write(socket, ptr, leftsize);
        if (pipe_broken) {
            fputs("AISC server: pipe broken\n", stderr);
            return -1;
        }
        if (writesize<0) return -1;
        ptr += writesize;
        leftsize -= writesize;
        if (leftsize) usleep(10000); // 10 ms
    }

#if defined(DUMP_COMMUNICATION)
    aisc_dump_hex("aisc_s_write: ", ptr-size, size);
#endif // DUMP_COMMUNICATION

    return 0;
}


// ----------------------------------------------
//      object+attr_names for error messages


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

Hs_struct *open_aisc_server(const char *path, int timeout, int fork) {
    Hs_struct *hs = new Hs_struct;
    if (hs) {
        hs->timeout = timeout;
        hs->fork    = fork;

        static int  so;
        const char *err = aisc_server_open_socket(path, TCP_NODELAY, 0, &so, &hs->unix_name);

        if (err) {
            if (*err) printf("Error in open_aisc_server: %s\n", err);
            shutdown(so, SHUT_RDWR);
            close(so);
            delete hs; hs = NULL;
        }
        else {
            // install signal handlers
            fprintf(stderr, "Installing signal handler from open_aisc_server\n"); fflush(stderr);
            old_sigsegv_handler = INSTALL_SIGHANDLER(SIGSEGV, aisc_server_sigsegv, "open_aisc_server");
            ASSERT_RESULT_PREDICATE(is_default_or_ignore_sighandler, INSTALL_SIGHANDLER(SIGPIPE, aisc_server_sigpipe, "open_aisc_server"));

            aisc_server_bytes_first = 0;
            aisc_server_bytes_last  = 0;
            // simply take first address
            if (listen(so, MAX_QUEUE_LEN) < 0) {
                printf("Error in open_aisc_server: could not listen (errno=%i)\n", errno);
                delete hs; hs = NULL;
            }
            else {
                hs->hso = so;
            }
        }
    }
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

static long aisc_talking_get(long *in_buf, int size, long *out_buf, int) { // handles AISC_GET
    aisc_server_error = NULL;

    long in_pos      = 0;
    long out_pos     = 0;
    long object      = in_buf[in_pos++];
    long object_type = (in_buf[in_pos] & AISC_OBJ_TYPE_MASK);
    

    if (object_type > (AISC_MAX_OBJECT*0x10000)) {
        aisc_server_error = "UNKNOWN OBJECT";
        object = 0;
    }
    else {
        aisc_server_error = test_address_valid((void *)object, object_type);
    }
    object_type = object_type >> (16);

    AISC_DUMP_SEP();
    AISC_DUMP(aisc_talking_get, int, object_type);

    long attribute = 0;
    long erg       = 0;
    while (!aisc_server_error && (in_pos < size)) {
        long code = in_buf[in_pos];
        long type = (code & AISC_VAR_TYPE_MASK);
        attribute = (code & AISC_ATTR_MASK);

        aisc_talking_func_long *functions = aisc_talking_functions_get[object_type];

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
        aisc_talking_func_long function = functions[attribute];
        if (!function) {
            sprintf(error_buf, "DON'T KNOW ATTRIBUTE %li",
                    attribute);
            aisc_server_error = error_buf;
            break;
        }

        AISC_DUMP(aisc_talking_get, int, attribute);
        AISC_DUMP(aisc_talking_get, int, type);

        double_xfer derg;
        STATIC_ASSERT(sizeof(derg.as_double) <= sizeof(derg.as_int));
        
        if (type == AISC_TYPE_DOUBLE) {
            aisc_talking_func_double dfunction = (aisc_talking_func_double) function;
            derg.as_double = dfunction(object);
        }
        else {
            erg = function(object);
        }
        if (aisc_server_error) {
            break;
        }
        switch (type) {
            case AISC_TYPE_INT:
            case AISC_TYPE_COMMON:
                AISC_DUMP(aisc_talking_get, int, erg);
                out_buf[out_pos++] = erg;
                break;

            case AISC_TYPE_DOUBLE:
                AISC_DUMP(aisc_talking_get, double, derg.as_double);
                out_buf[out_pos++] = derg.as_int[0];
                out_buf[out_pos++] = derg.as_int[1];
                break;

            case AISC_TYPE_STRING: {
                if (!erg) erg = (long) "(null)";
                long len = strlen((char *)erg);
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
            }
            case AISC_TYPE_BYTES:
                {
                    bytestring *bs = (bytestring *)erg;

                    AISC_DUMP(aisc_talking_get, int, bs->size);
#if defined(DUMP_COMMUNICATION)
                    aisc_dump_hex("aisc_talking_get bytestring: ", bs->data, bs->size);
#endif // DUMP_COMMUNICATION

                    if (bs->data && bs->size)
                        aisc_s_add_to_bytes_queue(bs->data, bs->size);
                    out_buf[out_pos++] = bs->size;              // size
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

static int aisc_server_index = -1;

static void aisc_talking_set_index(int */*obj*/, int i) {
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
    object_type         = (object_type & AISC_OBJ_TYPE_MASK);

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
        code      = in_buf[in_pos++];
        attribute = code & AISC_ATTR_MASK;
        type      = code & AISC_VAR_TYPE_MASK;
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
            case AISC_TYPE_INT:
            case AISC_TYPE_COMMON:

                AISC_DUMP(aisc_talking_sets, long, in_buf[in_pos]);

                function((long)object, in_buf[in_pos++]);
                break;
            case AISC_TYPE_DOUBLE:
                {
                    double_xfer derg;
                    derg.as_int[0] = in_buf[in_pos++];
                    derg.as_int[1] = in_buf[in_pos++];
                    
                    AISC_DUMP(aisc_talking_sets, double, derg.as_double);

                    function((long)object, derg.as_double);
                    break;
                }
            case AISC_TYPE_STRING:
                {
                    char *str = strdup((char *)&(in_buf[in_pos+1]));

                    AISC_DUMP(aisc_talking_sets, charPtr, str);

                    function((long)object, str);
                    in_pos    += in_buf[in_pos]+1;
                    break;
                }
            case AISC_TYPE_BYTES:
                bsize = (int)in_buf[in_pos++];

                AISC_DUMP(aisc_talking_sets, int, bsize);

                if (bsize) {
                    long *ptr = (long*)calloc(sizeof(char), bsize);
                    blen = aisc_s_read(aisc_server_con, (char *)ptr, bsize);
                    if (bsize!=blen) {
                        aisc_server_error = "CONNECTION PROBLEMS IN BYTESTRING";
                        free(ptr);
                    }
                    else {
                        bytestring bs;
                        bs.data = (char *)ptr;
                        bs.size = bsize;

#if defined(DUMP_COMMUNICATION)
                        aisc_dump_hex("aisc_talking_sets bytestring: ", (char*)ptr, bsize);
#endif // DUMP_COMMUNICATION

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

static long aisc_talking_set(long *in_buf, int size, long *out_buf, int) { // handles AISC_SET
    aisc_server_error = NULL;

    int  in_pos      = 0;
    long object      = in_buf[in_pos++];
    int  object_type = ((int)in_buf[in_pos++]) & AISC_OBJ_TYPE_MASK;

    return aisc_talking_sets(&(in_buf[in_pos]), size-in_pos, out_buf, (long *)object, object_type);
}

static long aisc_talking_nset(long *in_buf, int size, long *out_buf, int) { // handles AISC_NSET 
    aisc_server_error = NULL;

    int  in_pos      = 0;
    long object      = in_buf[in_pos++];
    int  object_type = (int)(in_buf[in_pos++] & AISC_OBJ_TYPE_MASK);

    aisc_talking_sets(&(in_buf[in_pos]), size-in_pos, out_buf, (long *)object, object_type);
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

static long aisc_talking_create(long *in_buf, int size, long *out_buf, int) { // handles AISC_CREATE
    aisc_server_error = NULL;

    int  in_pos      = 0;
    long father_type = in_buf[in_pos++];
    long father      = in_buf[in_pos++];

    long *erg = 0;
    for (int i=0; i<1; i++) {
        if ((father_type&0xff00ffff) ||
            (((unsigned int)father_type& 0xff0000) >= (AISC_MAX_OBJECT*0x10000))) {
            aisc_server_error = "AISC_CREATE_SERVER_ERROR: FATHER UNKNOWN";
            break;
        }
        aisc_server_error = test_address_valid((void *)father, father_type);
        if (aisc_server_error) break;

        father_type                        = father_type>>16;
        aisc_talking_func_longp *functions = aisc_talking_functions_create[father_type];

        long code        = in_buf[in_pos++];
        long attribute   = code & AISC_ATTR_MASK;
        long object_type = in_buf[in_pos++];

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
        aisc_talking_func_longp function = functions[attribute];
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

static long aisc_talking_copy(long *in_buf, int size, long *out_buf, int /*max_size*/) { // handles AISC_COPY
    aisc_server_error = NULL;

    int  in_pos      = 0;
    long object      = in_buf[in_pos++];
    int  father_type = (int)in_buf[in_pos++];
    long father      = in_buf[in_pos++];

    long *erg = 0;
    for (int i=0; i<1; i++) {
        if ((father_type&0xff00ffff) ||
             (((unsigned int)father_type& 0xff0000) >= (AISC_MAX_OBJECT*0x10000))) {
            aisc_server_error = "AISC_COPY_SERVER_ERROR: FATHER UNKNOWN";
            break;
        }
        aisc_server_error = test_address_valid((void *)father, father_type);
        if (aisc_server_error) break;

        father_type                        = father_type>>16;
        aisc_talking_func_longp *functions = aisc_talking_functions_copy[father_type];

        if (!functions) {
            aisc_server_error = "AISC_COPY_SERVER_ERROR: FATHER DOESN'T HAVE TARGET-ATTRIBUTES";
            break;
        }

        int code        = (int)in_buf[in_pos++];
        int object_type = (int)in_buf[in_pos++];
        int attribute   = code & AISC_ATTR_MASK;

        if (attribute > AISC_MAX_ATTR) {
            aisc_server_error = "AISC_COPY_SERVER_ERROR: UNKNOWN ATTRIBUTE";
            break;
        }
        aisc_talking_func_longp function = functions[attribute];
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
    if (aisc_server_error) {
        sprintf((char *) out_buf, "%s", aisc_server_error);
        return -((strlen(aisc_server_error) + 1) / sizeof(long) + 1);
    }
    else {
        out_buf[0] = (long)erg;
        return 1;
    }
}

static long aisc_talking_find(long *in_buf, int /*size*/, long *out_buf, int /*max_size*/) { // handles AISC_FIND
    aisc_server_error = NULL;

    int  in_pos      = 0;
    long father_type = in_buf[in_pos++];
    long father      = in_buf[in_pos++];

    long *erg = 0;
    for (int i = 0; i < 1; i++) {
        if ((father_type & 0xff00ffff) ||
            (((unsigned int) father_type & 0xff0000) >= (AISC_MAX_OBJECT*0x10000))) {
            aisc_server_error = "AISC_FIND_SERVER_ERROR: FATHER UNKNOWN";
            break;
        }
        aisc_server_error = test_address_valid((void *)father, father_type);
        if (aisc_server_error)
            break;

        father_type = father_type>>16;
        aisc_talking_func_longp *functions   = aisc_talking_functions_find[father_type];

        long code      = in_buf[in_pos++];
        long attribute = code & AISC_ATTR_MASK;

        if (!functions) {
            aisc_server_error = "AISC_FIND_SERVER_ERROR: FATHER DON'T KNOW ATTRIBUTES FOR SEARCH";
            break;
        }
        if (attribute > AISC_MAX_ATTR) {
            aisc_server_error = "AISC_FIND_SERVER_ERROR: UNKNOWN ATTRIBUTE";
            break;
        }
        aisc_talking_func_longp function = functions[attribute];
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

static long aisc_talking_init(long */*in_buf*/, int /*size*/, long *out_buf, int /*max_size*/) { // handles AISC_INIT
    aisc_server_error = NULL;
    out_buf[0]        = (long)aisc_main;
    return 1;
}

static long aisc_fork_server(long */*in_buf*/, int /*size*/, long */*out_buf*/, int /*max_size*/) { // handles AISC_FORK_SERVER
    pid_t pid = fork();
    return pid<0 ? 0 : pid; // return OK(=0) when fork does not work
}

static long aisc_talking_delete(long *in_buf, int /*size*/, long *out_buf, int /*max_size*/) { // handles AISC_DELETE
    int             in_pos, out_pos;
    long             object_type;

    aisc_talking_func_long function;

    int             i;
    long             object;
    in_pos = out_pos = 0;
    aisc_server_error = NULL;
    object_type = in_buf[in_pos++];
    object_type = (object_type & AISC_OBJ_TYPE_MASK);
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
        sprintf((char *) out_buf, "%s", aisc_server_error);
        return -((strlen(aisc_server_error) + 1) / sizeof(long) + 1);
    }
    return 0;
}

static long aisc_talking_debug_info(long *in_buf, int /*size*/, long *out_buf, int /*max_size*/) { // handles AISC_DEBUG_INFO
    int  in_pos, out_pos;
    long object_type, attribute;

    aisc_talking_func_long *functionsg;
    aisc_talking_func_long *functionss;
    aisc_talking_func_longp *functions;

    int   i;
    long *object;

    in_pos            = out_pos = 0;
    aisc_server_error = NULL;

    for (i=0; i<256; i++) out_buf[i] = 0;
    for (i = 0; i < 1; i++) {
        object            = (long *)in_buf[in_pos++];
        attribute         = in_buf[in_pos++];
        aisc_server_error = test_address_valid((void *)object, 0);

        if (aisc_server_error)
            break;

        object_type = *object;
        if ((object_type > (AISC_MAX_OBJECT*0x10000)) || (object_type&0xff00ffff) || (object_type<0x00010000)) {
            aisc_server_error = "AISC_DEBUGINFO_SERVER_ERROR: UNKNOWN OBJECT";
            break;
        }
        attribute   &= AISC_ATTR_MASK;
        object_type  = object_type>>16;

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

        aisc_assert(pad >= 0);

        memcpy(strStart, message, size+1);
        if (pad) memset(strStart+size+1, 0, pad); // avoid to send uninitialized bytes
    }

    aisc_assert(sizeL >= 1);

    out_buf[0] = sizeL+1;
    out_buf[1] = AISC_CCOM_MESSAGE;
    out_buf[2] = message_type;

    for (si=hs->soci; si; si=si->next) {
        aisc_s_write(si->socket, (char *)out_buf, (sizeL + 3) * sizeof(long));
    }
    free(out_buf);
    return 0;
}

#ifdef __cplusplus
extern "C" {
#endif

    typedef long (*aisc_talking_function_type)(long*, int, long*, int);

#ifdef __cplusplus
}
#endif

static aisc_talking_function_type aisc_talking_functions[] = {
    aisc_talking_get,        // AISC_GET
    aisc_talking_set,        // AISC_SET
    aisc_talking_nset,       // AISC_NSET
    aisc_talking_create,     // AISC_CREATE
    aisc_talking_find,       // AISC_FIND
    aisc_talking_copy,       // AISC_COPY
    aisc_talking_delete,     // AISC_DELETE
    aisc_talking_init,       // AISC_INIT
    aisc_talking_debug_info, // AISC_DEBUG_INFO
    aisc_fork_server         // AISC_FORK_SERVER
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
        if (!anz) { // timed out
            return hs;
        }
        // an event has occurred
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
                if (si == hs->soci) {   // first one
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
                if (hs->nsoc == 0) { // no clients left
                    if (hs->fork) exit(EXIT_SUCCESS); // child exits
                    return hs; // parent exits
                }
                break;
#else
                // normal behavior
                if (hs->nsoc == 0 && hs->fork) exit(EXIT_SUCCESS);
                break;
#endif
            }
        }
    } // while main loop

    return hs;
}

void aisc_server_shutdown(Hs_struct*& hs) {
    Socinf *si;

    for (si=hs->soci; si; si=si->next) {
        shutdown(si->socket, SHUT_RDWR);
        close(si->socket);
    }
    shutdown(hs->hso, SHUT_RDWR);
    close(hs->hso);
    if (hs->unix_name) unlink(hs->unix_name);
    delete hs; hs = NULL;
}

void aisc_server_shutdown_and_exit(Hs_struct *hs, int exitcode) {
    /* goes to header:
     * __ATTR__NORETURN
     * __ATTR__DEPRECATED_TODO("cause it hides a call to exit() inside a library")
     */

    aisc_server_shutdown(hs);
    printf("Server terminates with code %i.\n", exitcode);
    exit(exitcode);
}


// ---------------------------
//      special functions


int aisc_add_destroy_callback(aisc_destroy_callback callback, long clientdata) {        // call from server function
    Socinf    *si;
    int        socket = aisc_server_con;
    Hs_struct *hs     = aisc_server_hs;
    if (!hs)
        return socket;
    for (si = hs->soci; si; si = si->next) {
        if (si->socket == socket) {
            if (si->destroy_callback) {
                fputs("Error: destroy_callback already bound (did you open two connections in client?)\n", stderr);
                fputs("Note: calling bound and installing new destroy_callback\n", stderr);
                si->destroy_callback(si->destroy_clientdata);
            }

            si->destroy_callback   = callback;
            si->destroy_clientdata = clientdata;
        }
    }
    return socket;
}

void aisc_remove_destroy_callback() {   // call from server function
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

int aisc_server_save_token(FILE *fd, const char *buffer, int maxsize) {
    putc('{',fd);
    const char *p = buffer;
    while (maxsize-->0) {
        int c = *(p++);
        if (!c) break;
        if (c=='}' || c == '\\') putc('\\',fd);
        putc(c,fd);
    }
    putc('}',fd);
    return 0;
}

int aisc_server_load_token(FILE *fd, char *buffer, int maxsize) {
    int   in_brackets = 0;
    char *p           = buffer;
    int   result      = EOF;

    while (maxsize-- > 0) {
        int c = getc(fd);
        if (c==EOF) break;
        if (in_brackets) {
            if (c=='\\') {
                c = getc(fd);
                *(p++) = c;
            }
            else if (c!='}') {
                *(p++) = c;
            }
            else {
                result = 0;
                break;
            }
        }
        else if (c=='{') { 
            if (p!=buffer) {
                *(p++) = '{';
                *p=0;
                return 0;
            }
            else {
                in_brackets = 1; 
            }
        }
        else if (c==' ' || c=='\n') {
            if (p!=buffer) {
                result = 0;
                break;
            }
        }
        else if (c=='}') {
            *(p++) = '}';
            result = 0;
            break;
        }
        else {
            *(p++) = c;
        }
    }
    
    *p = 0;
    return result; // read error maxsize reached
}

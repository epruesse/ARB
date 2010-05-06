// =============================================================== //
//                                                                 //
//   File      : gb_memory.h                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_MEMORY_H
#define GB_MEMORY_H

#if defined(DEBUG)
#define MEMORY_TEST 1
// MEMORY_TEST == 1 uses malloc and normal pointers for internal ARBDB memory
// -> memory checkers like valgrand work
// -> loading DB is slower (file must be parsed completely)
#else
#define MEMORY_TEST 0
// MEMORY_TEST == 0 uses own allocation and relative pointers for internal ARBDB memory
// -> memory checkers like valgrand won't be very useful
// -> debugging is difficult, cause it's difficult to follow pointers
// -> DB loads w/o delay, cause it's mmap'ped into memory
#endif

struct gb_if_entries;
struct gb_index_files_struct;
struct gb_header_list_struct;
struct GBDATA;
struct GBCONTAINER;

#if (MEMORY_TEST==1)

typedef void                   *GB_REL_ADD;
typedef char                   *GB_REL_STRING;
typedef GBDATA                 *GB_REL_GBDATA;
typedef GBCONTAINER            *GB_REL_CONTAINER;
typedef gb_header_list_struct  *GB_REL_HLS;
typedef gb_if_entries          *GB_REL_IFES;
typedef gb_index_files_struct  *GB_REL_IFS;
typedef gb_if_entries         **GB_REL_PIFES;

#define UNUSED_IN_MEMTEST(param) param = param

#else

typedef long GB_REL_ADD;
typedef long GB_REL_STRING;
typedef long GB_REL_GBDATA;
typedef long GB_REL_CONTAINER;
typedef long GB_REL_HLS;
typedef long GB_REL_IFES;
typedef long GB_REL_IFS;
typedef long GB_REL_PIFES;

#endif

// -------------------------------
//      GB_RESOLVE / GB_SETREL
//
// set GB_REL_* "pointers"

#if (MEMORY_TEST==1)

#define GB_RESOLVE(typ, struct_add, member_name)    ((typ)((struct_add)->member_name))
#define GB_SETREL(struct_add, member_name, address) (struct_add)->member_name = (address)

#else

#define GB_RESOLVE(typ, struct_add, member_name)                       \
    ((typ)(((struct_add)->member_name)                                 \
           ? (typ) (((char*)(struct_add))+((struct_add)->member_name)) \
           : NULL))

#define GB_SETREL(struct_add, member_name, address)                     \
    do {                                                                \
        char *pntr = (char *)(address);                                 \
        if (pntr) {                                                     \
            (struct_add)->member_name = (char*)(pntr)-(char*)(struct_add); \
        }                                                               \
        else {                                                          \
            (struct_add)->member_name = 0;                              \
        }                                                               \
    } while (0)


#endif

// --------------------------------
//      ARBDB memory functions

enum ARB_MEMORY_INDEX {
    GBM_CB_INDEX     = -1,
    GBM_HASH_INDEX   = -2,
    GBM_HEADER_INDEX = -3,
    GBM_UNDO         = -4,
    GBM_DICT_INDEX   = -5,
    GBM_USER_INDEX   = -6
};

#if (MEMORY_TEST==1)

void *GB_calloc(unsigned int nelem, unsigned int elsize);

inline char *gbm_get_mem(size_t size, long )          { return (char*)GB_calloc(1, size); }
inline void gbm_free_mem(char *block, size_t , long ) { free(block); }

#else

char *gbmGetMemImpl(size_t size, long index);
void gbmFreeMemImpl(char *data, size_t size, long index);

inline char *gbm_get_mem(size_t size, long index)              { return gbmGetMemImpl(size, index); }
inline void gbm_free_mem(char *block, size_t size, long index) { gbmFreeMemImpl(block, size, index); }

#endif

#else
#error gb_memory.h included twice
#endif // GB_MEMORY_H
